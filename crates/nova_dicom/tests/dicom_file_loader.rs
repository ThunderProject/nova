use std::{
    path::{Path, PathBuf},
    time::Instant,
};

use nova_dicom::{
    dicom::{Modality, PixelDataInfo}, file_loader::dicom_file_loader::{process_directory, process_file}
};

const TEST_DICOM_FILE: &str = "data/chest_ct/instance-0001.dcm";

fn init_tracing() {
    let _ = tracing_subscriber::fmt()
        .with_test_writer()
        .with_env_filter(
            tracing_subscriber::EnvFilter::try_from_default_env()
                .unwrap_or_else(|_| "info".into()),
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
    let dicom = process_file(&path).expect("expected DICOM file to load");
    let elapsed = start.elapsed();
    let elapsed_ms = elapsed.as_secs_f64() * 1000.0;

    assert!(
        !dicom.metadata.study.instance_uid.is_empty(),
        "expected StudyInstanceUID"
    );

    assert!(
        !dicom.metadata.series.instance_uid.is_empty(),
        "expected SeriesInstanceUID"
    );

    assert_ne!(
        dicom.metadata.series.modality,
        Modality::Unknown,
        "expected known modality"
    );

    assert!(
        dicom.pixel_data_info.dims.width > 0,
        "expected pixel width > 0"
    );

    assert!(
        dicom.pixel_data_info.dims.height > 0,
        "expected pixel height > 0"
    );

    assert!(
        dicom.pixel_data_info.dims.frames > 0,
        "expected frame count > 0"
    );

    assert!(
        !dicom.pixel_data.is_empty(),
        "expected pixel data"
    );

    assert_eq!(
        dicom.pixel_data.len(),
        expected_pixel_buffer_len(&dicom.pixel_data_info),
        "pixel data length should match pixel metadata"
    );

    tracing::info!(
        elapsed_ms,
        bytes = dicom.pixel_data.len(),
        study_uid = %dicom.metadata.study.instance_uid,
        series_uid = %dicom.metadata.series.instance_uid,
        modality = ?dicom.metadata.series.modality,
        width = dicom.pixel_data_info.dims.width,
        height = dicom.pixel_data_info.dims.height,
        frames = dicom.pixel_data_info.dims.frames,
        "processed full DICOM file"
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

    let total_pixel_bytes: usize = files
        .iter()
        .map(|file| file.pixel_data.len())
        .sum();

    for file in &files {
        assert!(
            !file.metadata.study.instance_uid.is_empty(),
            "expected StudyInstanceUID"
        );

        assert!(
            !file.metadata.series.instance_uid.is_empty(),
            "expected SeriesInstanceUID"
        );

        assert_ne!(
            file.metadata.series.modality,
            Modality::Unknown,
            "expected known modality"
        );

        assert!(
            file.pixel_data_info.dims.width > 0,
            "expected pixel width > 0"
        );

        assert!(
            file.pixel_data_info.dims.height > 0,
            "expected pixel height > 0"
        );

        assert!(
            file.pixel_data_info.dims.frames > 0,
            "expected frame count > 0"
        );

        assert!(
            file.pixel_data_info.samples_per_pixel > 0,
            "expected samples_per_pixel > 0"
        );

        assert!(
            file.pixel_data_info.bits_allocated > 0,
            "expected bits_allocated > 0"
        );

        assert!(
            !file.pixel_data.is_empty(),
            "expected pixel data"
        );

        assert_eq!(
            file.pixel_data.len(),
            expected_pixel_buffer_len(&file.pixel_data_info),
            "pixel data length should match pixel metadata"
        );
    }

    let elapsed_ms = elapsed.as_secs_f64() * 1000.0;
    let us_per_file = elapsed.as_secs_f64() * 1_000_000.0 / files.len() as f64;
    let mib = total_pixel_bytes as f64 / (1024.0 * 1024.0);
    let throughput_mib_s = mib / elapsed.as_secs_f64();

    tracing::info!(
        file_count = files.len(),
        elapsed_ms,
        us_per_file,
        total_pixel_bytes,
        mib,
        throughput_mib_s,
        "processed full DICOM directory"
    );
}

fn expected_pixel_buffer_len(info: &PixelDataInfo) -> usize {
    let pixel_count =
        info.dims.width as usize * info.dims.height as usize * info.dims.frames as usize;

    let bytes_per_sample = (info.bits_allocated as usize).div_ceil(8);

    pixel_count * info.samples_per_pixel as usize * bytes_per_sample
}