#pragma once

#include <SFML/Graphics/Color.hpp>
#include <cstdint>

namespace echo {

enum class SurfaceMaterial : std::uint8_t {
    Stone = 0,         // Cold slate-blue stone masonry (82% reflectivity)
    Concrete = 1,      // Industrial reinforced concrete (74% reflectivity)
    Metal = 2,         // Forged iron & steel bulkheads (94% reflectivity, long ring)
    Water = 3,         // Subterranean flooded canal (64% reflectivity, teal scatter)
    DampMoss = 4,      // Overgrown acoustic-damping moss (36% reflectivity)
    ResonanceDoor = 5  // Harmonic lock barrier (90% reflectivity, violet hum)
};

enum class WaveSignature : std::uint8_t {
    FootstepSneak = 0,
    FootstepWalk = 1,
    FootstepSprint = 2,
    DirectionalPing = 3,
    RadialPulse = 4,
    DecoyImpact = 5,
    MechanismClunk = 6,
    EnemyTremor = 7,
    AmbientDrip = 8
};

enum class AppState : std::uint8_t {
    MainMenu = 0,
    Playing = 1,
    Paused = 2,
    Settings = 3,
    Controls = 4,
    GameOver = 5,
    Victory = 6
};

struct MaterialProperties {
    float reflectivity;      // [0.0, 1.0] fraction of energy retained after reflection
    float persistenceSec;    // How long the illuminated wall trace stays visible
    float wallThickness;     // Visual bevel thickness in world units
    sf::Color echoColor;     // Primary acoustic illumination color
    sf::Color fillTint;      // Dark structural fill color when illuminated
    const char* label;
};

inline MaterialProperties getMaterialProperties(SurfaceMaterial mat) noexcept {
    switch (mat) {
        case SurfaceMaterial::Stone:
            return {0.82f, 4.0f, 10.0f, sf::Color(56, 189, 248), sf::Color(15, 32, 52), "Stone Masonry"};
        case SurfaceMaterial::Concrete:
            return {0.74f, 3.2f, 11.0f, sf::Color(125, 211, 252), sf::Color(22, 34, 48), "Reinforced Concrete"};
        case SurfaceMaterial::Metal:
            return {0.94f, 5.4f, 8.0f, sf::Color(34, 211, 238), sf::Color(18, 42, 56), "Forged Steel"};
        case SurfaceMaterial::Water:
            return {0.64f, 2.8f, 6.0f, sf::Color(45, 212, 191), sf::Color(10, 38, 44), "Subterranean Water"};
        case SurfaceMaterial::DampMoss:
            return {0.36f, 1.7f, 12.0f, sf::Color(52, 211, 153), sf::Color(12, 34, 28), "Damp Acoustic Moss"};
        case SurfaceMaterial::ResonanceDoor:
            return {0.90f, 4.8f, 9.0f, sf::Color(167, 139, 250), sf::Color(36, 22, 62), "Resonance Gate"};
    }
    return {0.82f, 4.0f, 10.0f, sf::Color(56, 189, 248), sf::Color(15, 32, 52), "Stone Masonry"};
}

namespace cfg {
    constexpr unsigned int kDefaultWindowWidth = 1280;
    constexpr unsigned int kDefaultWindowHeight = 800;
    constexpr float kWorldWidth = 1280.0f;
    constexpr float kWorldHeight = 800.0f;
    constexpr float kFixedTimeStep = 1.0f / 60.0f;

    // Pre-allocated simulation pool sizes (zero per-frame heap allocations)
    constexpr std::size_t kMaxActiveRays = 2560;
    constexpr std::size_t kMaxEchoTraces = 2048;
    constexpr std::size_t kMaxWaveRings = 48;
    constexpr std::uint8_t kDefaultMaxReflections = 3;

    // Player physical & movement parameters
    constexpr float kPlayerRadius = 12.0f;
    constexpr float kEnemyRadius = 14.0f;
    constexpr float kSpeedSneak = 86.0f;
    constexpr float kSpeedWalk = 154.0f;
    constexpr float kSpeedSprint = 238.0f;

    // Ability cooldowns (seconds)
    constexpr float kCooldownPing = 0.50f;
    constexpr float kCooldownRadialPulse = 2.10f;
    constexpr float kCooldownDecoy = 2.40f;
} // namespace cfg

} // namespace echo
