#include "EnemyAI.hpp"
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
} // namespace

EnemyAI::EnemyAI() = default;

const char* EnemyAI::stateName(EnemyState s) noexcept {
    switch (s) {
        case EnemyState::Idle:             return "IDLE";
        case EnemyState::Patrol:           return "PATROL";
        case EnemyState::InvestigateSound: return "INVESTIGATING SOUND";
        case EnemyState::SearchLastKnown:  return "SEARCHING SECTOR";
        case EnemyState::PursuePlayer:     return "PURSUING SOURCE";
        case EnemyState::Attack:           return "CONTACT / LETHAL";
        case EnemyState::ReturnToPatrol:   return "RETURNING TO PATROL";
    }
    return "PATROL";
}

void EnemyAI::configurePatrol(const std::vector<sf::Vector2f>& waypoints) {
    m_waypoints = waypoints;
    m_waypointIdx = 0;
    m_suspicion = 0.0f;
    m_stateTimer = 0.0f;
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

void EnemyAI::moveAlongDirection(
    sf::Vector2f target,
    float speed,
    float dt,
    const std::vector<WallSegment>& walls
) noexcept {
    const sf::Vector2f delta = target - m_position;
    const float dist = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    if (dist < 2.0f) return;

    const sf::Vector2f dir = delta / dist;
    const sf::Vector2f candidate = m_position + dir * (speed * dt);
    if (hasLineOfSight(candidate, walls)) {
        m_position = candidate;
    } else {
        // Slide along cardinal axes if direct ray is blocked
        const sf::Vector2f slideX{m_position.x + dir.x * speed * dt, m_position.y};
        const sf::Vector2f slideY{m_position.x, m_position.y + dir.y * speed * dt};
        if (hasLineOfSight(slideX, walls)) m_position = slideX;
        else if (hasLineOfSight(slideY, walls)) m_position = slideY;
    }
}

void EnemyAI::update(
    float dt,
    sf::Vector2f playerPos,
    const std::vector<WallSegment>& walls,
    AcousticSystem& acoustics
) {
    if (!m_active || m_state == EnemyState::Attack) return;

    const float distToPlayer = distanceBetween(m_position, playerPos);
    if (distToPlayer <= 24.0f) {
        m_state = EnemyState::Attack;
        return;
    }

    // 1. Evaluate acoustic events emitted in the current frame
    for (const auto& ev : acoustics.recentEvents()) {
        if (!ev.emittedByPlayer) continue;
        const float dist = distanceBetween(m_position, ev.origin);
        if (dist > ev.radius * 1.15f) continue;

        const bool occluded = !hasLineOfSight(ev.origin, walls);
        const float occlusionFactor = occluded ? 0.52f : 1.0f;
        const float perceived = ev.loudness * occlusionFactor * (1.0f - dist / (ev.radius * 1.25f));

        if (perceived > 0.14f) {
            m_targetPos = ev.origin;
            m_suspicion = std::min(100.0f, m_suspicion + perceived * 55.0f);

            if (perceived > 0.65f || m_suspicion >= 75.0f) {
                m_state = EnemyState::PursuePlayer;
                m_stateTimer = 5.0f;
            } else if (m_state != EnemyState::PursuePlayer) {
                m_state = EnemyState::InvestigateSound;
                m_stateTimer = 4.5f;
            }
        }
    }

    // 2. State machine update
    m_suspicion = std::max(0.0f, m_suspicion - 6.5f * dt);

    switch (m_state) {
        case EnemyState::Idle:
            break;

        case EnemyState::Patrol: {
            if (!m_waypoints.empty()) {
                const sf::Vector2f wp = m_waypoints[m_waypointIdx];
                moveAlongDirection(wp, 78.0f, dt, walls);
                if (distanceBetween(m_position, wp) < 16.0f) {
                    m_waypointIdx = (m_waypointIdx + 1) % m_waypoints.size();
                }
            }
            break;
        }

        case EnemyState::InvestigateSound: {
            moveAlongDirection(m_targetPos, 128.0f, dt, walls);
            m_stateTimer -= dt;
            if (distanceBetween(m_position, m_targetPos) < 18.0f || m_stateTimer <= 0.0f) {
                m_state = EnemyState::SearchLastKnown;
                m_stateTimer = 3.2f;
            }
            break;
        }

        case EnemyState::SearchLastKnown: {
            m_stateTimer -= dt;
            const sf::Vector2f orbitOffset{
                std::cos(m_stateTimer * 2.5f) * 40.0f,
                std::sin(m_stateTimer * 2.5f) * 40.0f
            };
            moveAlongDirection(m_targetPos + orbitOffset, 92.0f, dt, walls);
            if (m_stateTimer <= 0.0f) {
                m_state = EnemyState::ReturnToPatrol;
            }
            break;
        }

        case EnemyState::PursuePlayer: {
            moveAlongDirection(m_targetPos, 182.0f, dt, walls);
            m_stateTimer -= dt;
            if (distanceBetween(m_position, m_targetPos) < 20.0f && distToPlayer > 45.0f) {
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
            moveAlongDirection(home, 88.0f, dt, walls);
            if (distanceBetween(m_position, home) < 20.0f) {
                m_state = EnemyState::Patrol;
            }
            break;
        }

        case EnemyState::Attack:
            break;
    }

    // 3. Emit subtle crimson acoustic tremor from enemy footsteps
    m_tremorTimer += dt;
    const float tremorInterval = (m_state == EnemyState::PursuePlayer) ? 0.42f : 0.85f;
    if (m_tremorTimer >= tremorInterval) {
        m_tremorTimer = 0.0f;
        acoustics.emitPulse(
            m_position,
            260.0f,
            195.0f,
            0.55f,
            28,
            WaveSignature::EnemyTremor,
            0.0f,
            6.2831853f,
            false
        );
    }
}

} // namespace echo
