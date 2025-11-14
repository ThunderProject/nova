use axum::Json;
use axum::response::IntoResponse;
use serde::{Deserialize, Serialize};
use tracing::info;

#[derive(Debug, Deserialize, Serialize)]
pub struct LoginRequest {
    username: String,
    password: String,
}
pub async fn handle_login(Json(payload): Json<LoginRequest>) -> impl IntoResponse {
    info!("Login attempt: user={}", payload.username);
    Json("response")
}