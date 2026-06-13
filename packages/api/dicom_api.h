#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace nova::dicom {
    class dicom_reader;
}

namespace nova::ffi::dicom {
    class dicom_api final {
    public:
        dicom_api();
        ~dicom_api();

        void load(const std::string& path);
        [[nodiscard]] std::unique_ptr<std::vector<std::uint8_t>> read_file() const;
    private:
        std::unique_ptr<nova::dicom::dicom_reader> m_reader;
    };

    [[nodiscard]] std::unique_ptr<dicom_api> dicom_api_create();
}
