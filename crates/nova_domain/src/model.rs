use crate::{image::{ImageVolume, ImageVolumes}, patient::Patient};

#[derive(Debug, Default)]
pub struct NovaModel {
    pub patients: Vec<Patient>,
    pub image_volumes: ImageVolumes,
}

impl NovaModel {
    pub fn middle_image_volume(&self) -> Option<ImageVolume<'_>> {
        self.image_volumes.middle()
    }
}