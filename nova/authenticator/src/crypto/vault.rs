use serde::Deserialize;
use thiserror::Error;
use zeroize::Zeroizing;

#[derive(Debug, Error)]
pub enum VaultError {
    #[error("vault cli command failed: {0}")]
    CommandFailed(String),

    #[error("failed to fetch field `{0}`: {1}")]
    FetchFieldFailed(String, String),

    #[error("utf-8 error")]
    Utf8Error(#[from] std::string::FromUtf8Error),

    #[error("io error: {0}")]
    IoError(#[from] std::io::Error),

    #[error("toml parse error: {0}")]
    TomlError(#[from] toml::de::Error),

    #[error("serde json error: {0}")]
    JsonError(#[from] serde_json::Error),
}

pub struct VaultSecrets {
    pub password: Zeroizing<String>,
    pub pepper: Zeroizing<String>,
}

#[derive(Debug, Deserialize)]
struct VaultField {
    name: String,
    value: Option<String>,
    #[serde(default)]
    _type: i32,
    #[serde(rename = "linkedId")]
    _linked_id: Option<String>,
}

#[derive(Debug, Deserialize)]
struct VaultItem {
    fields: Option<Vec<VaultField>>,
}

#[derive(Deserialize)]
pub struct Vault {
    #[serde(rename = "vault_session")]
    session_id: String,

    #[serde(rename = "vault_cli_path")]
    cli_path: String,

    #[serde(rename = "vault_item_name")]
    item_name: String,
}

impl Vault {
    pub fn new<Path: AsRef<std::path::Path>>(config_path: &Path) -> Result<Self, VaultError> {
        let contents = std::fs::read_to_string(config_path)?;
        let this = toml::from_str::<Self>(&contents)?;
        Ok(this)
    }

    pub async fn logout(&self) -> Result<(), VaultError> {
        self.execute_command(&["logout"]).await?;
        Ok(())
    }

    pub async fn fetch_password(&self) -> Result<String, VaultError> {
        self.execute_command(&[
            "get", "password", &self.item_name, "--session", &self.session_id
        ]).await
    }

    pub async fn fetch_pepper(&self) -> Result<String, VaultError> {
        self.fetch_field("pepper").await
    }

    pub async fn fetch_secrets(&self) -> Result<VaultSecrets, VaultError> {
        let (password, pepper) = tokio::try_join!(
            self.fetch_password(),
            self.fetch_pepper(),
        )?;

        Ok(VaultSecrets {
            password: Zeroizing::new(password),
            pepper: Zeroizing::new(pepper),
        })
    }

    async fn fetch_field(&self, field_name: &str) -> Result<String, VaultError> {
        let json = self.execute_command(&[
            "get", "item", &self.item_name, "--session", &self.session_id
        ]).await?;

        let item: VaultItem = serde_json::from_str(&json)?;

        let field = item
            .fields
            .as_ref()
            .and_then(|fields| { fields.iter().find(|field| field.name == field_name) })
            .and_then(|field| field.value.clone());

        match field {
            Some(value) => Ok(value),
            None => Err(VaultError::FetchFieldFailed(
                field_name.to_string(),
                "missing in item".into(),
            )),
        }
    }

    async fn execute_command(&self, args: &[&str]) -> Result<String, VaultError> {
        let mut cmd = tokio::process::Command::new(&self.cli_path);
        cmd.args(args);

        let out = cmd.output().await?;

        if !out.status.success() {
            return Err(VaultError::CommandFailed(format!(
                "command failed with exit {:?}",
                out.status.code()
            )));
        }

        let out = String::from_utf8(out.stdout)?;
        Ok(out)
    }
}