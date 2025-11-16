use nova::ioc;
use nova::auth::auth_service::AuthService;
use tracing::{debug};

#[tauri::command]
pub async fn login(username: String, password: String) -> Result<(), String> {
    let auth = ioc::singleton::ioc().resolve::<AuthService>();

    match auth.login(&username, &password).await {
        Ok(()) => Ok(()),
        Err(err) => {
            debug!("Login failed: {err}");
            Err("Failed to sign in. Please check your username and password and try again.".to_string())
        }
    }
}