use std::sync::Arc;
use std::sync::atomic::{AtomicBool, Ordering};
use arc_swap::ArcSwapOption;
use tracing::warn;
use crate::api::authenticator_api::AuthenticatorApi;

#[derive(Clone)]
pub struct Tokens {
    pub access: String,
    pub refresh: String,
}

pub struct AuthService {
    logged_in: AtomicBool,
    auth_api: AuthenticatorApi,
    tokens: ArcSwapOption<Tokens>
}

impl AuthService {
    pub fn new() -> Self {
        Self {
            logged_in: AtomicBool::from(false),
            auth_api: AuthenticatorApi::new("https://127.0.0.1:5643".to_owned()),
            tokens: ArcSwapOption::empty(),
        }
    }

    pub async fn login(&self, username: &str, password: &str) -> anyhow::Result<()> {
        if self.logged_in.load(Ordering::Acquire) {
            anyhow::bail!("Already logged in");
        }

        let response= self.auth_api.login(username, password).await?;

        let result = self.logged_in.compare_exchange(
            false,
            true,
            Ordering::AcqRel,
            Ordering::Acquire
        );

        if result.is_err() {
            warn!("another login attempt was made before this one completed, ignoring this one");
            anyhow::bail!("Failed to login");
        }

        self.tokens.store(
            Some(Arc::new(Tokens {
                access: response.access_token,
                refresh: response.refresh_token
            }))
        );

        Ok(())
    }
}