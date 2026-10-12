#pragma once

#include "AcousticSystem.hpp"
#include "AssetManager.hpp"
#include "Config.hpp"
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>
#include <vector>

namespace echo {

struct DecoyProjectile {
    sf::Vector2f startPos;
    sf::Vector2f currentPos;
    sf::Vector2f targetPos;
    float progress{0.0f};
    float duration{0.35f};
    bool active{false};
};

// Block 3.3: Delayed Echo Trap / Beacon Mine
// Placed on the floor; triggers after 3.0s delay and emits a series of 3 sonar bursts from its position.
struct EchoTrapMine {
    sf::Vector2f position{640.0f, 400.0f};
    float delayTimer{0.0f};
    float pulseIntervalTimer{0.0f};
    int pulsesRemaining{0};
    bool active{false};
};

class Player {
public:
    Player();

    void reset(sf::Vector2f spawnPos) noexcept;
    void setPosition(sf::Vector2f pos) noexcept;

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

    // Block 3.1: Narrow 15-degree Whistle beam (long range, only +5 noise)
    bool triggerWhistle(sf::Vector2f aimTarget, AcousticSystem& acoustics);

    // Block 3.2: Wide Stomp shockwave (pushes nearby enemies, shatters Glass/Shale, +35 noise)
    bool triggerStomp(AcousticSystem& acoustics);

    // Block 3.3: Delayed Echo Trap / Beacon Mine (triggers after 3.0s delay, emits series of pings)
    bool placeEchoTrap() noexcept;

    // Block 3.4: Sonar Frequency Tuning (Low / Mid / High)
    void setFrequency(AcousticFrequency freq, AcousticSystem& acoustics) noexcept;
    void cycleFrequency(int delta, AcousticSystem& acoustics) noexcept;
    AcousticFrequency activeFrequency() const noexcept { return m_activeFrequency; }

    void render(sf::RenderTarget& target, const AssetManager* assets = nullptr) const;

    // Detects the nearest wall/floor surface material for footsteps
    SurfaceMaterial detectFloorMaterial(
        const std::vector<WallSegment>& walls,
        SurfaceMaterial roomDefault
    ) const noexcept;

    // Hull Integrity (HP) & Combat Damage (supports 1-HP Silence Hardcore mode)
    int health() const noexcept { return m_health; }
    int maxHealth() const noexcept { return m_maxHealth; }
    void setHealth(int hp) noexcept { m_health = std::clamp(hp, 0, m_maxHealth); }
    void setMaxHealth(int maxHp) noexcept {
        m_maxHealth = m_oneLifeLocked ? 1 : std::max(1, maxHp);
        m_health = std::min(m_health, m_maxHealth);
    }
    void setOneLifeLocked(bool locked) noexcept {
        m_oneLifeLocked = locked;
        if (locked) {
            m_maxHealth = 1;
            m_health = std::min(m_health, 1);
        }
    }
    bool isOneLifeLocked() const noexcept { return m_oneLifeLocked; }
    void heal(int amount) noexcept;
    bool applyDamage(int amount, AcousticSystem& acoustics);
    bool isInvulnerable() const noexcept { return m_invulnTimer > 0.0f; }
    bool isDead() const noexcept { return m_health <= 0; }

    // Echo Shards Currency & Relic Inventory
    int echoShards() const noexcept { return m_echoShards; }
    void setEchoShards(int shards) noexcept { m_echoShards = std::max(0, shards); }
    void addEchoShards(int delta) noexcept { m_echoShards = std::max(0, m_echoShards + delta); }
    bool spendEchoShards(int cost) noexcept;

    bool hasRelic(RelicId id) const noexcept;
    bool addRelic(RelicId id, AcousticSystem* acoustics = nullptr);
    const std::vector<RelicId>& relics() const noexcept { return m_relics; }
    void setRelics(const std::vector<RelicId>& list) { m_relics = list; }

    float sonarDamageMultiplier(bool isReflectedHit, bool isHighFreqHit = false) const noexcept;
    float resonatorChargeMultiplier() const noexcept;
    float noiseGainMultiplier(bool isFootstep) const noexcept;
    float silenceDecayMultiplier() const noexcept;
    float shardGainMultiplier() const noexcept;

    sf::Vector2f position() const noexcept { return m_position; }
    float facingAngleRad() const noexcept { return m_facingAngle; }
    MovementStance activeStance() const noexcept { return m_activeStance; }
    bool movedLastFrame() const noexcept { return m_movedLastFrame; }

    float stamina() const noexcept { return m_stamina; }
    void setStamina(float s) noexcept { m_stamina = std::clamp(s, 0.0f, 100.0f); }
    void restoreStamina(float amount) noexcept;

    int decoyCount() const noexcept { return m_decoyCount; }
    void setDecoyCount(int c) noexcept { m_decoyCount = std::max(0, c); }
    void addDecoy(int delta = 1) noexcept { m_decoyCount = std::max(0, m_decoyCount + delta); }

    int echoTrapCount() const noexcept { return m_echoTrapCount; }
    void setEchoTrapCount(int c) noexcept { m_echoTrapCount = std::max(0, c); }
    void addEchoTrap(int delta = 1) noexcept { m_echoTrapCount = std::max(0, m_echoTrapCount + delta); }
    const EchoTrapMine& activeEchoTrap() const noexcept { return m_echoTrap; }

private:
    void resolveWallCollisions(const std::vector<WallSegment>& walls) noexcept;

    sf::Vector2f m_position{640.0f, 400.0f};
    float m_facingAngle{0.0f};
    MovementStance m_activeStance{MovementStance::Walk};
    AcousticFrequency m_activeFrequency{AcousticFrequency::Mid};

    int m_health{6};
    int m_maxHealth{6};
    bool m_oneLifeLocked{false};
    float m_invulnTimer{0.0f};

    float m_stamina{100.0f};
    int m_echoShards{0};
    int m_decoyCount{4};
    int m_echoTrapCount{cfg::kDefaultEchoTrapCount};
    std::vector<RelicId> m_relics;

    float m_stepAccumulator{0.0f};
    float m_walkCyclePhase{0.0f};
    float m_breathPhase{0.0f};
    float m_pingRecoilTimer{0.0f};

    float m_cooldownPing{0.0f};
    float m_cooldownPulse{0.0f};
    float m_cooldownDecoy{0.0f};
    float m_cooldownWhistle{0.0f};
    float m_cooldownStomp{0.0f};
    float m_cooldownTrap{0.0f};
    bool m_movedLastFrame{false};

    DecoyProjectile m_decoyProj;
    EchoTrapMine m_echoTrap;
};

} // namespace echo
