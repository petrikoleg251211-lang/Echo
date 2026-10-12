#pragma once

#include "Config.hpp"
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <string>

namespace echo {

struct PostFxUniforms {
    float bloomIntensity{0.42f};
    float chromaticAberration{0.0f};
    float crtScanlineStrength{0.11f};
    sf::Vector2f shockwaveCenterUv{0.5f, 0.5f};
    float shockwaveRadius{0.0f};
    float shockwaveStrength{0.0f};
};

class PostProcessor {
public:
    PostProcessor() = default;

    // Initializes off-screen RenderTexture (1280x800) and compiles GLSL shader from disk or embedded source
    bool initialize(unsigned int width, unsigned int height, const std::string& exeDir = "");

    // Triggers an expanding UV refraction ripple from a world position (e.g., Radial Pulse or Stomp)
    void triggerShockwave(sf::Vector2f worldOrigin, float strength = 0.014f) noexcept;

    void update(float dt) noexcept;

    // Deterministic calculation of shader uniforms for GPU & headless self-test verification
    static PostFxUniforms computeUniforms(
        bool postFxEnabled,
        bool reduceFlashes,
        float noiseLevel0to100,
        float hitStopTimerSec,
        int activeRayCount,
        float shockwaveTimerSec,
        sf::Vector2f shockwaveOriginWorld
    ) noexcept;

    // Begins off-screen scene capture if RenderTexture is ready; returns fallback target otherwise
    sf::RenderTarget& beginSceneCapture(sf::RenderTarget& windowTarget, const sf::View& worldView);

    // Presents captured scene to windowTarget using GLSL shader (or CPU scanline/aberration fallback)
    void presentToWindow(
        sf::RenderTarget& windowTarget,
        const sf::View& letterboxView,
        const PostFxUniforms& uniforms
    );

    void setEnabled(bool enabled) noexcept { m_enabled = enabled; }
    bool isEnabled() const noexcept { return m_enabled; }
    bool isOffscreenReady() const noexcept { return m_offscreenReady; }
    bool isShaderCompiled() const noexcept { return m_shaderCompiled; }
    static const char* embeddedFragmentShaderSource() noexcept;

private:
    sf::RenderTexture m_sceneTexture;
    sf::Shader m_shader;
    bool m_enabled{true};
    bool m_offscreenReady{false};
    bool m_shaderCompiled{false};
    float m_timeSec{0.0f};
    float m_shockwaveTimer{0.0f};
    float m_shockwaveInitialStrength{0.012f};
    sf::Vector2f m_shockwaveWorldOrigin{640.0f, 400.0f};
};

} // namespace echo
