use crate::{examination::Examination, ids::StudyId};

#[derive(Debug, Clone)]
pub struct Study {
    pub id: StudyId,
    pub name: String,
    pub examinations: Vec<Examination>,
}