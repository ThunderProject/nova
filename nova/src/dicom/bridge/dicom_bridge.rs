use std::ffi::{c_char, CStr};
use parking_lot::Once;
use tracing::{debug, error, info};
use tracing::log::warn;

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

fn log_thunk(level: &str, msg: &str) {
    const TARGET: &str = "nova::cxx";

    match level {
        "debug" => debug!(target: TARGET, "{}", msg),
        "info"  => info!(target: TARGET, "{}", msg),
        "warn" | "warning"  => warn!(target: TARGET, "{msg}"),
        "error" | "fatal" | "critical" => error!(target: TARGET, "{}", msg),
        unknown_lvl => warn!(target: TARGET,"Unknown log level '{unknown_lvl}', message: {unknown_lvl}"),
    }
}

unsafe extern "C" {
    fn set_log_callback(cb: extern "C" fn(level: *const c_char, msg: *const c_char));
}

extern "C" fn cxx_log_callback(level: *const c_char, msg: *const c_char) {
    unsafe {
        if let Ok(level) = CStr::from_ptr(level).to_str()
            && let Ok(msg) = CStr::from_ptr(msg).to_str()
        {
            log_thunk(level, msg);
        }
        else {
            warn!(target: "nova::cxx", "Invalid UTF-8 from native logger");
        }
    }
}

pub fn register_logger_service() {
    static INIT: Once = Once::new();
    INIT.call_once(|| unsafe {
        set_log_callback(cxx_log_callback);
    });
}