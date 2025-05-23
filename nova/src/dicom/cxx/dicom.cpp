#include "dicom.h"
#include <utility>

using namespace nova;

dcm::dicom_tag_data dcm::resolve_dicom_tag(dicom_tag tag) {
    switch (tag) {
        case dcm::dicom_tag::modality: return dcm::dicom_tag_data {
            .group = 0x0008,
            .element = 0x0060
        };
         case dcm::dicom_tag::samples_per_pixel: return dcm::dicom_tag_data {
             .group = 0x0028,
             .element = 0x0002
         };
         case dcm::dicom_tag::photometric_interpretation: return dcm::dicom_tag_data {
             .group = 0x0028,
             .element = 0x0004
         };
         case dcm::dicom_tag::rows: return dcm::dicom_tag_data {
             .group = 0x0028,
             .element = 0x0010
         };
         case dcm::dicom_tag::columns: return dcm::dicom_tag_data {
             .group = 0x0028,
             .element = 0x0011
         };
         case dcm::dicom_tag::pixel_aspect_ratio: return dcm::dicom_tag_data {
             .group = 0x0028,
             .element = 0x0034
         };
         case dcm::dicom_tag::bits_allocated: return dcm::dicom_tag_data {
             .group = 0x0028,
             .element = 0x0100
         };
         case dcm::dicom_tag::bits_stored: return dcm::dicom_tag_data {
             .group = 0x0028,
             .element = 0x0101
         };
         case dcm::dicom_tag::high_bit: return dcm::dicom_tag_data {
             .group = 0x0028,
             .element = 0x0102
         };
         case dcm::dicom_tag::pixel_representation: return dcm::dicom_tag_data {
             .group = 0x0028,
             .element = 0x0103
         };
         case dcm::dicom_tag::smallest_image_pixel_value: return dcm::dicom_tag_data {
             .group = 0x0028,
             .element = 0x0106
         };
         case dcm::dicom_tag::largest_image_pixel_value: return dcm::dicom_tag_data {
             .group = 0x0028,
             .element = 0x0107
         };
         case dcm::dicom_tag::pixel_padding_range_limit: return dcm::dicom_tag_data {
             .group = 0x0028,
             .element = 0x0121
         };
         case dcm::dicom_tag::color_space: return dcm::dicom_tag_data {
             .group = 0x0028,
             .element = 0x2002
         };
         case dcm::dicom_tag::pixel_data: return dcm::dicom_tag_data {
             .group = 0x7FE0,
             .element = 0x0010
         };
         case dcm::dicom_tag::image_type: return dcm::dicom_tag_data {
             .group = 0x0008,
             .element = 0x0008
         };
         case dcm::dicom_tag::slice_thickness: return dcm::dicom_tag_data {
             .group = 0x0018,
             .element = 0x0050
         };
         case dcm::dicom_tag::spacing_between_slices: return dcm::dicom_tag_data {
             .group = 0x0018,
             .element = 0x0088
         };
         case dcm::dicom_tag::image_position: return dcm::dicom_tag_data {
             .group = 0x0020,
             .element = 0x0032
         };
         case dcm::dicom_tag::image_orientation: return dcm::dicom_tag_data {
             .group = 0x0020,
             .element = 0x0037
         };
         case dcm::dicom_tag::slice_location: return dcm::dicom_tag_data {
             .group = 0x0020,
             .element = 0x1041
         };
         case dcm::dicom_tag::pixel_spacing: return dcm::dicom_tag_data {
             .group = 0x0028,
             .element = 0x0030
         };
         case dcm::dicom_tag::window_center: return dcm::dicom_tag_data {
             .group = 0x0028,
             .element = 0x1050
         };
         case dcm::dicom_tag::window_width: return dcm::dicom_tag_data {
             .group = 0x0028,
             .element = 0x1051
         };
         case dcm::dicom_tag::rescale_intercept: return dcm::dicom_tag_data {
             .group = 0x0028,
             .element = 0x1052
         };
         case dcm::dicom_tag::rescale_slope: return dcm::dicom_tag_data {
             .group = 0x0028,
             .element = 0x1053
         };
        case dcm::dicom_tag::body_part_examined: return dcm::dicom_tag_data {
            .group = 0x0018,
            .element = 0x0015
        };
         default: std::unreachable();
     }
}

result<dcm::modality> dcm::resolve_modality(std::string_view modality) {
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
