#include "dicom_reader.h"
#include "core/result.h"
#include "dicom.h"
#include <assert.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <dcmdata/dcdatset.h>
#include <dcmdata/dcdeftag.h>
#include <dcmdata/dcfilefo.h>
#include <dcmdata/dcitem.h>
#include <dcmdata/dctagkey.h>
#include <exception>
#include <filesystem>
#include "hash_map.h"
#include "logging/logger.h"
#include <format>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <ofstd/ofcond.h>
#include <ofstd/oftypes.h>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

using namespace nova::dicom;

namespace {
    template<class T>
    concept valid_tag_type =
        std::is_same_v<T, std::string> ||
        std::is_same_v<T, uint16_t> ||
        std::is_same_v<T, int16_t>  ||
        std::is_same_v<T, uint32_t> ||
        std::is_same_v<T, int32_t>;

    template<class T>
    concept valid_pixel_data_type =
        std::is_same_v<T, uint8_t> ||
        std::is_same_v<T, uint16_t> ||
        std::is_same_v<T, int16_t>  ||
        std::is_same_v<T, uint32_t> ||
        std::is_same_v<T, int32_t>;

    template<pixel_sample_format Format>
    struct format_type_mapper;

    template<>

    struct format_type_mapper<pixel_sample_format::u8> {
        using Type = uint8_t;
    };

    template<>
    struct format_type_mapper<pixel_sample_format::u16> {
        using Type = uint16_t;
    };

    template<>
    struct format_type_mapper<pixel_sample_format::s16> {
        using Type = int16_t;
    };

    template<>
    struct format_type_mapper<pixel_sample_format::u32> {
        using Type = uint32_t;
    };

    template<>
    struct format_type_mapper<pixel_sample_format::s32> {
        using Type = int32_t;
    };

