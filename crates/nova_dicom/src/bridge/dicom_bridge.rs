use cxx::{let_cxx_string, UniquePtr};
use serde::Deserialize;
use std::path::Path;

#[cxx::bridge(namespace = "nova::ffi::dicom")]
mod ffi {
    unsafe extern "C++" {
        include!("dicom_api.h");

        type dicom_api;

        fn dicom_api_create() -> UniquePtr<dicom_api>;

        fn load(self: Pin<&mut dicom_api>, path: &CxxString) -> Result<()>;
        fn read_metadata(self: &dicom_api) -> Result<UniquePtr<CxxString>>;
        fn read_pixel_data_info(self: &dicom_api) -> Result<UniquePtr<CxxString>>;
        fn read_pixel_buffer(self: &dicom_api) -> Result<UniquePtr<CxxVector<u8>>>;
    }
}

#[derive(Debug, Clone, Deserialize)]
pub struct Metadata {
    pub patient: Patient,
    pub study: Study,
    pub series: Series,
}

#[derive(Debug, Clone, Deserialize)]
pub struct Patient {
    pub name: String,
    pub id: String,
    pub birth_date: String,
    pub birth_time: String,
    pub sex: String,
}

#[derive(Debug, Clone, Deserialize)]
pub struct Study {
    pub instance_uid: String,
    pub id: String,
    pub date: String,
    pub time: String,
    pub accession_number: String,
    pub description: String,
    pub referring_physician_name: String,
}

#[derive(Debug, Clone, Deserialize)]
pub struct Series {
    pub instance_uid: String,
    pub date: String,
    pub time: String,
    pub description: String,
    pub number: String,
    pub body_part_examined: String,
    pub performing_physician_name: String,
    pub smallest_pixel_value: String,
    pub largest_pixel_value: String,
    pub modality: String,
}

#[derive(Debug, Clone, Deserialize)]
pub struct PixelDataInfo {
    pub width: u32,
    pub height: u32,
    pub frames: u32,
    pub samples_per_pixel: u16,
    pub bits_allocated: u16,
    pub bits_stored: u16,
    pub high_bit: u16,
    pub planar_configuration: u16,
    pub photometric_interpretation: String,
    pub sample_format: u8,
}

#[derive(Debug, Clone)]
pub struct PixelData {
    pub info: PixelDataInfo,
    pub bytes: Vec<u8>,
}

#[derive(Debug, thiserror::Error)]
pub enum DicomError {
    #[error("C++ DICOM error: {0}")]
    Cxx(#[from] cxx::Exception),

    #[error("DICOM JSON parse error: {0}")]
    Json(#[from] serde_json::Error),

    #[error("DICOM reader was null")]
    NullReader,

    #[error("DICOM string was null")]
    NullString,

    #[error("DICOM pixel buffer was null")]
    NullPixelBuffer,
}

pub type Result<T> = std::result::Result<T, DicomError>;

pub struct DicomReader {
    inner: UniquePtr<ffi::dicom_api>,
}

impl DicomReader {
    #[must_use]
    pub fn new() -> Self {
        Self {
            inner: ffi::dicom_api_create(),
        }
    }

    pub fn load(&mut self, path: impl AsRef<Path>) -> Result<()> {
        let path = path.as_ref().display().to_string();
        let_cxx_string!(cxx_path = path);

        self.inner.pin_mut().load(&cxx_path)?;

        Ok(())
    }

    pub fn metadata(&self) -> Result<Metadata> {
        let reader = self.inner.as_ref().ok_or(DicomError::NullReader)?;
        let json = reader.read_metadata()?;
        let json = json.as_ref().ok_or(DicomError::NullString)?;

        Ok(serde_json::from_str(&json.to_string_lossy())?)
    }

    pub fn pixel_data_info(&self) -> Result<PixelDataInfo> {
        let reader = self.inner.as_ref().ok_or(DicomError::NullReader)?;
        let json = reader.read_pixel_data_info()?;
        let json = json.as_ref().ok_or(DicomError::NullString)?;

        Ok(serde_json::from_str(&json.to_string_lossy())?)
    }

    pub fn pixel_buffer(&self) -> Result<Vec<u8>> {
        let reader = self.inner.as_ref().ok_or(DicomError::NullReader)?;
        let buffer = reader.read_pixel_buffer()?;
        let buffer = buffer.as_ref().ok_or(DicomError::NullPixelBuffer)?;

        Ok(buffer.iter().copied().collect())
    }

    pub fn pixel_data(&self) -> Result<PixelData> {
        let info = self.pixel_data_info()?;
        let bytes = self.pixel_buffer()?;

        Ok(PixelData { info, bytes })
    }
}

impl Default for DicomReader {
    fn default() -> Self {
        Self::new()
    }
}