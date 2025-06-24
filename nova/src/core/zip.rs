use std::fs::File;
use std::{fs, io};
use std::path::{Path, PathBuf};
use ripunzip::{NullProgressReporter, UnzipEngine, UnzipOptions};
use thiserror::Error;
use tracing::debug;

#[derive(Error, Debug)]
pub enum UnzipAppError {
    #[error("I/O error: {0}")]
    Io(#[from] io::Error),

    #[error("Output directory not valid: {0}")]
    InvalidOutputDir(PathBuf),

    #[error("Unzip failed: {0}")]
    Anyhow(#[from] anyhow::Error),
}

pub struct Zip {}

impl Zip {
    pub fn unzip(input: &str, output: &str) -> Result<(), UnzipAppError> {
        debug!("Unzipping {} to {}", input, output);

        let output_dir = Path::new(output);

        if output_dir.exists() && !output_dir.is_dir() {
            return Err(UnzipAppError::InvalidOutputDir(output_dir.to_path_buf()));
        }
        else if !output_dir.exists() {
            fs::create_dir_all(output_dir)?;
        }

        let file = File::open(input)?;
        let engine = UnzipEngine::for_file(file)?;

        let options = UnzipOptions {
            output_directory: Some(output_dir.to_path_buf()),
            password: None,
            single_threaded: false,
            filename_filter: None,
            progress_reporter: Box::new(NullProgressReporter),
        };

        engine.unzip(options)?;

        debug!("successfully unzipped file!");
        Ok(())
    }
}