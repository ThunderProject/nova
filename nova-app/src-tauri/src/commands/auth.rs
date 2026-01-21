pub(crate) use crate::auth_state::auth_state::AuthState;
use authenticated_command::authenticated_command;
use nova_auth::auth_service::*;
use std::sync::atomic;
use tauri::State;
use tracing::{debug, error, info, warn};
use nova_di::ioc;

#[tauri::command]
pub async fn signup(username: String, password: String) -> Result<(), String> {
    let auth = ioc::singleton::ioc().resolve::<AuthService>();

    match auth.signup(&username, &password).await {
        Ok(()) => Ok(()),
        Err(err) => {
            debug!("Signup failed: {err}");

            // Signup could fail after the account has been successfully created on the server.
            // (consider e.g., the scenario where the account is created on the server but the client fails to parse the response, etc)
            // Let's launch a background task to (try) delete the account in case the signup failed.
            let username_cleanup = username.clone();
            tokio::spawn(async move {
                let auth = ioc::singleton::ioc().resolve::<AuthService>();
                let cleanup_task = async {
                    //delete_account will fail if the account does not exist, we don't care about that
                    auth.delete_account(&username_cleanup).await
                };

                // What to do here? The account creation failed from the user's POV, but it might have succeeded from the server's POV.
                if let Err(_elapsed) = tokio::time::timeout(std::time::Duration::from_secs(10), cleanup_task).await {
                    warn!("Failed to delete account after signup failed.");
                }
            });

            // Do not provide any sensitive information in this error message because the frontend might display it to the user.
            let error_message = match err {
                LoginError::RateLimitReached => "Too many signup attempts. Please try again later.",
                LoginError::UserAlreadyExists => "User already exists. Please try another username",
                _ => "Failed to signup. Try again later."
            };

            Err(error_message.to_string())
        }
    }
}

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
        error!("Failed to logout. Reason: user not logged in.");
        return Err("Access denied.".to_string());
    }

    let auth = ioc::singleton::ioc().resolve::<AuthService>();
    if auth.logout().await {
        info!("User successfully logged out.");
        state.authenticated.store(true, atomic::Ordering::Relaxed);
    }
    Ok(())
}
