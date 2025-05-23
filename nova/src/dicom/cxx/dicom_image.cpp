#include "dicom_image.h"
#include <libassert/assert.hpp>
#include <ranges>
#include <algorithm>
#include "lib/logger.h"
#include "dicom_windowing.h"
#include "lib/enum_utils.h"
#include "lib/json.h"

namespace rn = std::ranges;
namespace vi = rn::views;
using namespace nova;

cv::Mat dcm::unit_converter::to_hounsfield(const cv::Mat& input, const float slope, const float intercept) {
    cv::Mat output;
    input.convertTo(output, CV_32F, slope, intercept);
    return output;
}

std::unique_ptr<dcm::dicom_image_loader> dcm::dicom_image_loader_factory::create(const modality modality) {
    switch (modality) {
        case modality::ComputedTomography: return std::make_unique<dicom_ct_image_loader>();
        default: std::unreachable();
    }
}

result<ok> dcm::dicom_ct_image_loader::load_image(dicom_image& image) {
    if (!image.fill_image_data()) {
        logger::error("Failed to extract image data from DICOM.");
        return std::unexpected("Failed to extract image data");
    }

    logger::debug("Successfully extracted image data.");

    const int cvDepth = (image.m_imageData->bytesPerPixel == 1) ? CV_8U : CV_16U;
    const int cvType = CV_MAKETYPE(cvDepth, image.m_imageData->samplesPerPixel);
    const cv::Mat rawMat(
        image.m_imageData->dimensions.height,
        image.m_imageData->dimensions.width,
        cvType,
        image.m_imageData->pixelData.data()
    );

    const auto huMat = unit_converter::to_hounsfield(rawMat, image.m_imageData->slope, image.m_imageData->intercept);
    auto windowed = image.apply_initial_windowing(huMat);

    if (image.m_imageData->photometricInterpretation == photometric_interpretation::monochrome1) {
        cv::bitwise_not(windowed, windowed);
    }

    logger::debug("applied windowing: width={}, level={}", image.m_imageData->windowing.width[0], image.m_imageData->windowing.level[0]);

    auto outputPath = std::filesystem::path("D:/repos/nova/nova/src/dicom/cxx/dicom_output");
    outputPath += ".png";

    logger::info("Writing image to '{}'", outputPath.string());
    if (!cv::imwrite(outputPath.string(), windowed)) {
        return std::unexpected("Failed to write image to disk");
    }

    logger::info("successfully exported dicom to {}", outputPath.string());
    return ok{};
}

dcm::dicom_image::dicom_image(std::filesystem::path pathToDicom)
    :
    m_dicomPath(std::move(pathToDicom).string())
{}

result<dcm::image_data> dcm::dicom_image::load_image() {
    try {
        logger::info("Starting export for DICOM: {}", m_dicomPath);

        gdcm::ImageReader reader;
        reader.SetFileName(m_dicomPath.c_str());

        if (!reader.Read()) {
            logger::error("Failed to read DICOM file: {}", m_dicomPath);
            return std::unexpected("Failed to read DICOM file");
        }

        m_dicomImage = &reader.GetImage();
        m_dicomDataSet = &reader.GetFile().GetDataSet();

        const auto modalityResult = resolve_modality();
        if (!modalityResult) {
            return std::unexpected("Failed to resolve modality");
        }

        m_modality = *modalityResult;
        if (!is_export_supported()) {
            logger::warn("Modality '{}' is currently not supported for export.", enum_to_string(m_modality));
            return std::unexpected("image exporter only supports CT's for now");
        }

        //The is_export_supported() above make sure wwe can't pass an unsupported/invalid modality to the factory
        const auto exporter = dicom_image_loader_factory::create(m_modality);
        const auto exportResult = exporter->load_image(*this);

        if (!exportResult) {
            return std::unexpected(exportResult.error());
        }
        return m_imageData.value();
    }
    catch (const std::exception& e) {
        logger::error("Failed to export image data: {}", e.what());
        return std::unexpected("Failed to export image");
    }
}

bool dcm::dicom_image::fill_image_data() {
    DEBUG_ASSERT(m_dicomImage != nullptr);

    const auto samplesPerPixel = m_dicomImage->GetPixelFormat().GetSamplesPerPixel();
    auto pixelData = fetch_pixel_data();

    logger::debug("Pixel data fetched successfully. Size: {}", pixelData->size());

    if (!pixelData) {
        logger::error("Failed to fetch pixel data.");
        return false;
    }

    image_data imgData {
        .dimensions = resolve_image_dimensions(),
        .bytesPerPixel = m_dicomImage->GetPixelFormat().GetPixelSize() / samplesPerPixel,
        .samplesPerPixel = samplesPerPixel,
        .pixelData = std::move(*pixelData),
        .slope = m_dicomImage->GetSlope(),
        .intercept = m_dicomImage->GetIntercept(),
        .photometricInterpretation = static_cast<photometric_interpretation>(m_dicomImage->GetPhotometricInterpretation().GetType()),
        .windowing = fetch_windowing_data()
    };

    logger::debug("Rescale slope: {}, intercept: {}", m_dicomImage->GetSlope(), m_dicomImage->GetIntercept());

    m_imageData = std::move(imgData);

    return true;
}

