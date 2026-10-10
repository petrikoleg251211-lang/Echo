#include "AudioManager.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace echo {

namespace {
constexpr unsigned int kSampleRate = 44100;
constexpr float kTwoPi = 6.28318530717958647692f;

// Deterministic pseudo-random noise for footsteps & mechanical impacts
float hashNoise(std::uint32_t seed) noexcept {
    seed = (seed ^ 61u) ^ (seed >> 16u);
    seed *= 9u;
    seed = seed ^ (seed >> 4u);
    seed *= 0x27d4eb2du;
    seed = seed ^ (seed >> 15u);
    return (static_cast<float>(seed & 0xFFFFu) / 32768.0f) - 1.0f;
}

template <typename SampleFn>
sf::SoundBuffer makeBuffer(
    float durationSec,
    SampleFn&& sampleFn
) {
    const std::size_t count = static_cast<std::size_t>(kSampleRate * durationSec);
    std::vector<sf::Int16> pcm(count, 0);
    for (std::size_t i = 0; i < count; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float val = std::clamp(sampleFn(t, static_cast<std::uint32_t>(i)), -1.0f, 1.0f);
        pcm[i] = static_cast<sf::Int16>(val * 26000.0f);
    }
    sf::SoundBuffer buf;
    buf.loadFromSamples(pcm.data(), pcm.size(), 1, kSampleRate);
    return buf;
}
} // namespace

AudioManager::AudioManager() = default;

AudioManager::~AudioManager() {
    stopAmbientDrone();
}

void AudioManager::initialize() {
    if (m_initialized) return;
    buildProceduralBuffers();
    m_ambientSound.setBuffer(m_ambientBuffer);
    m_ambientSound.setLoop(true);
    m_ambientSound.setVolume(m_muted ? 0.0f : m_masterVolume * 0.28f);
    m_initialized = true;
}

