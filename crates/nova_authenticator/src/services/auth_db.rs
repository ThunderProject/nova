use aead::OsRng;
use argon2::{Argon2, PasswordHasher};
use sqlx::PgPool;
use thiserror::Error;
use tracing::{error, info};
use argon2::password_hash::SaltString;

#[allow(dead_code, unused)]
pub struct User {
    pub username: String,
    pub password_hash: String,
}

pub struct AuthDb {
    db_pool: Option<PgPool>
}

const DATABASE_URL: &str = "postgres://postgres:admin@localhost/heimdall";

#[derive(Debug, Error)]
pub enum AuthDbError {
    #[error("failed to connect to database backend")]
    DatabaseConnectionFailed,

    #[error("failed to execute SQL query")]
    QueryExecutionFailed,

    #[error("user not found")]
    UserNotFound,

    #[error("user already exists")]
    UserAlreadyExists,

    #[error("failed to hash password")]
    PasswordHashError,
}

impl AuthDb {
    pub async fn new() -> Self {
        Self {
            db_pool: PgPool::connect(DATABASE_URL).await.ok(),
        }
    }

    pub async fn fetch_user(&self, username: &String) -> Result<User, AuthDbError> {
        info!("Trying to fetch user with username \"{username}\" from database");

        let pool = self.db_pool.as_ref().ok_or(AuthDbError::DatabaseConnectionFailed)?;

        let row = sqlx::query_as("SELECT username, password FROM users WHERE username = $1")
            .bind(username)
            .fetch_optional(pool)
            .await
            .map_err(|err| {
                error!("Failed to execute SQL query. Reason: {err}");
                AuthDbError::QueryExecutionFailed
            })?;

        match row {
            Some((stored_user, stored_password)) => {
                info!("Successfully fetched user from database!");
                Ok(User { username: stored_user, password_hash: stored_password })
            }
            None => {
                error!("User with username \"{username}\" not found");
                Err(AuthDbError::UserNotFound)
            }
        }
    }

    pub async fn create_user(&self, username: &str, password: &str) -> Result<(), AuthDbError> {
        info!("Trying to create new user \"{}\"", username);

        let pool = self.db_pool.as_ref().ok_or(AuthDbError::DatabaseConnectionFailed)?;

        let salt = SaltString::generate(&mut OsRng);
        let argon2 = Argon2::default();

        let pw_hash = argon2
            .hash_password(password.as_bytes(), &salt)
            .map_err(|err| {
                error!("Failed to create password hash: {err}");
                AuthDbError::PasswordHashError
            })?
            .to_string();

        let result = sqlx::query("INSERT INTO users (username, password) VALUES ($1, $2)")
            .bind(username)
            .bind(&pw_hash)
            .execute(pool)
            .await;

        match result {
            Ok(_) => {
                info!("Successfully created user \"{}\"", username);
                Ok(())
            }
            Err(err) => {
                // Check for unique constraint violation (Postgres error code 23505)
                if let Some(db_err) = err.as_database_error() && db_err.code().as_deref() == Some("23505") {
                    error!("User \"{}\" already exists", username);
                    return Err(AuthDbError::UserAlreadyExists);
                }

                error!("Failed to execute SQL query when inserting user. Reason: {err}");
                Err(AuthDbError::QueryExecutionFailed)
            }
        }
    }
}
