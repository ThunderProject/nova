pub mod dicom;
pub mod core;
mod project;
pub mod application;
pub mod fs;

pub use dicom::bridge::dicom_bridge::dicom_api::init;

pub fn add(left: u64, right: u64) -> u64 {
    left + right
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn it_works() {
        let result = add(2, 2);
        assert_eq!(result, 4);
    }
}
