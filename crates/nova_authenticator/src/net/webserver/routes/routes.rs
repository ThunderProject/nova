use std::sync::Arc;
use axum::Router;
use axum::routing::post;
use crate::net::webserver::routes::auth;
use crate::net::webserver::webserver::AppState;

pub fn api_router(state: Arc<AppState>) -> Router {
    Router::new()
        .route(
            "/login",
            post(auth::handle_login).with_state(state.clone())
        )
}
