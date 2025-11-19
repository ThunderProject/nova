use std::sync::atomic;
use nova::ioc;
use nova::auth::auth_service::AuthService;
use tauri::State;
use tracing::{debug};
use crate::auth_state::auth_state::AuthState;

#[tauri::command]
pub async fn login(username: String, password: String, state: State<'_, AuthState>) -> Result<(), ()> {
    let auth = ioc::singleton::ioc().resolve::<AuthService>();

    match auth.login(&username, &password).await {
        Ok(()) => {
            state.authenticated.store(true, atomic::Ordering::Relaxed);
            Ok(())
        } ,
        Err(err) => {
            debug!("Login failed: {err}");
            Err(())
        }
    }
}