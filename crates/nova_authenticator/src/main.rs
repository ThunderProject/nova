mod net;
mod services;
mod crypto;
mod auth;

use crate::net::webserver::webserver::WebServer;
use clap::Parser;
use mimalloc::MiMalloc;
use std::path::{Path, PathBuf};
use tracing::Level;
use nova_di::ioc;
use crate::crypto::vault::Vault;

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

fn init_vault(vault_config_path: &Path) {
    let path = vault_config_path.to_path_buf();

    ioc::singleton::ioc().register(move || Vault::new(path.clone())
        .expect("Failed to initialize vault"));
}

async fn run_webserver() -> anyhow::Result<()> {
    WebServer::install_crypto_provider()?;
    let app = WebServer::new().await;

    app.run().await?;
    Ok(())
}

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    let args = CliArgs::parse();
    
    init_vault(&args.vault_config_path);
    init_logger();
    run_webserver().await
}