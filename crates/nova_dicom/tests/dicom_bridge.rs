use nova_dicom::DicomReader;

fn ct_head_path() -> std::path::PathBuf {
    std::path::PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("tests")
        .join("data")
        .join("CTHead1.dcm")
}

#[test]
fn creates_reader() {
    let _reader = DicomReader::new();
}

#[test]
fn load_missing_file_returns_error() {
    let mut reader = DicomReader::new();

    let result = reader.load("/definitely/does/not/exist/file.dcm");

    assert!(
        result.is_err(),
        "loading a missing DICOM file should return an error"
    );
}

#[test]
fn load_ct_head_fixture() -> Result<(), Box<dyn std::error::Error>> {
    let path = ct_head_path();
    assert!(path.exists(), "missing fixture: {}", path.display());

    let mut reader = DicomReader::new();
    reader.load(&path)?;

    Ok(())
}

#[test]
fn read_ct_head_metadata() -> Result<(), Box<dyn std::error::Error>> {
    let path = ct_head_path();
    let mut reader = DicomReader::new();
    reader.load(&path).unwrap();

    let metadata = reader.metadata()?;

    assert!(
        !metadata.series.modality.is_empty(),
        "metadata series modality should not be empty"
    );

    Ok(())
}

#[test]
fn read_ct_head_pixel_data() -> Result<(), Box<dyn std::error::Error>> {
    let path = ct_head_path();
    let mut reader = DicomReader::new();
    reader.load(&path).unwrap();

    let pixel_data = reader.pixel_data()?;

    assert!(
        !pixel_data.bytes.is_empty(),
        "pixel data should not be empty"
    );

    assert!(
        pixel_data.info.width > 0,
        "pixel width should be greater than zero"
    );

    assert!(
        pixel_data.info.height > 0,
        "pixel height should be greater than zero"
    );

    Ok(())
}