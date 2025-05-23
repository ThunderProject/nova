#include "dicom_windowing.h"
#include "dicom_image.h"
#include "lib/json.h"
#include "lib/hash_map.h"
#include "lib/logger.h"

using namespace nova;
namespace rn = std::ranges;

dcm::dicom_window dcm::dicom_windowing::load_presets(const dicom_image& image) {
    auto result = json::parse<nova::hash_map<std::string, dicom_window>>(m_presetPath);

    if (result) {
        const auto tag = image.read_tag(dicom_tag::body_part_examined);
        if (!tag) {
            return default_window();
        }

        const auto key = normalize_body_part(*tag);
        if (!key) {
            return default_window();
        }

        return result->contains(*key)
            ? (*result)[*key]
            : default_window();
    }
    return default_window();
}

dcm::dicom_window dcm::dicom_windowing::default_window() {
    logger::debug("returning default windowing values");

    return dicom_window {
        .width = { 400 },
        .level = { 40 }
    };
}

std::optional<std::string> dcm::dicom_windowing::normalize_body_part(std::string tagValue) noexcept {
    try {
        DEBUG_ASSERT(!tagValue.empty());

        rn::transform(tagValue, tagValue.begin(), ::tolower);
        if (tagValue.empty()) {
            logger::error("Failed to normalize body part tag. reason: tag is empty");
            return std::nullopt;
        }

        tagValue[0] = std::toupper(tagValue[0]);
        return tagValue;
    }
    catch (const std::exception& e) {
        logger::error("Failed to normalize body part. reason: {}", e.what());
        return std::nullopt;
    }
}
