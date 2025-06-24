#include "dicom_api.h"
#include <libassert/assert.hpp>
#include "../dicom_image.h"
#include "../lib/json.h"
#include "../lib/logger.h"

struct nova::api::dicom_handle::dicom_handle_impl {
    dicom_handle_impl()
        :
        m_image(std::make_unique<dcm::dicom_image>("D:/repos/nova/nova-cli/input/CT-MONO2-16-brain.jls.dcm"))
    {}

    result<ok> load_image() {
        DEBUG_ASSERT(m_image != nullptr);
        const auto result = m_image->load_image();
        if (!result) {
            return std::unexpected(result.error());
        }
        m_imageData = *result;
        return ok();
    }

    std::unique_ptr<dcm::dicom_image> m_image{nullptr};
    std::optional<dcm::image_data> m_imageData{std::nullopt};

    [[nodiscard]] std::string get_metadata() const {
        DEBUG_ASSERT(m_image != nullptr && m_imageData != std::nullopt);
        auto str = json::to_string(*m_imageData);
        logger::info("get_image_metadata returning \n{}", str);
        return str;
    }
};

nova::api::dicom_handle::dicom_handle()
    :
    m_impl(std::make_unique<dicom_handle_impl>())
{}

nova::api::dicom_handle::~dicom_handle() = default;

void nova::api::dicom_handle::load_image() {
    DEBUG_ASSERT(m_impl != nullptr);
    const auto result = m_impl->load_image();
    if (!result) {
        throw std::runtime_error(result.error());
    }
}

std::unique_ptr<std::string> nova::api::dicom_handle::get_image_metadata() const {
    DEBUG_ASSERT(m_impl != nullptr);
    return std::make_unique<std::string>(m_impl->get_metadata());
}

// std::unique_ptr<std::vector<uint8_t>> nova::api::dicom_handle::get_image_pixeldata() const {
//     DEBUG_ASSERT(m_imageData != nullptr);
//     return std::make_unique<std::vector<uint8_t>>(m_imageData->pixelData);
// }

void nova::api::init() {
    logger::init();
    logger::info("nova::api::init()");
}

std::unique_ptr<nova::api::dicom_handle> nova::api::new_dicom_handle() {
    auto handle = std::make_unique<dicom_handle>();
    handle->load_image();
    return handle;
}