#include "dicom_reader.h"
#include "dicom.h"
#include <assert.hpp>
#include <dcmdata/dcdatset.h>
#include <dcmdata/dcdeftag.h>
#include <dcmdata/dcfilefo.h>
#include <dcmdata/dctagkey.h>
#include <exception>
#include <filesystem>
#include "dcmtk/dcmdata/dctk.h"
#include "logging/logger.h"
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <optional>
#include <string>
#include <utility>

using namespace nova::dicom;
class dicom_reader::impl final {
public:
    [[nodiscard]] std::optional<std::string> load(const std::filesystem::path& path) noexcept {
        try {
            clear();
            auto file = std::make_unique<DcmFileFormat>();

            const auto status = file->loadFile(path.string());
            if(status.bad()) {
                return std::string(status.text());
            }

            m_file = std::move(file);
            m_file_path = path;

            return std::nullopt;
        }
        catch(const std::exception& e) {
            return e.what();
        }
    }

    [[nodiscard]] DcmDataset* dataset() const noexcept {
        DEBUG_ASSERT(m_file != nullptr);
        return m_file->getDataset();
    }

    [[nodiscard]] std::string read_tag(dicom_tag tag) const noexcept {
        DcmDataset* dataset = this->dataset();
        DEBUG_ASSERT(dataset);

        const char* value = nullptr;
        const auto[group, element] = resolve_dicom_tag(tag);
        const auto status = dataset->findAndGetString({group, element}, value);

        if(status.bad() || value == nullptr) {
            logger::warn("Failed to read tag {}", magic_enum::enum_name(tag));
            return "";
        }
        return value;
    }

    metadata read_metadata() {
        metadata metadata{};
        metadata.patient.name = read_tag(dicom_tag::patient_name);
        metadata.patient.birth_date = read_tag(dicom_tag::patient_birth_date);
        metadata.patient.id = read_tag(dicom_tag::patient_id);
        metadata.patient.sex = read_tag(dicom_tag::patient_sex);

        return metadata;
    }

    void clear() noexcept {
        m_file.reset();
        m_file_path.clear();
    }

    [[nodiscard]] bool is_loaded() const noexcept {
        return m_file != nullptr;
    }

private:
    std::unique_ptr<DcmFileFormat> m_file{nullptr};
    std::filesystem::path m_file_path;
};

dicom_reader::dicom_reader()
    :
    m_impl{}
{}

std::optional<std::string> dicom_reader::load(const std::filesystem::path& path) {
    return m_impl->load(path);
}
