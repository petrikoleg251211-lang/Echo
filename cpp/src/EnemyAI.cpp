#include "EnemyAI.hpp"
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
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

EnemyAI::EnemyAI() {
    m_bolts.resize(24);
}

const char* EnemyAI::stateName(EnemyState s) noexcept {
    switch (s) {
        case EnemyState::Idle:             return "DORMANT";
        case EnemyState::Patrol:           return "PATROLLING";
        case EnemyState::InvestigateSound: return "INVESTIGATING";
        case EnemyState::SearchLastKnown:  return "SEARCHING";
        case EnemyState::PursuePlayer:     return "HUNTING";
        case EnemyState::Attack:           return "CONTACT";
        case EnemyState::ReturnToPatrol:   return "RETURNING";
    }
    return "PATROLLING";
}

void EnemyAI::clearContactState() noexcept {
    if (m_state == EnemyState::Attack) {
        m_state = EnemyState::PursuePlayer;
        m_stateTimer = 3.5f;
        m_meleeTelegraphProgress = 0.0f;
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
    const float radius = hitRadius();
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

void EnemyAI::steerToward(sf::Vector2f target, float speed, float dt, const std::vector<WallSegment>& walls) noexcept {
    sf::Vector2f desiredTarget = target;

    if (!hasLineOfSight(target, walls) && !m_waypoints.empty()) {
        float bestScore = 1e9f;
        for (const auto& wp : m_waypoints) {
            if (hasLineOfSight(wp, walls)) {
                const float score = distanceBetween(m_position, wp) * 0.45f + distanceBetween(wp, target);
                if (score < bestScore) {
                    bestScore = score;
                    desiredTarget = wp;
                }
            }
        }
    }

    const sf::Vector2f delta = desiredTarget - m_position;
    const float dist = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    if (dist > 1.5f) {
        const sf::Vector2f dir = delta / dist;
        m_facingAngle = std::atan2(dir.y, dir.x);
        m_position += dir * (speed * dt);
        resolveWallCollisions(walls);
    }
}

bool EnemyAI::update(
    float dt,
    sf::Vector2f playerPos,
    const std::vector<WallSegment>& walls,
    AcousticSystem& acoustics
) {
    if (!m_active) return false;

    m_hitFlashTimer = std::max(0.0f, m_hitFlashTimer - dt);
    m_staggerTimer = std::max(0.0f, m_staggerTimer - dt);
    m_swarmAmplifyCooldown = std::max(0.0f, m_swarmAmplifyCooldown - dt);

    const auto varIdx = std::min<std::size_t>(
        static_cast<std::size_t>(m_variant),
        cfg::balance::kDefaultBalance.enemies.size() - 1
    );
    const auto& bal = cfg::balance::kDefaultBalance.enemies[varIdx];
    float speedScale = bal.speedScale * m_difficultySpeedMult;

    if (m_variant == EnemyVariant::Mimic && m_disguised) {
        speedScale = 0.0f;
    } else if (m_variant == EnemyVariant::AbyssalBoss) {
        const int phase = bossPhase();
        speedScale = ((phase == 3) ? 1.25f : (phase == 2 ? 1.05f : 0.88f)) * m_difficultySpeedMult;
    }

    if (m_staggerTimer > 0.0f) {
        speedScale *= 0.25f;
    }

    const float animSpeed = (m_state == EnemyState::PursuePlayer) ? 8.5f : 4.5f;
    m_animPhase += dt * animSpeed;

    const float targetAlert = (m_state == EnemyState::PursuePlayer)
        ? 1.0f
        : (m_state == EnemyState::InvestigateSound || m_state == EnemyState::SearchLastKnown ? 0.55f : 0.0f);
    m_alertBlend += (targetAlert - m_alertBlend) * std::min(1.0f, dt * 5.0f);

    const EnemyState prevState = m_state;

    const float catchRadius = cfg::kPlayerRadius + hitRadius() - 1.5f;
    const float telegraphRadius = catchRadius + cfg::feel::kMeleeTelegraphRangeMargin;
    const float distToPlayer = distanceBetween(m_position, playerPos);

    // Block 4.3: Mimic unmasks and ambushes if player walks too close (< 78 px)
    if (m_variant == EnemyVariant::Mimic && m_disguised && distToPlayer <= 78.0f) {
        m_disguised = false;
        m_targetPos = playerPos;
        m_suspicion = 100.0f;
        m_state = EnemyState::PursuePlayer;
        m_stateTimer = 5.5f;
    }

    // Block 4.1: SilentStalker emits a short high-frequency inhale 0.5s before melee strike (< 88 px)
    if (m_variant == EnemyVariant::SilentStalker) {
        if (distToPlayer <= 88.0f && !m_inhaleWarningEmitted) {
            m_inhaleWarningEmitted = true;
            acoustics.emitPulse(
                m_position,
                380.0f,
                135.0f,
                0.78f,
                24,
                WaveSignature::EnemyTremor,
                0.0f,
                6.2831853f,
                false
            );
        } else if (distToPlayer > 140.0f) {
            m_inhaleWarningEmitted = false;
        }
    }

    // Stage 2: Leech (Acoustic Parasite) latches onto the player on close approach, draining stamina & pulsing +8 noise
    if (m_variant == EnemyVariant::Leech && m_staggerTimer <= 0.0f) {
        if (m_leechedOnPlayer) {
            const float orbitAng = m_animPhase * 3.2f;
            m_position = playerPos + sf::Vector2f{std::cos(orbitAng), std::sin(orbitAng)} * 14.0f;
            m_leechDrainTimer += dt;
            if (m_leechDrainTimer >= 1.35f) {
                m_leechDrainTimer = 0.0f;
                m_triggeredLeechDrain = true;
                acoustics.emitPulse(
                    m_position,
                    340.0f,
                    280.0f,
                    0.82f,
                    32,
                    WaveSignature::ResonatorHum,
                    0.0f,
                    6.2831853f,
                    false
                );
            }
            return false;
        } else if (distToPlayer <= catchRadius + 10.0f) {
            m_leechedOnPlayer = true;
            m_leechDrainTimer = 1.15f;
            m_state = EnemyState::PursuePlayer;
            m_position = playerPos + sf::Vector2f{12.0f, 0.0f};
            return true;
        }
    }

    // Stage 1.1: Melee Attack Telegraphing (0.45s windup with acoustic + audio + visual cue)
    if (!m_disguised && m_variant != EnemyVariant::Leech && distToPlayer <= telegraphRadius) {
        if (m_meleeTelegraphProgress <= 0.001f) {
            m_telegraphCuePending = true;
            acoustics.emitPulse(
                m_position,
                370.0f,
                150.0f,
                0.82f,
                24,
                WaveSignature::EnemyTremor,
                0.0f,
                6.2831853f,
                false
            );
        }
        m_meleeTelegraphProgress = std::min(
            1.0f,
            m_meleeTelegraphProgress + dt / std::max(0.15f, cfg::feel::kEnemyMeleeTelegraphSec)
        );
        // Slow lunge movement slightly while winding up strike so player can react
        speedScale *= 0.58f;

        if (distToPlayer <= catchRadius && m_meleeTelegraphProgress >= 1.0f) {
            m_disguised = false;
            m_state = EnemyState::Attack;
            return true;
        }
    } else {
        m_meleeTelegraphProgress = std::max(0.0f, m_meleeTelegraphProgress - dt * 2.5f);
    }

    const std::size_t eventCount = acoustics.recentEvents().size();
    for (std::size_t idx = 0; idx < eventCount; ++idx) {
        const AcousticEvent ev = acoustics.recentEvents()[idx];
        if (!ev.emittedByPlayer) continue;

        if (m_variant == EnemyVariant::DeafBrute && !ev.isGroundVibration) {
            continue;
        }

        const float dist = distanceBetween(m_position, ev.origin);
        if (dist > ev.radius * 1.15f) continue;

        if (m_variant == EnemyVariant::Mimic && m_disguised) {
            if (ev.signature == WaveSignature::WhistleBeam ||
                ev.frequency == AcousticFrequency::High ||
                ev.signature == WaveSignature::StompWave) {
                m_disguised = false;
                m_targetPos = ev.origin;
                m_suspicion = 100.0f;
                m_state = EnemyState::PursuePlayer;
                m_stateTimer = 5.5f;
            }
            continue;
        }

        const float transmissionFactor = AcousticSystem::computeTransmissionFactor(ev.origin, m_position, walls, ev.frequency);
        const float sensitivity = (m_variant == EnemyVariant::Shrieker ||
                                   m_variant == EnemyVariant::EchoSwarm ||
                                   m_variant == EnemyVariant::Banshee ||
                                   m_variant == EnemyVariant::Leech) ? 1.25f : 1.0f;
        const float perceived = ev.loudness * sensitivity * transmissionFactor * (1.0f - dist / (ev.radius * 1.25f));

        if (perceived > 0.14f) {
            m_targetPos = ev.origin;
            m_suspicion = std::min(100.0f, m_suspicion + perceived * 54.0f);

            if (m_variant == EnemyVariant::EchoSwarm && m_swarmAmplifyCooldown <= 0.0f) {
                m_swarmAmplifyCooldown = 1.15f;
                m_triggeredSwarmAmplification = true;
                acoustics.emitPulse(
                    m_position,
                    390.0f,
                    460.0f,
                    0.95f,
                    48,
                    WaveSignature::ResonatorHum,
                    0.0f,
                    6.2831853f,
                    false
                );
            }

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

    // Stage 2: Banshee & Phase-2 Conductor periodic sonic wail (+22 noise & chamber-wide wave)
    if ((m_variant == EnemyVariant::Banshee || (m_variant == EnemyVariant::TheConductor && m_hp <= m_maxHp * 0.55f)) &&
        (m_state == EnemyState::InvestigateSound || m_state == EnemyState::SearchLastKnown || m_state == EnemyState::PursuePlayer)) {
        m_bansheeWailCooldown -= dt;
        if (m_bansheeWailCooldown <= 0.0f) {
            m_bansheeWailCooldown = (m_variant == EnemyVariant::TheConductor) ? 4.4f : 3.6f;
            m_triggeredBansheeWail = true;
            acoustics.emitPulse(
                m_position,
                430.0f,
                520.0f,
                1.00f,
                56,
                WaveSignature::ResonatorHum,
                0.0f,
                6.2831853f,
                false
            );
        }
    }

    switch (m_state) {
        case EnemyState::Idle:
            break;

        case EnemyState::Patrol: {
            if (!m_waypoints.empty() && !m_disguised) {
                const sf::Vector2f wp = m_waypoints[m_waypointIdx];
                steerToward(wp, bal.patrolSpeed * speedScale, dt, walls);
                if (distanceBetween(m_position, wp) < 18.0f) {
                    m_waypointIdx = (m_waypointIdx + 1) % m_waypoints.size();
                }
            }
            break;
        }

        case EnemyState::InvestigateSound: {
            steerToward(m_targetPos, bal.investigateSpeed * speedScale, dt, walls);
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
            if (distToPlayer <= telegraphRadius * 1.35f) {
                m_targetPos = playerPos;
            }
            steerToward(m_targetPos, bal.pursueSpeed * speedScale, dt, walls);
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

    // Update ranged projectiles & Boss/Shrieker/TuningFork/Conductor volleys (implemented in BossAI.cpp)
    updateBossAndRangedVolleys(dt, walls, acoustics);

    if (m_variant == EnemyVariant::SilentStalker) {
        // SilentStalker emits zero passive movement tremors
    } else if (m_variant == EnemyVariant::Mimic && m_disguised) {
        m_tremorTimer += dt;
        if (m_tremorTimer >= 1.55f) {
            m_tremorTimer = 0.0f;
            acoustics.emitPulse(
                m_position,
                260.0f,
                230.0f,
                0.62f,
                32,
                WaveSignature::ResonatorHum,
                0.0f,
                6.2831853f,
                false
            );
        }
    } else {
        m_tremorTimer += dt;
        const float tremorInterval = (m_state == EnemyState::PursuePlayer) ? 0.42f : 0.85f;
        if (m_tremorTimer >= tremorInterval) {
            m_tremorTimer = 0.0f;
            const float tremorRadius = (m_variant == EnemyVariant::AbyssalBoss || m_variant == EnemyVariant::TheConductor)
                ? 310.0f
                : ((m_variant == EnemyVariant::Warden || m_variant == EnemyVariant::DeafBrute || m_variant == EnemyVariant::TuningForkConstruct) ? 245.0f : 190.0f);
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
    }

    const bool newlyAlerted =
        (prevState == EnemyState::Patrol || prevState == EnemyState::ReturnToPatrol || prevState == EnemyState::Idle) &&
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

    for (const auto& b : m_bolts) {
        if (!b.active) continue;
        sf::CircleShape halo(b.radius + 4.0f);
        halo.setOrigin(b.radius + 4.0f, b.radius + 4.0f);
        halo.setPosition(b.pos);
        halo.setFillColor(sf::Color(244, 63, 94, 70));
        target.draw(halo);

        sf::CircleShape core(b.radius);
        core.setOrigin(b.radius, b.radius);
        core.setPosition(b.pos);
        core.setFillColor(sf::Color(253, 164, 175));
        core.setOutlineThickness(1.5f);
        core.setOutlineColor(sf::Color(225, 29, 72));
        target.draw(core);
    }

    if (m_variant == EnemyVariant::Mimic && m_disguised) {
        const float vis = std::clamp(std::max(0.38f, echoIllumination), 0.20f, 1.0f);
        const auto alpha = static_cast<sf::Uint8>(vis * 245.0f);
        const bool sonarRevealingGlitch = (echoIllumination >= 0.35f);
        const sf::Color disguiseCol = sonarRevealingGlitch
            ? sf::Color(244, 63, 94, alpha)
            : sf::Color(167, 139, 250, alpha);

        const float jitter = sonarRevealingGlitch ? std::sin(m_animPhase * 9.0f) * 3.0f : 0.0f;
        const sf::Vector2f drawPos{m_position.x + jitter, m_position.y};

        sf::CircleShape ring(19.0f + std::sin(m_animPhase * 1.4f) * 2.5f);
        ring.setOrigin(ring.getRadius(), ring.getRadius());
        ring.setPosition(drawPos);
        ring.setFillColor(sf::Color(disguiseCol.r, disguiseCol.g, disguiseCol.b, 35));
        ring.setOutlineThickness(2.0f);
        ring.setOutlineColor(disguiseCol);
        target.draw(ring);

        if (assets != nullptr && assets->isInitialized()) {
            assets->renderMesh25D(
                target,
                ModelId::ResonatorCrystal,
                drawPos,
                m_animPhase * 0.4f,
                15.0f,
                sf::Color(28, 20, 52, alpha),
                disguiseCol,
                vis,
                playerPos,
                0.12f
            );
        }
        return;
    }

    const float telegraphProg = attackTelegraphProgress();
    float baseVis = (m_state == EnemyState::PursuePlayer || telegraphProg > 0.05f || m_leechedOnPlayer)
        ? 0.96f
        : (m_state == EnemyState::InvestigateSound || m_state == EnemyState::SearchLastKnown ? 0.75f : 0.36f);

    if (m_variant == EnemyVariant::Phantom && m_hitFlashTimer <= 0.0f && telegraphProg <= 0.05f) {
        baseVis *= 0.42f;
    } else if (m_variant == EnemyVariant::SilentStalker && m_hitFlashTimer <= 0.0f && !m_inhaleWarningEmitted && telegraphProg <= 0.05f) {
        baseVis = 0.02f;
    }

    const float minVisFloor = (m_variant == EnemyVariant::SilentStalker && m_hitFlashTimer <= 0.0f && !m_inhaleWarningEmitted && telegraphProg <= 0.05f)
        ? 0.02f
        : 0.12f;
    const float vis = std::clamp(std::max(baseVis, echoIllumination), minVisFloor, 1.0f);
    if (vis <= 0.04f) {
        return;
    }
    const auto alpha = static_cast<sf::Uint8>(vis * 245.0f);
    const float radius = hitRadius();

    // Stage 1.1: Visual Attack Telegraph Ring (converging crimson/amber warning circle + aim line)
    if (telegraphProg > 0.02f) {
        const float outerR = radius + 6.0f + (1.0f - telegraphProg) * 28.0f;
        sf::CircleShape warnRing(outerR);
        warnRing.setOrigin(outerR, outerR);
        warnRing.setPosition(m_position);
        warnRing.setFillColor(sf::Color(244, 63, 94, static_cast<sf::Uint8>(40.0f + telegraphProg * 75.0f)));
        warnRing.setOutlineThickness(2.2f);
        warnRing.setOutlineColor(sf::Color(250, 204, 21, static_cast<sf::Uint8>(140.0f + telegraphProg * 115.0f)));
        target.draw(warnRing);

        sf::VertexArray strikeLine(sf::Lines, 2);
        const sf::Vector2f aimDir{std::cos(m_facingAngle), std::sin(m_facingAngle)};
        strikeLine[0] = sf::Vertex(m_position, sf::Color(250, 204, 21, 220));
        strikeLine[1] = sf::Vertex(m_position + aimDir * (radius + 26.0f), sf::Color(244, 63, 94, 240));
        target.draw(strikeLine);
    }

    if (m_variant == EnemyVariant::EchoSwarm) {
        for (int i = 0; i < 5; ++i) {
            const float ang = m_animPhase * 1.8f + static_cast<float>(i) * 1.256637f;
            const float orbitR = 9.0f + std::sin(m_animPhase * 3.0f + static_cast<float>(i)) * 4.0f;
            const sf::Vector2f subPos = m_position + sf::Vector2f{std::cos(ang), std::sin(ang)} * orbitR;
            sf::CircleShape node(4.5f);
            node.setOrigin(4.5f, 4.5f);
            node.setPosition(subPos);
            node.setFillColor(m_hitFlashTimer > 0.0f ? sf::Color(165, 243, 252, alpha) : sf::Color(232, 121, 249, alpha));
            node.setOutlineThickness(1.2f);
            node.setOutlineColor(sf::Color(250, 204, 21, alpha));
            target.draw(node);
        }
    } else if (m_variant == EnemyVariant::TuningForkConstruct || m_variant == EnemyVariant::TheConductor) {
        // Render resonant tuning-fork prongs / conductor baton arcs
        const sf::Vector2f fwd{std::cos(m_facingAngle), std::sin(m_facingAngle)};
        const sf::Vector2f side{-fwd.y, fwd.x};
        sf::VertexArray prongs(sf::Lines, 4);
        const sf::Color prongCol = (m_variant == EnemyVariant::TheConductor)
            ? sf::Color(244, 114, 182, alpha)
            : sf::Color(165, 243, 252, alpha);
        prongs[0] = sf::Vertex(m_position + side * 7.0f, prongCol);
        prongs[1] = sf::Vertex(m_position + side * 9.0f + fwd * (radius + 12.0f), prongCol);
        prongs[2] = sf::Vertex(m_position - side * 7.0f, prongCol);
        prongs[3] = sf::Vertex(m_position - side * 9.0f + fwd * (radius + 12.0f), prongCol);
        target.draw(prongs);
    }

    const float pulseR = radius + 6.0f + std::sin(m_animPhase) * (2.5f + m_alertBlend * 2.5f);
    sf::CircleShape outerRing(pulseR);
    outerRing.setOrigin(pulseR, pulseR);
    outerRing.setPosition(m_position);
    outerRing.setFillColor(sf::Color(48, 10, 18, static_cast<sf::Uint8>(vis * 105.0f)));
    outerRing.setOutlineThickness(1.5f);
    outerRing.setOutlineColor(
        m_hitFlashTimer > 0.0f
            ? sf::Color(125, 211, 252, 240)
            : sf::Color(239, 68, 68, static_cast<sf::Uint8>(vis * 145.0f))
    );
    target.draw(outerRing);

    const sf::Vector2f forward{std::cos(m_facingAngle), std::sin(m_facingAngle)};
    const sf::Vector2f right{-forward.y, forward.x};
    sf::VertexArray legs(sf::Lines);

    const float legReach = radius + 6.0f + m_alertBlend * 4.0f;
    for (int side = -1; side <= 1; side += 2) {
        for (int legIdx = 0; legIdx < 3; ++legIdx) {
            const float phaseOffset = static_cast<float>(legIdx) * 2.094395f + (side > 0 ? 1.570796f : 0.0f);
            const float strideSwing = std::sin(m_animPhase + phaseOffset) * (5.5f + m_alertBlend * 3.5f);
            const float longOffset = (static_cast<float>(legIdx) - 1.0f) * 8.0f;

            const sf::Vector2f hip = m_position + forward * (longOffset * 0.4f) + right * (static_cast<float>(side) * 6.0f);
            const sf::Vector2f knee = m_position
                + forward * (longOffset * 0.8f + strideSwing * 0.5f)
                + right * (static_cast<float>(side) * (legReach * 0.62f))
                + sf::Vector2f{0.0f, -4.5f};
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

    sf::Color baseCol(52, 12, 22, alpha);
    sf::Color rimCol(248, 113, 113, alpha);

    if (m_variant == EnemyVariant::Warden) {
        baseCol = sf::Color(72, 16, 28, alpha);
        rimCol = sf::Color(251, 146, 60, alpha);
    } else if (m_variant == EnemyVariant::DeafBrute) {
        baseCol = sf::Color(68, 42, 18, alpha);
        rimCol = sf::Color(250, 204, 21, alpha);
    } else if (m_variant == EnemyVariant::Shrieker) {
        baseCol = sf::Color(48, 16, 68, alpha);
        rimCol = sf::Color(216, 180, 254, alpha);
    } else if (m_variant == EnemyVariant::Phantom || m_variant == EnemyVariant::SilentStalker) {
        baseCol = sf::Color(16, 42, 58, alpha);
        rimCol = sf::Color(45, 212, 191, alpha);
    } else if (m_variant == EnemyVariant::Mimic) {
        baseCol = sf::Color(70, 14, 48, alpha);
        rimCol = sf::Color(232, 121, 249, alpha);
    } else if (m_variant == EnemyVariant::EchoSwarm) {
        baseCol = sf::Color(58, 18, 64, alpha);
        rimCol = sf::Color(240, 171, 252, alpha);
    } else if (m_variant == EnemyVariant::Leech) {
        baseCol = sf::Color(16, 58, 44, alpha);
        rimCol = sf::Color(52, 211, 153, alpha);
    } else if (m_variant == EnemyVariant::Banshee) {
        baseCol = sf::Color(64, 14, 58, alpha);
        rimCol = sf::Color(244, 114, 182, alpha);
    } else if (m_variant == EnemyVariant::TuningForkConstruct) {
        baseCol = sf::Color(30, 58, 82, alpha);
        rimCol = sf::Color(165, 243, 252, alpha);
    } else if (m_variant == EnemyVariant::TheConductor) {
        baseCol = sf::Color(86, 20, 68, alpha);
        rimCol = sf::Color(232, 121, 249, alpha);
    } else if (m_variant == EnemyVariant::AbyssalBoss) {
        baseCol = sf::Color(88, 12, 36, alpha);
        rimCol = (bossPhase() == 3) ? sf::Color(250, 204, 21, alpha) : sf::Color(244, 63, 94, alpha);
    }

    if (m_hitFlashTimer > 0.0f) {
        baseCol = sf::Color(56, 189, 248, 235);
        rimCol = sf::Color(224, 242, 254, 255);
    }

    if (assets != nullptr && assets->isInitialized()) {
        ModelId modelId = ModelId::ListenerStalker;
        float meshScale = 16.0f;
        if (m_variant == EnemyVariant::Warden || m_variant == EnemyVariant::DeafBrute || m_variant == EnemyVariant::TuningForkConstruct) {
            modelId = ModelId::ListenerWarden;
            meshScale = (m_variant == EnemyVariant::DeafBrute) ? 21.5f : 19.5f;
        } else if (m_variant == EnemyVariant::Shrieker || m_variant == EnemyVariant::Mimic || m_variant == EnemyVariant::Banshee) {
            modelId = ModelId::ResonatorCrystal;
            meshScale = 17.5f;
        } else if (m_variant == EnemyVariant::TheConductor) {
            modelId = ModelId::ListenerWarden;
            meshScale = 23.5f;
        } else if (m_variant == EnemyVariant::AbyssalBoss) {
            modelId = ModelId::ListenerWarden;
            meshScale = 27.0f;
        }
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

    if (m_hp < m_maxHp || m_variant == EnemyVariant::AbyssalBoss || m_variant == EnemyVariant::TheConductor) {
        const float barW = (m_variant == EnemyVariant::AbyssalBoss || m_variant == EnemyVariant::TheConductor) ? 58.0f : 32.0f;
        const float barH = 4.0f;
        const sf::Vector2f barPos{m_position.x - barW * 0.5f, m_position.y - radius - 16.0f};

        sf::RectangleShape bg({barW, barH});
        bg.setPosition(barPos);
        bg.setFillColor(sf::Color(15, 23, 42, 200));
        bg.setOutlineThickness(1.0f);
        bg.setOutlineColor(sf::Color(71, 85, 105, 180));
        target.draw(bg);

        const float ratio = std::clamp(m_hp / std::max(1.0f, m_maxHp), 0.0f, 1.0f);
        sf::RectangleShape fill({barW * ratio, barH});
        fill.setPosition(barPos);
        fill.setFillColor(m_hitFlashTimer > 0.0f ? sf::Color(125, 211, 252) : sf::Color(244, 63, 94));
        target.draw(fill);
    }
}

} // namespace echo
