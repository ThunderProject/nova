use sqlx::PgPool;
use thiserror::Error;
use tracing::{error, info};

pub struct User {
    pub username: String,
    pub password: String,
}

pub struct AuthDb {
    db_pool: Option<PgPool>
}

#[derive(Debug, Error)]
pub enum AuthDbError {
    #[error("failed to connect to database backend")]
    DatabaseConnectionFailed,

    #[error("failed to execute SQL query")]
    QueryExecutionFailed,

    #[error("user not found")]
    UserNotFound,
}

impl AuthDb {
    pub async fn new() -> Self {
        Self {
            db_pool: PgPool::connect("postgres://postgres:admin@localhost/heimdall").await.ok(),
        }
    }

    pub async fn fetch_user(&self, username: &String) -> anyhow::Result<User, AuthDbError> {
        info!("Trying to fetch user with username \"{username}\" from database");

        let pool = self.db_pool.as_ref().ok_or(AuthDbError::DatabaseConnectionFailed)?;

        let row: Option<(String, String)> = sqlx::query_as("SELECT username, password FROM users WHERE username = $1")
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
                Ok(User { username: stored_user, password: stored_password })
            }
            None => {
                error!("User with username \"{username}\" not found");
                Err(AuthDbError::UserNotFound)
            }
        }
    }
}