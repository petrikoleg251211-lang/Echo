#pragma once

#include "Config.hpp"
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>
#include <string>
#include <vector>

namespace echo {

struct WallSegment {
    sf::Vector2f a;
    sf::Vector2f b;
    SurfaceMaterial material{SurfaceMaterial::Stone};
    int doorId{-1}; // -1 if static wall, >= 0 if tied to a harmonic mechanism or resonator
    bool isOpen{false};
};

// Acoustic Resonance Crystal: charges up when struck by player sonar rays (including reflected rays)
struct AcousticResonator {
    int id{0};
    sf::Vector2f position;
    float radius{16.0f};
    float energy{0.0f}; // [0.0, 1.0]; locks at 1.0 once fully energized
    bool activated{false};
    int unlocksDoorId{-1};
    std::string label;
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
    std::uint8_t maxReflections{cfg::kDefaultMaxReflections};
    WaveSignature signature{WaveSignature::RadialPulse};
    sf::Color tint{56, 189, 248};
    bool emittedByPlayer{true};
    bool active{false};
};

struct WallEchoTrace {
    sf::Vector2f hitPoint;
    sf::Vector2f tangent;
    sf::Vector2f normal;
    float halfLength{14.0f};
    float intensity{1.0f};
    float age{0.0f};
    float lifetime{3.5f};
    SurfaceMaterial material{SurfaceMaterial::Stone};
    bool active{false};
};

struct ExpandingWaveRing {
    sf::Vector2f origin;
    float radius{0.0f};
    float maxRadius{400.0f};
    float speed{360.0f};
    float intensity{1.0f};
    float centerAngle{0.0f};
    float spreadAngle{6.2831853f};
    sf::Color color{56, 189, 248};
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

    // Updates rays, wall echoes, and optionally charges AcousticResonators or reveals Secret Walls (doorId >= 900).
    // Returns IDs of any resonators or secret doors that reached activation during this step.
    std::vector<int> update(
        float dt,
        const std::vector<WallSegment>& walls,
        std::vector<AcousticResonator>* resonators = nullptr,
        float resonatorChargeScale = 1.0f
    );

    // Consumes player sonar rays (DirectionalPing / RadialPulse / DecoyImpact) intersecting (targetPos, targetRadius),
    // spawning echo traces on the target and returning total harmonic impact energy.
    float consumeRayImpactAt(
        sf::Vector2f targetPos,
        float targetRadius,
        bool& outReflectedHit
    ) noexcept;

    void render(
        sf::RenderTarget& target,
        float ambientVisibility = 0.03f,
        const std::vector<WallSegment>* walls = nullptr
    ) const;

    // Computes how strongly a given point in the world is currently revealed by sound waves/echoes
    float queryIlluminationAt(sf::Vector2f point, sf::Vector2f playerPos) const noexcept;

    // Computes multi-wall material transmission factor in [0.05, 1.0] between sound source and listener
    static float computeTransmissionFactor(
        sf::Vector2f source,
        sf::Vector2f listener,
        const std::vector<WallSegment>& walls
    ) noexcept;

    const std::vector<AcousticEvent>& recentEvents() const noexcept { return m_recentEvents; }
    void clearRecentEvents() noexcept { m_recentEvents.clear(); }

    void setMaxReflections(std::uint8_t maxRefl) noexcept { m_maxReflections = maxRefl; }
    std::uint8_t maxReflections() const noexcept { return m_maxReflections; }

    void setRayDensityScale(float scale) noexcept { m_rayDensityScale = scale; }
    float rayDensityScale() const noexcept { return m_rayDensityScale; }

    std::size_t activeRayCount() const noexcept { return m_activeRayCount; }
    std::size_t activeEchoCount() const noexcept { return m_activeEchoCount; }
    std::size_t totalPulsesEmitted() const noexcept { return m_totalPulsesEmitted; }
    void resetStats() noexcept { m_totalPulsesEmitted = 0; }

private:
    void spawnEchoTrace(
        sf::Vector2f hitPoint,
        sf::Vector2f wallTangent,
        sf::Vector2f wallNormal,
        float intensity,
        SurfaceMaterial material
    );

    std::vector<AcousticRay> m_rays;
    std::vector<WallEchoTrace> m_echoes;
    std::vector<ExpandingWaveRing> m_rings;
    std::vector<AcousticEvent> m_recentEvents;

    mutable sf::VertexArray m_wallQuads;
    mutable sf::VertexArray m_echoLines;
    mutable sf::VertexArray m_waveLines;

    std::size_t m_rayCursor{0};
    std::size_t m_echoCursor{0};
    std::size_t m_ringCursor{0};
    std::size_t m_activeRayCount{0};
    std::size_t m_activeEchoCount{0};
    std::size_t m_totalPulsesEmitted{0};

    std::uint8_t m_maxReflections{cfg::kDefaultMaxReflections};
    float m_rayDensityScale{1.0f};
};

} // namespace echo
