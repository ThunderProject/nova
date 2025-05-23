#pragma once
#include <cstdint>
#include "lib/result.h"

namespace nova::dcm {
    enum class dicom_tag {
        modality,
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
        window_center,
        window_width,
        rescale_intercept,
        rescale_slope,
        body_part_examined
    };

    enum class modality {
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

    dicom_tag_data resolve_dicom_tag(dicom_tag tag);
    result<modality> resolve_modality(std::string_view modality);
}
