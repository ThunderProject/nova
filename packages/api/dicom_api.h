#include "dicom/dicom_reader.h"
#include <cstdint>
#include <string>
#include <vector>

namespace nova::ffi::dicom {
    struct Patient final {
        std::string name;
        std::string id;
        std::string birth_date;
        std::string birth_time;
        std::string sex;
    };

    struct Study final {
        std::string instance_uid;
        std::string id;
        std::string date;
        std::string time;
        std::string accession_number;
        std::string description;
        std::string referring_physician_name;
    };

    struct Series final {
        std::string instance_uid;
        std::string date;
        std::string time;
        std::string description;
        std::string number;
        std::string body_part_examined ;
        std::string performing_physician_name;
        std::string smallest_pixel_value;
        std::string largest_pixel_value;
        std::string modality;
    };

    struct Metadata final {
        Patient patient;
        Study study;
        Series series;
    };

    struct PixelDataInfo final {
        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t frames;
        std::uint16_t samples_per_pixel;
        std::uint16_t bits_allocated;
        std::uint16_t bits_stored;
        std::uint16_t high_bit;
        std::uint16_t planar_configuration;
        std::int32_t pixel_representation;
        std::int32_t photometric_interpretation;
        std::int32_t sample_format;
    };

    struct PixelBuffer final {
        PixelDataInfo info;
        std::vector<std::uint8_t> data;
    };

    class dicom_api final {
    public:
        dicom_api() = default;
        ~dicom_api() = default;

        void load(const std::string& path);
        [[nodiscard]] Metadata read_metadata() const;
        //[[nodiscard]] RustPixelBuffer read_pixel_data() const;
    private:
        nova::dicom::dicom_reader m_reader;
    };
}
