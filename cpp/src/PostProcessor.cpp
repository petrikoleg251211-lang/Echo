#include "PostProcessor.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>

namespace echo {

const char* PostProcessor::embeddedFragmentShaderSource() noexcept {
    return R"GLSL(
#version 120
uniform sampler2D u_texture;
uniform vec2 u_resolution;
uniform float u_time;
uniform float u_bloomIntensity;
uniform float u_chromaticAberration;
uniform float u_crtScanline;
uniform vec2 u_shockwaveCenterUv;
uniform float u_shockwaveRadius;
uniform float u_shockwaveStrength;

void main() {
    vec2 uv = gl_TexCoord[0].xy;

    if (u_shockwaveStrength > 0.0005 && u_shockwaveRadius > 0.01) {
        vec2 aspect = vec2(u_resolution.x / max(1.0, u_resolution.y), 1.0);
        vec2 delta = (uv - u_shockwaveCenterUv) * aspect;
        float dist = length(delta);
        float band = abs(dist - u_shockwaveRadius);
        if (band < 0.065) {
            float wave = (1.0 - band / 0.065) * sin((dist - u_shockwaveRadius) * 95.0);
            vec2 dir = (dist > 0.0001) ? normalize(delta) : vec2(0.0, 0.0);
            uv += dir * wave * u_shockwaveStrength;
        }
    }

    uv = clamp(uv, vec2(0.001), vec2(0.999));

    vec2 centerOffset = uv - vec2(0.5, 0.5);
    float radialFactor = dot(centerOffset, centerOffset) * 2.2;
    vec2 caShift = centerOffset * u_chromaticAberration * (0.45 + radialFactor);

    float rChan = texture2D(u_texture, clamp(uv + caShift, vec2(0.001), vec2(0.999))).r;
    vec4 baseSample = texture2D(u_texture, uv);
    float gChan = baseSample.g;
    float bChan = texture2D(u_texture, clamp(uv - caShift, vec2(0.001), vec2(0.999))).b;
    vec3 color = vec3(rChan, gChan, bChan);

    if (u_bloomIntensity > 0.005) {
        vec2 texel = 2.25 / max(vec2(1.0, 1.0), u_resolution);
        vec3 bloomAccum = vec3(0.0);
        for (int ox = -1; ox <= 1; ++ox) {
            for (int oy = -1; oy <= 1; ++oy) {
                vec2 sampleUv = clamp(uv + vec2(float(ox), float(oy)) * texel, vec2(0.001), vec2(0.999));
                vec3 s = texture2D(u_texture, sampleUv).rgb;
                float luma = dot(s, vec3(0.299, 0.587, 0.114));
                float brightWeight = smoothstep(0.38, 0.88, luma);
                bloomAccum += s * brightWeight;
            }
        }
        bloomAccum /= 9.0;
        color += bloomAccum * u_bloomIntensity;
    }

    if (u_crtScanline > 0.001) {
        float scan = sin(uv.y * u_resolution.y * 3.14159265 + u_time * 1.5) * 0.5 + 0.5;
        color *= (1.0 - u_crtScanline * 0.65 * scan);
    }

    float vig = 1.0 - dot(centerOffset, centerOffset) * 0.48;
    color *= clamp(vig, 0.55, 1.0);

    gl_FragColor = vec4(clamp(color, vec3(0.0), vec3(1.0)), baseSample.a) * gl_Color;
}
)GLSL";
}

bool PostProcessor::initialize(unsigned int width, unsigned int height, const std::string& exeDir) {
    m_offscreenReady = m_sceneTexture.create(width, height);
    if (m_offscreenReady) {
        m_sceneTexture.setSmooth(true);
    }

    m_shaderCompiled = false;
    if (sf::Shader::isAvailable()) {
        std::error_code ec;
        std::filesystem::path shaderPath = exeDir.empty()
            ? std::filesystem::path("assets/shaders/acoustic_postfx.frag")
            : (std::filesystem::path(exeDir) / "assets" / "shaders" / "acoustic_postfx.frag");
        if (std::filesystem::exists(shaderPath, ec)) {
            m_shaderCompiled = m_shader.loadFromFile(shaderPath.string(), sf::Shader::Fragment);
        }
        if (!m_shaderCompiled) {
            m_shaderCompiled = m_shader.loadFromMemory(embeddedFragmentShaderSource(), sf::Shader::Fragment);
        }
    }
    return m_offscreenReady;
}

void PostProcessor::triggerShockwave(sf::Vector2f worldOrigin, float strength) noexcept {
    m_shockwaveWorldOrigin = worldOrigin;
    m_shockwaveInitialStrength = std::clamp(strength, 0.0f, 0.035f);
    m_shockwaveTimer = 0.55f;
}

void PostProcessor::update(float dt) noexcept {
    m_timeSec += dt;
    if (m_shockwaveTimer > 0.0f) {
        m_shockwaveTimer = std::max(0.0f, m_shockwaveTimer - dt);
    }
}

