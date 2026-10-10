#pragma once

#include "AcousticSystem.hpp"
#include "AssetManager.hpp"
#include "Labyrinth.hpp"
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>
#include <cstdint>
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

struct SonicBolt {
    sf::Vector2f pos;
    sf::Vector2f vel;
    float radius{6.5f};
    float lifetime{3.0f};
    bool active{false};
};

class EnemyAI {
public:
    EnemyAI();

    void configurePatrol(
        const std::vector<sf::Vector2f>& waypoints,
        EnemyVariant variant = EnemyVariant::Stalker
    );

    // Returns true if the enemy just transitioned to an alert state this frame
    bool update(
        float dt,
        sf::Vector2f playerPos,
        const std::vector<WallSegment>& walls,
        AcousticSystem& acoustics
    );

    // Block 1: Applies deterministic AI state transitions based on Noise Meter thresholds
    void applyNoiseTierStimulus(sf::Vector2f stimulusPos, NoiseTier tier) noexcept;

    // Applies harmonic sonar damage; returns true if this hit defeated the enemy
    bool applyAcousticDamage(
        float amount,
        bool isReflectedHit,
        sf::Vector2f sourcePos,
        const std::vector<WallSegment>& walls
    ) noexcept;

    // Checks if any ranged SonicBolt hit the player this frame (deactivates bolt on hit)
    bool consumeBoltHitOnPlayer(sf::Vector2f playerPos, float playerRadius) noexcept;

    void clearContactState() noexcept;

    void render(
        sf::RenderTarget& target,
        float echoIllumination,
        sf::Vector2f playerPos = {640.0f, 400.0f},
        const AssetManager* assets = nullptr
    ) const;

    bool isActive() const noexcept { return m_active; }
    sf::Vector2f position() const noexcept { return m_position; }
    sf::Vector2f targetPosition() const noexcept { return m_targetPos; }
    float hitRadius() const noexcept;
    EnemyState state() const noexcept { return m_state; }
    EnemyVariant variant() const noexcept { return m_variant; }
    float suspicion() const noexcept { return m_suspicion; }
    float health() const noexcept { return m_hp; }
    float maxHealth() const noexcept { return m_maxHp; }
    int bossPhase() const noexcept;
    bool hasCaughtPlayer() const noexcept { return m_state == EnemyState::Attack; }

    static const char* stateName(EnemyState s) noexcept;
    static const char* variantName(EnemyVariant v) noexcept;

private:
    bool hasLineOfSight(sf::Vector2f target, const std::vector<WallSegment>& walls) const noexcept;
    void steerToward(sf::Vector2f target, float speed, float dt, const std::vector<WallSegment>& walls) noexcept;
    void resolveWallCollisions(const std::vector<WallSegment>& walls) noexcept;
    void spawnSonicBolt(sf::Vector2f dir, float speed) noexcept;

    bool m_active{false};
    EnemyVariant m_variant{EnemyVariant::Stalker};
    sf::Vector2f m_position{800.0f, 300.0f};
    sf::Vector2f m_targetPos{800.0f, 300.0f};
    float m_facingAngle{3.14159f};
    std::vector<sf::Vector2f> m_waypoints;
    std::size_t m_waypointIdx{0};

    EnemyState m_state{EnemyState::Patrol};
    float m_hp{45.0f};
    float m_maxHp{45.0f};
    float m_stateTimer{0.0f};
    float m_suspicion{0.0f};
    float m_tremorTimer{0.0f};
    float m_rangedAttackTimer{0.0f};
    float m_hitFlashTimer{0.0f};
    float m_staggerTimer{0.0f};
    float m_animPhase{0.0f};
    float m_alertBlend{0.0f};

    std::vector<SonicBolt> m_bolts;
};

} // namespace echo
