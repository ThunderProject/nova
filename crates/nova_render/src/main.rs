use bevy::{
    image::{
        ImageAddressMode, ImageFilterMode, ImageLoaderSettings, ImageSampler,
        ImageSamplerDescriptor,
    },
    prelude::*,
};

fn main() {
    App::new()
        .add_plugins(DefaultPlugins)
        .add_systems(Startup, setup)
        .add_systems(Update, (fit_image_to_window, pan_camera, zoom_camera))
        .run();
}

#[derive(Component)]
struct MainViewportCamera;

#[derive(Component)]
struct ViewportImage {
    handle: Handle<Image>,
}

fn setup(mut commands: Commands, asset_server: Res<AssetServer>) {
    commands.spawn((
        Camera2d,
        MainViewportCamera,
    ));

    let image: Handle<Image> = asset_server.load_with_settings(
        "cherry_blossom_cloeseup.ktx2",
        |settings: &mut ImageLoaderSettings| {
            settings.sampler = ImageSampler::Descriptor(ImageSamplerDescriptor {
                mag_filter: ImageFilterMode::Linear,
                min_filter: ImageFilterMode::Linear,
                mipmap_filter: ImageFilterMode::Linear,
                address_mode_u: ImageAddressMode::ClampToEdge,
                address_mode_v: ImageAddressMode::ClampToEdge,
                address_mode_w: ImageAddressMode::ClampToEdge,
                ..default()
            });
        },
    );

    commands.spawn((
        Sprite::from_image(image.clone()),
        Transform::from_xyz(0.0, 0.0, 0.0),
        ViewportImage { handle: image },
    ));
}

fn fit_image_to_window(
    windows: Query<&Window>,
    images: Res<Assets<Image>>,
    mut query: Query<(&ViewportImage, &mut Transform)>,
) {
    let Ok(window) = windows.single() else {
        return;
    };

    for (viewport_image, mut transform) in &mut query {
        let Some(image) = images.get(&viewport_image.handle) else {
            continue;
        };

        let size = image.texture_descriptor.size;

        let image_width = size.width as f32;
        let image_height = size.height as f32;

        let scale = (window.width() / image_width)
            .min(window.height() / image_height);

        transform.scale = Vec3::splat(scale);
    }
}

fn pan_camera(
    mut camera: Query<&mut Transform, With<MainViewportCamera>>,
    keyboard: Res<ButtonInput<KeyCode>>,
    time: Res<Time>,
) {
    let Ok(mut transform) = camera.single_mut() else {
        return;
    };

    let speed = 800.0 * time.delta_secs();

    if keyboard.pressed(KeyCode::KeyA) {
        transform.translation.x -= speed;
    }
    if keyboard.pressed(KeyCode::KeyD) {
        transform.translation.x += speed;
    }
    if keyboard.pressed(KeyCode::KeyW) {
        transform.translation.y += speed;
    }
    if keyboard.pressed(KeyCode::KeyS) {
        transform.translation.y -= speed;
    }
}

fn zoom_camera(
    mut camera: Query<&mut Projection, With<MainViewportCamera>>,
    keyboard: Res<ButtonInput<KeyCode>>,
    time: Res<Time>,
) {
    let Ok(mut projection) = camera.single_mut() else {
        return;
    };

    let Projection::Orthographic(ref mut ortho) = *projection else {
        return;
    };

    let zoom_speed = 2.0 * time.delta_secs();

    if keyboard.pressed(KeyCode::KeyQ) {
        ortho.scale *= 1.0 + zoom_speed;
    }

    if keyboard.pressed(KeyCode::KeyE) {
        ortho.scale *= 1.0 - zoom_speed;
        // ortho.scale = ortho.scale.max(0.01);
    }
}
