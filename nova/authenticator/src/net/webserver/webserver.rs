use crate::crypto::crypto::{decrypt_str, CryptoAlgo};
use crate::crypto::key_derivation::SALT_LEN;
use crate::crypto::vault::{Vault, VaultSecrets};
use crate::net::webserver::routes;
use crate::services::auth_service::Auth;
use axum::extract::FromRef;
use axum::handler::HandlerWithoutStateExt;
use axum::http::uri::Authority;
use axum::http::{StatusCode, Uri};
use axum::response::Redirect;
use axum_extra::extract::Host;
use axum_server::tls_rustls::RustlsConfig;
use chacha20poly1305::XChaCha20Poly1305;
use pkcs8::{DecodePrivateKey, LineEnding, SecretDocument};
use rustls::crypto::CryptoProvider;
use serde::Deserialize;
use std::net::SocketAddr;
use std::sync::Arc;
use thiserror::Error;
use tracing::{debug, error, info, warn};
use zeroize::Zeroizing;
use crate::ioc::singleton::ioc;

const AAD: &str = "nova-config-v1";

#[derive(Debug, Error)]
pub enum WebServerError {
    #[error("failed to bind to address {0}")]
    BindError(String, #[source] std::io::Error),
}

#[derive(Clone)]
pub struct AppState {
    pub auth_service: Arc<Auth>
}

#[derive(Clone)]
pub struct AuthState {
    pub auth_service: Arc<Auth>,
}

impl FromRef<Arc<AppState>> for AuthState {
    fn from_ref(state: &Arc<AppState>) -> Self {
        AuthState {
            auth_service: state.auth_service.clone(),
        }
    }
}

pub struct WebServer {
    pub auth_service: Arc<Auth>,
    ports: Ports,
}

#[derive(Clone, Copy)]
struct Ports {
    http: u16,
    https: u16,
}

#[derive(Debug, Deserialize)]
pub struct TlsConfig {
    pub server_key_password: Zeroizing<String>,
    pub server_key_pem: Zeroizing<String>,
    pub server_certificate: String,
}

impl WebServer {
    pub async fn new() -> Self {
        Self {
            auth_service: Arc::new(Auth::new().await),
            ports: Ports {
                http: 24982,
                https: 5643,
            },
        }
    }

    pub fn install_crypto_provider() -> anyhow::Result<()> {
        CryptoProvider::install_default(rustls::crypto::aws_lc_rs::default_provider())
            .map_err(|_| anyhow::anyhow!("Failed to install crypto provider"))?;
        Ok(())
    }

    pub async fn run(&self) -> anyhow::Result<()> {
        debug!("Starting webserver");

        let tls_cfg = self.load_tls_config().await?;

        let state = Arc::new(AppState {
            auth_service: self.auth_service.clone()
        });

        let router = routes::routes::api_router(state);

        let addr = SocketAddr::from(([127, 0, 0, 1], self.ports.https));
        debug!("Binding webserver to {addr}");

        let server = axum_server::bind_rustls(addr, tls_cfg).serve(router.into_make_service());

        info!("Webserver listening on {}", addr.to_string());

        // redirect http to https
        tokio::spawn(Self::tls_redirect(self.ports));

        server.await.map_err(|err| {
            error!("Failed to bind to {addr}: {err}");
            WebServerError::BindError(addr.to_string(), err)
        })?;

        Ok(())
    }

    async fn load_tls_config(&self) -> anyhow::Result<RustlsConfig> {
        //there is no security issue with embedding this string in the binary since it's encrypted
        let encrypted = include_str!("../../tls_config.txt");

        if encrypted.len() < SALT_LEN + XChaCha20Poly1305::NONCE_SIZE {
            error!("Encrypted file too small/corrupted");
        }

        let VaultSecrets { password, pepper } = ioc().resolve::<Vault>().fetch_tls_secrets().await?;
        let pepper_bytes = pepper.as_bytes();

        let plaintext = decrypt_str::<XChaCha20Poly1305>(encrypted, &password, AAD, Some(pepper_bytes))?;

        let cfg: TlsConfig = toml::from_str(&plaintext)?;
        let dec_pem = Self::decrypt_pkcs8_pem(&cfg.server_key_pem, &cfg.server_key_password)?;

        let config = RustlsConfig::from_pem(cfg.server_certificate.into_bytes(), dec_pem.into_bytes()).await?;

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