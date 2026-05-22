#[cxx::bridge(namespace = "nova::ffi::dicom")]
mod ffi {
    unsafe extern "C++" {
        include!("dicom_api.h");

        type Metadata;
        type PixelBuffer;
        type dicom_api;

        fn dicom_api_create() -> UniquePtr<dicom_api>;

        fn load(self: Pin<&mut dicom_api>, path: &CxxString) -> Result<()>;
        fn read_metadata(self: &dicom_api) -> Result<UniquePtr<Metadata>>;
        fn read_pixel_data(self: &dicom_api) -> Result<UniquePtr<PixelBuffer>>;
    }
}

pub use ffi::{
    Metadata, PixelBuffer,
};

pub struct DicomReader {
    inner: cxx::UniquePtr<ffi::dicom_api>,
}

impl DicomReader {
    #[must_use]
    pub fn new() -> Self { Self { inner: ffi::dicom_api_create(), } }

    pub fn load(&mut self, path: impl AsRef<std::path::Path>) -> Result<(), cxx::Exception> {
        let path = path.as_ref().to_string_lossy();
        cxx::let_cxx_string!(path = path.as_ref());
        self.inner.pin_mut().load(&path)
    }

    pub fn read_metadata(&self) -> Result<cxx::UniquePtr<Metadata>, cxx::Exception> {
        self.inner.read_metadata()
    }

    pub fn read_pixel_data(&self) -> Result<cxx::UniquePtr<PixelBuffer>, cxx::Exception> {
        self.inner.read_pixel_data()
    }
}

impl Default for DicomReader {
    fn default() -> Self {
        Self::new()
    }
}