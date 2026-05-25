use axum::extract::State;
use axum::http::StatusCode;
use axum::Json;
use axum::response::IntoResponse;
use serde::{Deserialize, Serialize};
use tracing::{error, info};
use crate::net::webserver::webserver::AuthState;
use crate::services::auth_db::AuthDbError;
use crate::services::auth_service::LoginFailureReason;

#[derive(Debug, Deserialize, Serialize)]
pub struct LoginRequest {
    username: String,
    password: String,
}

#[derive(Debug, Deserialize, Serialize)]
pub struct LoginResponse {
    access_token: String,
    refresh_token: String,
}

#[derive(Debug, Deserialize, Serialize)]
pub struct RefreshTokensRequest {
    refresh_token: String,
}

pub async fn handle_signup(State(auth_state): State<AuthState>, Json(payload): Json<LoginRequest>) -> impl IntoResponse {
    match auth_state.auth_service.signup(&payload.username, &payload.password).await {
        Ok(()) => {
            info!("Successfully signed up user {}", payload.username);
            StatusCode::OK.into_response()
        }
        Err(e) => {
            error!("Failed to sign up user {}: {}", payload.username, e);

            match e {
                LoginFailureReason::Database(AuthDbError::UserAlreadyExists) => {
                    (
                        StatusCode::CONFLICT,
                        "User already exists".to_string()
                    ).into_response()
                }
                _ => {
                    (StatusCode::INTERNAL_SERVER_ERROR, format!("{e}")).into_response()
                }
            }
        }
    }
}

pub async fn handle_login(State(auth_state): State<AuthState>, Json(payload): Json<LoginRequest>) -> impl IntoResponse {
   match auth_state.auth_service.login(&payload.username, &payload.password).await {
       Ok(tokens) => {
           info!("Successfully logged in as {}", payload.username);

           let response = LoginResponse {
               access_token: tokens.access,
               refresh_token: tokens.refresh,
           };

           (StatusCode::OK, Json(response)).into_response()
       },
       Err(e) => {
           error!("Failed to login with username {}: {}", payload.username, e);
           (StatusCode::UNAUTHORIZED,  format!("{e}")).into_response()
       }
   }
}

pub async fn handle_refresh_tokens(State(auth_state): State<AuthState>, Json(payload): Json<RefreshTokensRequest>) -> impl IntoResponse {
    match auth_state.auth_service.refresh_tokens(&payload.refresh_token).await {
        Ok(tokens) => {
            info!("Successfully refreshed access token");

            let response = LoginResponse {
                access_token: tokens.access,
                refresh_token: tokens.refresh,
            };

            (StatusCode::OK, Json(response)).into_response()
        }
        Err(e) => {
            error!("Failed to refresh access token: {e}");
            (StatusCode::UNAUTHORIZED,  format!("{e}")).into_response()
        }
    }
}
