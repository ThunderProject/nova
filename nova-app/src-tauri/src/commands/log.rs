use tracing::{debug, error, info, warn};

#[tauri::command]
pub fn log(level: String, msg: String) {
    match level.as_str() {
        "debug" => debug!(target: "nova-web", "{}", msg),
        "info" => info!(target: "nova-web", "{}", msg),
        "warn" => warn!(target: "nova-web", "{}", msg),
        "error" => error!(target: "nova-web", "{}", msg),
        "fatal" => error!(target: "nova-web", "{}", msg),
        _ => {},
    }
}
