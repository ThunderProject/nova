
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

use std::ffi::{c_char, CStr};
use tracing::{debug, error, info};
use tracing::log::warn;

fn log_thunk(level: &str, msg: &str) {
    const TARGET: &str = "nova::cxx";

    match level {
        "debug" => debug!(target: TARGET, "{}", msg),
        "info"  => info!(target: TARGET, "{}", msg),
        "warn" | "warning"  => warn!(target: TARGET, "{}", msg),
        "error" | "fatal" | "critical" => error!(target: TARGET, "{}", msg),
        unknown_lvl => warn!(
            target: TARGET,
            "Unknown log level '{}', message: {}", unknown_lvl, msg
        ),
    }
}

extern "C" {
    fn set_log_callback(cb: extern "C" fn(level: *const c_char, msg: *const c_char));
}

extern "C" fn cxx_log_callback(level: *const c_char, msg: *const c_char) {
    unsafe {
        let msg = CStr::from_ptr(msg).to_string_lossy();
        let level = CStr::from_ptr(level).to_string_lossy();

        log_thunk(&level, &msg);
    }
}

pub fn register_logger_service() {
    unsafe {
        set_log_callback(cxx_log_callback);
    }
}