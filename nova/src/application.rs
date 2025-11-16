use std::path::PathBuf;
use tracing::{info};
use crate::auth::auth_service::AuthService;
use crate::dicom::bridge::dicom_bridge::{dicom_api, register_logger_service};
use crate::fs::folder_resolver::FolderResolver;
use crate::ioc;

pub struct Settings {
    assets_directory: PathBuf,
}
pub struct App {
    settings: Settings,
} 

impl App {
    pub fn initialize()-> Self {
        info!("Initializing app");

        ioc::singleton::ioc().register(AuthService::new);
        register_logger_service();
        dicom_api::init();

        App {
            settings: Settings {
                assets_directory: FolderResolver::resolve_assets_directory()
            }
        }
    }
}

