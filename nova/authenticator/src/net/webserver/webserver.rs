use std::net::SocketAddr;
use std::path::PathBuf;
use axum::{Json, Router};
use axum::handler::HandlerWithoutStateExt;
use axum::http::{StatusCode, Uri};
use axum::http::uri::Authority;
use axum::response::{IntoResponse, Redirect};
use axum::routing::{get, post};
use axum_extra::extract::Host;
use base64::Engine;
use base64::prelude::BASE64_STANDARD;
use chacha20poly1305::XChaCha20Poly1305;
use serde::{Deserialize, Serialize};
use thiserror::Error;
use tracing::{debug, error, info, warn};
use crate::crypto::crypto::{decrypt, decrypt_str, CryptoAlgo};
use crate::crypto::key_derivation::{KeyDerivation, SALT_LEN};
use crate::crypto::vault::{Vault, VaultSecrets};
use pkcs8::{DecodePrivateKey, LineEnding, SecretDocument};
use axum_server::tls_rustls::RustlsConfig;
use tracing_subscriber::fmt::writer::EitherWriter::A;
use zeroize::{Zeroize, Zeroizing};

const AAD: &str = "nova-config-v1";

#[derive(Debug, Error)]
pub enum WebServerError {
    #[error("failed to bind to address {0}")]
    BindError(String, #[source] std::io::Error),

    #[error("server runtime error")]
    ServeError(#[source] std::io::Error),
}

pub struct WebServer {
    vault_config: PathBuf,
    ports: Ports
}

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

#[derive(Debug, Deserialize)]
pub struct TlsConfig {
    pub server_key_password: Zeroizing<String>,
    pub server_key_pem: Zeroizing<String>,
    pub server_certificate: String,
}

impl WebServer {
    pub async fn new(vault_config: PathBuf) -> Self {
        Self {
            vault_config,
            ports: Ports {
                http: 24982,
                https: 5643
            }
        }
    }
    pub async fn run(&self) -> anyhow::Result<()> {
        debug!("Starting webserver");

        let tls_cfg = self.load_tls_config().await?;

        let app = Router::new()
            .route("/", get(|| async { "Hello World!" }))
            .route("/login", post(handle_login));

        let addr = SocketAddr::from(([127, 0, 0, 1], self.ports.https));
        debug!("Binding webserver to {addr}");

        let server = axum_server::bind_rustls(addr, tls_cfg)
            .serve(app.into_make_service());

        info!("Webserver listening on {}", addr.to_string());

        // redirect http to https
        tokio::spawn(Self::tls_redirect(self.ports));

        server.await
            .map_err(
                |err| {
                    error!("Failed to bind to {addr}: {err}");
                    WebServerError::BindError(addr.to_string(), err)
                }
            )?;

        Ok(())
    }

    async fn load_tls_config(&self) -> anyhow::Result<RustlsConfig> {
        //there is no security issue with embedding this string in the binary since it's encrypted
        let encrypted = include_str!("../../tls_config.txt");
        let decoded_bytes =BASE64_STANDARD.decode(encrypted)?;

        if decoded_bytes.len() < SALT_LEN + XChaCha20Poly1305::NONCE_SIZE {
            error!("Encrypted file too small/corrupted");
        }

        let vault = Vault::new(&self.vault_config)?;
        let VaultSecrets { password, pepper } = vault.fetch_secrets().await?;
        let pepper_bytes = pepper.as_bytes();

        let decoded = String::from_utf8(decoded_bytes)?;
        let plaintext = decrypt_str::<XChaCha20Poly1305>(&decoded, &password, AAD, Some(pepper_bytes))?;

        let cfg: TlsConfig = toml::from_str(&plaintext)?;
        let dec_pem = Self::decrypt_pkcs8_pem(&cfg.server_key_pem, &cfg.server_key_password)?;

        let config = RustlsConfig::from_pem(
            cfg.server_certificate.into_bytes(),
            dec_pem.into_bytes(),
        ).await?;

        Ok(config)
    }

    fn decrypt_pkcs8_pem(pem: &str, password: impl AsRef<[u8]>) -> anyhow::Result<String> {
        let secret_doc = SecretDocument::from_pkcs8_encrypted_pem(pem, password)?;
        let pem = secret_doc.to_pem("PRIVATE KEY", LineEnding::LF)?;
        Ok(pem.to_string())
    }

    async fn tls_redirect(ports: Ports) -> anyhow::Result<()> {
        fn make_https(host: &str, uri: Uri, https_port: u16) -> anyhow::Result<Uri> {
            let mut parts = uri.into_parts();
            parts.scheme = Some(axum::http::uri::Scheme::HTTPS);

            if parts.path_and_query.is_none() {
                parts.path_and_query = Some("/".parse()?);
            }

            let authority: Authority = host.parse()?;
            let bare_host = authority.host();

            parts.authority = Some(format!("{bare_host}:{https_port}").parse()?);
            Ok(Uri::from_parts(parts)?)
        }

        let redirect = move |Host(host): Host, uri: Uri| async move {
            match make_https(&host, uri, ports.https) {
                Ok(uri) => {
                    let uri_str = uri.to_string();
                    debug!("Redirecting {host} to {uri_str}");
                    Ok(Redirect::permanent(&uri_str))
                }
                Err(error) => {
                    warn!("Failed to convert URI to HTTPS: {error}");
                    Err(StatusCode::BAD_REQUEST)
                }
            }
        };

        let addr = SocketAddr::from(([127, 0, 0, 1], ports.http));
        let listener = tokio::net::TcpListener::bind(addr).await?;

        info!("Redirecting HTTP to HTTPS on {}", addr.to_string());

        if let Err(err) = axum::serve(listener, redirect.into_make_service()).await {
            error!("http to https redirect server failed: {err}");
            info!("Only https on port {} will remain available", ports.https);

            return Err(anyhow::anyhow!(""));
        }

        Ok(())
    }
}

async fn handle_login(Json(payload): Json<LoginRequest>) -> impl IntoResponse {
    info!("Login attempt: user={}", payload.username);
    Json("response")
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test() {

    }
}