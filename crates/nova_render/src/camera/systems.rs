use bevy::prelude::*;

use crate::{camera::prelude::SliceCamera, viewport::prelude::NovaViewport};

pub fn update_slice_camera(
    mut cameras: Query<(&SliceCamera, &mut Transform), With<NovaViewport>>,
) {
    for (view, mut transform) in &mut cameras {
        let target = view.target + Vec3::new(
            view.pan.0.x,
            view.pan.0.y,
            0.0,
        );

        let distance = view.distance / view.zoom.max(0.001);

        transform.translation = target + Vec3::new(0.0, 0.0, distance);
        transform.look_at(target, Vec3::Y);
    }
}