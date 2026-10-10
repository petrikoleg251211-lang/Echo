#pragma once

#include "Config.hpp"
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>
#include <vector>

namespace echo {

struct WallSegment {
    sf::Vector2f a;
    sf::Vector2f b;
    SurfaceMaterial material{SurfaceMaterial::Stone};
    int doorId{-1}; // -1 if static wall, >= 0 if tied to a door/gate
    bool isOpen{false};
};

struct AcousticRay {
    sf::Vector2f pos;
    sf::Vector2f prevPos;
    sf::Vector2f dir;
    float speed{360.0f};
    float intensity{1.0f};
    float maxDistance{500.0f};
    float traveled{0.0f};
    std::uint8_t reflections{0};
    std::uint8_t maxReflections{cfg::kMaxReflections};
    WaveSignature signature{WaveSignature::RadialPulse};
    sf::Color tint{56, 189, 248};
    bool active{false};
};

struct WallEchoTrace {
    sf::Vector2f hitPoint;
    sf::Vector2f tangent;
    float halfLength{14.0f};
    float intensity{1.0f};
    float age{0.0f};
    float lifetime{3.5f};
    SurfaceMaterial material{SurfaceMaterial::Stone};
    bool active{false};
};

struct AcousticEvent {
    sf::Vector2f origin;
    float loudness{1.0f}; // Normalized [0.0, 1.5]
    float radius{300.0f};
    WaveSignature signature{WaveSignature::FootstepWalk};
    bool emittedByPlayer{true};
};

class AcousticSystem {
public:
    AcousticSystem();

    void clear() noexcept;

    // Emits a full 360-degree radial pulse or directional cone of rays
    void emitPulse(
        sf::Vector2f origin,
        float speed,
        float maxDistance,
        float initialIntensity,
        std::size_t rayCount,
        WaveSignature signature,
        float centerAngleRad = 0.0f,
        float spreadAngleRad = 6.2831853f,
        bool emittedByPlayer = true
    );

    // Advances wavefront rays, resolves segment intersections & specular reflections
    void update(float dt, const std::vector<WallSegment>& walls);

    // Batched additive rendering of wavefronts and wall phosphor echoes
    void render(sf::RenderTarget& target) const;

    // Consumes recent acoustic events so EnemyAI can react to sounds
    const std::vector<AcousticEvent>& recentEvents() const noexcept { return m_recentEvents; }
    void clearRecentEvents() noexcept { m_recentEvents.clear(); }

    std::size_t activeRayCount() const noexcept { return m_activeRayCount; }
    std::size_t activeEchoCount() const noexcept { return m_activeEchoCount; }

private:
    void spawnEchoTrace(sf::Vector2f hitPoint, sf::Vector2f wallDir, float intensity, SurfaceMaterial material);

    std::vector<AcousticRay> m_rays;
    std::vector<WallEchoTrace> m_echoes;
    std::vector<AcousticEvent> m_recentEvents;

    mutable sf::VertexArray m_waveVertices;
    mutable sf::VertexArray m_echoVertices;

    std::size_t m_rayCursor{0};
    std::size_t m_echoCursor{0};
    std::size_t m_activeRayCount{0};
    std::size_t m_activeEchoCount{0};
};

} // namespace echo
