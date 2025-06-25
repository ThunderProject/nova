use tracing::{debug, error, info};
use tracing::log::warn;

#[tauri::command]
pub fn log(level: String, msg: String) {
    match level.as_str() {
        "debug" => debug!("{}", msg),
        "info" => info!("{}", msg),
        "warn" => warn!("{}", msg),
        "error" => error!("{}", msg),
        "fatal" => error!("{}", msg),
        _ => {},  
    }
}