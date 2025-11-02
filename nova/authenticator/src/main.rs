mod net;
mod services;

use mimalloc::MiMalloc;
use tracing::Level;
use crate::net::webserver::webserver::WebServer;

#[global_allocator]
static GLOBAL: MiMalloc = MiMalloc;

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    tracing_subscriber::fmt()
        .with_max_level(Level::DEBUG)
        .init();
    
    let app = WebServer::new().await?;
    app.run().await?;
    Ok(())
}
