#include "Player.hpp"
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
} // namespace

Player::Player() = default;

void Player::reset(sf::Vector2f spawnPos) noexcept {
    m_position = spawnPos;
    m_stamina = 100.0f;
    m_stepAccumulator = 0.0f;
    m_cooldownPing = 0.0f;
    m_cooldownPulse = 0.0f;
    m_cooldownDecoy = 0.0f;
}

void Player::update(
    float dt,
    sf::Vector2f moveInput,
    MovementStance stance,
    const std::vector<WallSegment>& walls,
    AcousticSystem& acoustics
) {
    m_cooldownPing = std::max(0.0f, m_cooldownPing - dt);
    m_cooldownPulse = std::max(0.0f, m_cooldownPulse - dt);
    m_cooldownDecoy = std::max(0.0f, m_cooldownDecoy - dt);

    const float inputLenSq = moveInput.x * moveInput.x + moveInput.y * moveInput.y;
    const bool isMoving = inputLenSq > 0.01f;

    if (isMoving) {
        const float invLen = 1.0f / std::sqrt(inputLenSq);
        const sf::Vector2f dir = moveInput * invLen;
        m_facingAngle = std::atan2(dir.y, dir.x);

        if (stance == MovementStance::Sprint && m_stamina <= 5.0f) {
            stance = MovementStance::Walk;
        }

        float speed = cfg::kSpeedWalk;
        if (stance == MovementStance::Sneak) speed = cfg::kSpeedSneak;
        else if (stance == MovementStance::Sprint) speed = cfg::kSpeedSprint;

        m_position += dir * (speed * dt);
        resolveWallCollisions(walls);

        if (stance == MovementStance::Sprint) {
            m_stamina = std::max(0.0f, m_stamina - 24.0f * dt);
        } else {
            m_stamina = std::min(100.0f, m_stamina + 14.0f * dt);
        }

        // Footstep sound emission based on distance traveled
        m_stepAccumulator += speed * dt;
        const float stepStride = (stance == MovementStance::Sneak) ? 54.0f : (stance == MovementStance::Sprint ? 64.0f : 58.0f);
        if (m_stepAccumulator >= stepStride) {
            m_stepAccumulator = 0.0f;
            if (stance == MovementStance::Sneak) {
                acoustics.emitPulse(m_position, 220.0f, 115.0f, 0.25f, 24, WaveSignature::FootstepSneak);
            } else if (stance == MovementStance::Walk) {
                acoustics.emitPulse(m_position, 290.0f, 245.0f, 0.58f, 40, WaveSignature::FootstepWalk);
            } else {
                acoustics.emitPulse(m_position, 370.0f, 440.0f, 1.0f, 64, WaveSignature::FootstepSprint);
            }
        }
    } else {
        m_stamina = std::min(100.0f, m_stamina + 22.0f * dt);
    }
}

void Player::resolveWallCollisions(const std::vector<WallSegment>& walls) noexcept {
    for (int iter = 0; iter < 3; ++iter) {
        for (const auto& wall : walls) {
            if (wall.isOpen) continue;
            const sf::Vector2f closest = closestPointOnSegment(m_position, wall.a, wall.b);
            const sf::Vector2f delta = m_position - closest;
            const float distSq = delta.x * delta.x + delta.y * delta.y;
            const float minDist = cfg::kPlayerRadius + 2.0f;
            if (distSq < minDist * minDist && distSq > 1e-6f) {
                const float dist = std::sqrt(distSq);
                const sf::Vector2f pushNormal = delta / dist;
                m_position += pushNormal * (minDist - dist);
            }
        }
    }
}

bool Player::triggerDirectionalPing(sf::Vector2f aimTarget, AcousticSystem& acoustics) {
    if (m_cooldownPing > 0.0f) return false;
    m_cooldownPing = cfg::kCooldownPing;
    const sf::Vector2f d = aimTarget - m_position;
    const float angle = std::atan2(d.y, d.x);
    m_facingAngle = angle;
    acoustics.emitPulse(
        m_position,
        440.0f,
        560.0f,
        0.88f,
        56,
        WaveSignature::DirectionalPing,
        angle,
        1.0471975f, // 60 degree cone
        true
    );
    return true;
}

bool Player::triggerRadialPulse(AcousticSystem& acoustics) {
    if (m_cooldownPulse > 0.0f) return false;
    m_cooldownPulse = cfg::kCooldownPulse;
    acoustics.emitPulse(
        m_position,
        420.0f,
        680.0f,
        1.20f,
        140,
        WaveSignature::RadialPulse,
        0.0f,
        6.2831853f,
        true
    );
    return true;
}

bool Player::throwDecoy(sf::Vector2f targetPoint, AcousticSystem& acoustics) {
    if (m_decoyCount <= 0 || m_cooldownDecoy > 0.0f) return false;
    --m_decoyCount;
    m_cooldownDecoy = cfg::kCooldownDecoy;
    acoustics.emitPulse(
        targetPoint,
        400.0f,
        580.0f,
        1.35f,
        110,
        WaveSignature::DecoyImpact,
        0.0f,
        6.2831853f,
        true
    );
    return true;
}

} // namespace echo
