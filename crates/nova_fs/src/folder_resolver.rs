use std::path::PathBuf;
use std::sync::{LazyLock};
use tracing::{error, debug};

pub struct FolderResolver {}

static ASSETS_DIR: LazyLock<Result<PathBuf, std::io::Error>> = LazyLock::new(|| {
    let mut base = dirs::data_dir()
        .ok_or_else(|| std::io::Error::other("Failed to locate data directory"))?;

    base.push("nova");
    base.push("assets");

    std::fs::create_dir_all(&base)?;

    debug!("Assets directory: {:?}", base);

    Ok(base)
});

static SESSION_DIR: LazyLock<Result<PathBuf, std::io::Error>> = LazyLock::new(|| {
    let mut base = dirs::data_dir()
        .ok_or_else(|| std::io::Error::other("Failed to locate data directory"))?;

    base.push("nova");
    base.push("auth");

    debug!("Session directory: {:?}", base);

    std::fs::create_dir_all(&base)?;
    Ok(base)
});

static LOG_DIR: LazyLock<Result<PathBuf, std::io::Error>> = LazyLock::new(|| {
    let mut base = dirs::data_dir()
        .ok_or_else(|| std::io::Error::other("Failed to locate data directory"))?;

    base.push("nova");
    base.push("logs");

    std::fs::create_dir_all(&base)?;

    debug!("Assets directory: {:?}", base);

    Ok(base)
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

    pub fn resolve_session_dir() -> PathBuf {
        match &*SESSION_DIR {
            Ok(path) => path.clone(),
            Err(err) => {
                error!("Failed to resolve session directory: {:?}", err);
                panic!("Failed to resolve session directory");
            }
        }
    }

    pub fn resolve_log_dir() -> PathBuf {
        match &*LOG_DIR {
            Ok(path) => path.clone(),
            Err(err) => {
                error!("Failed to resolve session directory: {:?}", err);
                panic!("Failed to resolve session directory");
            }
        }
    }
}
