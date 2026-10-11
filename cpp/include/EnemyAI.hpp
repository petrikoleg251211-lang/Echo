#pragma once

#include "AcousticSystem.hpp"
#include "AssetManager.hpp"
#include "Labyrinth.hpp"
#include <SFML/Graphics/RenderTarget.hpp>
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

struct SonicBolt {
    sf::Vector2f pos;
    sf::Vector2f vel;
    float radius{6.0f};
    float lifetime{3.0f};
    bool active{false};
};

class EnemyAI {
public:
    EnemyAI();

    void configurePatrol(const std::vector<sf::Vector2f>& waypoints, EnemyVariant variant = EnemyVariant::Stalker);

    // Returns true if the enemy just transitioned into an alert/hunting state this frame
    bool update(
        float dt,
        sf::Vector2f playerPos,
        const std::vector<WallSegment>& walls,
        AcousticSystem& acoustics
    );

    void render(
        sf::RenderTarget& target,
        float echoIllumination,
        sf::Vector2f playerPos,
        const AssetManager* assets = nullptr
    ) const;

    // Applies offensive acoustic resonance damage from player Sonar Pings / Pulses / Stomp
    // Returns true if this hit disrupted (defeated) the enemy.
    bool applyAcousticDamage(
        float amount,
        bool isReflectedHit,
        sf::Vector2f sourcePos,
        const std::vector<WallSegment>& walls,
        bool isHighFreqHit = false
    ) noexcept;

    // Block 3.2: Wide Stomp pushes nearby enemies away and staggers them
    bool applyStompKnockback(
        sf::Vector2f stompOrigin,
        float stompRadius,
        float knockbackDist,
        const std::vector<WallSegment>& walls
    ) noexcept;

    // Block 1.2 & Block 4.2: Applies Noise Meter threshold stimulus to enemy AI
    // (DeafBrute ignores airborne stimuli unless isGroundVibration is true)
    void applyNoiseTierStimulus(
        sf::Vector2f stimulusPos,
        NoiseTier tier,
        bool isGroundVibration = false
    ) noexcept;

    // Checks if any active ranged SonicBolt hit the player (and deactivates that bolt)
    bool consumeBoltHitOnPlayer(sf::Vector2f playerPos, float playerRadius) noexcept;

    // Block 4.4: Returns true if EchoSwarm amplified a player sound this frame (adds +12 noise & alerts room)
    bool consumeSwarmAmplification() noexcept;

    void clearContactState() noexcept;

    bool isActive() const noexcept { return m_active; }
    EnemyState state() const noexcept { return m_state; }
    EnemyVariant variant() const noexcept { return m_variant; }
    sf::Vector2f position() const noexcept { return m_position; }
    float suspicion() const noexcept { return m_suspicion; }
    float health() const noexcept { return m_hp; }
    float maxHealth() const noexcept { return m_maxHp; }
    float hitRadius() const noexcept;
    int bossPhase() const noexcept;
    BossArenaAcousticPhase bossAcousticPhase() const noexcept;
    bool isDisguisedMimic() const noexcept { return m_variant == EnemyVariant::Mimic && m_disguised; }
    bool hasCaughtPlayer() const noexcept { return m_active && m_state == EnemyState::Attack; }

    static const char* stateName(EnemyState s) noexcept;
    static const char* variantName(EnemyVariant v) noexcept;

private:
    bool hasLineOfSight(sf::Vector2f target, const std::vector<WallSegment>& walls) const noexcept;
    void steerToward(sf::Vector2f target, float speed, float dt, const std::vector<WallSegment>& walls) noexcept;
    void resolveWallCollisions(const std::vector<WallSegment>& walls) noexcept;
    void spawnSonicBolt(sf::Vector2f dir, float speed) noexcept;

    bool m_active{false};
    EnemyVariant m_variant{EnemyVariant::Stalker};
    EnemyState m_state{EnemyState::Idle};

    sf::Vector2f m_position{640.0f, 400.0f};
    sf::Vector2f m_targetPos{640.0f, 400.0f};
    std::vector<sf::Vector2f> m_waypoints;
    std::size_t m_waypointIdx{0};

    float m_hp{45.0f};
    float m_maxHp{45.0f};
    float m_hitFlashTimer{0.0f};
    float m_staggerTimer{0.0f};
    float m_rangedAttackTimer{1.8f};

    float m_suspicion{0.0f};
    float m_stateTimer{0.0f};
    float m_tremorTimer{0.0f};
    float m_facingAngle{0.0f};
    float m_animPhase{0.0f};
    float m_alertBlend{0.0f};

    // Block 4 specialized acoustic state
    bool m_disguised{false};                  // Block 4.3: Mimic crystal disguise state
    bool m_inhaleWarningEmitted{false};       // Block 4.1: SilentStalker 0.5s pre-attack inhale
    bool m_triggeredSwarmAmplification{false};// Block 4.4: EchoSwarm sound relay trigger
    float m_swarmAmplifyCooldown{0.0f};

    std::vector<SonicBolt> m_bolts;
};

} // namespace echo
