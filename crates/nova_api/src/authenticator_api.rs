use reqwest::Certificate;
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

const SERVER_CERT: &str = "-----BEGIN CERTIFICATE-----
MIIB1DCCAVqgAwIBAgIUBlSxEB+6/TpwJSQF7HgnpIeCSHIwCgYIKoZIzj0EAwMw
HTEbMBkGA1UEAwwSbm92YSBhdXRoZW50aWNhdG9yMB4XDTI1MTEwOTEyMjQyOVoX
DTM1MTEwOTEyMjQyOVowHTEbMBkGA1UEAwwSbm92YSBhdXRoZW50aWNhdG9yMHYw
EAYHKoZIzj0CAQYFK4EEACIDYgAEfXRX0wqMujvog0GIsbcrcuXunj5l4D4INxbF
bUC6cDJnbn0BF6r2CeQqZyUxEEncHvCTYSg8LVcRPshq0vgqL/zCBGLotrOU3gVx
Jegk3n8CoUY2DiueIg0PVtaooXkJo1swWTAaBgNVHREEEzARhwR/AAABgglsb2Nh
bGhvc3QwDAYDVR0TAQH/BAIwADAOBgNVHQ8BAf8EBAMCB4AwHQYDVR0lBBYwFAYI
KwYBBQUHAwEGCCsGAQUFBwMCMAoGCCqGSM49BAMDA2gAMGUCMQDfsXLo4YzW3Pth
sRXSHbhUOpZ2xDBStZ+Wa8t6f54jbc91qwjVMrXK+zqJ/+FKv6sCMH2Qu9nS7rHQ
eQuQykYg+XUEeQo+K7PZgKCsClTN35mm8GLDijWo6nIeE4hExH/WTA==
-----END CERTIFICATE-----";

impl AuthenticatorApi {
    pub fn new(base_url: String) -> Self {
        let cert = Certificate::from_pem(SERVER_CERT.as_bytes())
            .expect("Failed to parse self-signed certificate");

        let http_client = reqwest::Client::builder()
            .add_root_certificate(cert)
            .build()
            .expect("Failed to build reqwest client");

        Self {
            base_url,
            http_client,
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
