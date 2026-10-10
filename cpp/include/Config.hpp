#pragma once

#include <SFML/Graphics/Color.hpp>
#include <cstdint>

namespace echo {

enum class SurfaceMaterial : std::uint8_t {
    Stone = 0,     // High reflection (0.82), cold cyan acoustic signature
    Metal = 1,     // Very high reflection (0.94), sharp amber resonance
    Water = 2,     // Medium reflection (0.64), teal ripple scattering
    DampMoss = 3,  // High absorption (0.35), muted sage dampening
    ResonanceDoor = 4 // Interactive gate (0.88), violet harmonic signature
};

enum class WaveSignature : std::uint8_t {
    FootstepSneak = 0,
    FootstepWalk = 1,
    FootstepSprint = 2,
    DirectionalPing = 3,
    RadialPulse = 4,
    DecoyImpact = 5,
    MechanismClunk = 6,
    EnemyTremor = 7
};

struct MaterialProperties {
    float reflectivity;      // [0.0, 1.0] fraction of wave intensity preserved on reflection
    float persistenceSec;    // How long the illuminated wall echo remains visible
    sf::Color echoColor;     // Characteristic acoustic visual color
    const char* label;
};

inline MaterialProperties getMaterialProperties(SurfaceMaterial mat) noexcept {
    switch (mat) {
        case SurfaceMaterial::Stone:
            return {0.82f, 3.8f, sf::Color(56, 189, 248), "Stone Masonry"};
        case SurfaceMaterial::Metal:
            return {0.94f, 5.0f, sf::Color(245, 158, 11), "Forged Iron"};
        case SurfaceMaterial::Water:
            return {0.64f, 2.6f, sf::Color(45, 212, 191), "Subterranean Water"};
        case SurfaceMaterial::DampMoss:
            return {0.35f, 1.6f, sf::Color(74, 222, 128), "Damp Moss"};
        case SurfaceMaterial::ResonanceDoor:
            return {0.88f, 4.5f, sf::Color(167, 139, 250), "Resonance Gate"};
    }
    return {0.80f, 3.5f, sf::Color(56, 189, 248), "Stone Masonry"};
}

namespace cfg {
    constexpr unsigned int kWindowWidth = 1280;
    constexpr unsigned int kWindowHeight = 800;
    constexpr float kFixedTimeStep = 1.0f / 60.0f;

    // Acoustic simulation limits (pre-allocated pools to prevent per-frame heap allocation)
    constexpr std::size_t kMaxActiveRays = 2048;
    constexpr std::size_t kMaxEchoTraces = 1536;
    constexpr std::uint8_t kMaxReflections = 3;

    // Player movement constants
    constexpr float kPlayerRadius = 12.0f;
    constexpr float kSpeedSneak = 82.0f;
    constexpr float kSpeedWalk = 150.0f;
    constexpr float kSpeedSprint = 235.0f;

    // Cooldowns & resources
    constexpr float kCooldownPing = 0.65f;
    constexpr float kCooldownRadialPulse = 2.6f;
    constexpr float kCooldownDecoy = 4.0f;
} // namespace cfg

} // namespace echo
