use std::sync::atomic;
use nova::ioc;
use nova::auth::auth_service::{AuthService, LoginError};
use tauri::State;
use tracing::{debug};
use crate::auth_state::auth_state::AuthState;

#[tauri::command]
pub async fn login(username: String, password: String, state: State<'_, AuthState>) -> Result<(), String> {
    let auth = ioc::singleton::ioc().resolve::<AuthService>();

    match auth.login(&username, &password).await {
        Ok(()) => {
            state.authenticated.store(true, atomic::Ordering::Relaxed);
            Ok(())
        } ,
        Err(err) => {
            debug!("Login failed: {err}");

            // Do not provide any sensitive information in this error message because the frontend might display it to the user.
            let error_message = if let LoginError::RateLimitReached = err {
                "Too many login attempts. Please try again later."
            }
            else {
                "Failed to login. Please check your username and password and try again."
            };

            Err(error_message.to_string())
        }
    }
}