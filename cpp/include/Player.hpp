#pragma once

#include "AcousticSystem.hpp"
#include "AssetManager.hpp"
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

    // Resets full player state for a brand-new roguelike run
    void reset(sf::Vector2f spawnPos) noexcept;

    // Repositions player when entering a new room via N/E/S/W doors without wiping run build/health
    void setPosition(sf::Vector2f pos) noexcept;

    // Returns true if a footstep or decoy impact sound was emitted this frame (outSignature set)
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

    SurfaceMaterial detectFloorMaterial(
        const std::vector<WallSegment>& walls,
        SurfaceMaterial roomDefault
    ) const noexcept;

    void render(sf::RenderTarget& target, const AssetManager* assets = nullptr) const;

    sf::Vector2f position() const noexcept { return m_position; }
    float facingAngleRad() const noexcept { return m_facingAngle; }

    // Hull Integrity (Health) & Invulnerability
    int health() const noexcept { return m_health; }
    int maxHealth() const noexcept { return m_maxHealth; }
    void setHealth(int hp) noexcept { m_health = hp; }
    void setMaxHealth(int maxHp) noexcept { m_maxHealth = maxHp; }
    void heal(int amount) noexcept;
    bool isInvulnerable() const noexcept { return m_invulnTimer > 0.0f; }
    bool isDead() const noexcept { return m_health <= 0; }
    bool applyDamage(int amount, AcousticSystem& acoustics);

    // Stamina & Stance
    float stamina() const noexcept { return m_stamina; }
    void setStamina(float stamina) noexcept { m_stamina = stamina; }
    void restoreStamina(float amount) noexcept;
    MovementStance activeStance() const noexcept { return m_activeStance; }

    // Consumables & Currency (Decoy Stones & Echo Shards)
    int decoyCount() const noexcept { return m_decoyCount; }
    void setDecoyCount(int count) noexcept { m_decoyCount = count; }
    void addDecoy(int count = 1) noexcept { m_decoyCount += count; }

    int echoShards() const noexcept { return m_echoShards; }
    void setEchoShards(int shards) noexcept { m_echoShards = shards; }
    void addEchoShards(int delta) noexcept { m_echoShards += delta; }
    bool spendEchoShards(int cost) noexcept;

    // Roguelike Relics & Build Synergies
    bool addRelic(RelicId id, AcousticSystem* acoustics = nullptr);
    bool hasRelic(RelicId id) const noexcept;
    const std::vector<RelicId>& relics() const noexcept { return m_relics; }
    void setRelics(const std::vector<RelicId>& relics) { m_relics = relics; }
    float sonarDamageMultiplier(bool isReflectedHit) const noexcept;
    float resonatorChargeMultiplier() const noexcept;

    float pingCooldown() const noexcept { return m_cooldownPing; }
    float pulseCooldown() const noexcept { return m_cooldownPulse; }
    float decoyCooldown() const noexcept { return m_cooldownDecoy; }

private:
    void resolveWallCollisions(const std::vector<WallSegment>& walls) noexcept;

    sf::Vector2f m_position{640.0f, 400.0f};
    float m_facingAngle{0.0f};
    int m_health{6};
    int m_maxHealth{6};
    float m_invulnTimer{0.0f};
    float m_stamina{100.0f};
    int m_echoShards{0};
    float m_stepAccumulator{0.0f};
    float m_walkCyclePhase{0.0f};
    float m_breathPhase{0.0f};
    float m_pingRecoilTimer{0.0f};
    float m_cooldownPing{0.0f};
    float m_cooldownPulse{0.0f};
    float m_cooldownDecoy{0.0f};
    MovementStance m_activeStance{MovementStance::Walk};
    int m_decoyCount{4};
    std::vector<RelicId> m_relics;
    DecoyProjectile m_decoyProj;
};

} // namespace echo
