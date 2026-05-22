#include <cstdint>
#include <memory>
#include <string>
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
        [[nodiscard]] std::unique_ptr<std::string> read_metadata() const;
        [[nodiscard]] std::unique_ptr<std::string> read_pixel_data_info() const;
        [[nodiscard]] std::unique_ptr<std::vector<std::uint8_t>> read_pixel_buffer() const;
    private:
        std::unique_ptr<nova::dicom::dicom_reader> m_reader;
    };

    [[nodiscard]] std::unique_ptr<dicom_api> dicom_api_create();
}
