use std::{path::{Path, PathBuf}, time::Instant};

use nova_dicom::file_loader::dicom_file_loader::{process_directory, process_file};

const TEST_DICOM_FILE: &str = "data/chest_ct/instance-0001.dcm";

fn init_tracing() {
    let _ = tracing_subscriber::fmt()
        .with_test_writer()
        .with_env_filter(
            tracing_subscriber::EnvFilter::try_from_default_env()
                .unwrap_or_else(|_| "debug".into()),
        )
        .try_init();
}

fn crate_root() -> &'static Path {
    Path::new(env!("CARGO_MANIFEST_DIR"))
}

fn test_data_path(relative: &str) -> PathBuf {
    crate_root().join(relative)
}

#[test]
fn process_single_dicom_file() {
    init_tracing();

    let path = test_data_path(TEST_DICOM_FILE);

    assert!(
        path.exists(),
        "missing test fixture: {}",
        path.display()
    );

    let start = Instant::now();
    let dicom_file = process_file(&path).expect("expected DICOM file to load");
    let elapsed_ms = start.elapsed().as_millis();

    assert_eq!(dicom_file.path, path);

    assert!(
        dicom_file.study_instance_uid.is_some(),
        "expected StudyInstanceUID"
    );

    assert!(
        dicom_file.series_instance_uid.is_some(),
        "expected SeriesInstanceUID"
    );

    tracing::info!(
        elapsed_ms = elapsed_ms,
        ?dicom_file,
        "processed DICOM file"
    );
}

#[test]
fn process_dicom_files_in_directory() {
    init_tracing();

    let dir = test_data_path("data/chest_ct");

    assert!(
        dir.exists(),
        "missing test DICOM directory: {}",
        dir.display()
    );

    assert!(
        dir.is_dir(),
        "test DICOM path is not a directory: {}",
        dir.display()
    );

    let start = Instant::now();
    let files = process_directory(&dir).expect("expected DICOM directory to load");
    let elapsed = start.elapsed();

    assert!(
        !files.is_empty(),
        "expected at least one processed DICOM file in {}",
        dir.display()
    );

    tracing::info!(
        file_count = files.len(),
        elapsed_ms = elapsed.as_millis(),
        "processed DICOM directory"
    );
}