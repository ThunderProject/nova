#pragma once

#include "core/indirect.h"
#include "core/result.h"
#include "dicom.h"
#include <cstddef>
#include <filesystem>
#include <span>

namespace nova::dicom {
    class dicom_reader final {
    public:
        dicom_reader();
        explicit dicom_reader(const dicom_reader&) = delete;
        auto operator=(const dicom_reader&) -> dicom_reader& = delete;
        explicit dicom_reader(dicom_reader&&) noexcept;
        auto operator=(dicom_reader&&) noexcept -> dicom_reader&;
        ~dicom_reader();

        nova::result<ok> load(const std::filesystem::path& path) noexcept;
        [[nodiscard]] nova::result<metadata> read_metadata() const noexcept;
        [[nodiscard]] nova::result<pixel_data_info> read_pixel_data_info() const noexcept;
        [[nodiscard]] nova::result<std::span<const std::byte>> read_pixel_data(const pixel_data_info& info) const noexcept;
    private:
        class impl;
        nova::indirect<impl> m_impl;
    };
}
