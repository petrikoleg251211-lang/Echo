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

// Segment-segment intersection: P + t*R (t in [0,1]) vs Q + u*S (u in [0,1])
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
        case WaveSignature::FootstepSneak:   return sf::Color(100, 116, 139);
        case WaveSignature::FootstepWalk:    return sf::Color(56, 189, 248);
        case WaveSignature::FootstepSprint:  return sf::Color(251, 191, 36);
        case WaveSignature::DirectionalPing: return sf::Color(56, 189, 248);
        case WaveSignature::RadialPulse:     return sf::Color(125, 211, 252);
        case WaveSignature::DecoyImpact:     return sf::Color(245, 158, 11);
        case WaveSignature::MechanismClunk:  return sf::Color(167, 139, 250);
        case WaveSignature::EnemyTremor:     return sf::Color(239, 68, 68);
    }
    return sf::Color(56, 189, 248);
}
} // namespace

AcousticSystem::AcousticSystem()
    : m_rays(cfg::kMaxActiveRays)
    , m_echoes(cfg::kMaxEchoTraces)
    , m_waveVertices(sf::Lines)
    , m_echoVertices(sf::Lines) {
    m_recentEvents.reserve(32);
}

void AcousticSystem::clear() noexcept {
    for (auto& r : m_rays) r.active = false;
    for (auto& e : m_echoes) e.active = false;
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
    if (rayCount == 0) return;

    const float startAngle = centerAngleRad - spreadAngleRad * 0.5f;
    const bool fullCircle = spreadAngleRad >= (2.0f * kPi - 0.01f);
    const float step = fullCircle
        ? (spreadAngleRad / static_cast<float>(rayCount))
        : (rayCount > 1 ? spreadAngleRad / static_cast<float>(rayCount - 1) : 0.0f);

    const sf::Color baseColor = signatureColor(signature);

    for (std::size_t i = 0; i < rayCount; ++i) {
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
        ray.maxReflections = cfg::kMaxReflections;
        ray.signature = signature;
        ray.tint = baseColor;
        ray.active = true;
    }

    m_recentEvents.push_back(AcousticEvent{
        origin,
        initialIntensity,
        maxDistance,
        signature,
        emittedByPlayer
    });
}

void AcousticSystem::spawnEchoTrace(
    sf::Vector2f hitPoint,
    sf::Vector2f wallDir,
    float intensity,
    SurfaceMaterial material
) {
    const MaterialProperties props = getMaterialProperties(material);
    WallEchoTrace& trace = m_echoes[m_echoCursor];
    m_echoCursor = (m_echoCursor + 1) % m_echoes.size();

    trace.hitPoint = hitPoint;
    trace.tangent = wallDir;
    trace.halfLength = 10.0f + 8.0f * std::min(intensity, 1.0f);
    trace.intensity = std::clamp(intensity, 0.1f, 1.0f);
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
        sf::Vector2f nextPos = ray.pos + ray.dir * stepDist;

        // Check nearest wall intersection along ray step
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
            spawnEchoTrace(bestHit, wallTangent, ray.intensity, hitWall->material);

            ray.reflections++;
            ray.intensity *= matProps.reflectivity;
            ray.tint = matProps.echoColor;

            if (ray.reflections > ray.maxReflections || ray.intensity < 0.08f) {
                ray.active = false;
                continue;
            }

            // Specular reflection: R = D - 2(D . N)N
            const float dn = dot2D(ray.dir, wallNormal);
            ray.dir = normalizeOrZero(ray.dir - 2.0f * dn * wallNormal);
            const float remainingStep = stepDist * (1.0f - bestT);
            ray.pos = bestHit + wallNormal * 0.5f + ray.dir * remainingStep;
        } else {
            ray.pos = nextPos;
        }

        ray.traveled += stepDist;
        const float distRatio = ray.traveled / std::max(1.0f, ray.maxDistance);
        if (distRatio >= 1.0f || ray.intensity < 0.05f) {
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
}

void AcousticSystem::render(sf::RenderTarget& target) const {
    m_waveVertices.clear();
    m_echoVertices.clear();

    // 1. Render lingering wall echo phosphor segments
    for (const auto& echo : m_echoes) {
        if (!echo.active) continue;
        const float lifeRatio = 1.0f - (echo.age / echo.lifetime);
        const float alphaF = std::clamp(lifeRatio * lifeRatio * echo.intensity, 0.0f, 1.0f);
        sf::Color c = getMaterialProperties(echo.material).echoColor;
        c.a = static_cast<sf::Uint8>(alphaF * 220.0f);

        const sf::Vector2f p1 = echo.hitPoint - echo.tangent * echo.halfLength;
        const sf::Vector2f p2 = echo.hitPoint + echo.tangent * echo.halfLength;
        m_echoVertices.append(sf::Vertex(p1, c));
        m_echoVertices.append(sf::Vertex(p2, c));
    }

    // 2. Render propagating wavefront rays as short glowing streaks
    for (const auto& ray : m_rays) {
        if (!ray.active) continue;
        const float fade = 1.0f - (ray.traveled / std::max(1.0f, ray.maxDistance));
        const float alphaF = std::clamp(fade * ray.intensity, 0.0f, 1.0f);

        sf::Color headColor = ray.tint;
        headColor.a = static_cast<sf::Uint8>(alphaF * 235.0f);

        sf::Color tailColor = ray.tint;
        tailColor.a = static_cast<sf::Uint8>(alphaF * 45.0f);

        const sf::Vector2f tailPos = ray.pos - ray.dir * 14.0f;
        m_waveVertices.append(sf::Vertex(tailPos, tailColor));
        m_waveVertices.append(sf::Vertex(ray.pos, headColor));
    }

    sf::RenderStates additiveStates(sf::BlendAdd);
    target.draw(m_echoVertices, additiveStates);
    target.draw(m_waveVertices, additiveStates);
}

} // namespace echo
