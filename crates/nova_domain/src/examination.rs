use crate::ids::{ExaminationId, ImageVolumeId};

#[derive(Debug, Clone)]
pub struct Examination {
    pub id: ExaminationId,
    pub name: String,
    pub image_volume: ImageVolumeId
}