#pragma once

#ifdef _WIN32
  #define NOVA_EXPORT __declspec(dllexport)
#else
  #define NOVA_EXPORT
#endif
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace nova::dcm {
    struct image_data;
    class dicom_image;
}

namespace nova::api {
    enum PhotometricInterpretation {
        unknown = 0,
        monochrome1,
        monochrome2,
        palette_color,
        rgb,
        hsv,
        argb, // retired
        cmyk,
        ybr_full,
        ybr_full_422,
        ybr_partial_422,
        ybr_partial_420,
        ybr_ict,
        ybr_rct,
    };

    struct ImageDimensions {
        uint64_t width;
        uint64_t height;
    };

    struct ImageData {
        ImageDimensions dimensions;
        int32_t bytes_per_pixel;
        uint16_t samples_per_pixel;
        double slope;
        double intercept;
        PhotometricInterpretation photometric_interpretation;
    };

    class NOVA_EXPORT dicom_handle {
    public:
        explicit dicom_handle();
        ~dicom_handle();

        void load_image();
        [[nodiscard]] std::unique_ptr<std::string> get_image_metadata() const;
        //std::unique_ptr<std::vector<uint8_t>> get_image_pixeldata() const;
    private:
        struct dicom_handle_impl;
        std::unique_ptr<dicom_handle_impl> m_impl;
    };

    NOVA_EXPORT void init();
    NOVA_EXPORT std::unique_ptr<dicom_handle> new_dicom_handle();
}
