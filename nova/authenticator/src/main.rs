mod net;
mod services;
mod crypto;

use crate::net::webserver::webserver::WebServer;
use clap::Parser;
use mimalloc::MiMalloc;
use rustls::crypto::CryptoProvider;
use std::path::PathBuf;
use tracing::Level;

#[global_allocator]
static GLOBAL: MiMalloc = MiMalloc;

#[derive(Parser, Debug)]
#[command(author, version, about)]
pub struct CliArgs {
    #[arg(
        long,
        default_value_os_t = default_vault_config_path(),
        value_name = "FILE",
        help = "Path to the vault configuration file"
    )]
    pub vault_config_path: PathBuf,
}

fn default_vault_config_path() -> PathBuf  {
    PathBuf::from("src/crypto/.vault_config.toml")
}

fn init_logger() {
    tracing_subscriber::fmt()
        .with_max_level(Level::DEBUG)
        .init();
}

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    init_logger();
    WebServer::install_crypto_provider()?;

    let args = CliArgs::parse();
    let app = WebServer::new(args.vault_config_path).await;
    
    app.run().await?;
    Ok(())
}