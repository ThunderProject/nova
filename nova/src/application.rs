use std::path::PathBuf;
use tracing::{info};
use crate::dicom::bridge::dicom_bridge::{dicom_api, register_logger_service};
use crate::fs::folder_resolver::FolderResolver;

pub struct Settings {
    assets_directory: PathBuf,
}
pub struct App {
    settings: Settings,
} 

impl App {
    pub fn initialize()-> Self {
        info!("Initializing app");

        register_logger_service();
        dicom_api::init();

        App {
            settings: Settings {
                assets_directory: FolderResolver::resolve_assets_directory()
            }
        }
    }
}

