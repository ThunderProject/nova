use std::fs;

use keyring::Entry;
use nova_crypto::key_derivation::PASSPHRASE_LENGTH;
use nova_crypto::password_generator::{PasswordGenerator};
use nova_crypto::crypto::*;
use chacha20poly1305::XChaCha20Poly1305;
use nova_fs::folder_resolver::FolderResolver;
use serde::Serialize;
use tracing::debug;

const SERVICE_NAME: &str = "com.nova.auth";
const USER_NAME: &str = "nova_token";
const AAD: &str = "nova_persist";
const SESSION_VERSION: &str = "1.0.0";

#[derive(Serialize)]
struct Session {
    session: SessionData,
    keys: SessionKeys,
}

#[derive(Serialize)]
struct SessionData {
    version: String,
    created_at: String,
}

#[derive(Serialize)]
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
                    SessionManager::create_entry(&password)?;
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

    fn create_entry(password: &str) -> anyhow::Result<()> {
        Entry::new(SERVICE_NAME, USER_NAME)?.set_password(password)?;
        debug!("Keyring entry created. service={SERVICE_NAME}, user={USER_NAME}");
        Ok(())
    }

    fn persist_session(token: &str, password: &str) -> anyhow::Result<()> {
        let session_dir = FolderResolver::resolve_session_dir();
        let file_path = session_dir.join("nova_session.toml");

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

}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn create_entry() {

        match SessionManager::persist_login("jaogren") {
            Ok(_) => println!("Successs persisting login"),
            Err(e) => println!("Faield to persist login: {e}")
        }
    }
}
