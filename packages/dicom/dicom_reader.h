#include "core/indirect.h"
#include <filesystem>
#include <optional>
#include <string>

namespace nova::dicom {
    class dicom_reader final {
    public:
        dicom_reader();
        explicit dicom_reader(const dicom_reader&) = delete;
        auto operator=(const dicom_reader&) -> dicom_reader& = delete;
        explicit dicom_reader(dicom_reader&&) noexcept;
        auto operator=(dicom_reader&&) noexcept -> dicom_reader&;
        ~dicom_reader() = default;

        std::optional<std::string> load(const std::filesystem::path& path);
    private:
        class impl;
        nova::indirect<impl> m_impl;
    };
}
