use arc_swap::ArcSwapOption;
use chrono::Duration;
use std::sync::Arc;
use std::sync::atomic::{AtomicBool, Ordering};
use thiserror::Error;
use tracing::{debug, error, info, warn};
use nova_api::authenticator_api::AuthenticatorApi;
use nova_rate_limit::fixed::RateLimiter;

use crate::session_manager::SessionManager;

#[derive(Clone)]
pub struct Tokens {
    pub access: String,
    pub refresh: String,
}

#[derive(Debug, Error)]
pub enum LoginError {
    #[error("Rate limit exceeded.")]
    RateLimitReached,

    #[error("Already logged in")]
    AlreadyLoggedIn,

    #[error("Failed to login: {0}")]
    FailedToLogin(String),

    #[error("Another login attempt completed first")]
    ConcurrentLogin,
}

pub struct AuthService {
    logged_in: AtomicBool,
    auth_api: AuthenticatorApi,
    tokens: ArcSwapOption<Tokens>,
    rate_limiter: parking_lot::Mutex<RateLimiter>,
}

impl Default for AuthService {
    fn default() -> Self {
        Self::new()
    }
}

impl AuthService {
    pub fn new() -> Self {
        Self {
            logged_in: AtomicBool::from(false),
            auth_api: AuthenticatorApi::new("https://127.0.0.1:5643".to_owned()),
            tokens: ArcSwapOption::empty(),
            rate_limiter: parking_lot::Mutex::new(RateLimiter::new(5, Duration::seconds(60))),
        }
    }

    pub async fn try_load_session(&self) -> Result<(), LoginError> {
        if self.logged_in.load(Ordering::Acquire) {
            //It does not make sense to load a session if we are already logged in
            return Err(LoginError::AlreadyLoggedIn);
        }

        let refresh_token = SessionManager::load_session()
            .map_err(|e| LoginError::FailedToLogin(e.to_string()))?;

        let response = self.auth_api.refresh(&refresh_token).await
            .map_err(|e| LoginError::FailedToLogin(e.to_string()))?;

        let result = self.logged_in.compare_exchange(false, true, Ordering::AcqRel, Ordering::Acquire);

        if result.is_err() {
            warn!("another login or load session attempt was made before this one completed, ignoring this one");
            return Err(LoginError::ConcurrentLogin);
        }

        self.tokens.store(
            Some(Arc::new(Tokens {
                access: response.access_token,
                refresh: response.refresh_token
            }))
        );

        Ok(())
    }

    pub async fn login(&self, username: &str, password: &str, keep_user_logged_in: bool) -> Result<(), LoginError> {
        if !self.rate_limiter.lock().try_acquire() {
            return Err(LoginError::RateLimitReached);
        }

        if self.logged_in.load(Ordering::Acquire) {
            return Err(LoginError::AlreadyLoggedIn);
        }

        let response = self.auth_api.login(username, password).await
            .map_err(|e| LoginError::FailedToLogin(e.to_string()))?;

        let result = self.logged_in.compare_exchange(false, true, Ordering::AcqRel, Ordering::Acquire);

        if result.is_err() {
            warn!("another login attempt was made before this one completed, ignoring this one");
            return Err(LoginError::ConcurrentLogin);
        }

        if keep_user_logged_in {
            match SessionManager::persist_login(&response.refresh_token) {
                Ok(_) => {
                    info!("Persistent session saved successfully.")
                },
                Err(err) => {
                    error!("Failed to persist login. User will have to login again if app is restarted. Error: {err}");
                }
            }
        }

        self.tokens.store(
            Some(Arc::new(Tokens {
                access: response.access_token,
                refresh: response.refresh_token
            }))
        );

        Ok(())
    }

    pub async fn logout(&self) -> bool {
        let result = self.logged_in.compare_exchange(true, false, Ordering::AcqRel, Ordering::Acquire);
        if result.is_ok() {
            self.tokens.store(None);
            return true;
        }
        false
    }
}
