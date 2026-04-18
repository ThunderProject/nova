#pragma once

#include "core/indirect.h"
#include "dicom.h"
#include <filesystem>
#include "core/result.h"

namespace nova::dicom {
    class dicom_reader final {
    public:
        dicom_reader();
        explicit dicom_reader(const dicom_reader&) = delete;
        auto operator=(const dicom_reader&) -> dicom_reader& = delete;
        explicit dicom_reader(dicom_reader&&) noexcept;
        auto operator=(dicom_reader&&) noexcept -> dicom_reader&;
        ~dicom_reader();

        nova::result<ok> load(const std::filesystem::path& path);
        [[nodiscard]] metadata read_metadata();
        [[nodiscard]] nova::result<pixel_buffer> read_pixel_data() const noexcept;
    private:
        class impl;
        nova::indirect<impl> m_impl;
    };
}
