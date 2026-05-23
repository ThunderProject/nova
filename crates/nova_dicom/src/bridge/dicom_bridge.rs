use cxx::{let_cxx_string, UniquePtr};
use std::path::Path;
use crate::types::dicom_types::*;

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