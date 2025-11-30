use argon2::{Argon2, PasswordHash, PasswordVerifier};
use thiserror::Error;
use tracing::{error};
use tracing::log::debug;
use nova_di::ioc::singleton::ioc;
use crate::{auth};
use crate::auth::jwt::JwtTokens;
use crate::crypto::vault::{Vault, VaultError};
use crate::services::auth_db::{AuthDb, AuthDbError};

pub struct Auth {
    auth_db: AuthDb,
}

#[derive(Debug, Error)]
pub enum LoginFailureReason {
    #[error("vault error: {0}")]
    VaultError(#[from] VaultError),

    #[error("database error: {0}")]
    Database(#[from] AuthDbError),

    #[error("wrong username or password")]
    WrongCredentials,

    #[error("failed to parse password hash")]
    PasswordHashError,

    #[error("An internal login error occurred. Please try again later.")]
    InternalError,
}

#[derive(Debug, Error)]
pub enum RefreshFailureReason {
    #[error("vault error: {0}")]
    VaultError(#[from] VaultError),

    #[error("refresh denied: invalid token")]
    InvalidToken,

    #[error("internal refresh failure")]
    InternalError,
}

impl Auth {
    pub async fn new() -> Self {
        Self {
            auth_db: AuthDb::new().await,
        }
    }

    pub async fn login(&self, username: &String, password: &String) -> Result<JwtTokens, LoginFailureReason> {
        let usr = self.auth_db.fetch_user(username).await?;
        let pw_hash = PasswordHash::new(&usr.password)
            .map_err(|err| {
                error!("Failed to create password hash: {err}");
                LoginFailureReason::PasswordHashError
            })?;

        match Argon2::default().verify_password(password.as_ref(), &pw_hash) {
            Ok(_) => {
                debug!("Successfully verified password for user {username}");

                let vault = ioc().resolve::<Vault>();
                let jwt_secrets = vault.fetch_jwt_secrets().await?;

                let jwt = auth::jwt::Jwt::new(&jwt_secrets.public_key, &jwt_secrets.private_key);
                if jwt.is_none() {
                    return Err(LoginFailureReason::InternalError);
                }

                let subject = username;

                match jwt.unwrap().create_tokens(subject) {
                    Some(token) => {
                        debug!("Successfully logged in as {username}");
                        Ok(token)
                    },
                    None => {
                        error!("Failed to logged in as {username}: failed to create jwt tokens");
                        Err(LoginFailureReason::InternalError)
                    }
                }
            }
            Err(e) => {
                error!("Failed to verify password for user {username}: {e}");
                Err(LoginFailureReason::WrongCredentials)
            }
        }
    }

    pub async fn refresh_tokens(&self, refresh_token: &str) -> Result<JwtTokens, RefreshFailureReason> {
        let vault = ioc().resolve::<Vault>();
        let jwt_secrets = vault.fetch_jwt_secrets().await?;

        let jwt = auth::jwt::Jwt::new(&jwt_secrets.public_key, &jwt_secrets.private_key)
            .ok_or(RefreshFailureReason::InternalError)?;

        let decoded = jwt.decode_token(refresh_token)
            .ok_or(RefreshFailureReason::InvalidToken)?;

        if !jwt.verify_decoded(&decoded, "refresh") {
            return Err(RefreshFailureReason::InvalidToken);
        }

        let new_tokens = jwt.create_tokens(&decoded.claims.sub).ok_or(RefreshFailureReason::InternalError)?;
        Ok(new_tokens)
    }
}
