#pragma once

#include "AcousticSystem.hpp"
#include "Config.hpp"
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>
#include <vector>

namespace echo {

enum class MovementStance : std::uint8_t {
    Sneak = 0,
    Walk = 1,
    Sprint = 2
};

struct DecoyProjectile {
    sf::Vector2f startPos;
    sf::Vector2f currentPos;
    sf::Vector2f targetPos;
    float progress{0.0f};
    float duration{0.28f};
    bool active{false};
};

class Player {
public:
    Player();

    void reset(sf::Vector2f spawnPos) noexcept;

    // Returns true if a footstep sound was emitted this frame (outSignature set)
    bool update(
        float dt,
        sf::Vector2f moveInput,
        MovementStance requestedStance,
        sf::Vector2f aimWorldPos,
        const std::vector<WallSegment>& walls,
        AcousticSystem& acoustics,
        WaveSignature& outEmittedSignature
    );

    bool triggerDirectionalPing(sf::Vector2f aimTarget, AcousticSystem& acoustics);
    bool triggerRadialPulse(AcousticSystem& acoustics);
    bool throwDecoy(sf::Vector2f targetPoint, const std::vector<WallSegment>& walls);

    void render(sf::RenderTarget& target) const;

    sf::Vector2f position() const noexcept { return m_position; }
    float facingAngleRad() const noexcept { return m_facingAngle; }
    float stamina() const noexcept { return m_stamina; }
    MovementStance activeStance() const noexcept { return m_activeStance; }

    int decoyCount() const noexcept { return m_decoyCount; }
    void setDecoyCount(int count) noexcept { m_decoyCount = count; }
    void addDecoy(int count = 1) noexcept { m_decoyCount += count; }

    float pingCooldown() const noexcept { return m_cooldownPing; }
    float pulseCooldown() const noexcept { return m_cooldownPulse; }
    float decoyCooldown() const noexcept { return m_cooldownDecoy; }

private:
    void resolveWallCollisions(const std::vector<WallSegment>& walls) noexcept;

    sf::Vector2f m_position{145.0f, 400.0f};
    float m_facingAngle{0.0f};
    float m_stamina{100.0f};
    float m_stepAccumulator{0.0f};
    float m_walkCyclePhase{0.0f};
    float m_cooldownPing{0.0f};
    float m_cooldownPulse{0.0f};
    float m_cooldownDecoy{0.0f};
    MovementStance m_activeStance{MovementStance::Walk};
    int m_decoyCount{4};
    DecoyProjectile m_decoyProj;
};

} // namespace echo
