#include "dicom_api.h"
#include "dicom/dicom.h"
#include <filesystem>
#include <format>
#include <magic_enum/magic_enum.hpp>
#include <stdexcept>
#include <string>

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
