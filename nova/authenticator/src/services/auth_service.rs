use argon2::{Argon2, PasswordHash, PasswordVerifier};
use serde::Serialize;
use thiserror::Error;
use tracing::{error, info};
use crate::services::auth_db::{AuthDb, AuthDbError};

#[derive(Debug, Serialize, Clone)]
pub struct AuthState {}
pub struct Auth {
    auth_db: AuthDb
}

#[derive(Debug, Error)]
pub enum LoginFailureReason {
    #[error("database error: {0}")]
    Database(#[from] AuthDbError),

    #[error("username conflict")]
    UsernameConflict,

    #[error("wrong username or password")]
    WrongCredentials,

    #[error("failed to parse password hash")]
    PasswordHashError,
}

impl Auth {
    pub async fn new() -> Self {
        Self {
            auth_db: AuthDb::new().await
        }
    }

    pub async fn login(&self, username: &String, password: &String) -> anyhow::Result<AuthState> {
        let usr = self.auth_db.fetch_user(username).await?;
        let pw_hash = PasswordHash::new(&usr.password).map_err(|err| {
            error!("Failed to create password hash: {err}");
            LoginFailureReason::PasswordHashError
        })?;

        Argon2::default().verify_password(password.as_ref(), &pw_hash)
            .map_err(|err| {
                error!("Failed to verify password: {err}");
                LoginFailureReason::WrongCredentials
            })?;

        info!("Successfully logged in as {username}");
        Ok(AuthState{})
    }
}