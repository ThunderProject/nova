#include "dicom_api.h"
#include "dicom/dicom.h"
#include <cstdint>
#include <filesystem>
#include <format>
#include <magic_enum/magic_enum.hpp>
#include <stdexcept>
#include <string>
#include <utility>

[[nodiscard]] nova::ffi::dicom::Metadata to_api_metadata(const nova::dicom::metadata& value) {
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

[[nodiscard]] nova::ffi::dicom::PixelDataInfo to_api_pixel_info(const nova::dicom::pixel_data_info& value) {
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

[[nodiscard]] nova::ffi::dicom::PixelBuffer
to_api_pixel_buffer(nova::dicom::pixel_buffer&& value) {
    return {
        .info = to_api_pixel_info(value.info),
        .data = std::move(value.buffer),
    };
}

void nova::ffi::dicom::dicom_api::load(const std::string& path) {
    const auto fs_path = std::filesystem::path(path);
    const auto result = m_reader.load(fs_path);
    if(!result) {
        throw std::runtime_error(std::format("Failed to load DICOM file {}", fs_path.filename().string()));
    }
}

nova::ffi::dicom::Metadata nova::ffi::dicom::dicom_api::read_metadata() const {
    const auto result = m_reader.read_metadata();
    if(!result) {
        throw std::runtime_error("Failed to read DICOM metadata");
    }
    return to_api_metadata(*result);
}

nova::ffi::dicom::PixelBuffer  nova::ffi::dicom::dicom_api::read_pixel_data() const {
    const auto result = m_reader.read_pixel_data();
    if(!result) {
        throw std::runtime_error("Failed to read DICOM pixeldata");
    }

    return to_api_pixel_buffer(std::move(*result));
}