result<std::string> dcm::dicom_image::read_tag(const dicom_tag dicomTag) const {
    DEBUG_ASSERT(m_dicomDataSet != nullptr);

    auto[group, element] = resolve_dicom_tag(dicomTag);
    const gdcm::Tag tag(group, element);

    if (!m_dicomDataSet->FindDataElement(tag)) {
        logger::warn("DICOM tag not found: ({:04x},{:04x})", group, element);
        return std::unexpected("DICOM tag not found");
    }

    const gdcm::DataElement& dataElement = m_dicomDataSet->GetDataElement(tag);
    const gdcm::ByteValue* byteValue = dataElement.GetByteValue();
    if (!byteValue) {
        logger::warn("DICOM tag: Failed to extract data element value");
        return std::unexpected("failed to extract data element value");
    }

    logger::debug("Read DICOM tag ({:04x},{:04x})", group, element);
    return std::string(byteValue->GetPointer(), byteValue->GetLength());
}

dcm::image_dimensions dcm::dicom_image::resolve_image_dimensions() const {
    DEBUG_ASSERT(m_dicomImage != nullptr);

    return {
        .width = m_dicomImage->GetDimension(0),
        .height =  m_dicomImage->GetDimension(1),
    };
}

result<std::vector<uint8_t>> dcm::dicom_image::fetch_pixel_data() {
    DEBUG_ASSERT(m_dicomImage != nullptr);

    std::vector<uint8_t> buffer(m_dicomImage->GetBufferLength());

    if (!m_dicomImage->GetBuffer(reinterpret_cast<char*>(buffer.data()))) {
        logger::error("GDCM failed to read pixel buffer");
        return std::unexpected("Failed to extract pixel data");
    }

    return buffer;
}

dcm::dicom_window dcm::dicom_image::fetch_windowing_data() {
    DEBUG_ASSERT(m_dicomDataSet != nullptr);

    const auto parse_window_values = [](const std::string& input) -> std::vector<float> {
        try {
            return input.empty()
            ? std::vector<float>{}
            : input
                | vi::split('\\')
                | vi::transform([](auto &&rng) { return std::stof(std::string(rng.begin(), rng.end())); })
                | rn::to<std::vector<float>>();
        }
        catch (const std::exception& e) {
            logger::error("Failed to fetch windowing data. reason: {}", e.what());
            return {};
        }
    };

    auto windowWidth = parse_window_values(read_tag(dicom_tag::window_width).value_or(""));
    auto windowLevel = parse_window_values(read_tag(dicom_tag::window_center).value_or(""));

    if (windowWidth.empty() || windowLevel.empty()) {
        logger::warn("window width or level not set; falling back to default DICOM presets");
        return dicom_windowing::load_presets(*this);
    }

    if (windowLevel.size() != windowWidth.size()) {
        logger::warn("Mismatch between window level (size: {}) and window width (size: {}); falling back to DICOM presets.", windowLevel.size(), windowWidth.size());
        return dicom_windowing::load_presets(*this);
    }

    return dicom_window {
        .width = std::move(windowWidth),
        .level = std::move(windowLevel)
    };
}

cv::Mat dcm::dicom_image::apply_initial_windowing(const cv::Mat& hounsfieldMatrix) {
    DEBUG_ASSERT(m_imageData != std::nullopt);
    DEBUG_ASSERT(m_imageData->windowing.level.size() >= 1 && m_imageData->windowing.width.size() >= 1);

    const auto level = m_imageData->windowing.level[0];
    const auto width = m_imageData->windowing.width[0];

    const auto minVal = level - width / 2.0f;
    const auto maxVal = level + width / 2.0f;

    const auto alpha = 255.0 / (maxVal - minVal);
    const auto beta = -minVal * 255.0 / (maxVal - minVal);

    cv::Mat windowed;
    hounsfieldMatrix.convertTo(windowed, CV_8U, alpha, beta);
    cv::threshold(windowed, windowed, 255, 255, cv::THRESH_TRUNC);
    cv::threshold(windowed, windowed, 0, 0, cv::THRESH_TOZERO);

    return windowed;
}

bool dcm::dicom_image::is_export_supported() const noexcept { return m_modality == modality::ComputedTomography; }

std::optional<dcm::modality> dcm::dicom_image::resolve_modality() const {
    const auto modalityString = read_tag(dicom_tag::modality);
    if (!modalityString) {
        logger::error("Failed to extract modality string from DICOM");
        return std::nullopt;
    }
    const auto modality = dcm::resolve_modality(*modalityString);
    if (!modality) {
        logger::error("Modality '{}' is not supported by the application.", *modalityString);
        return  std::nullopt;
    }
    return *modality;
}
