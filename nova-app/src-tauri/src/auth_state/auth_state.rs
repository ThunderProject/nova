use std::sync::atomic;

/// A simple local auth flag.
///
/// This boolean only controls whether the frontend is allowed to call
/// certain Tauri commands. It does *not* provide real security, just a lightweight
/// check to prevent calling certain commands before the user has logged in.
/// (basically restricting the capabilities of the application)
///
/// All sensitive operations still require valid access/refresh tokens,
/// which are always verified by the server. Even if this check were
/// bypassed, no authenticated data can be accessed without proper tokens.
#[derive(Default)]
pub struct AuthState {
    pub(crate) authenticated: atomic::AtomicBool,
}