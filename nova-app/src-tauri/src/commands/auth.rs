pub(crate) use crate::auth_state::auth_state::AuthState;
use authenticated_command::authenticated_command;
use nova_auth::auth_service::*;
use std::sync::atomic;
use tauri::State;
use tracing::debug;
use nova_di::ioc;

#[tauri::command]
pub async fn login(username: String, password: String, keep_user_logged_in: bool, state: State<'_, AuthState>) -> Result<(), String> {
    let auth = ioc::singleton::ioc().resolve::<AuthService>();

    match auth.login(&username, &password, keep_user_logged_in).await {
        Ok(()) => {
            state.authenticated.store(true, atomic::Ordering::Relaxed);
            Ok(())
        }
        Err(err) => {
            debug!("Login failed: {err}");

            // Do not provide any sensitive information in this error message because the frontend might display it to the user.
            let error_message = if let LoginError::RateLimitReached = err {
                "Too many login attempts. Please try again later."
            } else {
                "Failed to login. Please check your username and password and try again."
            };

            Err(error_message.to_string())
        }
    }
}

#[authenticated_command]
pub async fn is_authenticated() -> Result<bool, String> {
    Ok(true)
}

#[tauri::command]
pub async fn logout(state: State<'_, AuthState>) -> Result<(), String> {
    if !state.authenticated.load(atomic::Ordering::Relaxed) {
        return Err("Access denied.".to_string());
    }

    let auth = ioc::singleton::ioc().resolve::<AuthService>();
    if auth.logout().await {
        state.authenticated.store(true, atomic::Ordering::Relaxed);
    }
    Ok(())
}
