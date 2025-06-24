use crate::dicom::dicom::Dicom;

pub trait DicomExportFormat {
    fn export(dicom: &Dicom, output_path: &str) -> anyhow::Result<()>;
}

pub struct Stl;

impl DicomExportFormat for Stl {
    fn export(dicom: &Dicom, output_path: &str) -> anyhow::Result<()> {
        Ok(())
    }
}

pub struct DicomExporter;

impl DicomExporter {
    pub fn export<F: DicomExportFormat>(dicom: &Dicom, output_path: &str) -> anyhow::Result<()> {
        F::export(dicom, output_path)
    }
}