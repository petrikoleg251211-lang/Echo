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
    m_stamina = 100.0f;
    m_stepAccumulator = 0.0f;
    m_walkCyclePhase = 0.0f;
    m_breathPhase = 0.0f;
    m_pingRecoilTimer = 0.0f;
    m_cooldownPing = 0.0f;
    m_cooldownPulse = 0.0f;
    m_cooldownDecoy = 0.0f;
    m_decoyProj.active = false;
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

    m_cooldownPing = std::max(0.0f, m_cooldownPing - dt);
    m_cooldownPulse = std::max(0.0f, m_cooldownPulse - dt);
    m_cooldownDecoy = std::max(0.0f, m_cooldownDecoy - dt);
    m_pingRecoilTimer = std::max(0.0f, m_pingRecoilTimer - dt);
    m_breathPhase += dt * (m_stamina < 40.0f ? 6.5f : 2.8f);

    const sf::Vector2f aimDelta = aimWorldPos - m_position;
    if (aimDelta.x * aimDelta.x + aimDelta.y * aimDelta.y > 9.0f) {
        m_facingAngle = std::atan2(aimDelta.y, aimDelta.x);
    }

    if (m_decoyProj.active) {
        m_decoyProj.progress += dt / std::max(0.08f, m_decoyProj.duration);
        if (m_decoyProj.progress >= 1.0f) {
            m_decoyProj.active = false;
            acoustics.emitPulse(
                m_decoyProj.targetPos,
                410.0f,
                600.0f,
                1.38f,
                120,
                WaveSignature::DecoyImpact,
                0.0f,
                6.2831853f,
                true
            );
            outEmittedSignature = WaveSignature::DecoyImpact;
            emittedSound = true;
        } else {
            m_decoyProj.currentPos = m_decoyProj.startPos
                + (m_decoyProj.targetPos - m_decoyProj.startPos) * m_decoyProj.progress;
        }
    }

    const float inputLenSq = moveInput.x * moveInput.x + moveInput.y * moveInput.y;
    const bool isMoving = inputLenSq > 0.01f;

    m_activeStance = requestedStance;
    if (m_activeStance == MovementStance::Sprint && m_stamina <= 6.0f) {
        m_activeStance = MovementStance::Walk;
    }

    if (isMoving) {
        const float invLen = 1.0f / std::sqrt(inputLenSq);
        const sf::Vector2f dir = moveInput * invLen;

        float speed = cfg::kSpeedWalk;
        if (m_activeStance == MovementStance::Sneak) speed = cfg::kSpeedSneak;
        else if (m_activeStance == MovementStance::Sprint) speed = cfg::kSpeedSprint;

        m_position += dir * (speed * dt);
        resolveWallCollisions(walls);

        m_walkCyclePhase += speed * dt * 0.14f;

        if (m_activeStance == MovementStance::Sprint) {
            m_stamina = std::max(0.0f, m_stamina - 24.0f * dt);
        } else {
            m_stamina = std::min(100.0f, m_stamina + 15.0f * dt);
        }

        m_stepAccumulator += speed * dt;
        const float stepStride = (m_activeStance == MovementStance::Sneak)
            ? 55.0f
            : (m_activeStance == MovementStance::Sprint ? 64.0f : 58.0f);

        if (m_stepAccumulator >= stepStride) {
            m_stepAccumulator = 0.0f;
            if (m_activeStance == MovementStance::Sneak) {
                acoustics.emitPulse(m_position, 225.0f, 118.0f, 0.24f, 24, WaveSignature::FootstepSneak);
                outEmittedSignature = WaveSignature::FootstepSneak;
            } else if (m_activeStance == MovementStance::Walk) {
                acoustics.emitPulse(m_position, 295.0f, 245.0f, 0.56f, 40, WaveSignature::FootstepWalk);
                outEmittedSignature = WaveSignature::FootstepWalk;
            } else {
                acoustics.emitPulse(m_position, 375.0f, 445.0f, 1.02f, 68, WaveSignature::FootstepSprint);
                outEmittedSignature = WaveSignature::FootstepSprint;
            }
            emittedSound = true;
        }
    } else {
        m_stamina = std::min(100.0f, m_stamina + 24.0f * dt);
    }

    return emittedSound;
}

