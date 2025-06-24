
#[cxx::bridge]
pub mod dicom_api {
    extern "C++" {
        include!("dicom_api.h");
    }

    unsafe extern "C++" {
        #[namespace = "nova::api"]
        fn init();

        #[namespace = "nova::api"]
        type dicom_handle;

        #[namespace = "nova::api"]
        fn new_dicom_handle() -> Result<UniquePtr<dicom_handle>>;

        #[namespace = "nova::api"]
        fn get_image_metadata(&self) -> UniquePtr<CxxString>;

        // #[namespace = "nova::api"]
        // fn get_image_pixeldata(&self) -> UniquePtr<CxxVector<u8>>;
    }
}