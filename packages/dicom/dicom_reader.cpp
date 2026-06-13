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
#include "logging/logger.h"
#include <format>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <ofstd/ofcond.h>
#include <ofstd/oftypes.h>
#include <span>
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

            const auto path_string = path.string();
            const auto status = file->loadFile(path_string.c_str());
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

    [[nodiscard]] nova::result<metadata> read_metadata() const {
        if(!is_loaded()) [[unlikely]] {
            logger::error("unable to read metadata. Reason: no dicom file loaded");
            return nova::err();
        }

        return metadata {
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
                        return *result;
                    }
                    logger::error("Failed to read dicom modality: {}", result.error());
                    return modality::Unknown;
                }()
            }
        };
    }

    template<pixel_sample_format sampleFormat>
    [[nodiscard]] nova::result<std::span<const std::byte>> read_pixel_data(std::size_t expected_sample_count) const noexcept {
        auto* dataset = this->dataset();
        DEBUG_ASSERT(dataset != nullptr);

        using T = ::format_type_mapper_t<sampleFormat>;

        const auto read_array = [&]<class U>(pixel_reader_fnc_ptr<U> reader) noexcept -> nova::result<std::span<const std::byte>> {
            DEBUG_ASSERT(reader != nullptr);

            const U* src = nullptr;
            unsigned long count = 0;

            const auto status = (dataset->*reader)(
                DCM_PixelData,
                src,
                &count,
                OFFalse
            );

            if (status.bad() || src == nullptr) {
                nova::logger::error("Failed to read dicom pixel data: {}", status.text());
                return nova::err();
            }

            if (static_cast<std::size_t>(count) < expected_sample_count) {
                nova::logger::error("pixel data shorter than expected");
                return nova::err();
            }

            return std::as_bytes(std::span<const  U>{src, expected_sample_count});
        };

        if constexpr (std::is_same_v<T, std::uint8_t>) {
            return read_array(&DcmItem::findAndGetUint8Array);
        }
        else if constexpr (std::is_same_v<T, std::uint16_t>) {
            return read_array(&DcmItem::findAndGetUint16Array);
        }
        else if constexpr (std::is_same_v<T, std::int16_t>) {
            return read_array(&DcmItem::findAndGetUint16Array);
        }
        else if constexpr (std::is_same_v<T, std::uint32_t>) {
            return read_array(&DcmItem::findAndGetUint32Array);
        }
        else if constexpr (std::is_same_v<T, std::int32_t>) {
            return read_array(&DcmItem::findAndGetSint32Array);
        }
        else {
            UNREACHABLE();
        }
    }

    [[nodiscard]] nova::result<std::span<const std::byte>> read_pixel_data(const pixel_data_info& info) const noexcept {
        if (!is_loaded()) [[unlikely]] {
            logger::error("unable to read pixeldata. Reason: no dicom file loaded");
            return nova::err();
        }

        const auto expected_sample_count =
            info.pixel_count() * static_cast<std::size_t>(info.samples_per_pixel);

        switch (info.format) {
            case pixel_sample_format::u8:
                return read_pixel_data<pixel_sample_format::u8>(expected_sample_count);
            case pixel_sample_format::u16:
                return read_pixel_data<pixel_sample_format::u16>(expected_sample_count);
            case pixel_sample_format::s16:
                return read_pixel_data<pixel_sample_format::s16>(expected_sample_count);
            case pixel_sample_format::u32:
                return read_pixel_data<pixel_sample_format::u32>(expected_sample_count);
            case pixel_sample_format::s32:
                return read_pixel_data<pixel_sample_format::s32>(expected_sample_count);
            default:
                UNREACHABLE();
        }
    }

    [[nodiscard]] nova::result<pixel_data_info> read_pixel_data_info() const noexcept {
        if(!is_loaded()) [[unlikely]] {
            logger::error("unable to read pixeldata info. Reason: no dicom file loaded");
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

        return info;
    }

    void clear() noexcept {
        m_file.reset();
        m_file_path.clear();
    }

    [[nodiscard]] bool is_loaded() const noexcept {
        return m_file != nullptr;
    }

    [[nodiscard]] const std::filesystem::path& file_path() const noexcept {
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
    [[nodiscard]] nova::result<std::vector<std::uint8_t>> read_pixel_buffer(size_t expected_count) const noexcept {
        auto* dataset = this->dataset();
        DEBUG_ASSERT(dataset != nullptr);

        using T = ::format_type_mapper_t<sampleFormat>;

        const auto pixel_reader_lambda = [&dataset, &expected_count]<class Type>(pixel_reader_fnc_ptr<Type> reader) noexcept -> nova::result<std::vector<std::uint8_t>> {
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

            std::vector<std::uint8_t> bytes(expected_count * sizeof(T));
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
        using enum photometric_interpretation;

        if(value == "MONOCHROME1") { return { monochrome1 }; }
        if(value == "MONOCHROME2") { return { monochrome2 }; }
        if(value == "PALETTE COLOR") { return { palette_color }; }
        if(value == "RGB") { return { rgb }; }
        if(value == "HSV") { return { hsv }; }
        if(value == "ARGB") { return { argb }; }
        if(value == "CMYK") { return { cmyk }; }
        if(value == "YBR_FULL") { return { ybr_full }; }
        if(value == "YBR_FULL_422") { return { ybr_full_422 }; }
        if(value == "YBR_PARTIAL_422") { return { ybr_partial_422 }; }
        if(value == "YBR_PARTIAL_420") { return { ybr_partial_420 }; }
        if(value == "YBR_ICT") { return { ybr_ict }; }
        if(value == "YBR_RCT") { return { ybr_rct }; }

        return nova::err("failed to resolve photometric interpretation from dicom file");
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
dicom_reader& dicom_reader::operator=(dicom_reader&&) noexcept = default;
dicom_reader::~dicom_reader() = default;

nova::result<nova::ok> dicom_reader::load(const std::filesystem::path& path) noexcept {
    return m_impl->load(path);
}

nova::result<metadata> dicom_reader::read_metadata() const noexcept {
    return m_impl->read_metadata();
}

nova::result<std::span<const std::byte>> dicom_reader::read_pixel_data(const pixel_data_info& info) const noexcept {
    return m_impl->read_pixel_data(info);
}

nova::result<pixel_data_info> dicom_reader::read_pixel_data_info() const noexcept {
    return m_impl->read_pixel_data_info();
}