SurfaceMaterial Player::detectFloorMaterial(
    const std::vector<WallSegment>& walls,
    SurfaceMaterial roomDefault
) const noexcept {
    float bestDistSq = 85.0f * 85.0f;
    SurfaceMaterial mat = roomDefault;
    for (const auto& w : walls) {
        if (w.isOpen) continue;
        const sf::Vector2f c = closestPointOnSegment(m_position, w.a, w.b);
        const sf::Vector2f d = m_position - c;
        const float dSq = d.x * d.x + d.y * d.y;
        if (dSq < bestDistSq) {
            bestDistSq = dSq;
            mat = w.material;
        }
    }
    return mat;
}

void Player::resolveWallCollisions(const std::vector<WallSegment>& walls) noexcept {
    for (int iter = 0; iter < 4; ++iter) {
        for (const auto& wall : walls) {
            if (wall.isOpen) continue;
            const sf::Vector2f closest = closestPointOnSegment(m_position, wall.a, wall.b);
            const sf::Vector2f delta = m_position - closest;
            const float distSq = delta.x * delta.x + delta.y * delta.y;
            const float minDist = cfg::kPlayerRadius + 2.5f;
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
    m_pingRecoilTimer = 0.24f;
    const sf::Vector2f d = aimTarget - m_position;
    if (d.x * d.x + d.y * d.y > 1.0f) {
        m_facingAngle = std::atan2(d.y, d.x);
    }
    acoustics.emitPulse(
        m_position,
        445.0f,
        560.0f,
        0.92f,
        64,
        WaveSignature::DirectionalPing,
        m_facingAngle,
        1.0471975f,
        true
    );
    return true;
}

bool Player::triggerRadialPulse(AcousticSystem& acoustics) {
    if (m_cooldownPulse > 0.0f) return false;
    m_cooldownPulse = cfg::kCooldownRadialPulse;
    m_pingRecoilTimer = 0.35f;
    acoustics.emitPulse(
        m_position,
        420.0f,
        690.0f,
        1.22f,
        148,
        WaveSignature::RadialPulse,
        0.0f,
        6.2831853f,
        true
    );
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
    // 1. Render in-flight decoy projectile with parabolic arc height offset
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

    // 2. Render Directional Sonar Cone guide lines
    const float coneHalf = 0.523598f;
    const sf::Vector2f leftRay{
        std::cos(m_facingAngle - coneHalf) * 38.0f,
        std::sin(m_facingAngle - coneHalf) * 38.0f
    };
    const sf::Vector2f rightRay{
        std::cos(m_facingAngle + coneHalf) * 38.0f,
        std::sin(m_facingAngle + coneHalf) * 38.0f
    };

    sf::VertexArray coneLines(sf::Lines, 4);
    coneLines[0] = sf::Vertex(m_position, sf::Color(56, 189, 248, 90));
    coneLines[1] = sf::Vertex(m_position + leftRay, sf::Color(56, 189, 248, 0));
    coneLines[2] = sf::Vertex(m_position, sf::Color(56, 189, 248, 90));
    coneLines[3] = sf::Vertex(m_position + rightRay, sf::Color(56, 189, 248, 0));
    target.draw(coneLines);

    // 3. Surveyor outer acoustic aura ring (scales with stance + breathing)
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

    // 4. Articulated stride boots & tool arm animation
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

    // 5. Render 2.5D SurveyorRig 3D Mesh via AssetManager (or fallback core)
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

    // 6. Articulated right-hand Acoustic Emitter Dish with recoil glow
    const float recoilOffset = m_pingRecoilTimer * 14.0f;
    const sf::Vector2f dishPos = m_position + right * 8.5f + forward * (10.5f - recoilOffset);
    sf::CircleShape dish(3.8f + m_pingRecoilTimer * 4.0f);
    dish.setOrigin(dish.getRadius(), dish.getRadius());
    dish.setPosition(dishPos);
    dish.setFillColor(m_pingRecoilTimer > 0.0f ? sf::Color(224, 242, 254) : sf::Color(125, 211, 252));
    target.draw(dish);
}

} // namespace echo
