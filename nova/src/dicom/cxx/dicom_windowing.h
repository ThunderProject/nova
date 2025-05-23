#pragma once
#include <vector>
#include <nlohmann/json.hpp>

namespace nova::dcm {
    struct dicom_window {
        std::vector<float> width;
        std::vector<float> level;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(dicom_window, width, level);
    };

    class dicom_image;
    class dicom_windowing {
    public:
        static dicom_window load_presets(const dicom_image& image);
    private:
        static dicom_window default_window();
        static std::optional<std::string> normalize_body_part(std::string tagValue) noexcept;
        static inline const std::filesystem::path m_presetPath = "D:/repos/nova/nova/src/dicom/cxx/assets/presets/windowing.json";
    };
}
