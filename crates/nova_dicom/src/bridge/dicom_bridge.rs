use cxx::UniquePtr;

use crate::types::dicom_types::*;

#[cxx::bridge(namespace = "nova::ffi::dicom")]
pub(crate) mod ffi {
    unsafe extern "C++" {
        include!("dicom_api.h");

        type dicom_api;

        fn dicom_api_create() -> UniquePtr<dicom_api>;

        fn load(self: Pin<&mut dicom_api>, path: &CxxString) -> Result<()>;
        fn read_file(self: &dicom_api) -> Result<UniquePtr<CxxVector<u8>>>;
    }
}

pub struct CxxByteBuffer {
    pub buffer: UniquePtr<cxx::CxxVector<u8>>,
}

impl AsRef<[u8]> for CxxByteBuffer {
    #[inline]
    fn as_ref(&self) -> &[u8] {
        self.buffer
            .as_ref()
            .expect("CxxByteBuffer contained null CxxVector")
            .as_slice()
    }
}

unsafe impl Send for CxxByteBuffer {}
unsafe impl Sync for CxxByteBuffer {}

pub type Result<T> = std::result::Result<T, DicomError>;