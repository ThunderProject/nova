use axum::{Json, Router};
use axum::response::IntoResponse;
use axum::routing::{get, post};
use serde::{Deserialize, Serialize};
use thiserror::Error;
use tracing::{error, info};

#[derive(Debug, Error)]
pub enum WebServerError {
    #[error("failed to bind to address {0}")]
    BindError(String, #[source] std::io::Error),

    #[error("server runtime error")]
    ServeError(#[source] std::io::Error),
}

pub struct WebServer;
#[derive(Clone, Copy)]
struct Ports {
    http: u16,
    https: u16,
}

#[derive(Debug, Deserialize, Serialize)]
struct LoginRequest {
    username: String,
    password: String,
}

impl WebServer {
    pub async fn new() -> anyhow::Result<Self> {
        Ok(Self {})
    }
    pub async fn run(&self) -> anyhow::Result<()> {
        let app = Router::new()
            .route("/", get(|| async { "Hello World!" }))
            .route("/login", post(handle_login));

        let addr = "127.0.0.1:3000";
        info!("Binding webserver to {addr}");

        let listener = tokio::net::TcpListener::bind(addr)
            .await
            .map_err(|err| {
                error!("Failed to bind to {addr}: {err}");
                WebServerError::BindError(addr.to_string(), err)
            })?;

        info!("Server listening on http://{addr}");

        axum::serve(listener, app)
            .await
            .map_err(|err| {
                error!("Server encountered an error: {err}");
                WebServerError::ServeError(err)
            })?;

        Ok(())
    }
}

async fn handle_login(Json(payload): Json<LoginRequest>) -> impl IntoResponse {
    info!("Login attempt: user={}", payload.username);
    Json("response")
}