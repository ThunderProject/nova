use serde::{Deserialize, Serialize, de::DeserializeOwned};
use zeroize::Zeroizing;

#[derive(Serialize)]
struct LoginRequest {
    username: String,
    password: Zeroizing<String>,
}

#[derive(Serialize)]
struct RefreshRequest {
    refresh_token: String
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

        let response = self.post::<LoginRequest, LoginResponse>(&url, &body).await?;
        Ok(response)
    }

    pub async fn refresh(&self, refresh_token: &str) -> anyhow::Result<LoginResponse>  {
        let url = format!("{}/refresh", self.base_url);

        let body = RefreshRequest {
            refresh_token: refresh_token.to_owned()
        };

        let response = self.post::<RefreshRequest, LoginResponse>(&url, &body).await?;
        Ok(response)
    }

    async fn post<Body: Serialize + ?Sized, Response: DeserializeOwned>(&self, url: &str, body: &Body) -> anyhow::Result<Response> {
        let response = self.http_client
            .post(url)
            .json(body)
            .send()
            .await?
            .error_for_status()?
            .json::<Response>()
            .await?;

        Ok(response)
    }
}