PostFxUniforms PostProcessor::computeUniforms(
    bool postFxEnabled,
    bool reduceFlashes,
    float noiseLevel0to100,
    float hitStopTimerSec,
    int activeRayCount,
    float shockwaveTimerSec,
    sf::Vector2f shockwaveOriginWorld
) noexcept {
    PostFxUniforms u{};
    if (!postFxEnabled) {
        u.bloomIntensity = 0.0f;
        u.chromaticAberration = 0.0f;
        u.crtScanlineStrength = 0.0f;
        u.shockwaveRadius = 0.0f;
        u.shockwaveStrength = 0.0f;
        return u;
    }

    const float rayBoost = std::clamp(static_cast<float>(activeRayCount) / 650.0f, 0.0f, 0.28f);
    u.bloomIntensity = (0.38f + rayBoost) * (reduceFlashes ? 0.45f : 1.0f);
    u.crtScanlineStrength = 0.10f;

    // Chromatic aberration ramps up in Loud/Scream tiers and spikes briefly on Hit-Stop impact
    float ca = 0.0f;
    if (noiseLevel0to100 > cfg::noise::kThresholdLoud) {
        const float screamT = std::clamp((noiseLevel0to100 - cfg::noise::kThresholdLoud) / 40.0f, 0.0f, 1.0f);
        ca += screamT * 0.0085f;
    }
    if (hitStopTimerSec > 0.0f) {
        ca += 0.0065f;
    }
    if (reduceFlashes) {
        ca = std::min(ca * 0.35f, 0.0025f);
    }
    u.chromaticAberration = std::clamp(ca, 0.0f, 0.015f);

    u.shockwaveCenterUv = {
        std::clamp(shockwaveOriginWorld.x / cfg::kWorldWidth, 0.0f, 1.0f),
        std::clamp(1.0f - (shockwaveOriginWorld.y / cfg::kWorldHeight), 0.0f, 1.0f)
    };
    if (shockwaveTimerSec > 0.0f) {
        const float progress = 1.0f - std::clamp(shockwaveTimerSec / 0.55f, 0.0f, 1.0f);
        u.shockwaveRadius = 0.04f + progress * 0.58f;
        u.shockwaveStrength = (1.0f - progress) * (reduceFlashes ? 0.004f : 0.012f);
    } else {
        u.shockwaveRadius = 0.0f;
        u.shockwaveStrength = 0.0f;
    }
    return u;
}

sf::RenderTarget& PostProcessor::beginSceneCapture(sf::RenderTarget& windowTarget, const sf::View& worldView) {
    if (m_offscreenReady) {
        m_sceneTexture.setView(worldView);
        m_sceneTexture.clear(sf::Color(3, 5, 11));
        return m_sceneTexture;
    }
    windowTarget.setView(worldView);
    return windowTarget;
}

void PostProcessor::presentToWindow(
    sf::RenderTarget& windowTarget,
    const sf::View& letterboxView,
    const PostFxUniforms& uniforms
) {
    if (!m_offscreenReady) return;

    m_sceneTexture.display();
    windowTarget.setView(letterboxView);

    sf::Sprite sceneSprite(m_sceneTexture.getTexture());
    if (m_shaderCompiled && (uniforms.bloomIntensity > 0.001f || uniforms.chromaticAberration > 0.0001f || uniforms.crtScanlineStrength > 0.001f)) {
        m_shader.setUniform("u_texture", sf::Shader::CurrentTexture);
        m_shader.setUniform("u_resolution", sf::Glsl::Vec2(cfg::kWorldWidth, cfg::kWorldHeight));
        m_shader.setUniform("u_time", m_timeSec);
        m_shader.setUniform("u_bloomIntensity", uniforms.bloomIntensity);
        m_shader.setUniform("u_chromaticAberration", uniforms.chromaticAberration);
        m_shader.setUniform("u_crtScanline", uniforms.crtScanlineStrength);
        m_shader.setUniform("u_shockwaveCenterUv", sf::Glsl::Vec2(uniforms.shockwaveCenterUv.x, uniforms.shockwaveCenterUv.y));
        m_shader.setUniform("u_shockwaveRadius", uniforms.shockwaveRadius);
        m_shader.setUniform("u_shockwaveStrength", uniforms.shockwaveStrength);
        windowTarget.draw(sceneSprite, &m_shader);
    } else {
        windowTarget.draw(sceneSprite);
        // CPU fallback scanline & chromatic border pulse when GPU fragment shaders are unavailable
        if (uniforms.chromaticAberration > 0.002f) {
            const auto alpha = static_cast<std::uint8_t>(std::clamp(uniforms.chromaticAberration * 4800.0f, 0.0f, 65.0f));
            sf::RectangleShape caRim({cfg::kWorldWidth, cfg::kWorldHeight});
            caRim.setFillColor(sf::Color::Transparent);
            caRim.setOutlineThickness(-5.0f);
            caRim.setOutlineColor(sf::Color(244, 63, 94, alpha));
            windowTarget.draw(caRim);
        }
    }
}

} // namespace echo
