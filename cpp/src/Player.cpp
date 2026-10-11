#include "Player.hpp"
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <algorithm>
#include <cmath>

namespace echo {

namespace {
sf::Vector2f closestPointOnSegment(sf::Vector2f p, sf::Vector2f a, sf::Vector2f b) noexcept {
    const sf::Vector2f ab = b - a;
    const float lenSq = ab.x * ab.x + ab.y * ab.y;
    if (lenSq < 1e-6f) return a;
    const float t = std::clamp(((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / lenSq, 0.0f, 1.0f);
    return a + ab * t;
}

float cross2D(sf::Vector2f u, sf::Vector2f v) noexcept {
    return u.x * v.y - u.y * v.x;
}

bool intersectSegmentRay(
    sf::Vector2f p, sf::Vector2f p2,
    sf::Vector2f q, sf::Vector2f q2,
    float& outT, sf::Vector2f& outPt
) noexcept {
    const sf::Vector2f r = p2 - p;
    const sf::Vector2f s = q2 - q;
    const float rxs = cross2D(r, s);
    if (std::abs(rxs) < 1e-6f) return false;
    const sf::Vector2f qp = q - p;
    const float t = cross2D(qp, s) / rxs;
    const float u = cross2D(qp, r) / rxs;
    if (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f) {
        outT = t;
        outPt = p + r * t;
        return true;
    }
    return false;
}
} // namespace

Player::Player() = default;

void Player::reset(sf::Vector2f spawnPos) noexcept {
    m_position = spawnPos;
    m_health = 6;
    m_maxHealth = 6;
    m_invulnTimer = 0.0f;
    m_stamina = 100.0f;
    m_echoShards = 0;
    m_decoyCount = 4;
    m_echoTrapCount = cfg::kDefaultEchoTrapCount;
    m_activeFrequency = AcousticFrequency::Mid;
    m_relics.clear();
    m_stepAccumulator = 0.0f;
    m_walkCyclePhase = 0.0f;
    m_breathPhase = 0.0f;
    m_pingRecoilTimer = 0.0f;
    m_cooldownPing = 0.0f;
    m_cooldownPulse = 0.0f;
    m_cooldownDecoy = 0.0f;
    m_cooldownWhistle = 0.0f;
    m_cooldownStomp = 0.0f;
    m_cooldownTrap = 0.0f;
    m_movedLastFrame = false;
    m_decoyProj.active = false;
    m_echoTrap.active = false;
}

void Player::setPosition(sf::Vector2f pos) noexcept {
    m_position = pos;
    m_movedLastFrame = false;
    m_decoyProj.active = false;
    m_echoTrap.active = false;
}

void Player::heal(int amount) noexcept {
    if (amount <= 0) return;
    m_health = std::min(m_maxHealth, m_health + amount);
}

void Player::restoreStamina(float amount) noexcept {
    if (amount <= 0.0f) return;
    m_stamina = std::min(100.0f, m_stamina + amount);
}

bool Player::spendEchoShards(int cost) noexcept {
    if (cost < 0 || m_echoShards < cost) return false;
    m_echoShards -= cost;
    return true;
}

bool Player::hasRelic(RelicId id) const noexcept {
    return std::find(m_relics.begin(), m_relics.end(), id) != m_relics.end();
}

bool Player::addRelic(RelicId id, AcousticSystem* acoustics) {
    if (hasRelic(id)) return false;
    m_relics.push_back(id);

    if (id == RelicId::PiezoArmorCore) {
        m_maxHealth += 2;
        m_health = std::min(m_maxHealth, m_health + 2);
    } else if (id == RelicId::PhantomDecoyRig) {
        m_decoyCount += 3;
        m_echoTrapCount += 2;
    } else if (id == RelicId::UltrasonicLens) {
        m_echoTrapCount += 2;
    } else if (id == RelicId::PrismaticMirror && acoustics != nullptr) {
        const auto nextRefl = static_cast<std::uint8_t>(std::min<int>(8, acoustics->maxReflections() + 2));
        acoustics->setMaxReflections(nextRefl);
    }
    return true;
}

float Player::sonarDamageMultiplier(bool isReflectedHit, bool isHighFreqHit) const noexcept {
    float mult = 1.0f;
    if (hasRelic(RelicId::HarmonicAmplifier)) {
        mult *= 1.40f;
    }
    if (isReflectedHit) {
        mult *= hasRelic(RelicId::PrismaticMirror) ? 1.55f : 1.20f;
        // Synergy: FocusedTransducer + PrismaticMirror = Phase-Locked Resonance Beam
        if (hasRelic(RelicId::HarmonicAmplifier) && hasRelic(RelicId::PrismaticMirror)) {
            mult *= 1.25f;
        }
    }
    if (isHighFreqHit) {
        mult *= hasRelic(RelicId::UltrasonicLens) ? 1.56f : 1.12f;
    }
    if (hasRelic(RelicId::AbyssalTuningFork)) {
        mult *= 1.30f;
    }
    return mult;
}

float Player::resonatorChargeMultiplier() const noexcept {
    return hasRelic(RelicId::EchoSiphonCoil) ? 2.0f : 1.0f;
}

float Player::noiseGainMultiplier(bool isFootstep) const noexcept {
    float mult = 1.0f;
    if (hasRelic(RelicId::SilentStep)) {
        mult *= 0.65f;
    }
    if (isFootstep && hasRelic(RelicId::MossTreadBoots)) {
        mult *= 0.55f;
    }
    if (!isFootstep && hasRelic(RelicId::PhaseDampener)) {
        mult *= 0.70f;
    }
    return mult;
}

float Player::silenceDecayMultiplier() const noexcept {
    return hasRelic(RelicId::PhaseDampener) ? 1.50f : 1.0f;
}

float Player::shardGainMultiplier() const noexcept {
    return hasRelic(RelicId::AbyssalTuningFork) ? 1.25f : 1.0f;
}

void Player::setFrequency(AcousticFrequency freq, AcousticSystem& acoustics) noexcept {
    m_activeFrequency = freq;
    acoustics.setActiveFrequency(freq);
}

void Player::cycleFrequency(int delta, AcousticSystem& acoustics) noexcept {
    int idx = static_cast<int>(m_activeFrequency) + delta;
    while (idx < 0) idx += 3;
    idx %= 3;
    setFrequency(static_cast<AcousticFrequency>(idx), acoustics);
}

bool Player::applyDamage(int amount, AcousticSystem& acoustics) {
    if (m_invulnTimer > 0.0f || m_health <= 0 || amount <= 0) return false;
    m_health = std::max(0, m_health - amount);
    m_invulnTimer = 1.15f;

    // Retaliatory shockwave if PiezoArmorCore is equipped
    if (hasRelic(RelicId::PiezoArmorCore)) {
        acoustics.emitPulse(
            m_position,
            450.0f,
            480.0f,
            1.35f,
            96,
            WaveSignature::RadialPulse,
            0.0f,
            6.2831853f,
            true
        );
    }
    return true;
}

bool Player::update(
    float dt,
    sf::Vector2f moveInput,
    MovementStance requestedStance,
    sf::Vector2f aimWorldPos,
    const std::vector<WallSegment>& walls,
    AcousticSystem& acoustics,
    WaveSignature& outEmittedSignature
) {
    bool emittedSound = false;
    acoustics.setActiveFrequency(m_activeFrequency);

    m_invulnTimer = std::max(0.0f, m_invulnTimer - dt);
    m_cooldownPing = std::max(0.0f, m_cooldownPing - dt);
    m_cooldownPulse = std::max(0.0f, m_cooldownPulse - dt);
    m_cooldownDecoy = std::max(0.0f, m_cooldownDecoy - dt);
    m_cooldownWhistle = std::max(0.0f, m_cooldownWhistle - dt);
    m_cooldownStomp = std::max(0.0f, m_cooldownStomp - dt);
    m_cooldownTrap = std::max(0.0f, m_cooldownTrap - dt);
    m_pingRecoilTimer = std::max(0.0f, m_pingRecoilTimer - dt);
    m_breathPhase += dt * (m_stamina < 40.0f ? 6.5f : 2.8f);

    const sf::Vector2f aimDelta = aimWorldPos - m_position;
    if (aimDelta.x * aimDelta.x + aimDelta.y * aimDelta.y > 9.0f) {
        m_facingAngle = std::atan2(aimDelta.y, aimDelta.x);
    }

    // Block 3.3: Update Delayed Echo Trap Mine (3.0s countdown -> series of 3 sonar bursts)
    if (m_echoTrap.active) {
        if (m_echoTrap.delayTimer > 0.0f) {
            m_echoTrap.delayTimer = std::max(0.0f, m_echoTrap.delayTimer - dt);
            if (m_echoTrap.delayTimer <= 0.0f) {
                m_echoTrap.pulseIntervalTimer = 0.0f;
            }
        } else {
            m_echoTrap.pulseIntervalTimer -= dt;
            if (m_echoTrap.pulseIntervalTimer <= 0.0f && m_echoTrap.pulsesRemaining > 0) {
                --m_echoTrap.pulsesRemaining;
                m_echoTrap.pulseIntervalTimer = 0.45f;
                acoustics.emitPulse(
                    m_echoTrap.position,
                    445.0f,
                    640.0f,
                    1.35f,
                    128,
                    WaveSignature::EchoTrapPulse,
                    0.0f,
                    6.2831853f,
                    true
                );
                outEmittedSignature = WaveSignature::EchoTrapPulse;
                emittedSound = true;
                if (m_echoTrap.pulsesRemaining <= 0) {
                    m_echoTrap.active = false;
                }
            }
        }
    }

    if (m_decoyProj.active) {
        m_decoyProj.progress += dt / std::max(0.08f, m_decoyProj.duration);
        if (m_decoyProj.progress >= 1.0f) {
            m_decoyProj.active = false;
            const bool hasPhantomDecoy = hasRelic(RelicId::PhantomDecoyRig);
            const float decoyRadius = hasPhantomDecoy ? 740.0f : 600.0f;
            const float decoyIntensity = hasPhantomDecoy ? 1.65f : 1.38f;
            acoustics.emitPulse(
                m_decoyProj.targetPos,
                420.0f,
                decoyRadius,
                decoyIntensity,
                hasPhantomDecoy ? 160 : 120,
                WaveSignature::DecoyImpact,
                0.0f,
                6.2831853f,
                true
            );
            // Synergy: PhantomDecoyRig + PrismaticMirror emits a secondary harmonic shatter wave
            if (hasPhantomDecoy && hasRelic(RelicId::PrismaticMirror)) {
                acoustics.emitPulse(
                    m_decoyProj.targetPos,
                    520.0f,
                    500.0f,
                    1.25f,
                    96,
                    WaveSignature::DirectionalPing,
                    0.0f,
                    6.2831853f,
                    true
                );
            }
            outEmittedSignature = WaveSignature::DecoyImpact;
            emittedSound = true;
        } else {
            m_decoyProj.currentPos = m_decoyProj.startPos
                + (m_decoyProj.targetPos - m_decoyProj.startPos) * m_decoyProj.progress;
        }
    }

    float inputLen = std::sqrt(moveInput.x * moveInput.x + moveInput.y * moveInput.y);
    const bool isMoving = (inputLen > 0.05f);
    m_movedLastFrame = isMoving;
    if (isMoving) {
        moveInput /= inputLen;
    }

    m_activeStance = requestedStance;
    if (m_activeStance == MovementStance::Sprint && (!isMoving || m_stamina <= 3.0f)) {
        m_activeStance = MovementStance::Walk;
    }

    const float regenScale = hasRelic(RelicId::ResonanceCapacitor) ? 1.35f : 1.0f;
    if (isMoving && m_activeStance == MovementStance::Sprint) {
        m_stamina = std::max(0.0f, m_stamina - 25.0f * dt);
    } else {
        const float regenRate = isMoving ? 15.0f : 26.0f;
        m_stamina = std::min(100.0f, m_stamina + regenRate * regenScale * dt);
    }

    float speed = cfg::kSpeedWalk;
    float strideThreshold = 42.0f;
    float stepRadius = 215.0f;
    float stepIntensity = 0.42f;
    std::size_t stepRays = 38;
    WaveSignature stepSig = WaveSignature::FootstepWalk;

    if (m_activeStance == MovementStance::Sneak) {
        speed = cfg::kSpeedSneak;
        strideThreshold = 50.0f;
        stepRadius = 82.0f;
        stepIntensity = 0.14f;
        stepRays = 20;
        stepSig = WaveSignature::FootstepSneak;
    } else if (m_activeStance == MovementStance::Sprint) {
        speed = cfg::kSpeedSprint;
        strideThreshold = 34.0f;
        stepRadius = 430.0f;
        stepIntensity = 0.88f;
        stepRays = 68;
        stepSig = WaveSignature::FootstepSprint;
    }

    // SilentStep accelerator relic grants +18% movement speed and dampens footstep radius
    if (hasRelic(RelicId::SilentStep)) {
        speed *= 1.18f;
        stepRadius *= 0.72f;
        stepIntensity *= 0.72f;
    }

    // MossTreadBoots relic dampens footstep noise radius & intensity by 45%
    if (hasRelic(RelicId::MossTreadBoots)) {
        stepRadius *= 0.55f;
        stepIntensity *= 0.55f;
    }

    if (isMoving) {
        const sf::Vector2f prevPos = m_position;
        m_position += moveInput * (speed * dt);
        resolveWallCollisions(walls);

        const sf::Vector2f actualDelta = m_position - prevPos;
        const float movedDist = std::sqrt(actualDelta.x * actualDelta.x + actualDelta.y * actualDelta.y);
        m_stepAccumulator += movedDist;
        m_walkCyclePhase += movedDist * 0.14f;

        if (m_stepAccumulator >= strideThreshold) {
            m_stepAccumulator = 0.0f;
            acoustics.emitPulse(
                m_position,
                340.0f,
                stepRadius,
                stepIntensity,
                stepRays,
                stepSig,
                0.0f,
                6.2831853f,
                true
            );
            if (!emittedSound) {
                outEmittedSignature = stepSig;
                emittedSound = true;
            }
        }
    } else {
        m_walkCyclePhase *= std::max(0.0f, 1.0f - dt * 8.0f);
    }

    return emittedSound;
}

void Player::resolveWallCollisions(const std::vector<WallSegment>& walls) noexcept {
    for (int iter = 0; iter < 4; ++iter) {
        for (const auto& wall : walls) {
            if (wall.isOpen) continue;
            const sf::Vector2f closest = closestPointOnSegment(m_position, wall.a, wall.b);
            const sf::Vector2f delta = m_position - closest;
            const float distSq = delta.x * delta.x + delta.y * delta.y;
            const float minDist = cfg::kPlayerRadius + 2.5f;
            if (distSq < minDist * minDist) {
                if (distSq > 1e-6f) {
                    const float dist = std::sqrt(distSq);
                    m_position += (delta / dist) * (minDist - dist);
                } else {
                    m_position.y += minDist;
                }
            }
        }
    }
    m_position.x = std::clamp(m_position.x, 52.0f, cfg::kWorldWidth - 52.0f);
    m_position.y = std::clamp(m_position.y, 48.0f, cfg::kWorldHeight - 48.0f);
}

SurfaceMaterial Player::detectFloorMaterial(
    const std::vector<WallSegment>& walls,
    SurfaceMaterial roomDefault
) const noexcept {
    float bestDistSq = 48.0f * 48.0f;
    SurfaceMaterial bestMat = roomDefault;
    for (const auto& w : walls) {
        if (w.isOpen || w.material == SurfaceMaterial::ResonanceDoor) continue;
        const sf::Vector2f c = closestPointOnSegment(m_position, w.a, w.b);
        const sf::Vector2f d = m_position - c;
        const float dSq = d.x * d.x + d.y * d.y;
        if (dSq < bestDistSq) {
            bestDistSq = dSq;
            bestMat = w.material;
        }
    }
    return bestMat;
}

bool Player::triggerDirectionalPing(sf::Vector2f aimTarget, AcousticSystem& acoustics) {
    if (m_cooldownPing > 0.0f) return false;
    const float cdScale = hasRelic(RelicId::ResonanceCapacitor) ? 0.75f : 1.0f;
    m_cooldownPing = cfg::kCooldownPing * cdScale;
    m_pingRecoilTimer = 0.25f;

    const sf::Vector2f d = aimTarget - m_position;
    if (d.x * d.x + d.y * d.y > 4.0f) {
        m_facingAngle = std::atan2(d.y, d.x);
    }

    const bool hasFocused = hasRelic(RelicId::HarmonicAmplifier);
    const float range = hasFocused ? 755.0f : 560.0f;
    const float intensity = hasFocused ? 1.24f : 0.96f;
    const float spread = hasFocused ? 0.80f : 1.0471975f;

    acoustics.setActiveFrequency(m_activeFrequency);
    acoustics.emitPulse(
        m_position,
        465.0f,
        range,
        intensity,
        hasFocused ? 84 : 64,
        WaveSignature::DirectionalPing,
        m_facingAngle,
        spread,
        true
    );
    return true;
}

bool Player::triggerRadialPulse(AcousticSystem& acoustics) {
    if (m_cooldownPulse > 0.0f) return false;
    const float cdScale = hasRelic(RelicId::ResonanceCapacitor) ? 0.75f : 1.0f;
    m_cooldownPulse = cfg::kCooldownRadialPulse * cdScale;
    m_pingRecoilTimer = 0.35f;

    const bool hasFocused = hasRelic(RelicId::HarmonicAmplifier);
    acoustics.setActiveFrequency(m_activeFrequency);
    acoustics.emitPulse(
        m_position,
        430.0f,
        hasFocused ? 780.0f : 690.0f,
        1.25f,
        148,
        WaveSignature::RadialPulse,
        0.0f,
        6.2831853f,
        true
    );
    return true;
}

// Block 3.1: Narrow 15-degree Whistle beam (long range, only +5 noise)
bool Player::triggerWhistle(sf::Vector2f aimTarget, AcousticSystem& acoustics) {
    if (m_cooldownWhistle > 0.0f) return false;
    const float cdScale = hasRelic(RelicId::ResonanceCapacitor) ? 0.75f : 1.0f;
    m_cooldownWhistle = cfg::kCooldownWhistle * cdScale;
    m_pingRecoilTimer = 0.18f;

    const sf::Vector2f d = aimTarget - m_position;
    if (d.x * d.x + d.y * d.y > 4.0f) {
        m_facingAngle = std::atan2(d.y, d.x);
    }

    const bool hasFocused = hasRelic(RelicId::HarmonicAmplifier);
    const float range = hasFocused ? (cfg::kWhistleRange * 1.25f) : cfg::kWhistleRange;
    const float intensity = hasFocused ? 1.15f : 0.92f;

    acoustics.setActiveFrequency(m_activeFrequency);
    acoustics.emitPulse(
        m_position,
        520.0f,
        range,
        intensity,
        44,
        WaveSignature::WhistleBeam,
        m_facingAngle,
        cfg::kWhistleSpreadRad,
        true
    );
    return true;
}

// Block 3.2: Wide Stomp shockwave (pushes nearby enemies, shatters Glass/Shale, +35 noise)
bool Player::triggerStomp(AcousticSystem& acoustics) {
    if (m_cooldownStomp > 0.0f) return false;
    const float cdScale = hasRelic(RelicId::ResonanceCapacitor) ? 0.75f : 1.0f;
    m_cooldownStomp = cfg::kCooldownStomp * cdScale;
    m_pingRecoilTimer = 0.32f;

    acoustics.setActiveFrequency(m_activeFrequency);
    acoustics.emitPulse(
        m_position,
        470.0f,
        cfg::kStompRadius * 1.55f,
        1.45f,
        136,
        WaveSignature::StompWave,
        0.0f,
        6.2831853f,
        true
    );
    return true;
}

// Block 3.3: Place Delayed Echo Trap / Beacon Mine (triggers after 3.0s delay)
bool Player::placeEchoTrap() noexcept {
    if (m_echoTrapCount <= 0 || m_cooldownTrap > 0.0f || m_echoTrap.active) return false;
    --m_echoTrapCount;
    m_cooldownTrap = cfg::kCooldownEchoTrap;
    m_echoTrap.position = m_position;
    m_echoTrap.delayTimer = cfg::kEchoTrapDelaySec;
    m_echoTrap.pulseIntervalTimer = 0.0f;
    m_echoTrap.pulsesRemaining = 3;
    m_echoTrap.active = true;
    return true;
}

bool Player::throwDecoy(sf::Vector2f targetPoint, const std::vector<WallSegment>& walls) {
    if (m_decoyCount <= 0 || m_cooldownDecoy > 0.0f || m_decoyProj.active) return false;
    --m_decoyCount;
    m_cooldownDecoy = cfg::kCooldownDecoy;
    m_pingRecoilTimer = 0.20f;

    sf::Vector2f delta = targetPoint - m_position;
    float dist = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    if (dist > 460.0f) {
        delta *= (460.0f / dist);
        targetPoint = m_position + delta;
        dist = 460.0f;
    }

    float bestT = 1.0f;
    sf::Vector2f clampedTarget = targetPoint;
    for (const auto& w : walls) {
        if (w.isOpen) continue;
        float t = 0.0f;
        sf::Vector2f hit{};
        if (intersectSegmentRay(m_position, targetPoint, w.a, w.b, t, hit)) {
            if (t < bestT) {
                bestT = std::max(0.05f, t - 0.04f);
                clampedTarget = m_position + (targetPoint - m_position) * bestT;
            }
        }
    }

    m_decoyProj.startPos = m_position;
    m_decoyProj.currentPos = m_position;
    m_decoyProj.targetPos = clampedTarget;
    m_decoyProj.progress = 0.0f;
    m_decoyProj.duration = std::clamp(dist / 950.0f, 0.14f, 0.42f);
    m_decoyProj.active = true;
    return true;
}

void Player::render(sf::RenderTarget& target, const AssetManager* assets) const {
    // Render active Echo Trap Mine on the floor
    if (m_echoTrap.active) {
        const float pulse = 0.5f + 0.5f * std::sin(m_echoTrap.delayTimer * 14.0f);
        sf::CircleShape trapRing(12.0f + pulse * 5.0f);
        trapRing.setOrigin(trapRing.getRadius(), trapRing.getRadius());
        trapRing.setPosition(m_echoTrap.position);
        trapRing.setFillColor(sf::Color(232, 121, 249, 45));
        trapRing.setOutlineThickness(1.6f);
        trapRing.setOutlineColor(sf::Color(232, 121, 249, 210));
        target.draw(trapRing);

        sf::CircleShape trapCore(5.0f);
        trapCore.setOrigin(5.0f, 5.0f);
        trapCore.setPosition(m_echoTrap.position);
        trapCore.setFillColor(m_echoTrap.delayTimer > 0.0f ? sf::Color(250, 204, 21) : sf::Color(232, 121, 249));
        target.draw(trapCore);
    }

    if (m_decoyProj.active) {
        const float arcLift = std::sin(m_decoyProj.progress * 3.1415926f) * 16.0f;
        const sf::Vector2f elevatedPos{m_decoyProj.currentPos.x, m_decoyProj.currentPos.y - arcLift};

        sf::VertexArray trail(sf::Lines, 2);
        trail[0] = sf::Vertex(m_decoyProj.startPos, sf::Color(167, 139, 250, 45));
        trail[1] = sf::Vertex(elevatedPos, sf::Color(167, 139, 250, 220));
        target.draw(trail);

        sf::CircleShape shadow(4.0f);
        shadow.setOrigin(4.0f, 4.0f);
        shadow.setPosition(m_decoyProj.currentPos);
        shadow.setFillColor(sf::Color(5, 8, 15, 160));
        target.draw(shadow);

        sf::CircleShape stone(4.5f);
        stone.setOrigin(4.5f, 4.5f);
        stone.setPosition(elevatedPos);
        stone.setFillColor(sf::Color(216, 180, 254));
        target.draw(stone);
    }

    // Blink briefly during post-hit invulnerability frames
    if (m_invulnTimer > 0.0f && (static_cast<int>(m_invulnTimer * 18.0f) % 2 == 1)) {
        return;
    }

    const float coneHalf = hasRelic(RelicId::HarmonicAmplifier) ? 0.40f : 0.523598f;
    const sf::Vector2f leftRay{
        std::cos(m_facingAngle - coneHalf) * 42.0f,
        std::sin(m_facingAngle - coneHalf) * 42.0f
    };
    const sf::Vector2f rightRay{
        std::cos(m_facingAngle + coneHalf) * 42.0f,
        std::sin(m_facingAngle + coneHalf) * 42.0f
    };

    sf::VertexArray coneLines(sf::Lines, 4);
    coneLines[0] = sf::Vertex(m_position, sf::Color(56, 189, 248, 90));
    coneLines[1] = sf::Vertex(m_position + leftRay, sf::Color(56, 189, 248, 0));
    coneLines[2] = sf::Vertex(m_position, sf::Color(56, 189, 248, 90));
    coneLines[3] = sf::Vertex(m_position + rightRay, sf::Color(56, 189, 248, 0));
    target.draw(coneLines);

    const float breathScale = std::sin(m_breathPhase) * 0.8f;
    const float auraRadius = ((m_activeStance == MovementStance::Sneak)
        ? 15.0f
        : (m_activeStance == MovementStance::Sprint ? 22.0f : 18.0f)) + breathScale;
    sf::CircleShape aura(auraRadius);
    aura.setOrigin(auraRadius, auraRadius);
    aura.setPosition(m_position);
    aura.setFillColor(sf::Color(14, 30, 48, 85));
    aura.setOutlineThickness(1.0f);
    aura.setOutlineColor(sf::Color(56, 189, 248, 75));
    target.draw(aura);

    const sf::Vector2f forward{std::cos(m_facingAngle), std::sin(m_facingAngle)};
    const sf::Vector2f right{-forward.y, forward.x};
    const float stride = std::sin(m_walkCyclePhase) * 5.5f;

    const sf::Vector2f leftBootPos = m_position - right * 6.5f + forward * stride;
    const sf::Vector2f rightBootPos = m_position + right * 6.5f - forward * stride;

    sf::CircleShape boot(3.5f);
    boot.setOrigin(3.5f, 3.5f);
    boot.setFillColor(sf::Color(30, 58, 82));
    boot.setOutlineThickness(1.0f);
    boot.setOutlineColor(sf::Color(125, 211, 252, 160));
    boot.setPosition(leftBootPos);
    target.draw(boot);
    boot.setPosition(rightBootPos);
    target.draw(boot);

    const sf::Color rimCol = (m_activeStance == MovementStance::Sneak)
        ? sf::Color(45, 212, 191)
        : sf::Color(56, 189, 248);

    if (assets != nullptr && assets->isInitialized()) {
        const float pitchTilt = (m_activeStance == MovementStance::Sprint) ? 0.18f : 0.0f;
        assets->renderMesh25D(
            target,
            ModelId::SurveyorRig,
            m_position,
            m_facingAngle,
            15.5f,
            sf::Color(18, 38, 62),
            rimCol,
            0.95f,
            m_position + forward * 60.0f,
            pitchTilt
        );
    } else {
        sf::CircleShape body(cfg::kPlayerRadius);
        body.setOrigin(cfg::kPlayerRadius, cfg::kPlayerRadius);
        body.setPosition(m_position);
        body.setFillColor(sf::Color(12, 20, 34));
        body.setOutlineThickness(2.0f);
        body.setOutlineColor(rimCol);
        target.draw(body);
    }

    const float recoilOffset = m_pingRecoilTimer * 14.0f;
    const sf::Vector2f dishPos = m_position + right * 8.5f + forward * (10.5f - recoilOffset);
    sf::CircleShape dish(3.8f + m_pingRecoilTimer * 4.0f);
    dish.setOrigin(dish.getRadius(), dish.getRadius());
    dish.setPosition(dishPos);
    dish.setFillColor(m_pingRecoilTimer > 0.0f ? sf::Color(224, 242, 254) : sf::Color(125, 211, 252));
    target.draw(dish);
}

} // namespace echo
