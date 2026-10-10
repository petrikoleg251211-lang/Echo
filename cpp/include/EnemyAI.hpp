#pragma once

#include "AcousticSystem.hpp"
#include <SFML/System/Vector2.hpp>
#include <vector>

namespace echo {

enum class EnemyState : std::uint8_t {
    Idle = 0,
    Patrol = 1,
    InvestigateSound = 2,
    SearchLastKnown = 3,
    PursuePlayer = 4,
    Attack = 5,
    ReturnToPatrol = 6
};

class EnemyAI {
public:
    EnemyAI();

    void configurePatrol(const std::vector<sf::Vector2f>& waypoints);

    void update(
        float dt,
        sf::Vector2f playerPos,
        const std::vector<WallSegment>& walls,
        AcousticSystem& acoustics
    );

    bool isActive() const noexcept { return m_active; }
    sf::Vector2f position() const noexcept { return m_position; }
    EnemyState state() const noexcept { return m_state; }
    float suspicion() const noexcept { return m_suspicion; }
    bool hasCaughtPlayer() const noexcept { return m_state == EnemyState::Attack; }

    static const char* stateName(EnemyState s) noexcept;

private:
    bool hasLineOfSight(sf::Vector2f target, const std::vector<WallSegment>& walls) const noexcept;
    void moveAlongDirection(sf::Vector2f target, float speed, float dt, const std::vector<WallSegment>& walls) noexcept;

    bool m_active{false};
    sf::Vector2f m_position{800.0f, 300.0f};
    sf::Vector2f m_targetPos{800.0f, 300.0f};
    std::vector<sf::Vector2f> m_waypoints;
    std::size_t m_waypointIdx{0};

    EnemyState m_state{EnemyState::Patrol};
    float m_stateTimer{0.0f};
    float m_suspicion{0.0f}; // [0.0, 100.0]
    float m_tremorTimer{0.0f};
};

} // namespace echo