void AudioManager::buildProceduralBuffers() {
    // 0. Directional Sonar Ping: crystalline harmonic sweep
    m_buffers[static_cast<std::size_t>(SoundEffectId::SonarPing)] = makeBuffer(
        0.48f,
        [](float t, std::uint32_t) {
            const float env = std::exp(-t * 6.8f);
            const float f = 580.0f - t * 240.0f;
            const float s = std::sin(kTwoPi * f * t)
                          + 0.35f * std::sin(kTwoPi * (f * 1.5f) * t)
                          + 0.15f * std::sin(kTwoPi * (f * 2.0f) * t);
            return s * env * 0.55f;
        }
    );

    // 1. Radial 360 Pulse: deep resonant subterranean gong + high shimmer
    m_buffers[static_cast<std::size_t>(SoundEffectId::RadialPulse)] = makeBuffer(
        0.85f,
        [](float t, std::uint32_t) {
            const float env = std::exp(-t * 3.8f);
            const float sub = std::sin(kTwoPi * (220.0f - t * 60.0f) * t);
            const float over = std::sin(kTwoPi * 440.0f * t) * std::exp(-t * 6.0f);
            const float shimmer = std::sin(kTwoPi * 880.0f * t) * std::exp(-t * 9.0f);
            return (0.6f * sub + 0.3f * over + 0.15f * shimmer) * env * 0.7f;
        }
    );

    // 2. Footstep Sneak: soft low-frequency muffled brush
    m_buffers[static_cast<std::size_t>(SoundEffectId::FootstepSneak)] = makeBuffer(
        0.11f,
        [](float t, std::uint32_t idx) {
            const float env = std::exp(-t * 32.0f);
            const float thump = std::sin(kTwoPi * 75.0f * t);
            const float grit = hashNoise(idx) * 0.25f;
            return (thump * 0.7f + grit) * env * 0.28f;
        }
    );

    // 3. Footstep Walk: distinct stone heel tap
    m_buffers[static_cast<std::size_t>(SoundEffectId::FootstepWalk)] = makeBuffer(
        0.15f,
        [](float t, std::uint32_t idx) {
            const float env = std::exp(-t * 24.0f);
            const float thump = std::sin(kTwoPi * (125.0f - t * 300.0f) * t);
            const float grit = hashNoise(idx + 1024u) * 0.4f;
            return (thump * 0.75f + grit) * env * 0.48f;
        }
    );

    // 4. Footstep Sprint: sharp metallic/stone impact
    m_buffers[static_cast<std::size_t>(SoundEffectId::FootstepSprint)] = makeBuffer(
        0.20f,
        [](float t, std::uint32_t idx) {
            const float env = std::exp(-t * 18.0f);
            const float body = std::sin(kTwoPi * (175.0f - t * 400.0f) * t);
            const float ring = std::sin(kTwoPi * 620.0f * t) * std::exp(-t * 28.0f);
            const float grit = hashNoise(idx + 4096u) * 0.45f;
            return (0.6f * body + 0.3f * ring + 0.3f * grit) * env * 0.72f;
        }
    );

    // 5. Decoy Stone Impact: sharp double clack with metallic ring
    m_buffers[static_cast<std::size_t>(SoundEffectId::DecoyImpact)] = makeBuffer(
        0.60f,
        [](float t, std::uint32_t idx) {
            const float hit1 = std::exp(-t * 14.0f) * std::sin(kTwoPi * 780.0f * t);
            const float hit2 = (t > 0.09f)
                ? std::exp(-(t - 0.09f) * 16.0f) * std::sin(kTwoPi * 920.0f * (t - 0.09f))
                : 0.0f;
            const float transient = hashNoise(idx) * std::exp(-t * 35.0f) * 0.5f;
            return (0.6f * hit1 + 0.5f * hit2 + transient) * 0.75f;
        }
    );

    // 6. Switch Clunk: heavy mechanical latch + harmonic gate unlock chord
    m_buffers[static_cast<std::size_t>(SoundEffectId::SwitchClunk)] = makeBuffer(
        0.72f,
        [](float t, std::uint32_t idx) {
            const float clunk = std::exp(-t * 22.0f) * (std::sin(kTwoPi * 140.0f * t) + hashNoise(idx) * 0.4f);
            const float chordEnv = (t > 0.12f) ? std::exp(-(t - 0.12f) * 4.5f) : 0.0f;
            const float chord = std::sin(kTwoPi * 330.0f * t) + 0.7f * std::sin(kTwoPi * 495.0f * t);
            return (clunk * 0.6f + chord * chordEnv * 0.45f) * 0.75f;
        }
    );

    // 7. Lore Pickup: warm two-tone discovery chime
    m_buffers[static_cast<std::size_t>(SoundEffectId::LorePickup)] = makeBuffer(
        0.55f,
        [](float t, std::uint32_t) {
            const float f = (t < 0.14f) ? 528.0f : 660.0f;
            const float env = std::exp(-t * 5.0f);
            return std::sin(kTwoPi * f * t) * env * 0.5f;
        }
    );

    // 8. Enemy Alert: tense dissonant metallic scrape
    m_buffers[static_cast<std::size_t>(SoundEffectId::EnemyAlert)] = makeBuffer(
        0.65f,
        [](float t, std::uint32_t) {
            const float env = std::exp(-t * 4.2f);
            const float s1 = std::sin(kTwoPi * (190.0f + t * 80.0f) * t);
            const float s2 = std::sin(kTwoPi * (268.0f - t * 40.0f) * t);
            return (s1 + s2) * 0.4f * env;
        }
    );

    // 9. Enemy Tremor: sub-bass pulse
    m_buffers[static_cast<std::size_t>(SoundEffectId::EnemyTremor)] = makeBuffer(
        0.32f,
        [](float t, std::uint32_t) {
            const float env = std::exp(-t * 10.5f);
            return std::sin(kTwoPi * (64.0f - t * 80.0f) * t) * env * 0.6f;
        }
    );

    // 10. UI Move
    m_buffers[static_cast<std::size_t>(SoundEffectId::UiMove)] = makeBuffer(
        0.07f,
        [](float t, std::uint32_t) {
            return std::sin(kTwoPi * 420.0f * t) * std::exp(-t * 40.0f) * 0.35f;
        }
    );

    // 11. UI Select
    m_buffers[static_cast<std::size_t>(SoundEffectId::UiSelect)] = makeBuffer(
        0.16f,
        [](float t, std::uint32_t) {
            const float f = (t < 0.05f) ? 480.0f : 720.0f;
            return std::sin(kTwoPi * f * t) * std::exp(-t * 16.0f) * 0.45f;
        }
    );

    // 12. Player Caught: dramatic low impact
    m_buffers[static_cast<std::size_t>(SoundEffectId::PlayerCaught)] = makeBuffer(
        0.95f,
        [](float t, std::uint32_t idx) {
            const float env = std::exp(-t * 3.2f);
            const float sub = std::sin(kTwoPi * (92.0f - t * 45.0f) * t);
            const float dissonance = std::sin(kTwoPi * 131.0f * t);
            const float burst = hashNoise(idx) * std::exp(-t * 12.0f) * 0.4f;
            return (0.55f * sub + 0.35f * dissonance + burst) * env * 0.8f;
        }
    );

    // 13. Victory Chord: resonant harmonic triad
    m_buffers[static_cast<std::size_t>(SoundEffectId::VictoryChord)] = makeBuffer(
        1.40f,
        [](float t, std::uint32_t) {
            const float env = std::exp(-t * 2.1f);
            const float c1 = std::sin(kTwoPi * 261.63f * t);
            const float c2 = std::sin(kTwoPi * 329.63f * t);
            const float c3 = std::sin(kTwoPi * 392.00f * t);
            const float c4 = std::sin(kTwoPi * 523.25f * t);
            return (c1 + c2 + c3 + 0.6f * c4) * 0.25f * env;
        }
    );

    // Seamless 4-second subterranean cavern drone (integer number of cycles for click-free loop)
    m_ambientBuffer = makeBuffer(
        4.0f,
        [](float t, std::uint32_t) {
            const float lfo = 0.75f + 0.25f * std::sin(kTwoPi * 0.25f * t);
            const float drone1 = std::sin(kTwoPi * 55.0f * t);
            const float drone2 = 0.5f * std::sin(kTwoPi * 82.5f * t);
            const float airy = 0.2f * std::sin(kTwoPi * 110.0f * t + std::sin(kTwoPi * 0.5f * t));
            return (drone1 + drone2 + airy) * 0.22f * lfo;
        }
    );
}

