#pragma once

#include "Config.hpp"
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace echo {

struct WallSegment {
    sf::Vector2f a;
    sf::Vector2f b;
    SurfaceMaterial material{SurfaceMaterial::Stone};
    int doorId{-1};
    bool isOpen{false};
};

// Block 2.5: Acoustic Resonator Column / Crystal
// When struck by sonar, it hums for 4 seconds, illuminating a radius around itself and attracting enemies.
struct AcousticResonator {
    int id{0};
    sf::Vector2f position;
    float radius{18.0f};
    float energy{0.0f}; // 0.0 .. 1.0
    bool activated{false};
    int targetDoorId{-1};
    std::string label;
    float humTimer{0.0f};      // Block 2.5: 4.0s active humming illumination & enemy lure timer
    float humPulseAccum{0.0f}; // Periodic acoustic lure pulse timer while humming
};

struct AcousticRay {
    sf::Vector2f pos;
    sf::Vector2f prevPos;
    sf::Vector2f dir;
    float speed{420.0f};
    float intensity{1.0f};
    float maxDistance{520.0f};
    float traveled{0.0f};
    std::uint8_t reflections{0};
    std::uint8_t maxReflections{3};
    WaveSignature signature{WaveSignature::DirectionalPing};
    AcousticFrequency frequency{AcousticFrequency::Mid};
    sf::Color tint{56, 189, 248};
    bool emittedByPlayer{true};
    bool metalBonusApplied{false};
    bool waterBoostApplied{false};
    bool crystalSplitApplied{false};
    bool active{false};
};

struct WallEchoTrace {
    sf::Vector2f hitPoint;
    sf::Vector2f tangent;
    sf::Vector2f normal;
    float halfLength{14.0f};
    float intensity{1.0f};
    float age{0.0f};
    float lifetime{2.8f};
    SurfaceMaterial material{SurfaceMaterial::Stone};
    bool active{false};
};

struct ExpandingWaveRing {
    sf::Vector2f origin;
    float radius{4.0f};
    float maxRadius{450.0f};
    float speed{380.0f};
    float intensity{1.0f};
    float centerAngle{0.0f};
    float spreadAngle{6.2831853f};
    sf::Color color{56, 189, 248};
    bool active{false};
};

struct AcousticEvent {
    sf::Vector2f origin;
    float loudness{1.0f};
    float radius{400.0f};
    WaveSignature signature{WaveSignature::DirectionalPing};
    AcousticFrequency frequency{AcousticFrequency::Mid};
    bool emittedByPlayer{true};
    bool isGroundVibration{false};
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

    // Block 2.4: Emits concentric water rings when stepping on water (2x sound range)
    void emitWaterRipples(sf::Vector2f origin, float baseRadius = 240.0f);

    // Block 2: Updates rays, reflections (Moss -80%, Metal +2 bounces, Glass shattering, Water 2x range, EchoCrystal splitting)
    // and Resonator Column 4s humming.
    std::vector<int> update(
        float dt,
        std::vector<WallSegment>& walls,
        std::vector<AcousticResonator>* resonators = nullptr,
        float resonatorChargeScale = 1.0f
    );

    // Const-walls convenience overload for read-only simulations
    std::vector<int> update(
        float dt,
        const std::vector<WallSegment>& walls,
        std::vector<AcousticResonator>* resonators = nullptr,
        float resonatorChargeScale = 1.0f
    );

    // Consumes player-emitted offensive sonar rays intersecting a target (Enemy / Boss)
    // Returns accumulated acoustic impact energy and sets outReflectedHit / outHighFreqHit.
    float consumeRayImpactAt(
        sf::Vector2f targetPos,
        float targetRadius,
        bool& outReflectedHit,
        bool* outHighFreqHit = nullptr
    ) noexcept;

    // Computes multi-wall transmission factor [0.08 .. 1.0] between sound source and listener
    static float computeTransmissionFactor(
        sf::Vector2f source,
        sf::Vector2f listener,
        const std::vector<WallSegment>& walls,
        AcousticFrequency freq = AcousticFrequency::Mid
    ) noexcept;

    // Block 2.1: Checks if a position is inside a DampMoss acoustic shelter (halves noise / 2x silence decay)
    static bool isInDampMossShelter(
        sf::Vector2f pos,
        const std::vector<WallSegment>& walls,
        SurfaceMaterial floorMat = SurfaceMaterial::Stone
    ) noexcept;

    void render(
        sf::RenderTarget& target,
        float ambientVisibility,
        const std::vector<WallSegment>* walls = nullptr
    ) const;

    // Queries local illumination [0..1] at a world point from active rays, wall echoes, and humming resonators
    float queryIlluminationAt(
        sf::Vector2f point,
        sf::Vector2f playerPos,
        const std::vector<AcousticResonator>* resonators = nullptr
    ) const noexcept;

    const std::vector<AcousticEvent>& recentEvents() const noexcept { return m_recentEvents; }
    void clearRecentEvents() noexcept { m_recentEvents.clear(); }

    // Block 2.3: Consumes count of Glass walls shattered since last check (each adds +40 noise)
    int consumeShatteredGlassCount(sf::Vector2f* outLastShatterPos = nullptr) noexcept;

    // Stage 2: Consumes count of EchoCrystal ray splits since last check
    int consumeCrystalSplitCount() noexcept;

    void setRayDensityScale(float scale) noexcept { m_rayDensityScale = std::clamp(scale, 0.4f, 2.0f); }
    float rayDensityScale() const noexcept { return m_rayDensityScale; }

    void setMaxReflections(std::uint8_t maxRefl) noexcept { m_maxReflections = std::clamp<std::uint8_t>(maxRefl, 1, 8); }
    std::uint8_t maxReflections() const noexcept { return m_maxReflections; }

    void setActiveFrequency(AcousticFrequency freq) noexcept { m_activeFrequency = freq; }
    AcousticFrequency activeFrequency() const noexcept { return m_activeFrequency; }

    std::size_t activeRayCount() const noexcept { return m_activeRayCount; }
    std::size_t activeEchoCount() const noexcept { return m_activeEchoCount; }
    std::uint32_t totalPulsesEmitted() const noexcept { return m_totalPulsesEmitted; }

private:
    void spawnEchoTrace(
        sf::Vector2f hitPoint,
        sf::Vector2f wallTangent,
        sf::Vector2f wallNormal,
        float intensity,
        SurfaceMaterial material,
        AcousticFrequency freq
    );

    void spawnSplitRay(const AcousticRay& parentRay, sf::Vector2f splitOrigin, float angleOffsetRad) noexcept;

    std::vector<AcousticRay> m_rays;
    std::vector<WallEchoTrace> m_echoes;
    std::vector<ExpandingWaveRing> m_rings;
    std::vector<AcousticEvent> m_recentEvents;

    std::size_t m_rayCursor{0};
    std::size_t m_echoCursor{0};
    std::size_t m_ringCursor{0};

    std::size_t m_activeRayCount{0};
    std::size_t m_activeEchoCount{0};
    std::uint32_t m_totalPulsesEmitted{0};

    int m_shatteredGlassCount{0};
    int m_crystalSplitCount{0};
    sf::Vector2f m_lastGlassShatterPos{640.0f, 400.0f};

    float m_rayDensityScale{1.0f};
    std::uint8_t m_maxReflections{cfg::kDefaultMaxReflections};
    AcousticFrequency m_activeFrequency{AcousticFrequency::Mid};

    mutable sf::VertexArray m_wallQuads;
    mutable sf::VertexArray m_echoLines;
    mutable sf::VertexArray m_waveLines;
};

} // namespace echo
