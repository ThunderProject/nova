use bevy::prelude::*;

use super::types::ViewportId;

#[derive(Resource, Debug, Copy, Clone)]
pub struct ActiveViewport(pub ViewportId);

impl Default for ActiveViewport {
    fn default() -> Self {
        Self(ViewportId::Axial)
    }
}

#[derive(Resource, Debug, Copy, Clone)]
pub struct ViewportLayout {
    pub columns: u32,
    pub rows: u32,
    pub gap_px: u32,
    pub outer_margin_px: u32,
}

impl Default for ViewportLayout {
    fn default() -> Self {
        Self {
            columns: 2,
            rows: 2,
            gap_px: 8,
            outer_margin_px: 8,
        }
    }
}

#[derive(Debug, Copy, Clone, Eq, PartialEq)]
pub struct PhysicalRect {
    pub position: UVec2,
    pub size: UVec2,
}

impl PhysicalRect {
    pub fn contains(self, point: UVec2) -> bool {
        let min = self.position;
        let max = self.position + self.size;

        point.x >= min.x
            && point.x < max.x
            && point.y >= min.y
            && point.y < max.y
    }
}

impl ViewportLayout {
    pub fn rect_for_slot(self, full_size: UVec2, slot: UVec2) -> PhysicalRect {
        let columns = self.columns.max(1);
        let rows = self.rows.max(1);

        let margin = self.outer_margin_px;
        let gap = self.gap_px;

        let available_w = full_size
            .x
            .saturating_sub(margin * 2)
            .saturating_sub(gap * columns.saturating_sub(1));

        let available_h = full_size
            .y
            .saturating_sub(margin * 2)
            .saturating_sub(gap * rows.saturating_sub(1));

        let cell_w = available_w / columns;
        let cell_h = available_h / rows;

        let x = margin + slot.x * (cell_w + gap);
        let y = margin + slot.y * (cell_h + gap);

        PhysicalRect {
            position: UVec2::new(x, y),
            size: UVec2::new(cell_w, cell_h),
        }
    }
}