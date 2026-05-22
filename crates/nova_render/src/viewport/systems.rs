use bevy::{
    camera::{ClearColorConfig, Viewport as BevyViewport},
    prelude::*,
    window::{PrimaryWindow, WindowResized},
};

use crate::camera::prelude::SliceCamera;

use super::{
    components::{
        NovaViewport,
        ViewportFocusOutline,
        ViewportRect,
        ViewportSlot,
    },
    layout::{ActiveViewport, ViewportLayout},
    messages::ViewportCommand,
};

const FOCUS_OUTLINE_COLOR: Color = Color::srgb(0.2, 0.58, 1.0);
const FOCUS_OUTLINE_WIDTH_PX: f32 = 2.0;
const FOCUS_OUTLINE_CAMERA_ORDER: isize = 100;

pub fn spawn_viewport_focus_outline(mut commands: Commands) {
    commands.spawn((
        Name::new("Nova Viewport Focus UI Camera"),
        Camera2d,
        Camera {
            order: FOCUS_OUTLINE_CAMERA_ORDER,
            clear_color: ClearColorConfig::None,
            ..default()
        },
        IsDefaultUiCamera,
    ));

    commands.spawn((
        Name::new("Nova Viewport Focus Outline"),
        Node {
            position_type: PositionType::Absolute,
            width: Val::Px(0.0),
            height: Val::Px(0.0),
            border: UiRect::all(Val::Px(FOCUS_OUTLINE_WIDTH_PX)),
            border_radius: BorderRadius::ZERO,
            ..default()
        },
        BorderColor::all(FOCUS_OUTLINE_COLOR),
        ZIndex(FOCUS_OUTLINE_CAMERA_ORDER as i32),
        ViewportFocusOutline,
    ));
}

pub fn apply_viewport_layout(
    window: Single<&Window, With<PrimaryWindow>>,
    layout: Res<ViewportLayout>,
    mut cameras: Query<(&ViewportSlot, &mut ViewportRect, &mut Camera), With<NovaViewport>>,
) {
    let full_size = window.physical_size();

    if full_size.x == 0 || full_size.y == 0 {
        return;
    }

    for (slot, mut rect, mut camera) in &mut cameras {
        let computed = layout.rect_for_slot(full_size, slot.grid);

        rect.position = computed.position;
        rect.size = computed.size;

        camera.viewport = Some(BevyViewport {
            physical_position: computed.position,
            physical_size: computed.size,
            ..default()
        });
    }
}

pub fn refresh_viewport_layout_on_resize(
    mut resize_events: MessageReader<WindowResized>,
    window: Single<&Window, With<PrimaryWindow>>,
    layout: Res<ViewportLayout>,
    mut cameras: Query<(&ViewportSlot, &mut ViewportRect, &mut Camera), With<NovaViewport>>,
) {
    if resize_events.read().next().is_none() {
        return;
    }

    let full_size = window.physical_size();

    if full_size.x == 0 || full_size.y == 0 {
        return;
    }

    for (slot, mut rect, mut camera) in &mut cameras {
        let computed = layout.rect_for_slot(full_size, slot.grid);

        rect.position = computed.position;
        rect.size = computed.size;

        camera.viewport = Some(BevyViewport {
            physical_position: computed.position,
            physical_size: computed.size,
            ..default()
        });
    }
}

pub fn focus_viewport_from_click(
    buttons: Res<ButtonInput<MouseButton>>,
    window: Single<&Window, With<PrimaryWindow>>,
    viewports: Query<(&NovaViewport, &ViewportRect)>,
    mut writer: MessageWriter<ViewportCommand>,
) {
    if !buttons.just_pressed(MouseButton::Left) {
        return;
    }

    let Some(cursor_logical) = window.cursor_position() else {
        return;
    };

    let scale_factor = window.scale_factor() as f32;
    let cursor_physical = (cursor_logical * scale_factor).as_uvec2();

    for (viewport, rect) in &viewports {
        if rect.contains(cursor_physical) {
            writer.write(ViewportCommand::Focus {
                viewport: viewport.id,
            });
            break;
        }
    }
}

pub fn handle_viewport_commands(
    mut reader: MessageReader<ViewportCommand>,
    mut active: ResMut<ActiveViewport>,
    mut cameras: Query<(&NovaViewport, &mut SliceCamera)>,
) {
    for command in reader.read() {
        match *command {
            ViewportCommand::Focus { viewport } => {
                if active.0 != viewport {
                    active.0 = viewport;
                    info!("Active viewport: {viewport:?}");
                }
            }

            ViewportCommand::ResetView { viewport } => {
                for (vp, mut view) in &mut cameras {
                    if vp.id == viewport {
                        view.reset();
                    }
                }
            }

            ViewportCommand::Pan {
                viewport,
                delta_physical_px,
            } => {
                for (vp, mut view) in &mut cameras {
                    if vp.id != viewport {
                        continue;
                    }

                    let world_per_pixel = 0.01 / view.zoom.max(0.001);

                    view.pan.0.x -= delta_physical_px.x * world_per_pixel;
                    view.pan.0.y += delta_physical_px.y * world_per_pixel;
                }
            }

            ViewportCommand::Zoom {
                viewport,
                factor,
                anchor_physical_px: _,
            } => {
                for (vp, mut view) in &mut cameras {
                    if vp.id != viewport {
                        continue;
                    }

                    view.zoom = (view.zoom * factor).clamp(0.05, 64.0);
                }
            }
        }
    }
}

pub fn sync_focus_outline(
    window: Single<&Window, With<PrimaryWindow>>,
    active: Res<ActiveViewport>,
    viewports: Query<(&NovaViewport, &ViewportRect)>,
    mut outlines: Query<(&mut Node, &mut BorderColor), With<ViewportFocusOutline>>,
) {
    let Ok((mut node, mut border_color)) = outlines.single_mut() else {
        return;
    };

    let Some((_, rect)) = viewports
        .iter()
        .find(|(viewport, _)| viewport.id == active.0)
    else {
        border_color.set_all(Color::NONE);
        return;
    };

    if rect.is_empty() {
        border_color.set_all(Color::NONE);
        return;
    }

    let scale_factor = window.scale_factor() as f32;
    let left = rect.position.x as f32 / scale_factor;
    let top = rect.position.y as f32 / scale_factor;
    let width = rect.size.x as f32 / scale_factor;
    let height = rect.size.y as f32 / scale_factor;

    node.left = Val::Px(left);
    node.top = Val::Px(top);
    node.width = Val::Px(width);
    node.height = Val::Px(height);
    border_color.set_all(FOCUS_OUTLINE_COLOR);
}
