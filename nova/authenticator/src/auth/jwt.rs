use chrono::{Duration, Utc};
use jsonwebtoken::{Algorithm, DecodingKey, EncodingKey, Header, Validation};
use serde::{Deserialize, Serialize};
use tracing::{debug, error};

struct AccessToken;
struct RefreshToken;

trait TokenBehavior {
    fn issuer(base_issuer: &str) -> String;
    fn expiry(jwt: &Jwt) -> Duration;
}

impl TokenBehavior for AccessToken {
    fn issuer(base_issuer: &str) -> String { base_issuer.to_string() }
    fn expiry(jwt: &Jwt) -> Duration { jwt.expiry }
}

impl TokenBehavior for RefreshToken {
    fn issuer(base_issuer: &str) -> String { format!("{base_issuer}_refresh") }
    fn expiry(jwt: &Jwt) -> Duration { jwt.refresh_expiry }
}

#[derive(Debug, Serialize, Deserialize)]
pub struct JwtTokens {
    pub access: String,
    pub refresh: String,
}

pub struct Jwt {
    pub_key: DecodingKey,
    priv_key: EncodingKey,
    expiry: Duration,
    refresh_expiry: Duration,
    issuer: String,
}

#[derive(Debug, Serialize, Deserialize)]
struct Claims {
    sub: String,
    iss: String,
    iat: i64,
    exp: i64,
}

impl Jwt {
    pub fn new(pub_key: &str, priv_key: &str) -> Option<Self> {
        Some(Self {
            pub_key: DecodingKey::from_ec_pem(pub_key.as_bytes()).ok()?,
            priv_key: EncodingKey::from_ec_pem(priv_key.as_bytes()).ok()?,
            expiry: Duration::hours(1),
            refresh_expiry: Duration::weeks(1),
            issuer: String::new(),
        })
    }

    pub fn create_tokens(&self, subject: &str) -> Option<JwtTokens> {
        let access_token = self.create_token::<AccessToken>(subject)?;
        let refresh_token = self.create_token::<RefreshToken>(subject)?;

        Some(JwtTokens { access: access_token, refresh: refresh_token })
    }

    pub fn verify(&self, token: &str) -> bool {
        if token.is_empty() {
            error!("Failed to verify jwt token: token is empty");
            return false;
        }

        let validation = Validation::new(Algorithm::ES256);

        match jsonwebtoken::decode::<Claims>(token, &self.pub_key, &validation) {
            Ok(decoded) => {
                debug!("Successfully verified jwt token for subject: {}", decoded.claims.sub);
                true
            },
            Err(err) => {
                error!("Failed to verify jwt token: {err}");
                false
            }
        }
    }

