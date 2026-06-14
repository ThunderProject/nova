use crate::{ids::PatientId, study::Study};

#[derive(Debug, Clone)]
pub struct Patient {
    pub id: PatientId,
    pub display_name: String,
    pub studies: Vec<Study>,
}