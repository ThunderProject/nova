#include "dicom_reader.h"
#include "core/hash_map.h"
#include "core/result.h"
#include "dicom.h"
#include <assert.hpp>
#include <dcmdata/dcdatset.h>
#include <dcmdata/dcdeftag.h>
#include <dcmdata/dcfilefo.h>
#include <dcmdata/dctagkey.h>
#include <exception>
#include <filesystem>
#include "logging/logger.h"
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

using namespace nova::dicom;
class dicom_reader::impl final {
public:
    [[nodiscard]] nova::result<nova::ok> load(const std::filesystem::path& path) noexcept {
        try {
            clear();
            auto file = std::make_unique<DcmFileFormat>();

            const auto status = file->loadFile(path.string());
            if(status.bad()) {
                return nova::err(std::string(status.text()));
            }

            m_file = std::move(file);
            m_file_path = path;

            return nova::ok{};
        }
        catch(const std::exception& e) {
            return nova::err(e.what());
        }
    }

    [[nodiscard]] DcmDataset* dataset() const noexcept {
        DEBUG_ASSERT(m_file != nullptr);
        return m_file->getDataset();
    }

    [[nodiscard]] std::string read_tag(dicom_tag tag) const noexcept {
        DcmDataset* dataset = this->dataset();
        DEBUG_ASSERT(dataset);

        const char* value = nullptr;
        const auto[group, element] = resolve_dicom_tag(tag);
        const auto status = dataset->findAndGetString({group, element}, value);

        if(status.bad() || value == nullptr) {
            logger::warn("Failed to read tag {}", magic_enum::enum_name(tag));
            return "";
        }
        return value;
    }

    [[nodiscard]] metadata read_metadata() const {
        return {
            .patient{
                .name = read_tag(dicom_tag::patient_name),
                .id = read_tag(dicom_tag::patient_id),
                .birth_date = read_tag(dicom_tag::patient_birth_date),
                .birth_time = read_tag(dicom_tag::patient_birth_time),
                .sex = read_tag(dicom_tag::patient_sex),
            },
            .study {
                .instance_uid = read_tag(dicom_tag::study_instance_uid),
                .id = read_tag(dicom_tag::study_id),
                .date = read_tag(dicom_tag::study_date),
                .time = read_tag(dicom_tag::study_time),
                .accession_number = read_tag(dicom_tag::study_accession_number),
                .description = read_tag(dicom_tag::study_description),
                .referring_physician_name = read_tag(dicom_tag::study_referring_physician_name)
            },
            .series {
                .instance_uid = read_tag(dicom_tag::series_instance_uid),
                .date = read_tag(dicom_tag::series_date),
                .time = read_tag(dicom_tag::series_time),
                .description = read_tag(dicom_tag::series_description),
                .number = read_tag(dicom_tag::series_number),
                .body_part_examined = read_tag(dicom_tag::series_body_part_examined),
                .performing_physician_name = read_tag(dicom_tag::series_performing_physician_name),
                .smallest_pixel_value = read_tag(dicom_tag::series_smallest_pixel_value),
                .largest_pixel_value = read_tag(dicom_tag::series_largest_pixel_value),
                .modality = [this] -> modality {
                    const auto result = resolve_modality(read_tag(dicom_tag::series_modality));
                    if(result) {
                        return result.value();
                    }
                    logger::error("Failed to read dicom modality: {}", result.error());
                    return modality::Unknown;
                }()
            }
        };
    }

    [[nodiscard]] nova::result<pixel_buffer> read_pixel_data() const noexcept {
        if(!is_loaded()) [[unlikely]] {
            logger::error("unable to read pixeldata. Reason: no dicom file loaded");
        }

        auto photometric = resolve_photometric(read_tag(dicom_tag::photometric_interpretation));
        if(!photometric) {
            logger::error("{}", photometric.error());
            return nova::err();
        }
        logger::info("photometric_interpretation: {}", *photometric);

        return {};
    }

    void clear() noexcept {
        m_file.reset();
        m_file_path.clear();
    }

    [[nodiscard]] bool is_loaded() const noexcept {
        return m_file != nullptr;
    }

    [[nodiscard]] std::filesystem::path file_path() const noexcept {
        return m_file_path;
    }
private:
    [[nodiscard]] static nova::result<photometric_interpretation> resolve_photometric(std::string_view value) noexcept {
        const nova::hash_map<std::string_view, photometric_interpretation> photometric_map {
            { "MONOCHROME1", photometric_interpretation::monochrome1 },
            { "MONOCHROME2", photometric_interpretation::monochrome2 },
            { "PALETTE COLOR", photometric_interpretation::palette_color },
            { "RGB", photometric_interpretation::rgb },
            { "HSV", photometric_interpretation::hsv },
            { "ARGB", photometric_interpretation::argb },
            { "CMYK", photometric_interpretation::cmyk },
            { "YBR_FULL", photometric_interpretation::ybr_full },
            { "YBR_FULL_422", photometric_interpretation::ybr_full_422 },
            { "YBR_PARTIAL_422", photometric_interpretation::ybr_partial_422 },
            { "YBR_PARTIAL_420", photometric_interpretation::ybr_partial_420 },
            { "YBR_ICT", photometric_interpretation::ybr_ict },
            { "YBR_RCT", photometric_interpretation::ybr_rct },
        };

        return photometric_map.contains(value)
            ? nova::result<photometric_interpretation>(photometric_map.at(value))
            : nova::err("failed to resolve photometric interpretation from dicom file");
    }

    std::unique_ptr<DcmFileFormat> m_file{nullptr};
    std::filesystem::path m_file_path;
};

dicom_reader::dicom_reader() = default;
dicom_reader::dicom_reader(dicom_reader&&) noexcept = default;
auto dicom_reader::operator=(dicom_reader&&) noexcept -> dicom_reader& = default;
dicom_reader::~dicom_reader() = default;

nova::result<nova::ok> dicom_reader::load(const std::filesystem::path& path) {
    return m_impl->load(path);
}

metadata dicom_reader::read_metadata() {
    return m_impl->read_metadata();
}

nova::result<pixel_buffer> dicom_reader::read_pixel_data() const noexcept {
    return m_impl->read_pixel_data();
}
