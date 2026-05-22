use bevy::{
    camera::ClearColorConfig,
    prelude::*,
};

use crate::{camera::prelude::SliceCamera, viewport::prelude::*};

pub struct DummyScenePlugin;

const VIEWPORT_PLANE_SIZE: f32 = 4.0;

impl Plugin for DummyScenePlugin {
    fn build(&self, app: &mut App) {
        app.add_systems(Startup, setup_dummy_scene);
    }
}

fn setup_dummy_scene(
    mut commands: Commands,
    mut meshes: ResMut<Assets<Mesh>>,
    mut materials: ResMut<Assets<StandardMaterial>>,
) {
    commands.spawn((
        DirectionalLight {
            illuminance: 10_000.0,
            ..default()
        },
        Transform::from_xyz(0.0, 0.0, 10.0).looking_at(Vec3::ZERO, Vec3::Y),
    ));

    let plane_mesh = meshes.add(
        Plane3d::default()
            .mesh()
            .size(VIEWPORT_PLANE_SIZE, VIEWPORT_PLANE_SIZE),
    );

    for id in ViewportId::ALL {
        let color = color_for_viewport(id);
        let world_pos = world_pos_for_viewport(id);
        let plane_rotation = Quat::from_rotation_x(std::f32::consts::FRAC_PI_2);

        commands.spawn((
            Name::new(format!("Nova Plane {id:?}")),
            Mesh3d(plane_mesh.clone()),
            MeshMaterial3d(materials.add(StandardMaterial {
                base_color: color,
                unlit: true,
                ..default()
            })),
            Transform::from_translation(world_pos).with_rotation(plane_rotation),
            ViewportPlane { id }
        ));

        commands.spawn((
            Name::new(format!("Nova Camera {id:?}")),
            Camera3d::default(),
            Camera {
                order: id.camera_order(),
                clear_color: ClearColorConfig::Custom(Color::srgb(0.015, 0.017, 0.022)),
                ..default()
            },
            Transform::from_translation(world_pos + Vec3::new(0.0, 0.0, 8.0))
                .looking_at(world_pos, Vec3::Y),
            NovaViewport { id },
            ViewportSlot {
                grid: id.grid_position(),
            },
            ViewportRect::default(),
            SliceCamera::new(world_pos, 8.0),
        ));
    }
}

fn color_for_viewport(id: ViewportId) -> Color {
    match id {
        ViewportId::Axial => Color::srgb(1.0, 0.0, 0.0),
        ViewportId::Sagittal => Color::srgb(0.0, 1.0, 0.0),
        ViewportId::Coronal => Color::srgb(0.0, 0.0, 1.0),
        ViewportId::Volume3D => Color::srgb(1.0, 1.0, 0.0),
    }
}

fn world_pos_for_viewport(id: ViewportId) -> Vec3 {
    match id {
        ViewportId::Axial => Vec3::new(0.0, 0.0, 0.0),
        ViewportId::Sagittal => Vec3::new(100.0, 0.0, 0.0),
        ViewportId::Coronal => Vec3::new(200.0, 0.0, 0.0),
        ViewportId::Volume3D => Vec3::new(300.0, 0.0, 0.0),
    }
}
