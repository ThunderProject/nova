use serde::{Deserialize, Serialize};
use zeroize::Zeroizing;

#[derive(Serialize)]
struct LoginRequest {
    username: String,
    password: Zeroizing<String>,
}

#[derive(Debug, Deserialize)]
pub struct LoginResponse {
    pub access_token: String,
    pub refresh_token: String,
}

pub struct AuthenticatorApi {
    base_url: String,
    http_client: reqwest::Client,
}

impl AuthenticatorApi {
    pub fn new(base_url: String) -> Self {
        Self {
            base_url,
            http_client: reqwest::Client::new(),
        }
    }
    pub async fn login(&self, username: &str, password: &str) -> anyhow::Result<LoginResponse> {
        let url = format!("{}/login", self.base_url);

        let zeroized_pw = Zeroizing::new(password.to_owned());

        let body = LoginRequest {
            username: username.to_owned(),
            password: zeroized_pw.clone(),
        };

        let response = self.http_client
            .post(&url)
            .json(&body)
            .send()
            .await?
            .error_for_status()?
            .json::<LoginResponse>()
            .await?;

        Ok(response)
    }
}