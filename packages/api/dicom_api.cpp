#include "dicom_api.h"
#include "dicom/dicom_reader.h"
#include "dicom/dicom.h"
#include <assert.hpp>
#include <cstdint>
#include <filesystem>
#include <format>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <nlohmann/detail/macro_scope.hpp>
#include <stdexcept>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "core/json.h"

struct Patient final {
    std::string name;
    std::string id;
    std::string birth_date;
    std::string birth_time;
    std::string sex;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Patient, name, id, birth_date, birth_time, sex);
};

struct Study final {
    std::string instance_uid;
    std::string id;
    std::string date;
    std::string time;
    std::string accession_number;
    std::string description;
    std::string referring_physician_name;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Study, instance_uid, id, date, time, accession_number, description, referring_physician_name);
};

struct Series final {
    std::string instance_uid;
    std::string date;
    std::string time;
    std::string description;
    std::string number;
    std::string body_part_examined ;
    std::string performing_physician_name;
    std::string smallest_pixel_value;
    std::string largest_pixel_value;
    std::string modality;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Series,
        instance_uid, date, time, description, number,
        body_part_examined, performing_physician_name, smallest_pixel_value,
        largest_pixel_value, modality
    );
};

struct Metadata final {
    Patient patient;
    Study study;
    Series series;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Metadata, patient, study, series);
};

struct PixelDataInfo final {
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t frames;
    std::uint16_t samples_per_pixel;
    std::uint16_t bits_allocated;
    std::uint16_t bits_stored;
    std::uint16_t high_bit;
    std::uint16_t planar_configuration;
    std::string photometric_interpretation;
    std::uint8_t sample_format;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(PixelDataInfo,
        width,
        height,
        frames,
        samples_per_pixel,
        bits_allocated,
        bits_stored,
        high_bit,
        planar_configuration,
        photometric_interpretation,
        sample_format
    );
};

struct PixelBuffer final {
    PixelDataInfo info;
    std::vector<std::uint8_t> data;
};

nova::ffi::dicom::dicom_api::dicom_api()
    :
    m_reader(std::make_unique<nova::dicom::dicom_reader>())
{}
nova::ffi::dicom::dicom_api::~dicom_api() = default;

[[nodiscard]] Metadata to_api_metadata(const nova::dicom::metadata& value) {
    return {
        .patient {
            .name = value.patient.name,
            .id = value.patient.id,
            .birth_date = value.patient.birth_date,
            .birth_time = value.patient.birth_time,
            .sex = value.patient.sex
        },
        .study = {
            .instance_uid = value.study.instance_uid,
            .id = value.study.id,
            .date = value.study.date,
            .time = value.study.time,
            .accession_number = value.study.accession_number,
            .description = value.study.description,
            .referring_physician_name = value.study.referring_physician_name,
        },
        .series = {
            .instance_uid = value.series.instance_uid,
            .date = value.series.date,
            .time = value.series.time,
            .description = value.series.description,
            .number = value.series.number,
            .body_part_examined = value.series.body_part_examined,
            .performing_physician_name = value.series.performing_physician_name,
            .smallest_pixel_value = value.series.smallest_pixel_value,
            .largest_pixel_value = value.series.largest_pixel_value,
            .modality = std::string(magic_enum::enum_name(value.series.modality))
        }
    };
}

[[nodiscard]] PixelDataInfo to_api_pixel_info(const nova::dicom::pixel_data_info& value) {
    return {
        .width = value.dims.width,
        .height = value.dims.height,
        .frames = value.dims.frames,
        .samples_per_pixel = value.samples_per_pixel,
        .bits_allocated = value.bits_allocated,
        .bits_stored = value.bits_stored,
        .high_bit = value.high_bit,
        .planar_configuration = value.planar_configuration,
        .photometric_interpretation = std::string(magic_enum::enum_name(value.photometric)),
        .sample_format = static_cast<std::uint8_t>(value.format)
    };
}

void nova::ffi::dicom::dicom_api::load(const std::string& path) {
    DEBUG_ASSERT(m_reader != nullptr);

    const auto fs_path = std::filesystem::path(path);
    const auto result = m_reader->load(fs_path);
    if(!result) {
        throw std::runtime_error(std::format("Failed to load DICOM file {}", fs_path.filename().string()));
    }
}

std::unique_ptr<std::string> nova::ffi::dicom::dicom_api::read_metadata() const {
    DEBUG_ASSERT(m_reader != nullptr);

    const auto result = m_reader->read_metadata();
    if(!result) {
        throw std::runtime_error("Failed to read DICOM metadata");
    }
    return std::make_unique<std::string>(nova::json::to_string(to_api_metadata(*result)));
}

std::unique_ptr<std::string> nova::ffi::dicom::dicom_api::read_pixel_data_info() const {
    DEBUG_ASSERT(m_reader != nullptr);

    auto result = m_reader->read_pixel_data_info();
    if(!result) {
        throw std::runtime_error("Failed to read DICOM pixeldata info");
    }
    return std::make_unique<std::string>(nova::json::to_string(to_api_pixel_info(*result)));
}

std::unique_ptr<std::vector<std::uint8_t>> nova::ffi::dicom::dicom_api::read_pixel_buffer() const {
    DEBUG_ASSERT(m_reader != nullptr);

    auto result = m_reader->read_pixel_data();
    if(!result) {
        throw std::runtime_error("Failed to read DICOM pixeldata");
    }
    return std::make_unique<std::vector<std::uint8_t>>(*result);
}

[[nodiscard]] std::unique_ptr<nova::ffi::dicom::dicom_api> nova::ffi::dicom::dicom_api_create() {
    return std::make_unique<dicom_api>();
}