void AudioManager::playEffect(SoundEffectId id, float volumeScale, float pitch) {
    if (!m_initialized || m_muted) return;
    sf::Sound& voice = m_voices[m_voiceCursor];
    m_voiceCursor = (m_voiceCursor + 1) % m_voices.size();

    voice.stop();
    voice.setBuffer(m_buffers[static_cast<std::size_t>(id)]);
    voice.setPitch(pitch);
    voice.setVolume(std::clamp(m_masterVolume * volumeScale, 0.0f, 100.0f));
    voice.play();
}

void AudioManager::playWaveSignature(WaveSignature sig) {
    switch (sig) {
        case WaveSignature::FootstepSneak:
            playEffect(SoundEffectId::FootstepSneak, 0.55f, 0.96f);
            break;
        case WaveSignature::FootstepWalk:
            playEffect(SoundEffectId::FootstepWalk, 0.75f, 1.0f);
            break;
        case WaveSignature::FootstepSprint:
            playEffect(SoundEffectId::FootstepSprint, 0.95f, 1.04f);
            break;
        case WaveSignature::DirectionalPing:
            playEffect(SoundEffectId::SonarPing, 0.85f, 1.0f);
            break;
        case WaveSignature::RadialPulse:
            playEffect(SoundEffectId::RadialPulse, 1.0f, 1.0f);
            break;
        case WaveSignature::DecoyImpact:
            playEffect(SoundEffectId::DecoyImpact, 0.95f, 1.0f);
            break;
        case WaveSignature::MechanismClunk:
            playEffect(SoundEffectId::SwitchClunk, 0.95f, 1.0f);
            break;
        case WaveSignature::EnemyTremor:
            playEffect(SoundEffectId::EnemyTremor, 0.65f, 0.95f);
            break;
        case WaveSignature::AmbientDrip:
            playEffect(SoundEffectId::SonarPing, 0.18f, 1.65f);
            break;
    }
}

void AudioManager::setMasterVolume(float volume0to100) noexcept {
    m_masterVolume = std::clamp(volume0to100, 0.0f, 100.0f);
    if (!m_muted && m_initialized) {
        m_ambientSound.setVolume(m_masterVolume * 0.28f);
    }
}

void AudioManager::setMuted(bool muted) noexcept {
    m_muted = muted;
    if (!m_initialized) return;
    if (m_muted) {
        m_ambientSound.setVolume(0.0f);
    } else {
        m_ambientSound.setVolume(m_masterVolume * 0.28f);
    }
}

void AudioManager::startAmbientDrone() {
    if (!m_initialized) initialize();
    if (m_ambientSound.getStatus() != sf::Sound::Playing) {
        m_ambientSound.play();
    }
}

void AudioManager::stopAmbientDrone() {
    if (m_initialized && m_ambientSound.getStatus() == sf::Sound::Playing) {
        m_ambientSound.stop();
    }
}

void AudioManager::updateHeartbeat(float dangerLevel0to1, float dt) {
    if (dangerLevel0to1 <= 0.15f) {
        m_heartbeatTimer = 0.0f;
        return;
    }
    m_heartbeatTimer += dt;
    const float interval = std::clamp(1.05f - dangerLevel0to1 * 0.62f, 0.36f, 1.05f);
    if (m_heartbeatTimer >= interval) {
        m_heartbeatTimer = 0.0f;
        playEffect(SoundEffectId::EnemyTremor, 0.35f + dangerLevel0to1 * 0.5f, 1.15f);
    }
}

} // namespace echo
