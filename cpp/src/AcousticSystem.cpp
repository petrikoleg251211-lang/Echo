#include "AcousticSystem.hpp"
#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <algorithm>
#include <cmath>

namespace echo {

namespace {
constexpr float kPi = 3.14159265358979323846f;

float cross2D(sf::Vector2f u, sf::Vector2f v) noexcept {
    return u.x * v.y - u.y * v.x;
}

float dot2D(sf::Vector2f u, sf::Vector2f v) noexcept {
    return u.x * v.x + u.y * v.y;
}

sf::Vector2f normalizeOrZero(sf::Vector2f v) noexcept {
    const float lenSq = v.x * v.x + v.y * v.y;
    if (lenSq < 1e-6f) return {0.0f, 0.0f};
    const float inv = 1.0f / std::sqrt(lenSq);
    return {v.x * inv, v.y * inv};
}

sf::Vector2f closestPointOnSegment(sf::Vector2f p, sf::Vector2f a, sf::Vector2f b) noexcept {
    const sf::Vector2f ab = b - a;
    const float lenSq = ab.x * ab.x + ab.y * ab.y;
    if (lenSq < 1e-6f) return a;
    const float t = std::clamp(((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / lenSq, 0.0f, 1.0f);
    return a + ab * t;
}

bool intersectSegments(
    sf::Vector2f p, sf::Vector2f p2,
    sf::Vector2f q, sf::Vector2f q2,
    float& outT, sf::Vector2f& outPoint
) noexcept {
    const sf::Vector2f r = p2 - p;
    const sf::Vector2f s = q2 - q;
    const float rxs = cross2D(r, s);
    if (std::abs(rxs) < 1e-6f) {
        return false;
    }
    const sf::Vector2f qp = q - p;
    const float t = cross2D(qp, s) / rxs;
    const float u = cross2D(qp, r) / rxs;
    if (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f) {
        outT = t;
        outPoint = p + r * t;
        return true;
    }
    return false;
}

bool isGroundVibrationSignature(WaveSignature sig) noexcept {
    return sig == WaveSignature::FootstepSprint ||
           sig == WaveSignature::RadialPulse ||
           sig == WaveSignature::StompWave ||
           sig == WaveSignature::ShaleCrack ||
           sig == WaveSignature::GlassShatterWave ||
           sig == WaveSignature::EnemyTremor;
}

sf::Color signatureColor(WaveSignature sig, AcousticFrequency freq) noexcept {
    switch (sig) {
        case WaveSignature::FootstepSneak:    return sf::Color(100, 140, 175);
        case WaveSignature::FootstepWalk:     return sf::Color(56, 189, 248);
        case WaveSignature::FootstepSprint:   return sf::Color(34, 211, 238);
        case WaveSignature::DirectionalPing:
            if (freq == AcousticFrequency::Low) return sf::Color(59, 130, 246);
            if (freq == AcousticFrequency::High) return sf::Color(165, 243, 252);
            return sf::Color(56, 189, 248);
        case WaveSignature::RadialPulse:      return sf::Color(125, 211, 252);
        case WaveSignature::DecoyImpact:      return sf::Color(167, 139, 250);
        case WaveSignature::MechanismClunk:   return sf::Color(192, 132, 252);
        case WaveSignature::EnemyTremor:      return sf::Color(239, 68, 68);
        case WaveSignature::AmbientDrip:      return sf::Color(45, 212, 191);
        case WaveSignature::ShaleCrack:       return sf::Color(251, 191, 36);
        case WaveSignature::ResonatorHum:     return sf::Color(192, 132, 252);
        case WaveSignature::WhistleBeam:      return sf::Color(110, 231, 183);
        case WaveSignature::StompWave:        return sf::Color(251, 146, 60);
        case WaveSignature::EchoTrapPulse:    return sf::Color(232, 121, 249);
        case WaveSignature::GlassShatterWave: return sf::Color(224, 242, 254);
    }
    return sf::Color(56, 189, 248);
}

void appendThickQuad(
    sf::VertexArray& triangles,
    sf::Vector2f p1,
    sf::Vector2f p2,
    sf::Vector2f offset,
    sf::Color cOuter,
    sf::Color cInner
) {
    const sf::Vector2f q1 = p1 + offset;
    const sf::Vector2f q2 = p2 + offset;
    triangles.append(sf::Vertex(p1, cOuter));
    triangles.append(sf::Vertex(p2, cOuter));
    triangles.append(sf::Vertex(q2, cInner));

    triangles.append(sf::Vertex(p1, cOuter));
    triangles.append(sf::Vertex(q2, cInner));
    triangles.append(sf::Vertex(q1, cInner));
}
} // namespace

AcousticSystem::AcousticSystem()
    : m_rays(cfg::kMaxActiveRays)
    , m_echoes(cfg::kMaxEchoTraces)
    , m_rings(cfg::kMaxWaveRings)
    , m_wallQuads(sf::Triangles)
    , m_echoLines(sf::Lines)
    , m_waveLines(sf::Lines) {
    m_recentEvents.reserve(32);
}

void AcousticSystem::clear() noexcept {
    for (auto& r : m_rays) r.active = false;
    for (auto& e : m_echoes) e.active = false;
    for (auto& ring : m_rings) ring.active = false;
    m_recentEvents.clear();
    m_activeRayCount = 0;
    m_activeEchoCount = 0;
    m_shatteredGlassCount = 0;
    m_crystalSplitCount = 0;
}

int AcousticSystem::consumeShatteredGlassCount(sf::Vector2f* outLastShatterPos) noexcept {
    const int count = m_shatteredGlassCount;
    if (outLastShatterPos != nullptr && count > 0) {
        *outLastShatterPos = m_lastGlassShatterPos;
    }
    m_shatteredGlassCount = 0;
    return count;
}

int AcousticSystem::consumeCrystalSplitCount() noexcept {
    const int count = m_crystalSplitCount;
    m_crystalSplitCount = 0;
    return count;
}

void AcousticSystem::spawnSplitRay(const AcousticRay& parentRay, sf::Vector2f splitOrigin, float angleOffsetRad) noexcept {
    AcousticRay& child = m_rays[m_rayCursor];
    m_rayCursor = (m_rayCursor + 1) % m_rays.size();
    const float c = std::cos(angleOffsetRad);
    const float s = std::sin(angleOffsetRad);
    const sf::Vector2f rotatedDir{
        parentRay.dir.x * c - parentRay.dir.y * s,
        parentRay.dir.x * s + parentRay.dir.y * c
    };
    child.pos = splitOrigin;
    child.prevPos = splitOrigin;
    child.dir = normalizeOrZero(rotatedDir);
    child.speed = parentRay.speed * 1.04f;
    child.intensity = std::max(0.18f, parentRay.intensity * 0.78f);
    child.maxDistance = parentRay.maxDistance * 1.15f;
    child.traveled = parentRay.traveled * 0.65f;
    child.reflections = parentRay.reflections;
    child.maxReflections = static_cast<std::uint8_t>(std::min<int>(10, static_cast<int>(parentRay.maxReflections) + 1));
    child.signature = parentRay.signature;
    child.frequency = parentRay.frequency;
    child.tint = sf::Color(232, 121, 249);
    child.emittedByPlayer = parentRay.emittedByPlayer;
    child.metalBonusApplied = parentRay.metalBonusApplied;
    child.waterBoostApplied = parentRay.waterBoostApplied;
    child.crystalSplitApplied = true;
    child.active = true;
}

void AcousticSystem::emitPulse(
    sf::Vector2f origin,
    float speed,
    float maxDistance,
    float initialIntensity,
    std::size_t rayCount,
    WaveSignature signature,
    float centerAngleRad,
    float spreadAngleRad,
    bool emittedByPlayer
) {
    const std::size_t scaledCount = std::max<std::size_t>(
        8,
        static_cast<std::size_t>(static_cast<float>(rayCount) * m_rayDensityScale)
    );

    const float startAngle = centerAngleRad - spreadAngleRad * 0.5f;
    const bool fullCircle = spreadAngleRad >= (2.0f * kPi - 0.01f);
    const float step = fullCircle
        ? (spreadAngleRad / static_cast<float>(scaledCount))
        : (scaledCount > 1 ? spreadAngleRad / static_cast<float>(scaledCount - 1) : 0.0f);

    const sf::Color baseColor = signatureColor(signature, m_activeFrequency);

    for (std::size_t i = 0; i < scaledCount; ++i) {
        const float angle = startAngle + static_cast<float>(i) * step;
        AcousticRay& ray = m_rays[m_rayCursor];
        m_rayCursor = (m_rayCursor + 1) % m_rays.size();

        if (!ray.active) {
            ++m_activeRayCount;
        }
        ray.pos = origin;
        ray.prevPos = origin;
        ray.dir = {std::cos(angle), std::sin(angle)};
        ray.speed = speed;
        ray.intensity = initialIntensity;
        ray.maxDistance = maxDistance;
        ray.traveled = 0.0f;
        ray.reflections = 0;
        ray.maxReflections = m_maxReflections;
        ray.signature = signature;
        ray.frequency = m_activeFrequency;
        ray.tint = baseColor;
        ray.emittedByPlayer = emittedByPlayer;
        ray.metalBonusApplied = false;
        ray.waterBoostApplied = false;
        ray.crystalSplitApplied = false;
        ray.active = true;
    }

    ExpandingWaveRing& ring = m_rings[m_ringCursor];
    m_ringCursor = (m_ringCursor + 1) % m_rings.size();
    ring.origin = origin;
    ring.radius = 4.0f;
    ring.maxRadius = maxDistance * 0.65f;
    ring.speed = speed;
    ring.intensity = std::min(1.0f, initialIntensity * 0.65f);
    ring.centerAngle = centerAngleRad;
    ring.spreadAngle = spreadAngleRad;
    ring.color = baseColor;
    ring.active = true;

    m_recentEvents.push_back(AcousticEvent{
        origin,
        initialIntensity,
        maxDistance,
        signature,
        m_activeFrequency,
        emittedByPlayer,
        isGroundVibrationSignature(signature)
    });

    if (emittedByPlayer &&
        (signature == WaveSignature::DirectionalPing ||
         signature == WaveSignature::RadialPulse ||
         signature == WaveSignature::WhistleBeam ||
         signature == WaveSignature::StompWave ||
         signature == WaveSignature::EchoTrapPulse)) {
        ++m_totalPulsesEmitted;
    }
}

void AcousticSystem::emitWaterRipples(sf::Vector2f origin, float baseRadius) {
    // Block 2.4: Water conducts sound to 2x distance and spawns concentric wave rings visible to enemies
    const float doubledRange = baseRadius * getMaterialProperties(SurfaceMaterial::Water).distanceMultiplier;
    const sf::Color waterCol(45, 212, 191);

    for (int i = 0; i < 3; ++i) {
        ExpandingWaveRing& ring = m_rings[m_ringCursor];
        m_ringCursor = (m_ringCursor + 1) % m_rings.size();
        ring.origin = origin;
        ring.radius = 6.0f + static_cast<float>(i) * 18.0f;
        ring.maxRadius = doubledRange * (0.55f + static_cast<float>(i) * 0.22f);
        ring.speed = 310.0f - static_cast<float>(i) * 28.0f;
        ring.intensity = 0.85f - static_cast<float>(i) * 0.18f;
        ring.centerAngle = 0.0f;
        ring.spreadAngle = 6.2831853f;
        ring.color = waterCol;
        ring.active = true;
    }

    m_recentEvents.push_back(AcousticEvent{
        origin,
        0.78f,
        doubledRange,
        WaveSignature::AmbientDrip,
        m_activeFrequency,
        true,
        true
    });
}

void AcousticSystem::spawnEchoTrace(
    sf::Vector2f hitPoint,
    sf::Vector2f wallTangent,
    sf::Vector2f wallNormal,
    float intensity,
    SurfaceMaterial material,
    AcousticFrequency freq
) {
    const MaterialProperties props = getMaterialProperties(material);
    WallEchoTrace& trace = m_echoes[m_echoCursor];
    m_echoCursor = (m_echoCursor + 1) % m_echoes.size();

    const float lenScale = (freq == AcousticFrequency::Low) ? 1.25f : (freq == AcousticFrequency::High ? 0.85f : 1.0f);
    const float lifeScale = (freq == AcousticFrequency::Low) ? 1.35f : 1.0f;

    trace.hitPoint = hitPoint;
    trace.tangent = wallTangent;
    trace.normal = wallNormal;
    trace.halfLength = (10.0f + 9.0f * std::min(intensity, 1.0f)) * lenScale;
    trace.intensity = std::clamp(intensity, 0.12f, 1.0f);
    trace.age = 0.0f;
    trace.lifetime = props.persistenceSec * lifeScale;
    trace.material = material;
    trace.active = true;
}

std::vector<int> AcousticSystem::update(
    float dt,
    const std::vector<WallSegment>& walls,
    std::vector<AcousticResonator>* resonators,
    float resonatorChargeScale
) {
    std::vector<WallSegment> mutableCopy = walls;
    return update(dt, mutableCopy, resonators, resonatorChargeScale);
}

std::vector<int> AcousticSystem::update(
    float dt,
    std::vector<WallSegment>& walls,
    std::vector<AcousticResonator>* resonators,
    float resonatorChargeScale
) {
    std::vector<int> newlyActivatedResonators;

    // Block 2.5: Update AcousticResonator energy & 4-second harmonic humming lure
    if (resonators != nullptr) {
        for (auto& res : *resonators) {
            if (!res.activated && res.energy > 0.0f) {
                res.energy = std::max(0.0f, res.energy - 0.18f * dt);
            }
            if (res.humTimer > 0.0f) {
                res.humTimer = std::max(0.0f, res.humTimer - dt);
                res.humPulseAccum += dt;
                if (res.humPulseAccum >= 0.90f) {
                    res.humPulseAccum = 0.0f;
                    // Emit a humming ring and lure event from the resonator column so enemies investigate the column!
                    ExpandingWaveRing& ring = m_rings[m_ringCursor];
                    m_ringCursor = (m_ringCursor + 1) % m_rings.size();
                    ring.origin = res.position;
                    ring.radius = res.radius;
                    ring.maxRadius = cfg::kResonatorHumRadius;
                    ring.speed = 260.0f;
                    ring.intensity = 0.65f;
                    ring.centerAngle = 0.0f;
                    ring.spreadAngle = 6.2831853f;
                    ring.color = sf::Color(192, 132, 252);
                    ring.active = true;

                    m_recentEvents.push_back(AcousticEvent{
                        res.position,
                        0.72f,
                        cfg::kResonatorHumRadius * 1.4f,
                        WaveSignature::ResonatorHum,
                        AcousticFrequency::Mid,
                        true,
                        false
                    });
                }
            }
        }
    }

    const std::size_t rayPoolSize = m_rays.size();
    for (std::size_t idx = 0; idx < rayPoolSize; ++idx) {
        AcousticRay& ray = m_rays[idx];
        if (!ray.active) continue;

        const float stepDist = ray.speed * dt;
        ray.prevPos = ray.pos;
        const sf::Vector2f nextPos = ray.pos + ray.dir * stepDist;

        // Check if this ray energizes an AcousticResonator & triggers its 4-second hum (Block 2.5)
        if (resonators != nullptr && ray.emittedByPlayer &&
            (ray.signature == WaveSignature::DirectionalPing ||
             ray.signature == WaveSignature::RadialPulse ||
             ray.signature == WaveSignature::DecoyImpact ||
             ray.signature == WaveSignature::WhistleBeam ||
             ray.signature == WaveSignature::StompWave ||
             ray.signature == WaveSignature::EchoTrapPulse)) {
            for (auto& res : *resonators) {
                const sf::Vector2f c = closestPointOnSegment(res.position, ray.pos, nextPos);
                const sf::Vector2f d = res.position - c;
                if (d.x * d.x + d.y * d.y <= res.radius * res.radius) {
                    // Block 2.5: Start 4-second resonator hum on hit
                    if (res.humTimer <= 0.1f) {
                        res.humPulseAccum = 0.85f;
                    }
                    res.humTimer = cfg::kResonatorHumDurationSec;

                    if (!res.activated) {
                        res.energy = std::min(1.0f, res.energy + ray.intensity * 0.14f * resonatorChargeScale);
                        if (res.energy >= 1.0f) {
                            res.activated = true;
                            newlyActivatedResonators.push_back(res.id);
                        }
                    }
                }
            }
        }

        float bestT = 2.0f;
        sf::Vector2f bestHit{};
        WallSegment* hitWall = nullptr;

        for (auto& wall : walls) {
            if (wall.isOpen) continue;
            float t = 0.0f;
            sf::Vector2f pt{};
            if (intersectSegments(ray.pos, nextPos, wall.a, wall.b, t, pt)) {
                if (t < bestT) {
                    bestT = t;
                    bestHit = pt;
                    hitWall = &wall;
                }
            }
        }

        if (hitWall != nullptr) {
            const sf::Vector2f wallVec = hitWall->b - hitWall->a;
            const sf::Vector2f wallTangent = normalizeOrZero(wallVec);
            sf::Vector2f wallNormal{-wallTangent.y, wallTangent.x};
            if (dot2D(ray.dir, wallNormal) > 0.0f) {
                wallNormal = -wallNormal;
            }

            const MaterialProperties matProps = getMaterialProperties(hitWall->material);
            spawnEchoTrace(bestHit, wallTangent, wallNormal, ray.intensity, hitWall->material, ray.frequency);

            // Block 2.3: Brittle Glass shatters when hit by a powerful impulse
            const bool isHighPowerPulse =
                ray.emittedByPlayer &&
                (ray.signature == WaveSignature::RadialPulse ||
                 ray.signature == WaveSignature::StompWave ||
                 ray.signature == WaveSignature::EchoTrapPulse ||
                 (ray.signature == WaveSignature::DirectionalPing && ray.intensity >= 0.85f));

            if (hitWall->material == SurfaceMaterial::Glass && isHighPowerPulse) {
                hitWall->isOpen = true;
                ++m_shatteredGlassCount;
                m_lastGlassShatterPos = bestHit;
                if (hitWall->doorId != -1) {
                    if (std::find(newlyActivatedResonators.begin(), newlyActivatedResonators.end(), hitWall->doorId) ==
                        newlyActivatedResonators.end()) {
                        newlyActivatedResonators.push_back(hitWall->doorId);
                    }
                }
                // Ray continues right through the newly shattered glass barrier
                ray.pos = nextPos;
                ray.traveled += stepDist;
                if (ray.traveled >= ray.maxDistance || ray.intensity < 0.05f) {
                    ray.active = false;
                }
                continue;
            }

            // Concealed Secret Door (doorId >= 900): unseals when struck by high-energy player sonar ping or pulse
            if (hitWall->doorId >= 900 && ray.emittedByPlayer &&
                (ray.signature == WaveSignature::DirectionalPing ||
                 ray.signature == WaveSignature::RadialPulse ||
                 ray.signature == WaveSignature::DecoyImpact ||
                 ray.signature == WaveSignature::StompWave ||
                 ray.signature == WaveSignature::EchoTrapPulse) &&
                ray.intensity >= 0.35f) {
                if (std::find(newlyActivatedResonators.begin(), newlyActivatedResonators.end(), hitWall->doorId) ==
                    newlyActivatedResonators.end()) {
                    newlyActivatedResonators.push_back(hitWall->doorId);
                }
            }

            // Block 2.2: Polished Metal grants lossless reflection + up to +2 bonus reflections (+3 on High freq)
            if (hitWall->material == SurfaceMaterial::Metal && !ray.metalBonusApplied) {
                ray.metalBonusApplied = true;
                const int bonus = matProps.bonusReflections + (ray.frequency == AcousticFrequency::High ? 1 : 0);
                ray.maxReflections = static_cast<std::uint8_t>(std::min<int>(10, static_cast<int>(ray.maxReflections) + bonus));
            }

            // Block 2.4: Water conducts sound to 2x distance
            if (hitWall->material == SurfaceMaterial::Water && !ray.waterBoostApplied) {
                ray.waterBoostApplied = true;
                ray.maxDistance *= matProps.distanceMultiplier;
            }

            ++ray.reflections;
            ray.intensity *= matProps.reflectivity;
            // High frequency attenuates faster on DampMoss
            if (hitWall->material == SurfaceMaterial::DampMoss && ray.frequency == AcousticFrequency::High) {
                ray.intensity *= 0.60f;
            }
            ray.tint = matProps.echoColor;

            if (ray.reflections > ray.maxReflections || ray.intensity < 0.07f) {
                ray.active = false;
                continue;
            }

            const float dn = dot2D(ray.dir, wallNormal);
            ray.dir = normalizeOrZero(ray.dir - 2.0f * dn * wallNormal);
            const float remainingStep = stepDist * (1.0f - bestT);
            ray.pos = bestHit + wallNormal * 0.6f + ray.dir * remainingStep;

            // Stage 2: Crystal Vaults — EchoCrystal splits & scatters rays chaotically into bifurcated prism beams
            if (hitWall->material == SurfaceMaterial::EchoCrystal && !ray.crystalSplitApplied && ray.intensity >= 0.22f) {
                ray.crystalSplitApplied = true;
                ray.maxDistance *= matProps.distanceMultiplier;
                const float jitter = 0.38f + 0.12f * std::sin(bestHit.x * 0.07f + bestHit.y * 0.11f);
                const AcousticRay parentSnapshot = ray;
                spawnSplitRay(parentSnapshot, ray.pos, jitter);
                spawnSplitRay(parentSnapshot, ray.pos, -jitter * 0.85f);
                ++m_crystalSplitCount;
            }
        } else {
            ray.pos = nextPos;
        }

        ray.traveled += stepDist;
        if (ray.traveled >= ray.maxDistance || ray.intensity < 0.05f) {
            ray.active = false;
            continue;
        }
    }

    m_activeRayCount = 0;
    for (const auto& ray : m_rays) {
        if (ray.active) ++m_activeRayCount;
    }

    m_activeEchoCount = 0;
    for (auto& echo : m_echoes) {
        if (!echo.active) continue;
        echo.age += dt;
        if (echo.age >= echo.lifetime) {
            echo.active = false;
            continue;
        }
        ++m_activeEchoCount;
    }

    for (auto& ring : m_rings) {
        if (!ring.active) continue;
        ring.radius += ring.speed * dt;
        if (ring.radius >= ring.maxRadius) {
            ring.active = false;
        }
    }

    return newlyActivatedResonators;
}

float AcousticSystem::consumeRayImpactAt(
    sf::Vector2f targetPos,
    float targetRadius,
    bool& outReflectedHit,
    bool* outHighFreqHit
) noexcept {
    outReflectedHit = false;
    if (outHighFreqHit != nullptr) {
        *outHighFreqHit = false;
    }
    float totalImpact = 0.0f;
    const float radSq = (targetRadius + 5.0f) * (targetRadius + 5.0f);

    for (auto& ray : m_rays) {
        if (!ray.active || !ray.emittedByPlayer) continue;
        if (ray.signature != WaveSignature::DirectionalPing &&
            ray.signature != WaveSignature::RadialPulse &&
            ray.signature != WaveSignature::DecoyImpact &&
            ray.signature != WaveSignature::WhistleBeam &&
            ray.signature != WaveSignature::StompWave &&
            ray.signature != WaveSignature::EchoTrapPulse) {
            continue;
        }

        const sf::Vector2f c = closestPointOnSegment(targetPos, ray.prevPos, ray.pos);
        const sf::Vector2f d = targetPos - c;
        if (d.x * d.x + d.y * d.y <= radSq) {
            float weight = 0.85f;
            if (ray.signature == WaveSignature::DirectionalPing) weight = 1.25f;
            else if (ray.signature == WaveSignature::WhistleBeam) weight = 1.15f;
            else if (ray.signature == WaveSignature::StompWave) weight = 1.35f;
            else if (ray.signature == WaveSignature::EchoTrapPulse) weight = 1.18f;
            else if (ray.signature == WaveSignature::DecoyImpact) weight = 1.10f;

            if (ray.reflections > 0) {
                outReflectedHit = true;
                weight *= 1.35f;
            }
            if (ray.frequency == AcousticFrequency::High && outHighFreqHit != nullptr) {
                *outHighFreqHit = true;
            }
            totalImpact += ray.intensity * weight;

            const sf::Vector2f normal = normalizeOrZero(c - targetPos);
            const sf::Vector2f tangent{-normal.y, normal.x};
            spawnEchoTrace(c, tangent, normal, std::min(1.0f, ray.intensity * 1.15f), SurfaceMaterial::ResonanceDoor, ray.frequency);

            ray.active = false;
            if (m_activeRayCount > 0) --m_activeRayCount;
        }
    }

    return totalImpact;
}

float AcousticSystem::computeTransmissionFactor(
    sf::Vector2f source,
    sf::Vector2f listener,
    const std::vector<WallSegment>& walls,
    AcousticFrequency freq
) noexcept {
    float factor = 1.0f;
    int hitCount = 0;

    for (const auto& wall : walls) {
        if (wall.isOpen) continue;
        float t = 0.0f;
        sf::Vector2f pt{};
        if (intersectSegments(source, listener, wall.a, wall.b, t, pt)) {
            float trans = getMaterialProperties(wall.material).transmission;
            // Block 3.4: Low frequency penetrates thin walls more effectively
            if (freq == AcousticFrequency::Low) {
                trans = std::min(0.92f, std::sqrt(trans));
            } else if (freq == AcousticFrequency::High) {
                trans *= 0.82f;
            }
            factor *= trans;
            ++hitCount;
            if (hitCount >= 4) break;
        }
    }

    return std::clamp(factor, 0.08f, 1.0f);
}

bool AcousticSystem::isInDampMossShelter(
    sf::Vector2f pos,
    const std::vector<WallSegment>& walls,
    SurfaceMaterial floorMat
) noexcept {
    constexpr float kShelterDistSq = 56.0f * 56.0f;
    for (const auto& w : walls) {
        if (w.isOpen || w.material != SurfaceMaterial::DampMoss) continue;
        const sf::Vector2f c = closestPointOnSegment(pos, w.a, w.b);
        const sf::Vector2f d = pos - c;
        if (d.x * d.x + d.y * d.y <= kShelterDistSq) {
            return true;
        }
    }
    if (floorMat == SurfaceMaterial::DampMoss) {
        for (const auto& w : walls) {
            if (w.isOpen) continue;
            const sf::Vector2f c = closestPointOnSegment(pos, w.a, w.b);
            const sf::Vector2f d = pos - c;
            if (d.x * d.x + d.y * d.y <= 72.0f * 72.0f) {
                return true;
            }
        }
    }
    return false;
}

float AcousticSystem::queryIlluminationAt(
    sf::Vector2f point,
    sf::Vector2f playerPos,
    const std::vector<AcousticResonator>* resonators
) const noexcept {
    float maxIllum = 0.0f;

    const sf::Vector2f dp = point - playerPos;
    const float distPlayerSq = dp.x * dp.x + dp.y * dp.y;
    if (distPlayerSq < 75.0f * 75.0f) {
        maxIllum = std::max(maxIllum, 1.0f - std::sqrt(distPlayerSq) / 75.0f);
    }

    // Block 2.5: Humming AcousticResonator columns illuminate a radius around themselves for 4 seconds
    if (resonators != nullptr) {
        for (const auto& res : *resonators) {
            if (res.humTimer <= 0.0f) continue;
            const sf::Vector2f dr = point - res.position;
            const float drSq = dr.x * dr.x + dr.y * dr.y;
            const float humR = cfg::kResonatorHumRadius;
            if (drSq < humR * humR) {
                const float fade = std::min(1.0f, res.humTimer / 1.0f);
                const float prox = 1.0f - std::sqrt(drSq) / humR;
                maxIllum = std::max(maxIllum, fade * prox * 0.92f);
            }
        }
    }

    for (const auto& echo : m_echoes) {
        if (!echo.active) continue;
        const sf::Vector2f d = point - echo.hitPoint;
        const float dSq = d.x * d.x + d.y * d.y;
        if (dSq < 68.0f * 68.0f) {
            const float life = 1.0f - (echo.age / echo.lifetime);
            const float prox = 1.0f - std::sqrt(dSq) / 68.0f;
            maxIllum = std::max(maxIllum, life * prox * echo.intensity);
        }
    }

    for (const auto& ray : m_rays) {
        if (!ray.active) continue;
        const sf::Vector2f d = point - ray.pos;
        const float dSq = d.x * d.x + d.y * d.y;
        if (dSq < 48.0f * 48.0f) {
            const float fade = 1.0f - (ray.traveled / std::max(1.0f, ray.maxDistance));
            maxIllum = std::max(maxIllum, fade * ray.intensity);
        }
    }

    return std::clamp(maxIllum, 0.0f, 1.0f);
}

void AcousticSystem::render(
    sf::RenderTarget& target,
    float ambientVisibility,
    const std::vector<WallSegment>* walls
) const {
    m_wallQuads.clear();
    m_echoLines.clear();
    m_waveLines.clear();

    if (walls != nullptr && ambientVisibility > 0.002f) {
        const auto ambAlpha = static_cast<sf::Uint8>(std::clamp(ambientVisibility * 255.0f, 0.0f, 90.0f));
        for (const auto& w : *walls) {
            if (w.isOpen) continue;
            const MaterialProperties mp = getMaterialProperties(w.material, m_colorblindMode);
            sf::Color lineCol = mp.echoColor;
            lineCol.a = (w.material == SurfaceMaterial::Glass || w.material == SurfaceMaterial::EchoCrystal)
                ? static_cast<sf::Uint8>(std::min<int>(185, ambAlpha * 2 + 44))
                : ambAlpha;
            m_echoLines.append(sf::Vertex(w.a, lineCol));
            m_echoLines.append(sf::Vertex(w.b, lineCol));
        }
    }

    for (const auto& echo : m_echoes) {
        if (!echo.active) continue;
        const float lifeRatio = 1.0f - (echo.age / echo.lifetime);
        const float harmonicPulse = 1.0f + cfg::feel::kEchoPulseAmplitude *
            std::sin(echo.age * cfg::feel::kEchoPulseFreqRad + (echo.hitPoint.x + echo.hitPoint.y) * 0.025f);
        const float smoothAlpha = std::clamp(lifeRatio * lifeRatio * echo.intensity * harmonicPulse, 0.0f, 1.0f);
        const MaterialProperties mp = getMaterialProperties(echo.material, m_colorblindMode);

        const float pulsedHalfLen = echo.halfLength * (0.94f + 0.08f * harmonicPulse);
        const sf::Vector2f p1 = echo.hitPoint - echo.tangent * pulsedHalfLen;
        const sf::Vector2f p2 = echo.hitPoint + echo.tangent * pulsedHalfLen;

        sf::Color fillOuter = mp.fillTint;
        fillOuter.a = static_cast<sf::Uint8>(smoothAlpha * 185.0f);
        sf::Color fillInner = mp.fillTint;
        fillInner.a = 0;

        appendThickQuad(
            m_wallQuads,
            p1,
            p2,
            -echo.normal * mp.wallThickness,
            fillOuter,
            fillInner
        );

        sf::Color glowOuter = mp.echoColor;
        glowOuter.a = static_cast<sf::Uint8>(smoothAlpha * 65.0f);
        sf::Color glowInner = mp.echoColor;
        glowInner.a = 0;
        appendThickQuad(
            m_wallQuads,
            p1,
            p2,
            echo.normal * 7.0f,
            glowOuter,
            glowInner
        );

        sf::Color edgeColor = mp.echoColor;
        edgeColor.a = static_cast<sf::Uint8>(smoothAlpha * 235.0f);
        m_echoLines.append(sf::Vertex(p1, edgeColor));
        m_echoLines.append(sf::Vertex(p2, edgeColor));

        if (echo.material == SurfaceMaterial::Water) {
            sf::Color rippleCol = mp.echoColor;
            rippleCol.a = static_cast<sf::Uint8>(smoothAlpha * 120.0f);
            const float rippleOffset = 3.5f + std::sin(echo.age * 8.0f) * 2.0f;
            m_echoLines.append(sf::Vertex(p1 + echo.normal * rippleOffset, rippleCol));
            m_echoLines.append(sf::Vertex(p2 + echo.normal * rippleOffset, rippleCol));
        } else if (echo.material == SurfaceMaterial::EchoCrystal) {
            sf::Color prismCol = applyColorblindTransform(
                sf::Color(244, 114, 182, static_cast<sf::Uint8>(smoothAlpha * 210.0f)),
                m_colorblindMode
            );
            m_echoLines.append(sf::Vertex(echo.hitPoint - echo.normal * 6.0f - echo.tangent * 4.0f, prismCol));
            m_echoLines.append(sf::Vertex(echo.hitPoint + echo.normal * 6.0f + echo.tangent * 4.0f, prismCol));
            m_echoLines.append(sf::Vertex(echo.hitPoint - echo.normal * 6.0f + echo.tangent * 4.0f, prismCol));
            m_echoLines.append(sf::Vertex(echo.hitPoint + echo.normal * 6.0f - echo.tangent * 4.0f, prismCol));
        } else if (echo.material == SurfaceMaterial::Metal || echo.material == SurfaceMaterial::Glass) {
            sf::Color specCol(224, 242, 254, static_cast<sf::Uint8>(smoothAlpha * 185.0f));
            m_echoLines.append(sf::Vertex(echo.hitPoint - echo.normal * 3.5f, specCol));
            m_echoLines.append(sf::Vertex(echo.hitPoint + echo.normal * 3.5f, specCol));
        }
    }

    for (const auto& ring : m_rings) {
        if (!ring.active) continue;
        const float progress = ring.radius / std::max(1.0f, ring.maxRadius);
        const float alphaF = std::clamp((1.0f - progress) * ring.intensity * 0.45f, 0.0f, 1.0f);
        sf::Color ringCol = applyColorblindTransform(ring.color, m_colorblindMode);
        ringCol.a = static_cast<sf::Uint8>(alphaF * 255.0f);

        constexpr int kSegments = 36;
        const float startA = ring.centerAngle - ring.spreadAngle * 0.5f;
        const float stepA = ring.spreadAngle / static_cast<float>(kSegments);
        for (int i = 0; i < kSegments; ++i) {
            const float a0 = startA + static_cast<float>(i) * stepA;
            const float a1 = a0 + stepA;
            const sf::Vector2f u0 = ring.origin + sf::Vector2f{std::cos(a0), std::sin(a0)} * ring.radius;
            const sf::Vector2f u1 = ring.origin + sf::Vector2f{std::cos(a1), std::sin(a1)} * ring.radius;
            m_waveLines.append(sf::Vertex(u0, ringCol));
            m_waveLines.append(sf::Vertex(u1, ringCol));
        }
    }

    for (const auto& ray : m_rays) {
        if (!ray.active) continue;
        const float fade = 1.0f - (ray.traveled / std::max(1.0f, ray.maxDistance));
        const float alphaF = std::clamp(fade * ray.intensity, 0.0f, 1.0f);

        const sf::Color cbRayTint = applyColorblindTransform(ray.tint, m_colorblindMode);
        sf::Color headColor = cbRayTint;
        headColor.a = static_cast<sf::Uint8>(alphaF * 230.0f);

        sf::Color tailColor = cbRayTint;
        tailColor.a = static_cast<sf::Uint8>(alphaF * 30.0f);

        const sf::Vector2f tailPos = ray.pos - ray.dir * 16.0f;
        m_waveLines.append(sf::Vertex(tailPos, tailColor));
        m_waveLines.append(sf::Vertex(ray.pos, headColor));
    }

    target.draw(m_wallQuads);
    sf::RenderStates additiveStates(sf::BlendAdd);
    target.draw(m_echoLines, additiveStates);
    target.draw(m_waveLines, additiveStates);
}

} // namespace echo
