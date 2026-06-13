use nova_dicom::dicom::{Dicom, Modality, PixelDataInfo};

fn ct_head_path() -> std::path::PathBuf {
    std::path::PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("tests")
        .join("data")
        .join("CTHead1.dcm")
}

#[test]
fn load_missing_file_returns_error() {
    let result = Dicom::load("/definitely/does/not/exist/file.dcm");

    assert!(
        result.is_err(),
        "loading a missing DICOM file should return an error"
    );
}

#[test]
fn load_ct_head_fixture() -> Result<(), Box<dyn std::error::Error>> {
    let path = ct_head_path();
    assert!(path.exists(), "missing fixture: {}", path.display());

    let _dicom = Dicom::load(&path)?;

    Ok(())
}

#[test]
fn read_ct_head_metadata() -> Result<(), Box<dyn std::error::Error>> {
    let path = ct_head_path();
    let dicom = Dicom::load(&path)?;

    let metadata = &dicom.metadata;

    assert_ne!(
        metadata.series.modality,
        Modality::Unknown,
        "metadata series modality should not be unknown"
    );

    assert!(
        !metadata.study.instance_uid.is_empty(),
        "metadata study instance UID should not be empty"
    );

    assert!(
        !metadata.series.instance_uid.is_empty(),
        "metadata series instance UID should not be empty"
    );

    Ok(())
}

#[test]
fn read_ct_head_pixel_data_info() -> Result<(), Box<dyn std::error::Error>> {
    let path = ct_head_path();
    let dicom = Dicom::load(&path)?;

    let info = &dicom.pixel_data_info;

    assert!(info.dims.width > 0, "pixel width should be greater than zero");
    assert!(info.dims.height > 0, "pixel height should be greater than zero");
    assert!(info.dims.frames > 0, "frame count should be greater than zero");

    assert!(
        info.samples_per_pixel > 0,
        "samples per pixel should be greater than zero"
    );

    assert!(
        info.bits_allocated > 0,
        "bits allocated should be greater than zero"
    );

    Ok(())
}

#[test]
fn read_ct_head_pixel_buffer() -> Result<(), Box<dyn std::error::Error>> {
    let path = ct_head_path();
    let dicom = Dicom::load(&path)?;

    let info = &dicom.pixel_data_info;
    let buffer = &dicom.pixel_data;

    assert!(!buffer.is_empty(), "pixel buffer should not be empty");

    assert_eq!(
        buffer.len(),
        expected_pixel_buffer_len(info),
        "pixel buffer length should match pixel data info"
    );

    Ok(())
}

#[test]
fn read_ct_head_file() -> Result<(), Box<dyn std::error::Error>> {
    let path = ct_head_path();
    let dicom = Dicom::load(&path)?;

    assert_ne!(
        dicom.metadata.series.modality,
        Modality::Unknown,
        "file metadata series modality should not be unknown"
    );

    assert!(
        !dicom.metadata.study.instance_uid.is_empty(),
        "file metadata study instance UID should not be empty"
    );

    assert!(
        !dicom.metadata.series.instance_uid.is_empty(),
        "file metadata series instance UID should not be empty"
    );

    assert!(
        dicom.pixel_data_info.dims.width > 0,
        "file pixel width should be greater than zero"
    );

    assert!(
        dicom.pixel_data_info.dims.height > 0,
        "file pixel height should be greater than zero"
    );

    assert!(
        dicom.pixel_data_info.dims.frames > 0,
        "file frame count should be greater than zero"
    );

    assert!(
        !dicom.pixel_data.is_empty(),
        "file pixel data should not be empty"
    );

    assert_eq!(
        dicom.pixel_data.len(),
        expected_pixel_buffer_len(&dicom.pixel_data_info),
        "file pixel data length should match pixel data info"
    );

    Ok(())
}

#[test]
fn ct_head_pixel_data_len_matches_method() -> Result<(), Box<dyn std::error::Error>> {
    let path = ct_head_path();
    let dicom = Dicom::load(&path)?;

    assert_eq!(
        dicom.pixel_data.len(),
        dicom.pixel_data_info.expected_pixel_buffer_len(),
        "pixel data length should match PixelDataInfo::expected_pixel_buffer_len()"
    );

    Ok(())
}

fn expected_pixel_buffer_len(info: &PixelDataInfo) -> usize {
    let pixel_count = info.dims.width as usize
        * info.dims.height as usize
        * info.dims.frames as usize;

    let bytes_per_sample = (info.bits_allocated as usize).div_ceil(8);

    pixel_count * info.samples_per_pixel as usize * bytes_per_sample
}