use bevy::math::Vec2;

#[derive(Debug, Copy, Clone, PartialEq)]
pub struct WorldPosition(pub Vec2);

impl Default for WorldPosition {
    fn default() -> Self {
        Self(Vec2::ZERO)
    }
}