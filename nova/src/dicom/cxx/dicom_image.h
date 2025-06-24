#pragma once
#include <gdcmImageReader.h>
#include <gdcmImage.h>
#include <opencv2/opencv.hpp>
#include <expected>
#include <filesystem>
#include <nlohmann/json.hpp>

#include "lib/result.h"
#include "dicom.h"
#include "dicom_windowing.h"

namespace nova::dcm {
    namespace unit_converter {
       [[nodiscard]] cv::Mat to_hounsfield(const cv::Mat& input, float slope, float intercept);
    }

    enum class photometric_interpretation {
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

    enum class image_type {
        png,
        jpg
    };

    struct image_dimensions {
        uint64_t width;
        uint64_t height;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(image_dimensions, width, height)
    };

    struct image_data {
        image_dimensions dimensions;
        int32_t bytesPerPixel;
        uint16_t samplesPerPixel;
        std::vector<uint8_t> pixelData;
        double slope;
        double intercept;
        photometric_interpretation photometricInterpretation;
        dicom_window windowing;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(
            image_data,
            dimensions,
            bytesPerPixel,
            samplesPerPixel,
            slope,
            intercept,
            photometricInterpretation,
            windowing
        );
    };

    class dicom_image;
    class dicom_image_loader {
    public:
        result<ok> virtual load_image(dicom_image& image) = 0;
        virtual ~dicom_image_loader() = default;
    };

    class dicom_image_loader_factory {
    public:
        static std::unique_ptr<dicom_image_loader> create(modality modality);
    };

    class dicom_ct_image_loader final : public dicom_image_loader {
    public:
        result<ok> load_image(dicom_image& image) override;
    };

    class dicom_image {
    public:
        explicit dicom_image(std::filesystem::path pathToDicom);

        [[nodiscard]] result<image_data> load_image();
        [[nodiscard]] result<std::string> read_tag(dicom_tag dicomTag) const;
    private:
        [[nodiscard]] bool fill_image_data();
        [[nodiscard]] image_dimensions resolve_image_dimensions() const;
        [[nodiscard]] result<std::vector<uint8_t>> fetch_pixel_data() const;
        [[nodiscard]] dicom_window fetch_windowing_data() const;
        [[nodiscard]] cv::Mat apply_initial_windowing(const cv::Mat& hounsfieldMatrix) const;
        [[nodiscard]] bool is_export_supported() const noexcept;
        [[nodiscard]] std::optional<modality> resolve_modality() const;

        gdcm::Image* m_dicomImage{nullptr};
        gdcm::DataSet* m_dicomDataSet{nullptr};
        std::optional<image_data> m_imageData{std::nullopt};
        std::string m_dicomPath;
        modality m_modality{};

        friend dicom_ct_image_loader;
    };
}
