use axum::extract::State;
use axum::http::StatusCode;
use axum::Json;
use axum::response::IntoResponse;
use serde::{Deserialize, Serialize};
use tracing::{error, info};
use crate::net::webserver::webserver::AuthState;

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