use std::fs;
use std::path::PathBuf;

use keyring::Entry;
use nova_crypto::key_derivation::PASSPHRASE_LENGTH;
use nova_crypto::password_generator::{PasswordGenerator};
use nova_crypto::crypto::*;
use chacha20poly1305::XChaCha20Poly1305;
use nova_fs::folder_resolver::FolderResolver;
use serde::{Deserialize, Serialize};
use tracing::debug;

const SERVICE_NAME: &str = "com.nova.auth";
const USER_NAME: &str = "nova_token";
const AAD: &str = "nova_persist";
const SESSION_VERSION: &str = "1.0.0";

#[derive(Serialize, Deserialize)]
struct Session {
    session: SessionData,
    keys: SessionKeys,
}

#[derive(Serialize, Deserialize)]
struct SessionData {
    version: String,
    created_at: String,
}

#[derive(Serialize, Deserialize)]
struct SessionKeys {
    refresh: String,
}

pub struct SessionManager {}

impl SessionManager {
    pub fn persist_login(token: &str) -> anyhow::Result<()> {
        debug!("Attempting to persist login state...");

        let entry = Entry::new(SERVICE_NAME, USER_NAME)?;

        match entry.get_password() {
            Ok(password) => {
                debug!("Found existing keyring entry. Using stored passphrase.");
                SessionManager::persist_session(token, &password)?;
            }
            Err(_) => {
                debug!("No existing keyring entry found. Creating a new passphrase.");

                if let Some(password) = PasswordGenerator::default().generate(PASSPHRASE_LENGTH) {
                    SessionManager::store_keyring_password(&password)?;
                    SessionManager::persist_session(token, &password)?;
                }
                else {
                    debug!("Failed to generate encryption passphrase.");
                    anyhow::bail!("Failed to persist login state");
                }
            }
        }

        debug!("Login state successfully persisted.");
        Ok(())
    }

    pub fn load_session() -> anyhow::Result<String> {
        let file_path = SessionManager::session_path();

        if !file_path.exists() {
            debug!("Failed to load persisted session. Reason: file does not exist");
            anyhow::bail!("Failed to load session. File does not exist");
        }

        let file_content = fs::read_to_string(&file_path)?;
        let parsed: Session = toml::from_str(&file_content)?;

        let keyring_password = SessionManager::fetch_keyring_password()?;
        let token = decrypt_str::<XChaCha20Poly1305>(&parsed.keys.refresh, &keyring_password, AAD, None)?;

        debug!("Session loaded successfully");

        Ok(token)
    }

    pub fn remove_session() -> anyhow::Result<()> {
        let file_path = SessionManager::session_path();

        if file_path.exists() {
            match fs::remove_file(&file_path) {
                Ok(_) => {},
                Err(err) => {
                    debug!("Failed to remove session file. Reason: {err}");
                    anyhow::bail!(err);
                }
            }
            debug!("Removed session file: {:?}", &file_path);
        }
        else {
            debug!("No session file found. Nothing to remove.");
        }

        debug!("Session successfully removed.");
        Ok(())
    }

    fn store_keyring_password(password: &str) -> anyhow::Result<()> {
        Entry::new(SERVICE_NAME, USER_NAME)?.set_password(password)?;
        debug!("Stored password to keyring entry. service={SERVICE_NAME}, user={USER_NAME}");
        Ok(())
    }

    fn fetch_keyring_password() -> anyhow::Result<String> {
        debug!("Fetch password from keyring entry. service={SERVICE_NAME}, user={USER_NAME}");

        let password = Entry::new(SERVICE_NAME, USER_NAME)?.get_password()?;
        Ok(password)
    }

    fn persist_session(token: &str, password: &str) -> anyhow::Result<()> {
        let file_path = SessionManager::session_path();

        let encrypted = encrypt_str::<XChaCha20Poly1305>(token, password, AAD, None)?;

        let toml = Session {
            session: SessionData {
                version: SESSION_VERSION.to_string(),
                created_at: chrono::Utc::now().to_rfc3339()
            },
            keys: SessionKeys {
                refresh: encrypted
            },
        };

        let toml_string = toml::to_string(&toml)?;
        fs::write(&file_path, toml_string)?;

        debug!("Persist session saved to: {:?}", file_path);

        Ok(())
    }

    fn session_path() -> PathBuf {
        FolderResolver::resolve_session_dir().join("nova_session.toml")
    }
}
