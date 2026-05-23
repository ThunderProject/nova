use std::path::PathBuf;

#[derive(Debug, thiserror::Error)]
pub enum DicomFileError {
    #[error("failed to read DICOM file `{path}`: {message}")]
    ReadFailed {
        path: PathBuf,
        message: String,
    },

    #[error("file is missing required DICOM identity tags: {path}")]
    MissingIdentity {
        path: PathBuf,
    },

    #[error("failed to traverse directory `{path}`: {message}")]
    DirectoryTraversal {
        path: PathBuf,
        message: String,
    },

    #[error("provided path is not a directory: {path}")]
    NotADirectory {
        path: PathBuf,
    },
}