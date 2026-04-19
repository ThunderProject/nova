#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include "core/result.h"

namespace nova::dicom {
    enum class dicom_tag : uint8_t {
        patient_name,
        patient_id,
        patient_birth_date,
        patient_birth_time,
        patient_sex,
        study_date,
        study_time,
        study_instance_uid,
        study_id,
        study_accession_number,
        study_description,
        study_referring_physician_name,
        series_date,
        series_time,
        series_instance_uid,
        series_description,
        series_number,
        series_modality,
        series_body_part_examined,
        series_performing_physician_name,
        series_smallest_pixel_value,
        series_largest_pixel_value,
        samples_per_pixel,
        photometric_interpretation,
        rows,
        columns,
        pixel_aspect_ratio,
        bits_allocated,
        bits_stored,
        high_bit,
        pixel_representation,
        smallest_image_pixel_value,
        largest_image_pixel_value,
        pixel_padding_range_limit,
        color_space,
        pixel_data,
        image_type,
        slice_thickness,
        spacing_between_slices,
        image_position,
        image_orientation,
        slice_location,
        pixel_spacing,
        number_of_frames,
        planar_configuration,
        window_center,
        window_width,
        rescale_intercept,
        rescale_slope,
    };

    enum class modality : uint8_t {
        Unknown,
        // Ultrasound-based measurement of bone density
        UltraSoundBoneDensitometry,
        // Measurement of magnetic fields produced by electrical activity in organs
        BiomagneticImaging,
        // X-ray based measurement of bone density
        XrayBoneDensitometry,
        // Digital X-ray imaging using phosphor plates instead of film
        ComputedRadiography,
        // 3D X-ray imaging using computer-processed combinations of many X-ray measurements taken from different angles
        ComputedTomography,
        // Transillumination technique for breast imaging
        Diaphanography,
        //Direct digital capture of X-ray images
        DigitalRadiography,
        // X-ray imaging of teeth and supporting structures of the jaw
        IntraOralRadiography,
        // Specialized X-ray imaging for breast examination
        Mammography,
        // Imaging technique that uses strong magnetic fields and radio waves to generate detailed anatomical images
        MagneticResonance,
        //For radiotherapy plans
        Plan,
        // Functional imaging technique using radioactive tracers to observe metabolic processes
        PositronEmissionTomography,
        // Radiotherapy images
        RTImage,
        // Radiotherapy dose data
        RTDose,
        // Radiotherapy structure sets
        RTStruct,
        // Radiotherapy treatment plans
        RTPlan,
        // Radiotherapy treatment records
        RTRecord,
        // Segmented data from images
        Seg,
        // Imaging using high-frequency sound waves to visualize internal body structures
        Ultrasound,
    };

    struct dicom_tag_data {
        uint16_t group;
        uint16_t element;
    };

    struct patient {
        std::string name;
        std::string id;
        std::string birth_date;
        std::string birth_time;
        std::string sex;
    };

    struct study {
        std::string instance_uid;
        std::string id;
        std::string date;
        std::string time;
        std::string accession_number;
        std::string description;
        std::string referring_physician_name;
    };

    struct series {
        std::string instance_uid;
        std::string date;
        std::string time;
        std::string description;
        std::string number;
        std::string body_part_examined ;
        std::string performing_physician_name;
        std::string smallest_pixel_value;
        std::string largest_pixel_value;
        modality modality;
    };

    struct metadata {
        patient patient;
        study study;
        series series;
    };

    enum class photometric_interpretation : uint8_t {
        monochrome1,
        monochrome2,
        palette_color,
        rgb,
        hsv,
        argb,
        cmyk,
        ybr_full,
        ybr_full_422,
        ybr_partial_422,
        ybr_partial_420,
        ybr_ict,
        ybr_rct,
    };

    enum class pixel_sample_format : uint8_t {
        u8,
        u16,
        s16,
        u32,
        s32,
    };

    struct image_dimensions final {
        uint32_t width{};
        uint32_t height{};
        uint32_t frames{1};
    };

    struct pixel_data_info {
        image_dimensions dims;
        uint16_t samples_per_pixel;
        uint16_t planar_configuration;
        uint16_t bits_allocated{};
        uint16_t bits_stored{};
        uint16_t high_bit{};
        photometric_interpretation photometric;
        pixel_sample_format format;

        [[nodiscard]] std::size_t pixel_count() const noexcept {
            return static_cast<size_t>(dims.width) * static_cast<size_t>(dims.height) * static_cast<size_t>(dims.frames);
        }
    };

    struct pixel_buffer {
        pixel_data_info info{};
        std::vector<std::byte> buffer;
    };

    [[nodiscard]] dicom_tag_data resolve_dicom_tag(dicom_tag tag);
    [[nodiscard]] nova::result<modality> resolve_modality(std::string_view modality);
}
