#pragma once

#include "AcousticSystem.hpp"
#include "Config.hpp"
#include <SFML/System/Vector2.hpp>
#include <vector>

namespace echo {

enum class MovementStance : std::uint8_t {
    Sneak = 0,
    Walk = 1,
    Sprint = 2
};

class Player {
public:
    Player();

    void reset(sf::Vector2f spawnPos) noexcept;

    void update(
        float dt,
        sf::Vector2f moveInput,
        MovementStance stance,
        const std::vector<WallSegment>& walls,
        AcousticSystem& acoustics
    );

    bool triggerDirectionalPing(sf::Vector2f aimTarget, AcousticSystem& acoustics);
    bool triggerRadialPulse(AcousticSystem& acoustics);
    bool throwDecoy(sf::Vector2f targetPoint, AcousticSystem& acoustics);

    sf::Vector2f position() const noexcept { return m_position; }
    float facingAngleRad() const noexcept { return m_facingAngle; }
    float stamina() const noexcept { return m_stamina; }
    int decoyCount() const noexcept { return m_decoyCount; }
    void addDecoy(int count = 1) noexcept { m_decoyCount += count; }

    float pingCooldown() const noexcept { return m_cooldownPing; }
    float pulseCooldown() const noexcept { return m_cooldownPulse; }

private:
    void resolveWallCollisions(const std::vector<WallSegment>& walls) noexcept;

    sf::Vector2f m_position{160.0f, 400.0f};
    float m_facingAngle{0.0f};
    float m_stamina{100.0f};
    float m_stepAccumulator{0.0f};
    float m_cooldownPing{0.0f};
    float m_cooldownPulse{0.0f};
    float m_cooldownDecoy{0.0f};
    int m_decoyCount{3};
};

} // namespace echo
