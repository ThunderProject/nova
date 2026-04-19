#include "dicom.h"
#include "core/result.h"
#include "libassert/assert.hpp"
#include <expected>
#include <string>
#include <string_view>

using namespace nova::dicom;

dicom_tag_data nova::dicom::resolve_dicom_tag(dicom_tag tag) {
    switch (tag) {
         case dicom_tag::samples_per_pixel: return {
             .group = 0x0028,
             .element = 0x0002
         };
         case dicom_tag::photometric_interpretation: return {
             .group = 0x0028,
             .element = 0x0004
         };
         case dicom_tag::rows: return {
             .group = 0x0028,
             .element = 0x0010
         };
         case dicom_tag::columns: return {
             .group = 0x0028,
             .element = 0x0011
         };
         case dicom_tag::pixel_aspect_ratio: return {
             .group = 0x0028,
             .element = 0x0034
         };
         case dicom_tag::bits_allocated: return {
             .group = 0x0028,
             .element = 0x0100
         };
         case dicom_tag::bits_stored: return {
             .group = 0x0028,
             .element = 0x0101
         };
         case dicom_tag::high_bit: return {
             .group = 0x0028,
             .element = 0x0102
         };
         case dicom_tag::pixel_representation: return {
             .group = 0x0028,
             .element = 0x0103
         };
         case dicom_tag::smallest_image_pixel_value: return {
             .group = 0x0028,
             .element = 0x0106
         };
         case dicom_tag::largest_image_pixel_value: return {
             .group = 0x0028,
             .element = 0x0107
         };
         case dicom_tag::pixel_padding_range_limit: return {
             .group = 0x0028,
             .element = 0x0121
         };
         case dicom_tag::color_space: return {
             .group = 0x0028,
             .element = 0x2002
         };
         case dicom_tag::pixel_data: return {
             .group = 0x7FE0,
             .element = 0x0010
         };
         case dicom_tag::image_type: return {
             .group = 0x0008,
             .element = 0x0008
         };
         case dicom_tag::slice_thickness: return {
             .group = 0x0018,
             .element = 0x0050
         };
         case dicom_tag::spacing_between_slices: return {
             .group = 0x0018,
             .element = 0x0088
         };
         case dicom_tag::image_position: return {
             .group = 0x0020,
             .element = 0x0032
         };
         case dicom_tag::image_orientation: return {
             .group = 0x0020,
             .element = 0x0037
         };
         case dicom_tag::slice_location: return {
             .group = 0x0020,
             .element = 0x1041
         };
         case dicom_tag::pixel_spacing: return {
             .group = 0x0028,
             .element = 0x0030
         };
         case dicom_tag::window_center: return {
             .group = 0x0028,
             .element = 0x1050
         };
         case dicom_tag::window_width: return {
             .group = 0x0028,
             .element = 0x1051
         };
         case dicom_tag::rescale_intercept: return {
             .group = 0x0028,
             .element = 0x1052
         };
         case dicom_tag::rescale_slope: return {
             .group = 0x0028,
             .element = 0x1053
         };
        case dicom_tag::patient_name: return {
            .group = 0x0010,
            .element = 0x0010
        };
        case dicom_tag::patient_id: return {
            .group = 0x0010,
            .element = 0x0020
        };
        case dicom_tag::patient_birth_date: return {
            .group = 0x0010,
            .element = 0x0030
        };
        case dicom_tag::patient_birth_time: return {
            .group = 0x0010,
            .element = 0x0032
        };
        case dicom_tag::patient_sex: return {
            .group = 0x0010,
            .element = 0x0040
        };
        case dicom_tag::study_date: return {
            .group = 0x0008,
            .element = 0x0020
        };
        case dicom_tag::study_time: return {
            .group = 0x0008,
            .element = 0x0030
        };
        case dicom_tag::study_instance_uid: return {
            .group = 0x0020,
            .element = 0x000D
        };
        case dicom_tag::study_id: return {
            .group = 0x0020,
            .element = 0x0010
        };
        case dicom_tag::study_accession_number: return {
            .group = 0x0008,
            .element = 0x0050
        };
        case dicom_tag::study_description:
        return {
            .group = 0x0008,
            .element = 0x1030
        };
        case dicom_tag::study_referring_physician_name: return {
            .group = 0x0008,
            .element = 0x0090
        };
        case dicom_tag::series_instance_uid: return {
            .group = 0x0020,
            .element = 0x000E
        };
        case dicom_tag::series_description: return {
            .group = 0x0008,
            .element = 0x103E
        };
        case dicom_tag::series_number: return {
            .group = 0x0020,
            .element = 0x0011
        };
        case dicom_tag::series_modality: return {
            .group = 0x0008,
            .element = 0x0060
        };
        case dicom_tag::series_body_part_examined: return {
            .group = 0x0018,
            .element = 0x0015
        };
        case dicom_tag::series_performing_physician_name: return {
            .group = 0x0008,
            .element = 0x1050
        };
        case dicom_tag::series_smallest_pixel_value: return {
            .group = 0x0028,
            .element = 0x0108
        };
        case dicom_tag::series_largest_pixel_value: return {
            .group = 0x0028,
            .element = 0x0109
        };
        case dicom_tag::series_date: return {
            .group = 0x0008,
            .element = 0x0021
        };
        case dicom_tag::series_time:  return {
            .group = 0x0008,
            .element = 0x0031
        };
        case dicom_tag::number_of_frames: return {
            .group = 0x0028,
            .element = 0x0008
        };
        case dicom_tag::planar_configuration: return {
            .group = 0x0028,
            .element = 0x0006
        };
    }
}

nova::result<modality> nova::dicom::resolve_modality(std::string_view modality) {
    if (modality == "BDUS") {
        return modality::UltraSoundBoneDensitometry;
    }
    if (modality == "BI") {
        return modality::BiomagneticImaging;
    }
    if (modality == "BMD") {
        return modality::XrayBoneDensitometry;
    }
    if (modality == "CR") {
        return modality::ComputedRadiography;
    }
    if (modality == "CT") {
        return modality::ComputedTomography;
    }
    if (modality == "DG") {
        return modality::Diaphanography;
    }
    if (modality == "DX") {
        return modality::DigitalRadiography;
    }
    if (modality == "IO") {
        return modality::IntraOralRadiography;
    }
    if (modality == "MG") {
        return modality::Mammography;
    }
    if (modality == "MR") {
        return modality::MagneticResonance;
    }
    if (modality == "PLAN") {
        return modality::Plan;
    }
    if (modality == "PT") {
        return modality::PositronEmissionTomography;
    }
    if (modality == "RTIMAGE") {
        return modality::RTImage;
    }
    if (modality == "RTDOSE") {
        return modality::RTDose;
    }
    if (modality == "RTSTRUCT") {
        return modality::RTStruct;
    }
    if (modality == "RTPLAN") {
        return modality::RTPlan;
    }
    if (modality == "RTRECORD") {
        return modality::RTRecord;
    }
    if (modality == "SEG") {
        return modality::Seg;
    }
    if (modality == "US") {
        return modality::Ultrasound;
    }
    return std::unexpected("Incorrect or unsupported modality");
}
