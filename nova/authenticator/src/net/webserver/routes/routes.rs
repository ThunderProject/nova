use axum::Router;
use axum::routing::post;
use crate::net::webserver::routes::auth;

pub fn api_router() -> Router {
    Router::new()
        .route(
            "/login",
            post(auth::handle_login)
        )
}