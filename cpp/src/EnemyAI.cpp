#include "EnemyAI.hpp"
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <algorithm>
#include <cmath>

namespace echo {

namespace {
float distanceBetween(sf::Vector2f a, sf::Vector2f b) noexcept {
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    return std::sqrt(dx * dx + dy * dy);
}

float cross2(sf::Vector2f u, sf::Vector2f v) noexcept {
    return u.x * v.y - u.y * v.x;
}

bool segmentsIntersectSimple(sf::Vector2f p, sf::Vector2f p2, sf::Vector2f q, sf::Vector2f q2) noexcept {
    const sf::Vector2f r = p2 - p;
    const sf::Vector2f s = q2 - q;
    const float rxs = cross2(r, s);
    if (std::abs(rxs) < 1e-6f) return false;
    const sf::Vector2f qp = q - p;
    const float t = cross2(qp, s) / rxs;
    const float u = cross2(qp, r) / rxs;
    return (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f);
}

sf::Vector2f closestPointOnSegment(sf::Vector2f p, sf::Vector2f a, sf::Vector2f b) noexcept {
    const sf::Vector2f ab = b - a;
    const float lenSq = ab.x * ab.x + ab.y * ab.y;
    if (lenSq < 1e-6f) return a;
    const float t = std::clamp(((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / lenSq, 0.0f, 1.0f);
    return a + ab * t;
}
} // namespace

EnemyAI::EnemyAI() = default;

const char* EnemyAI::stateName(EnemyState s) noexcept {
    switch (s) {
        case EnemyState::Idle:             return "DORMANT";
        case EnemyState::Patrol:           return "PATROLLING";
        case EnemyState::InvestigateSound: return "INVESTIGATING";
        case EnemyState::SearchLastKnown:  return "SEARCHING";
        case EnemyState::PursuePlayer:     return "HUNTING";
        case EnemyState::Attack:           return "LETHAL CONTACT";
        case EnemyState::ReturnToPatrol:   return "RETURNING";
    }
    return "PATROLLING";
}

void EnemyAI::configurePatrol(const std::vector<sf::Vector2f>& waypoints, EnemyVariant variant) {
    m_waypoints = waypoints;
    m_variant = variant;
    m_waypointIdx = 0;
    m_suspicion = 0.0f;
    m_stateTimer = 0.0f;
    m_tremorTimer = 0.0f;
    m_alertBlend = 0.0f;
    if (m_waypoints.empty()) {
        m_active = false;
        m_state = EnemyState::Idle;
    } else {
        m_active = true;
        m_position = m_waypoints.front();
        m_targetPos = m_waypoints.front();
        m_state = EnemyState::Patrol;
    }
}

bool EnemyAI::hasLineOfSight(sf::Vector2f target, const std::vector<WallSegment>& walls) const noexcept {
    for (const auto& wall : walls) {
        if (wall.isOpen) continue;
        if (segmentsIntersectSimple(m_position, target, wall.a, wall.b)) {
            return false;
        }
    }
    return true;
}

void EnemyAI::resolveWallCollisions(const std::vector<WallSegment>& walls) noexcept {
    const float radius = (m_variant == EnemyVariant::Warden) ? (cfg::kEnemyRadius + 3.0f) : cfg::kEnemyRadius;
    for (int iter = 0; iter < 4; ++iter) {
        for (const auto& wall : walls) {
            if (wall.isOpen) continue;
            const sf::Vector2f closest = closestPointOnSegment(m_position, wall.a, wall.b);
            const sf::Vector2f delta = m_position - closest;
            const float distSq = delta.x * delta.x + delta.y * delta.y;
            const float minDist = radius + 2.5f;
            if (distSq < minDist * minDist && distSq > 1e-6f) {
                const float dist = std::sqrt(distSq);
                m_position += (delta / dist) * (minDist - dist);
            }
        }
    }
}

void EnemyAI::steerToward(
    sf::Vector2f target,
    float speed,
    float dt,
    const std::vector<WallSegment>& walls
) noexcept {
    sf::Vector2f goal = target;

    if (!hasLineOfSight(target, walls) && !m_waypoints.empty()) {
        float bestScore = 1e9f;
        for (const auto& wp : m_waypoints) {
            if (hasLineOfSight(wp, walls)) {
                const float score = distanceBetween(m_position, wp) + distanceBetween(wp, target);
                if (score < bestScore) {
                    bestScore = score;
                    goal = wp;
                }
            }
        }
    }

    const sf::Vector2f delta = goal - m_position;
    const float dist = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    if (dist < 2.0f) return;

    m_facingAngle = std::atan2(delta.y, delta.x);
    const sf::Vector2f dir = delta / dist;
    m_position += dir * (speed * dt);
    resolveWallCollisions(walls);
}

bool EnemyAI::update(
    float dt,
    sf::Vector2f playerPos,
    const std::vector<WallSegment>& walls,
    AcousticSystem& acoustics
) {
    if (!m_active || m_state == EnemyState::Attack) return false;

    const float speedScale = (m_variant == EnemyVariant::Warden) ? 0.90f : 1.0f;
    const float animSpeed = (m_state == EnemyState::PursuePlayer) ? 8.5f : 4.5f;
    m_animPhase += dt * animSpeed;

    const float targetAlert = (m_state == EnemyState::PursuePlayer)
        ? 1.0f
        : (m_state == EnemyState::InvestigateSound || m_state == EnemyState::SearchLastKnown ? 0.55f : 0.0f);
    m_alertBlend += (targetAlert - m_alertBlend) * std::min(1.0f, dt * 5.0f);

    const EnemyState prevState = m_state;

    const float catchRadius = cfg::kPlayerRadius + cfg::kEnemyRadius
        + ((m_variant == EnemyVariant::Warden) ? 1.5f : -2.0f);
    const float distToPlayer = distanceBetween(m_position, playerPos);
    if (distToPlayer <= catchRadius) {
        m_state = EnemyState::Attack;
        return true;
    }

    for (const auto& ev : acoustics.recentEvents()) {
        if (!ev.emittedByPlayer) continue;
        const float dist = distanceBetween(m_position, ev.origin);
        if (dist > ev.radius * 1.15f) continue;

        const float transmissionFactor = AcousticSystem::computeTransmissionFactor(ev.origin, m_position, walls);
        const float perceived = ev.loudness * transmissionFactor * (1.0f - dist / (ev.radius * 1.25f));

        if (perceived > 0.14f) {
            m_targetPos = ev.origin;
            m_suspicion = std::min(100.0f, m_suspicion + perceived * 54.0f);

            if (perceived > 0.65f || m_suspicion >= 74.0f) {
                m_state = EnemyState::PursuePlayer;
                m_stateTimer = 5.0f;
            } else if (m_state != EnemyState::PursuePlayer) {
                m_state = EnemyState::InvestigateSound;
                m_stateTimer = 4.4f;
            }
        }
    }

    m_suspicion = std::max(0.0f, m_suspicion - 6.5f * dt);

    switch (m_state) {
        case EnemyState::Idle:
            break;

        case EnemyState::Patrol: {
            if (!m_waypoints.empty()) {
                const sf::Vector2f wp = m_waypoints[m_waypointIdx];
                steerToward(wp, 78.0f * speedScale, dt, walls);
                if (distanceBetween(m_position, wp) < 18.0f) {
                    m_waypointIdx = (m_waypointIdx + 1) % m_waypoints.size();
                }
            }
            break;
        }

        case EnemyState::InvestigateSound: {
            steerToward(m_targetPos, 128.0f * speedScale, dt, walls);
            m_stateTimer -= dt;
            if (distanceBetween(m_position, m_targetPos) < 20.0f || m_stateTimer <= 0.0f) {
                m_state = EnemyState::SearchLastKnown;
                m_stateTimer = 3.2f;
            }
            break;
        }

        case EnemyState::SearchLastKnown: {
            m_stateTimer -= dt;
            const sf::Vector2f orbitOffset{
                std::cos(m_stateTimer * 2.6f) * 42.0f,
                std::sin(m_stateTimer * 2.6f) * 42.0f
            };
            steerToward(m_targetPos + orbitOffset, 92.0f * speedScale, dt, walls);
            if (m_stateTimer <= 0.0f) {
                m_state = EnemyState::ReturnToPatrol;
            }
            break;
        }

        case EnemyState::PursuePlayer: {
            steerToward(m_targetPos, 178.0f * speedScale, dt, walls);
            m_stateTimer -= dt;
            if (distanceBetween(m_position, m_targetPos) < 20.0f && distToPlayer > 44.0f) {
                m_state = EnemyState::SearchLastKnown;
                m_stateTimer = 3.5f;
            } else if (m_stateTimer <= 0.0f) {
                m_state = EnemyState::SearchLastKnown;
                m_stateTimer = 3.0f;
            }
            break;
        }

        case EnemyState::ReturnToPatrol: {
            if (m_waypoints.empty()) {
                m_state = EnemyState::Idle;
                break;
            }
            const sf::Vector2f home = m_waypoints[m_waypointIdx];
            steerToward(home, 88.0f * speedScale, dt, walls);
            if (distanceBetween(m_position, home) < 20.0f) {
                m_state = EnemyState::Patrol;
            }
            break;
        }

        case EnemyState::Attack:
            break;
    }

    m_tremorTimer += dt;
    const float tremorInterval = (m_state == EnemyState::PursuePlayer) ? 0.42f : 0.85f;
    if (m_tremorTimer >= tremorInterval) {
        m_tremorTimer = 0.0f;
        const float tremorRadius = (m_variant == EnemyVariant::Warden) ? 235.0f : 190.0f;
        acoustics.emitPulse(
            m_position,
            255.0f,
            tremorRadius,
            0.55f,
            28,
            WaveSignature::EnemyTremor,
            0.0f,
            6.2831853f,
            false
        );
    }

    const bool newlyAlerted =
        (prevState == EnemyState::Patrol || prevState == EnemyState::ReturnToPatrol) &&
        (m_state == EnemyState::InvestigateSound || m_state == EnemyState::PursuePlayer);
    return newlyAlerted;
}

void EnemyAI::render(
    sf::RenderTarget& target,
    float echoIllumination,
    sf::Vector2f playerPos,
    const AssetManager* assets
) const {
    if (!m_active) return;

    const float baseVis = (m_state == EnemyState::PursuePlayer)
        ? 0.94f
        : (m_state == EnemyState::InvestigateSound || m_state == EnemyState::SearchLastKnown ? 0.75f : 0.36f);
    const float vis = std::clamp(std::max(baseVis, echoIllumination), 0.0f, 1.0f);
    const auto alpha = static_cast<sf::Uint8>(vis * 245.0f);

    const float radius = (m_variant == EnemyVariant::Warden) ? (cfg::kEnemyRadius + 3.5f) : cfg::kEnemyRadius;

    // 1. Outer acoustic resonance membrane
    const float pulseR = radius + 6.0f + std::sin(m_animPhase) * (2.5f + m_alertBlend * 2.5f);
    sf::CircleShape outerRing(pulseR);
    outerRing.setOrigin(pulseR, pulseR);
    outerRing.setPosition(m_position);
    outerRing.setFillColor(sf::Color(48, 10, 18, static_cast<sf::Uint8>(vis * 105.0f)));
    outerRing.setOutlineThickness(1.5f);
    outerRing.setOutlineColor(sf::Color(239, 68, 68, static_cast<sf::Uint8>(vis * 145.0f)));
    target.draw(outerRing);

    // 2. Articulated 6-limb multi-jointed arachnid/bipedal stride animation
    const sf::Vector2f forward{std::cos(m_facingAngle), std::sin(m_facingAngle)};
    const sf::Vector2f right{-forward.y, forward.x};
    sf::VertexArray legs(sf::Lines);

    const float legReach = ((m_variant == EnemyVariant::Warden) ? 23.0f : 19.0f) + m_alertBlend * 4.0f;
    for (int side = -1; side <= 1; side += 2) {
        for (int legIdx = 0; legIdx < 3; ++legIdx) {
            const float phaseOffset = static_cast<float>(legIdx) * 2.094395f + (side > 0 ? 1.570796f : 0.0f);
            const float strideSwing = std::sin(m_animPhase + phaseOffset) * (5.5f + m_alertBlend * 3.5f);
            const float longOffset = (static_cast<float>(legIdx) - 1.0f) * 8.0f;

            const sf::Vector2f hip = m_position + forward * (longOffset * 0.4f) + right * (static_cast<float>(side) * 6.0f);
            const sf::Vector2f knee = m_position
                + forward * (longOffset * 0.8f + strideSwing * 0.5f)
                + right * (static_cast<float>(side) * (legReach * 0.62f))
                + sf::Vector2f{0.0f, -4.5f}; // 2.5D knee elevation
            const sf::Vector2f foot = m_position
                + forward * (longOffset + strideSwing)
                + right * (static_cast<float>(side) * legReach);

            const sf::Color hipCol(248, 113, 113, alpha);
            const sf::Color kneeCol(251, 146, 60, alpha);
            const sf::Color footCol(239, 68, 68, static_cast<sf::Uint8>(vis * 110.0f));

            legs.append(sf::Vertex(hip, hipCol));
            legs.append(sf::Vertex(knee, kneeCol));
            legs.append(sf::Vertex(knee, kneeCol));
            legs.append(sf::Vertex(foot, footCol));
        }
    }
    target.draw(legs);

    // 3. Render 3D Carapace Mesh (ListenerStalker or ListenerWarden) via AssetManager
    const sf::Color baseCol = (m_variant == EnemyVariant::Warden)
        ? sf::Color(72, 16, 28, alpha)
        : sf::Color(52, 12, 22, alpha);
    const sf::Color rimCol = (m_state == EnemyState::PursuePlayer)
        ? sf::Color(252, 165, 165, alpha)
        : sf::Color(248, 113, 113, alpha);

    if (assets != nullptr && assets->isInitialized()) {
        const ModelId modelId = (m_variant == EnemyVariant::Warden)
            ? ModelId::ListenerWarden
            : ModelId::ListenerStalker;
        const float meshScale = (m_variant == EnemyVariant::Warden) ? 19.5f : 16.0f;
        const float pitchLunge = (m_state == EnemyState::PursuePlayer || m_state == EnemyState::Attack) ? 0.24f : 0.05f;
        assets->renderMesh25D(
            target,
            modelId,
            m_position,
            m_facingAngle,
            meshScale,
            baseCol,
            rimCol,
            vis,
            playerPos,
            pitchLunge
        );
    } else {
        sf::CircleShape core(radius - 2.0f);
        core.setOrigin(radius - 2.0f, radius - 2.0f);
        core.setPosition(m_position);
        core.setFillColor(baseCol);
        core.setOutlineThickness(2.0f);
        core.setOutlineColor(rimCol);
        target.draw(core);
    }
}

} // namespace echo
