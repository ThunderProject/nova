use std::env;
use std::path::PathBuf;
use std::sync::{LazyLock};
use tracing::{error, info};

pub struct FolderResolver {}

static ASSETS_DIR: LazyLock<Result<PathBuf, std::io::Error>> = LazyLock::new(|| {
    let exe_path = env::current_exe()?;
    let exe_dir = exe_path.parent().ok_or_else(|| {
        std::io::Error::new(std::io::ErrorKind::Other, "Failed to get executable directory")
    })?;

    info!("Using executable directory: {}", exe_dir.display());
    let assets_path = exe_dir.join("assets");

    match assets_path.is_dir() {
        true => Ok(assets_path),
        false => Err(std::io::Error::new(
            std::io::ErrorKind::NotFound,
            format!("Assets directory not found at {:?}", assets_path),
        ))
    }
});

impl FolderResolver {
    pub fn resolve_assets_directory() -> PathBuf {
        match &*ASSETS_DIR {
            Ok(path) => path.clone(),
            Err(err) => {
                error!("Failed to resolve assets directory: {:?}", err);
                panic!("Failed to resolve assets directory");
            }
        }
    }
}