    fn create_token<T: TokenBehavior>(&self, subject: &str) -> Option<String> {
        if subject.is_empty() {
            error!("Failed to create jwt token: missing subject");
            return None;
        }

        let now = Utc::now();
        let claims = Claims {
            sub: subject.to_string(),
            iss: T::issuer(&self.issuer),
            iat: now.timestamp(),
            exp: (now + T::expiry(self)).timestamp(),
        };

        jsonwebtoken::encode(
            &Header::new(Algorithm::ES256),
            &claims,
            &self.priv_key,
        ).map_err(|err| {
            error!("Failed to encode jwt token: {}", err);
            err
        }).ok()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    const TEST_PRIVATE_KEY: &str =
        r#"
        -----BEGIN PRIVATE KEY-----
        MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQg3D8QCSn67UERnMdl
        0yH8ZfTCtWiPChYfykY7Bvm+bmGhRANCAAT15vkr32H4ipZWQtT4R9dSWOeKW3W4
        T3tQldxJEnpVa+3wHOSkdFvQUQupqwiileNWkWoA+u9JkiRUZdSgDZQc
        -----END PRIVATE KEY-----
        "#;

    const TEST_PUBLIC_KEY: &str =
        r#"
        -----BEGIN PUBLIC KEY-----
        MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE9eb5K99h+IqWVkLU+EfXUljnilt1
        uE97UJXcSRJ6VWvt8BzkpHRb0FELqasIopXjVpFqAPrvSZIkVGXUoA2UHA==
        -----END PUBLIC KEY-----
        "#;

    fn make_jwt() -> Option<Jwt> {
        let mut jwt = Jwt::new(TEST_PUBLIC_KEY, TEST_PRIVATE_KEY)?;
        jwt.issuer = "nova".into();
        jwt.expiry = Duration::seconds(5);
        jwt.refresh_expiry = Duration::seconds(10);
        Some(jwt)
    }

    #[test]
    fn test_create_access_and_refresh_tokens() {
        let jwt = make_jwt();
        assert!(jwt.is_some());

        let maybe_tokens = jwt.unwrap().create_tokens("alice");

        assert!(maybe_tokens.is_some());

        let tokens = maybe_tokens.unwrap();

        assert!(!tokens.access.is_empty());
        assert!(!tokens.refresh.is_empty());
        assert_ne!(tokens.access, tokens.refresh, "access and refresh tokens must not be equal");
    }

    #[test]
    fn test_verify_valid_token() {
        let maybe_jwt = make_jwt();
        assert!(maybe_jwt.is_some());

        let jwt = maybe_jwt.unwrap();

        let maybe_tokens = jwt.create_tokens("alice");

        assert!(maybe_tokens.is_some());

        let tokens = maybe_tokens.unwrap();

        assert!(jwt.verify(&tokens.access));
        assert!(jwt.verify(&tokens.refresh));
    }

    #[test]
    fn test_verify_invalid_token() {
        let jwt = make_jwt();
        assert!(jwt.is_some());
        assert!(!jwt.unwrap().verify("invalid_token"));
    }

    #[test]
    fn test_verify_fails_missing_keys() {
        let jwt = Jwt::new("", "");
        assert!(jwt.is_none());
    }

    #[test]
    fn test_verify_fails_with_wrong_public_key() {
        let jwt = Jwt::new("-----BEGIN PUBLIC KEY-----\nBob\n-----END PUBLIC KEY-----", TEST_PRIVATE_KEY);
        assert!(jwt.is_none());
    }

    #[test]
    fn test_create_token_fails_on_empty_subject() {
        let maybe_jwt = make_jwt();
        assert!(maybe_jwt.is_some());

        let jwt = maybe_jwt.unwrap();

        let result = jwt.create_token::<AccessToken>("");
        assert!(result.is_none());
    }

    #[test]
    fn test_create_token_fails_on_invalid_private_key() {
        let jwt = Jwt::new(TEST_PUBLIC_KEY, "INVALID");
        assert!(jwt.is_none());
    }

    #[test]
    fn test_access_and_refresh_have_different_issuers() {
        let maybe_jwt = make_jwt();
        assert!(maybe_jwt.is_some());

        let jwt = maybe_jwt.unwrap();
        let maybe_tokens = jwt.create_tokens("alice");

        assert!(maybe_tokens.is_some());

        let tokens = maybe_tokens.unwrap();

        let access_claims = jsonwebtoken::decode::<Claims>(
            &tokens.access,
            &jwt.pub_key,
            &Validation::new(Algorithm::ES256),
        ).unwrap().claims;

        let refresh_claims = jsonwebtoken::decode::<Claims>(
            &tokens.refresh,
            &jwt.pub_key,
            &Validation::new(Algorithm::ES256),
        ).unwrap().claims;

        assert_eq!(access_claims.iss, "nova");
        assert_eq!(refresh_claims.iss, "nova_refresh");
    }

    #[test]
    fn test_tampered_token_fails() {
        let maybe_jwt = make_jwt();
        assert!(maybe_jwt.is_some());

        let mut jwt = maybe_jwt.unwrap();

        let maybe_tokens = jwt.create_tokens("alice");

        assert!(maybe_tokens.is_some());

        let tokens = maybe_tokens.unwrap();

        let mut parts: Vec<&str> = tokens.access.split('.').collect();
        parts[1] = "AAAA";
        let tampered = parts.join(".");

        assert!(!jwt.verify(&tampered));
    }
}