    template<pixel_sample_format Format>
    using format_type_mapper_t = format_type_mapper<Format>::Type;
};

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
            return nova::err();
        }

        auto photometric = resolve_photometric(read_tag(dicom_tag::photometric_interpretation));
        if(!photometric) {
            logger::error("{}", photometric.error());
            return nova::err();
        }

        const auto pixel_representation = read_tag<uint16_t>(dicom_tag::pixel_representation);
        const auto bits_allocated = read_tag<uint16_t>(dicom_tag::bits_allocated);

        const auto format = resolve_pixel_sample_format(bits_allocated, pixel_representation);
        if(!format) {
            logger::error("{}", format.error());
            return nova::err();
        }

        pixel_data_info info {
            .dims = image_dimensions {
                .width = read_tag<uint16_t>(dicom_tag::columns),
                .height = read_tag<uint16_t>(dicom_tag::rows),
                .frames = read_tag<uint16_t>(dicom_tag::number_of_frames, 1)
            },
            .samples_per_pixel = read_tag<uint16_t>(dicom_tag::samples_per_pixel),
            .planar_configuration = read_tag<uint16_t>(dicom_tag::planar_configuration),
            .bits_allocated = bits_allocated,
            .bits_stored = read_tag<uint16_t>(dicom_tag::bits_stored),
            .high_bit = read_tag<uint16_t>(dicom_tag::high_bit),
            .photometric = *photometric,
            .format = *format
        };

        auto buffer = [&info, this]() {
            switch (info.format) {
                case pixel_sample_format::u8: return read_pixel_buffer<pixel_sample_format::u8>(info.pixel_count());
                case pixel_sample_format::u16: return read_pixel_buffer<pixel_sample_format::u16>(info.pixel_count());
                case pixel_sample_format::s16:return read_pixel_buffer<pixel_sample_format::s16>(info.pixel_count());
                case pixel_sample_format::u32:return read_pixel_buffer<pixel_sample_format::u32>(info.pixel_count());
                case pixel_sample_format::s32:return read_pixel_buffer<pixel_sample_format::s32>(info.pixel_count());
                default: UNREACHABLE();
            }
        }();

        if(!buffer) {
            logger::error("Failed to read pixel buffer");
            return nova::err();
        }

        return pixel_buffer {
            .info = info,
            .buffer = std::move(*buffer)
        };
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
    template<class T = std::string>
    requires valid_tag_type<T>
    [[nodiscard]] T read_tag(dicom_tag tag, const T& default_value = {}) const noexcept {
        auto* dataset = this->dataset();
        DEBUG_ASSERT(dataset != nullptr);

        if constexpr (std::is_same_v<T, std::string>) {
            const char* value = nullptr;
            const auto[group, element] = resolve_dicom_tag(tag);
            const auto status = dataset->findAndGetString({group, element}, value);

            if(status.bad() || value == nullptr) {
                nova::logger::warn("Failed to read tag {}", magic_enum::enum_name(tag));
                return default_value;
            }
            return value;
        }
        else if constexpr (std::is_same_v<T, uint16_t>) {
            uint16_t value{};
            const auto[group, element] = resolve_dicom_tag(tag);
            const auto status = dataset->findAndGetUint16({group, element}, value);

            if(status.bad()) {
                nova::logger::warn("Failed to read tag {}", magic_enum::enum_name(tag));
                return default_value;
            }
            return value;
        }
        else if constexpr (std::is_same_v<T, int16_t>) {
            int16_t value{};
            const auto[group, element] = resolve_dicom_tag(tag);
            const auto status = dataset->findAndGetSint16({group, element}, value);

            if(status.bad()) {
                nova::logger::warn("Failed to read tag {}", magic_enum::enum_name(tag));
                return default_value;
            }
            return value;
        }
        else if constexpr (std::is_same_v<T, uint32_t>) {
            uint32_t value{};
            const auto[group, element] = resolve_dicom_tag(tag);
            const auto status = dataset->findAndGetUint32({group, element}, value);

            if(status.bad()) {
                nova::logger::warn("Failed to read tag {}", magic_enum::enum_name(tag));
                return default_value;
            }
            return value;
        }
        else if constexpr (std::is_same_v<T, int32_t>) {
            int32_t value{};
            const auto[group, element] = resolve_dicom_tag(tag);
            const auto status = dataset->findAndGetSint32({group, element}, value);

            if(status.bad()) {
                nova::logger::warn("Failed to read tag {}", magic_enum::enum_name(tag));
                return default_value;
            }
            return value;
        }
        else {
            UNREACHABLE();
        }
    }

    template<class T>
    using pixel_reader_fnc_ptr = OFCondition(DcmItem::*)(const DcmTagKey&, const T*& value, unsigned long*, const OFBool);

    template<pixel_sample_format sampleFormat>
    [[nodiscard]] nova::result<std::vector<std::byte>> read_pixel_buffer(size_t expected_count) const noexcept {
        auto* dataset = this->dataset();
        DEBUG_ASSERT(dataset != nullptr);

        using T = ::format_type_mapper_t<sampleFormat>;

        const auto pixel_reader_lambda = [&dataset, &expected_count]<class Type>(pixel_reader_fnc_ptr<Type> reader) noexcept -> nova::result<std::vector<std::byte>> {
            DEBUG_ASSERT(reader != nullptr);

            const Type* src = nullptr;
            uint64_t count = 0;
            const auto status = (dataset->*reader)(DCM_PixelData, src, &count, OFFalse);

            if(status.bad() || src == nullptr) {
                nova::logger::error("Failed to read dicom pixel data: {}", status.text());
                return nova::err();
            }

            if(count < expected_count) {
                nova::logger::error("pixel data shorter than expected");
                return nova::err();
            }

            std::vector<std::byte> bytes(expected_count * sizeof(T));
            std::memcpy(bytes.data(), src, bytes.size());
            return bytes;
        };

        if constexpr (std::is_same_v<T, uint8_t>) {
            return pixel_reader_lambda(&DcmItem::findAndGetUint8Array);
        }
        else if constexpr (std::is_same_v<T, uint16_t>) {
            return pixel_reader_lambda(&DcmItem::findAndGetUint16Array);
        }
        else if constexpr (std::is_same_v<T, int16_t>) {
            return pixel_reader_lambda(&DcmItem::findAndGetUint16Array);
        }
        else if constexpr (std::is_same_v<T, uint32_t>) {
            return pixel_reader_lambda(&DcmItem::findAndGetUint32Array);
        }
        else if constexpr (std::is_same_v<T, int32_t>) {
            return pixel_reader_lambda(&DcmItem::findAndGetSint32Array);
        }
        else {
            UNREACHABLE();
        }
    }

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

   [[nodiscard]] static nova::result<pixel_sample_format> resolve_pixel_sample_format(uint16_t bits_allocated, uint16_t pixel_representation) {
       if(bits_allocated == 1) {
           return nova::err("Failed to resolve pixel sample format: 1 bit images are not supported");
       }

       if(bits_allocated == 8) {
           return pixel_sample_format::u8;
       }

       if(bits_allocated == 16) {
           return pixel_representation == 0
               ? pixel_sample_format::u16
               : pixel_sample_format::s16;
       }

       return nova::err(
           std::format(
               "Failed to resolve  pixel sample forma: Unsupported format. bits allocated={}, pixel representation={}",
               bits_allocated,
               pixel_representation
           )
       );
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
