use argon2::{Algorithm, Argon2, Params, Version};
use rand::{rngs::OsRng, RngCore, TryRngCore};
use zeroize::{Zeroizing};

#[derive(Debug, thiserror::Error)]
pub enum KeyDerivationError {
    #[error("invalid params: {0}")]
    InvalidParams(String),
    #[error("argon2 failed: {0}")]
    Argon2(String),

    #[error("Failed to generate salt")]
    SaltGenerationFailed,

    #[error("salt length must be >= {required}, got {got}")]
    SaltTooShort {
        required: usize,
        got: usize,
    },
}

pub const SALT_LEN: usize = 32;
pub const KEY_LENGTH: usize = 32;
pub const PASSPHRASE_LENGTH: usize = 32;

const DEFAULT_MEMORY_COST: u32 = 64 * 1024;
const DEFAULT_TIME_COST: u32 = 3;

#[derive(Clone)]
pub struct KeyDerivation {
    params: Params,
    pepper: Option<Zeroizing<Vec<u8>>>,
}

impl KeyDerivation {
    pub fn new(pepper: Option<Vec<u8>>) -> Result<Self, KeyDerivationError> {
        let params = Params::new(DEFAULT_MEMORY_COST, DEFAULT_TIME_COST, KeyDerivation::default_parallelism(), Some(KEY_LENGTH))
            .map_err(|e| KeyDerivationError::InvalidParams(e.to_string()))?;

        Ok(
            Self {
                params,
                pepper: pepper.map(Zeroizing::new),
            }
        )
    }

    pub fn with_params(mut self, params: Params) -> Self {
        self.params = params;
        self
    }

    pub fn generate_salt(&self) -> Result<[u8; SALT_LEN], KeyDerivationError> {
        let mut salt = [0u8; SALT_LEN];
        OsRng.try_fill_bytes(&mut salt).map_err(|_| KeyDerivationError::SaltGenerationFailed)?;
        Ok(salt)
    }

    pub fn derive(&self, passphrase: &str,  salt: &[u8]) -> Result<Zeroizing<[u8; KEY_LENGTH]>, KeyDerivationError> {
        if salt.len() < SALT_LEN {
            return Err(KeyDerivationError::SaltTooShort {
                required: SALT_LEN,
                got: salt.len(),
            });
        }

        let params = self.params.clone();

        let argon = match &self.pepper {
            Some(pepper) => {
                Argon2::new_with_secret(pepper, Algorithm::Argon2id, Version::V0x13, params)
                    .map_err(|e| KeyDerivationError::Argon2(e.to_string()))?
            },
            None => Argon2::new(Algorithm::Argon2id, Version::V0x13, params)
        };

        let mut key = Zeroizing::new([0u8; KEY_LENGTH]);

        argon.hash_password_into(passphrase.as_bytes(), salt, key.as_mut())
            .map_err(|e| KeyDerivationError::Argon2(e.to_string()))?;

        Ok(key)
    }

    fn default_parallelism() -> u32 {
        let cpus = num_cpus::get() as u32;
        cpus.min(8)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn derive_ok() {
        let key_derivation_result = KeyDerivation::new(None);
        assert!(key_derivation_result.is_ok());

        let key_derivation = key_derivation_result.unwrap();
        let salt_result = key_derivation.generate_salt();
        assert!(salt_result.is_ok());

        let salt = salt_result.unwrap();
        let key_result = key_derivation.derive("correct horse battery staple", &salt);
        assert!(key_result.is_ok());

        let key = key_result.unwrap();
        assert_eq!(key.len(), KEY_LENGTH);
    }

    #[test]
    fn derive_with_pepper_ok() {
        let key_derivation_result = KeyDerivation::new(None);
        assert!(key_derivation_result.is_ok());

        let key_derivation = key_derivation_result.unwrap();
        let salt_result = key_derivation.generate_salt();
        assert!(salt_result.is_ok());

        let salt = salt_result.unwrap();
        let key_result = key_derivation.derive("correct horse battery staple", &salt);
        assert!(key_result.is_ok());

        let key = key_result.unwrap();
        assert_eq!(key.len(), KEY_LENGTH);
    }

    #[test]
    fn derive_with_pepper_changes_output() {
        let without_pepper_result = KeyDerivation::new(None);
        let with_pepper_result = KeyDerivation::new(Some(b"pepper123".to_vec()));

        assert!(without_pepper_result.is_ok());
        assert!(with_pepper_result.is_ok());

        let without_pepper = without_pepper_result.unwrap();
        let with_pepper = with_pepper_result.unwrap();

        let salt_result = without_pepper.generate_salt();
        assert!(salt_result.is_ok());

        let salt = salt_result.unwrap();

        let key_without_pepper_result = without_pepper.derive("pw", &salt);
        let key_with_pepper_result = with_pepper.derive("pw", &salt);
        assert!(key_without_pepper_result.is_ok());
        assert!(key_with_pepper_result.is_ok());

        let key_without_pepper = key_without_pepper_result.unwrap();
        let key_with_pepper = key_with_pepper_result.unwrap();

        assert_ne!(key_without_pepper.as_ref(), key_with_pepper.as_ref());
    }

    #[test]
    fn short_salt_rejected() {
        let key_derivation = KeyDerivation::new(None).unwrap();
        let err = key_derivation.derive("pw", b"short").unwrap_err();
        matches!(err, KeyDerivationError::SaltTooShort { required: _, got: _ });
    }
}

