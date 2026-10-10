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

sf::Color signatureColor(WaveSignature sig) noexcept {
    switch (sig) {
        case WaveSignature::FootstepSneak:   return sf::Color(100, 140, 175);
        case WaveSignature::FootstepWalk:    return sf::Color(56, 189, 248);
        case WaveSignature::FootstepSprint:  return sf::Color(34, 211, 238);
        case WaveSignature::DirectionalPing: return sf::Color(56, 189, 248);
        case WaveSignature::RadialPulse:     return sf::Color(125, 211, 252);
        case WaveSignature::DecoyImpact:     return sf::Color(167, 139, 250);
        case WaveSignature::MechanismClunk:  return sf::Color(192, 132, 252);
        case WaveSignature::EnemyTremor:     return sf::Color(239, 68, 68);
        case WaveSignature::AmbientDrip:     return sf::Color(45, 212, 191);
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

    const sf::Color baseColor = signatureColor(signature);

    for (std::size_t i = 0; i < scaledCount; ++i) {
        const float angle = startAngle + static_cast<float>(i) * step;
        AcousticRay& ray = m_rays[m_rayCursor];
        m_rayCursor = (m_rayCursor + 1) % m_rays.size();

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
        ray.tint = baseColor;
        ray.active = true;
    }

    // Spawn an expanding wavefront ring visual
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
        emittedByPlayer
    });

    if (emittedByPlayer && (signature == WaveSignature::DirectionalPing || signature == WaveSignature::RadialPulse)) {
        ++m_totalPulsesEmitted;
    }
}

void AcousticSystem::spawnEchoTrace(
    sf::Vector2f hitPoint,
    sf::Vector2f wallTangent,
    sf::Vector2f wallNormal,
    float intensity,
    SurfaceMaterial material
) {
    const MaterialProperties props = getMaterialProperties(material);
    WallEchoTrace& trace = m_echoes[m_echoCursor];
    m_echoCursor = (m_echoCursor + 1) % m_echoes.size();

    trace.hitPoint = hitPoint;
    trace.tangent = wallTangent;
    trace.normal = wallNormal;
    trace.halfLength = 10.0f + 9.0f * std::min(intensity, 1.0f);
    trace.intensity = std::clamp(intensity, 0.12f, 1.0f);
    trace.age = 0.0f;
    trace.lifetime = props.persistenceSec;
    trace.material = material;
    trace.active = true;
}

