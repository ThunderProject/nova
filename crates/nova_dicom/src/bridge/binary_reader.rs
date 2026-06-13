use bytes::{Buf, Bytes, TryGetError};
use thiserror::Error;

#[derive(Debug, Error)]
pub enum BinaryReadError {
    #[error("unexpected end of buffer: needed {needed} bytes, remaining {remaining}")]
    UnexpectedEof {
        needed: usize,
        remaining: usize,
    },

    #[error("buffer read error")]
    TryGetError(#[from] TryGetError),

    #[error("invalid UTF-8 string")]
    InvalidUtf8(#[from] std::string::FromUtf8Error),

    #[error("invalid modality value: {0}")]
    InvalidModality(u8),

    #[error("invalid photometric interpretation value: {0}")]
    InvalidPhotometricInterpretation(u8),

    #[error("invalid pixel sample format value: {0}")]
    InvalidPixelSampleFormat(u8),

    #[error("trailing bytes after parse: {0}")]
    TrailingBytes(usize),
}

pub type Result<T> = std::result::Result<T, BinaryReadError>;

#[derive(Debug, Clone)]
pub struct BinaryReader<B> {
    buf: B,
}

impl<B> BinaryReader<B>
where B : Buf,
{
    pub fn new(buf: B) -> Self {
        Self { buf }
    }

    #[inline]
    pub fn remaining(&self) -> usize {
        self.buf.remaining()
    }

    #[inline]
    fn require(&self, n: usize) -> Result<()> {
        let remaining = self.buf.remaining();

        if remaining < n {
            return Err(BinaryReadError::UnexpectedEof {
                needed: n,
                remaining,
            });
        }

        Ok(())
    }

    #[inline]
    pub fn read_u8(&mut self) -> Result<u8> {
        Ok(self.buf.try_get_u8()?)
    }

    #[inline]
    pub fn read_u16(&mut self) -> Result<u16> {
         Ok(self.buf.try_get_u16_le()?)
    }

    #[inline]
    pub fn read_i16(&mut self) -> Result<i16> {
        Ok(self.buf.try_get_i16_le()?)
    }

    #[inline]
    pub fn read_u32(&mut self) -> Result<u32> {
        Ok(self.buf.try_get_u32_le()?)
    }

    #[inline]
    pub fn read_i32(&mut self) -> Result<i32> {
        Ok(self.buf.try_get_i32_le()?)
    }

    #[inline]
    pub  fn read_u64(&mut self) -> Result<u64> {
        Ok(self.buf.try_get_u64_le()?)
    }

    #[inline]
    pub  fn read_i64(&mut self) -> Result<i64> {
        Ok(self.buf.try_get_i64_le()?)
    }

    #[inline]
    pub fn read_f32(&mut self) -> Result<f32> {
        Ok(self.buf.try_get_f32_le()?)
    }

    #[inline]
    pub fn read_f64(&mut self) -> Result<f64> {
        Ok(self.buf.try_get_f64_le()?)
    }

    #[inline]
    pub fn read_bytes(&mut self, len: usize) -> Result<Bytes> {
        self.require(len)?;
        Ok(self.buf.copy_to_bytes(len))
    }

    #[inline]
    pub fn read_string(&mut self) -> Result<String> {
        let len = self.read_u32()? as usize;
        let bytes = self.read_bytes(len)?;

        Ok(String::from_utf8(bytes.to_vec())?)
    }

    #[inline]
    pub fn finish(self) -> Result<()> {
        let remaining = self.remaining();
        match remaining {
            0 => Ok(()),
            _ => Err(BinaryReadError::TrailingBytes(remaining))
        }
    }
}