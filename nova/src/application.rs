use std::path::PathBuf;
use tracing::Level;
use crate::fs::folder_resolver::FolderResolver;

pub struct Settings {
    assets_directory: PathBuf,
}
pub struct App {
    settings: Settings,
} 

impl App {
    pub fn initialize()-> Self {
        App {
            settings: Settings {
                assets_directory: FolderResolver::resolve_assets_directory()
            }
        }
    }
}