void AcousticSystem::update(float dt, const std::vector<WallSegment>& walls) {
    m_activeRayCount = 0;
    for (auto& ray : m_rays) {
        if (!ray.active) continue;

        const float stepDist = ray.speed * dt;
        ray.prevPos = ray.pos;
        const sf::Vector2f nextPos = ray.pos + ray.dir * stepDist;

        float bestT = 2.0f;
        sf::Vector2f bestHit{};
        const WallSegment* hitWall = nullptr;

        for (const auto& wall : walls) {
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
            spawnEchoTrace(bestHit, wallTangent, wallNormal, ray.intensity, hitWall->material);

            ++ray.reflections;
            ray.intensity *= matProps.reflectivity;
            ray.tint = matProps.echoColor;

            if (ray.reflections > ray.maxReflections || ray.intensity < 0.07f) {
                ray.active = false;
                continue;
            }

            const float dn = dot2D(ray.dir, wallNormal);
            ray.dir = normalizeOrZero(ray.dir - 2.0f * dn * wallNormal);
            const float remainingStep = stepDist * (1.0f - bestT);
            ray.pos = bestHit + wallNormal * 0.6f + ray.dir * remainingStep;
        } else {
            ray.pos = nextPos;
        }

        ray.traveled += stepDist;
        if (ray.traveled >= ray.maxDistance || ray.intensity < 0.05f) {
            ray.active = false;
            continue;
        }

        ++m_activeRayCount;
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
}

float AcousticSystem::queryIlluminationAt(sf::Vector2f point, sf::Vector2f playerPos) const noexcept {
    float maxIllum = 0.0f;

    // Tactile close-range player aura (very small radius around surveyor)
    const sf::Vector2f dp = point - playerPos;
    const float distPlayerSq = dp.x * dp.x + dp.y * dp.y;
    if (distPlayerSq < 75.0f * 75.0f) {
        maxIllum = std::max(maxIllum, 1.0f - std::sqrt(distPlayerSq) / 75.0f);
    }

    // Check active wall echoes
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

    // Check active rays
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

    // 1. Subtle ambient architectural silhouette if ambientVisibility > 0
    if (walls != nullptr && ambientVisibility > 0.002f) {
        const auto ambAlpha = static_cast<sf::Uint8>(std::clamp(ambientVisibility * 255.0f, 0.0f, 90.0f));
        for (const auto& w : *walls) {
            if (w.isOpen) continue;
            const MaterialProperties mp = getMaterialProperties(w.material);
            sf::Color lineCol = mp.echoColor;
            lineCol.a = ambAlpha;
            m_echoLines.append(sf::Vertex(w.a, lineCol));
            m_echoLines.append(sf::Vertex(w.b, lineCol));
        }
    }

    // 2. Render 3D-extruded wall illumination bevels and glowing acoustic edges
    for (const auto& echo : m_echoes) {
        if (!echo.active) continue;
        const float lifeRatio = 1.0f - (echo.age / echo.lifetime);
        const float smoothAlpha = std::clamp(lifeRatio * lifeRatio * echo.intensity, 0.0f, 1.0f);
        const MaterialProperties mp = getMaterialProperties(echo.material);

        const sf::Vector2f p1 = echo.hitPoint - echo.tangent * echo.halfLength;
        const sf::Vector2f p2 = echo.hitPoint + echo.tangent * echo.halfLength;

        // Extrude away from the side the wave struck to depict solid masonry depth
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

        // Soft inner bloom toward the room
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

        // Crisp primary acoustic strike line
        sf::Color edgeColor = mp.echoColor;
        edgeColor.a = static_cast<sf::Uint8>(smoothAlpha * 235.0f);
        m_echoLines.append(sf::Vertex(p1, edgeColor));
        m_echoLines.append(sf::Vertex(p2, edgeColor));

        // Material-specific texture accents (rivets for Metal, dual ripple for Water)
        if (echo.material == SurfaceMaterial::Water) {
            sf::Color rippleCol = mp.echoColor;
            rippleCol.a = static_cast<sf::Uint8>(smoothAlpha * 120.0f);
            const float rippleOffset = 3.5f + std::sin(echo.age * 8.0f) * 2.0f;
            m_echoLines.append(sf::Vertex(p1 + echo.normal * rippleOffset, rippleCol));
            m_echoLines.append(sf::Vertex(p2 + echo.normal * rippleOffset, rippleCol));
        } else if (echo.material == SurfaceMaterial::Metal) {
            sf::Color specCol(224, 242, 254, static_cast<sf::Uint8>(smoothAlpha * 170.0f));
            m_echoLines.append(sf::Vertex(echo.hitPoint - echo.normal * 3.0f, specCol));
            m_echoLines.append(sf::Vertex(echo.hitPoint + echo.normal * 3.0f, specCol));
        }
    }

    // 3. Render expanding wave rings
    for (const auto& ring : m_rings) {
        if (!ring.active) continue;
        const float progress = ring.radius / std::max(1.0f, ring.maxRadius);
        const float alphaF = std::clamp((1.0f - progress) * ring.intensity * 0.45f, 0.0f, 1.0f);
        sf::Color ringCol = ring.color;
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

    // 4. Render propagating ray wavefront streaks
    for (const auto& ray : m_rays) {
        if (!ray.active) continue;
        const float fade = 1.0f - (ray.traveled / std::max(1.0f, ray.maxDistance));
        const float alphaF = std::clamp(fade * ray.intensity, 0.0f, 1.0f);

        sf::Color headColor = ray.tint;
        headColor.a = static_cast<sf::Uint8>(alphaF * 230.0f);

        sf::Color tailColor = ray.tint;
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
