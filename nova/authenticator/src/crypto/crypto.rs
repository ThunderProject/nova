use aead::array::typenum::Unsigned;
use aead::{Aead, AeadCore, KeyInit, Nonce, Payload};
use base64::prelude::*;
use rand::rngs::OsRng;
use rand::{TryRngCore};
use crate::crypto::key_derivation::{KeyDerivation, KEY_LENGTH, PASSPHRASE_LENGTH, SALT_LEN};

#[derive(Debug, thiserror::Error)]
pub enum CryptoError {
    #[error("invalid key length")]
    InvalidKeyLength,

    #[error("invalid passphrase length")]
    InvalidPassphraseLength,

    #[error("nonce generation failed: {0}")]
    NonceGenerationFailed(#[source] rand::rand_core::OsError),

    #[error("nonce has invalid length or format")]
    InvalidNonce,

    #[error("encryption failed")]
    EncryptionFailed,

    #[error("decryption failed")]
    DecryptionFailed,

    #[error("base64 decode failed: {0}")]
    Base64DecodeFailed(#[source] base64::DecodeError),

    #[error("utf8 decode error: {0}")]
    Utf8Error(#[source] std::string::FromUtf8Error),

    #[error("data truncated (missing nonce)")]
    TruncatedInput,

    #[error("Key derivation error: {0:?}")]
    KeyDerivationError(#[from] crate::crypto::key_derivation::KeyDerivationError),
}

pub trait CryptoAlgo: Aead + KeyInit {
    const NONCE_SIZE: usize;
}

impl<T> CryptoAlgo for T where  T: Aead + KeyInit {
    const NONCE_SIZE: usize = <T::NonceSize as Unsigned>::USIZE;
}
pub fn encrypt<Algo: CryptoAlgo>(plain: &[u8], key: &[u8], aad: &[u8]) -> Result<(Vec<u8>, Vec<u8>), CryptoError> {
    if key.len() < KEY_LENGTH {
        return Err(CryptoError::InvalidKeyLength);
    }

    let cipher = Algo::new_from_slice(key).map_err(|_| CryptoError::InvalidKeyLength)?;

    let mut nonce_bytes = vec![0u8; Algo::NONCE_SIZE];
    OsRng.try_fill_bytes(&mut nonce_bytes).map_err(CryptoError::NonceGenerationFailed)?;

    let nonce = Nonce::<Algo>::try_from(nonce_bytes.as_slice()).map_err(|_| CryptoError::InvalidNonce)?;

    let ciphertext = cipher
        .encrypt(&nonce, Payload { msg: plain, aad })
        .map_err(|_| CryptoError::EncryptionFailed)?;

    Ok((ciphertext, nonce_bytes))
}

pub fn encrypt_str<Algo: CryptoAlgo>(plain: &str, passphrase: &str, aad: &str) -> Result<String, CryptoError> {
    if passphrase.len() < PASSPHRASE_LENGTH {
        return Err(CryptoError::InvalidPassphraseLength);
    }

    let key_derivation = KeyDerivation::new(None)?;
    let salt = key_derivation.generate_salt()?;
    let key = key_derivation.derive(passphrase, &salt)?;

    let (ciphertext, nonce) = encrypt::<Algo>(plain.as_bytes(), key.as_ref(), aad.as_bytes())?;

    let mut combined = Vec::with_capacity(salt.len() + nonce.len() + ciphertext.len());

    combined.extend_from_slice(&salt);
    combined.extend_from_slice(&nonce);
    combined.extend_from_slice(&ciphertext);

    Ok(BASE64_STANDARD.encode(combined))
}

pub fn decrypt<Algo: CryptoAlgo>(key: &[u8], ciphertext: &[u8], nonce: &[u8], aad: &[u8]) -> Result<Vec<u8>, CryptoError> {
    let cipher = Algo::new_from_slice(key).map_err(|_| CryptoError::InvalidKeyLength)?;

    let nonce = Nonce::<Algo>::try_from(nonce).map_err(|_| CryptoError::InvalidNonce)?;

    let plaintext = cipher
        .decrypt(&nonce, Payload { msg: ciphertext, aad })
        .map_err(|_| CryptoError::DecryptionFailed)?;

    Ok(plaintext)
}

pub fn decrypt_str<Algo: CryptoAlgo>(base64_cipher: &str, passphrase: &str, aad: &str) -> Result<String, CryptoError> {
    let decoded = BASE64_STANDARD.decode(base64_cipher).map_err(CryptoError::Base64DecodeFailed)?;

    if decoded.len() < Algo::NONCE_SIZE {
        return Err(CryptoError::TruncatedInput);
    }

    let (salt, rest) = decoded.split_at(SALT_LEN);
    let (nonce, ciphertext) = rest.split_at(Algo::NONCE_SIZE);

    let key_derivation = KeyDerivation::new(None)?;
    let key = key_derivation.derive(passphrase, salt)?;

    let plaintext_bytes = decrypt::<Algo>(key.as_ref(), ciphertext, nonce, aad.as_bytes())?;
    let plaintext = String::from_utf8(plaintext_bytes).map_err(CryptoError::Utf8Error)?;

    Ok(plaintext)
}

#[cfg(test)]
mod tests {
    use super::*;
    use aes_gcm::Aes256Gcm;
    use chacha20poly1305::{ChaCha20Poly1305, XChaCha20Poly1305};

    #[test]
    fn encrypt_xchacha20poly1305() {
        let key = [1u8; 32];
        let plain = b"hello world";
        let aad = b"test";

        let encryption_result = encrypt::<XChaCha20Poly1305>(plain, &key, aad);

        assert!(encryption_result.is_ok());

        let (cipher, nonce) = encryption_result.unwrap();

        assert_ne!(cipher, plain);
        assert_eq!(nonce.len(), XChaCha20Poly1305::NONCE_SIZE);

        let decrypted = decrypt::<XChaCha20Poly1305>(&key, &cipher, &nonce, aad).unwrap();
        assert_eq!(decrypted, plain);
    }

    #[test]
    fn encrypt_chacha20poly1305() {
        let key = [1u8; 32];
        let plain = b"hello world";
        let aad = b"test";

        let encryption_result = encrypt::<ChaCha20Poly1305>(plain, &key, aad);

        assert!(encryption_result.is_ok());

        let (cipher, nonce) = encryption_result.unwrap();

        assert_ne!(cipher, plain);
        assert_eq!(nonce.len(), ChaCha20Poly1305::NONCE_SIZE);

        let decrypted = decrypt::<ChaCha20Poly1305>(&key, &cipher, &nonce, aad).unwrap();
        assert_eq!(decrypted, plain);
    }
    #[test]
    fn encrypt_aes256gcm_ok() {
        let key = [1u8; 32];
        let plain = b"hello world";
        let aad   = b"test";

        let encryption_result = encrypt::<Aes256Gcm>(plain, &key, aad);

        assert!(encryption_result.is_ok());

        let (cipher, nonce) = encryption_result.unwrap();

        assert_ne!(cipher, plain);
        assert_eq!(nonce.len(), Aes256Gcm::NONCE_SIZE);

        let decrypted = decrypt::<Aes256Gcm>(&key, &cipher, &nonce, aad).unwrap();
        assert_eq!(decrypted, plain);
    }

    #[test]
    fn encrypt_invalid_key_len_fails() {
        let key = [1u8; 16];
        let plain = b"hello world";
        let aad   = b"test";

        let result = encrypt::<Aes256Gcm>(plain, &key, aad);

        assert!(result.is_err(), "short key should fail");
    }

    #[test]
    fn encrypt_str_xchacha20poly1305() {
        let key = "01234567890123456789012345678901";
        let plain = "hello string";
        let aad = "test";

        let encryption_result = encrypt_str::<XChaCha20Poly1305>(plain, key, aad);
        assert!(encryption_result.is_ok());

        let encrypted = encryption_result.unwrap();
        assert!(!encrypted.is_empty(), "encrypted string should not be empty");

        let decryption_result = decrypt_str::<XChaCha20Poly1305>(&encrypted, key, aad);
        assert!(decryption_result.is_ok());

        let decrypted = decryption_result.unwrap();
        assert_eq!(decrypted, plain);
    }

    #[test]
    fn encrypt_str_chacha20poly1305() {
        let key = "01234567890123456789012345678901";
        let plain = "hello string";
        let aad = "test";

        let encryption_result = encrypt_str::<ChaCha20Poly1305>(plain, key, aad);
        assert!(encryption_result.is_ok());

        let encrypted = encryption_result.unwrap();
        assert!(!encrypted.is_empty(), "encrypted string should not be empty");

        let decryption_result = decrypt_str::<ChaCha20Poly1305>(&encrypted, key, aad);
        assert!(decryption_result.is_ok());

        let decrypted = decryption_result.unwrap();
        assert_eq!(decrypted, plain);
    }

    #[test]
    fn encrypt_str_aes256gcm() {
        let key = "01234567890123456789012345678901";
        let plain = "hello string";
        let aad = "test";

        let encryption_result = encrypt_str::<Aes256Gcm>(plain, key, aad);
        assert!(encryption_result.is_ok());

        let encrypted = encryption_result.unwrap();
        assert!(!encrypted.is_empty(), "encrypted string should not be empty");

        let decryption_result = decrypt_str::<Aes256Gcm>(&encrypted, key, aad);
        assert!(decryption_result.is_ok());

        let decrypted = decryption_result.unwrap();
        assert_eq!(decrypted, plain);
    }

    #[test]
    fn encrypt_str_invalid_key_len_fails() {
        let key = "0123456789012345";
        let plain = "hello string";
        let aad = "test";

        let result = encrypt_str::<Aes256Gcm>(plain, key, aad);

        assert!(result.is_err(), "short key should fail");
    }

    #[test]
    fn decrypt_str_invalid_b64_fails() {
        let key = "01234567890123456789012345678901";
        let plain = "@@@ invalid base64 ###";
        let aad = "test";

        let result = decrypt_str::<ChaCha20Poly1305>(plain, key, aad);

        assert!(result.is_err());
    }
}