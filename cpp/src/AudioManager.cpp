#include "AudioManager.hpp"
#include <SFML/Audio/Listener.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace echo {

namespace {
constexpr unsigned int kSampleRate = 44100;
constexpr float kTwoPi = 6.28318530717958647692f;
constexpr float kPi = 3.14159265358979323846f;

float hashNoise(std::uint32_t seed) noexcept {
    seed = (seed ^ 61u) ^ (seed >> 16u);
    seed *= 9u;
    seed = seed ^ (seed >> 4u);
    seed *= 0x27d4eb2du;
    seed = seed ^ (seed >> 15u);
    return (static_cast<float>(seed & 0xFFFFu) / 32768.0f) - 1.0f;
}

template <typename SampleFn>
sf::SoundBuffer makeBuffer(float durationSec, SampleFn&& sampleFn) {
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

float AudioManager::nextPitchVariation(float spread) noexcept {
    m_rngState ^= m_rngState << 13u;
    m_rngState ^= m_rngState >> 17u;
    m_rngState ^= m_rngState << 5u;
    const float norm = (static_cast<float>(m_rngState & 0xFFFFu) / 32768.0f) - 1.0f;
    return 1.0f + norm * spread;
}

void AudioManager::initialize(const std::string& assetRootDir) {
    if (m_initialized) return;
    buildProceduralBuffers();
    buildProceduralSoundtracks(assetRootDir);

    sf::Listener::setGlobalVolume(100.0f);
    sf::Listener::setUpVector(0.0f, 1.0f, 0.0f);
    updateListener({640.0f, 400.0f}, 0.0f);

    m_loopExplore.setBuffer(m_bufExploreDrone);
    m_loopExplore.setLoop(true);
    m_loopExplore.setRelativeToListener(true);

    m_loopTension.setBuffer(m_bufTensionLayer);
    m_loopTension.setLoop(true);
    m_loopTension.setRelativeToListener(true);

    m_loopPursuit.setBuffer(m_bufPursuitPulse);
    m_loopPursuit.setLoop(true);
    m_loopPursuit.setRelativeToListener(true);

    m_loopSanctum.setBuffer(m_bufSanctumTheme);
    m_loopSanctum.setLoop(true);
    m_loopSanctum.setRelativeToListener(true);

    m_loopBossChoir.setBuffer(m_bufBossChoir);
    m_loopBossChoir.setLoop(true);
    m_loopBossChoir.setRelativeToListener(true);

    m_initialized = true;
    syncLoopLayerVolumes();
}

void AudioManager::updateListener(sf::Vector2f playerPos, float facingAngleRad) noexcept {
    m_listenerPos = playerPos;
    sf::Listener::setPosition(playerPos.x, 0.0f, playerPos.y);
    sf::Listener::setDirection(std::cos(facingAngleRad), 0.0f, std::sin(facingAngleRad));
}

void AudioManager::buildProceduralBuffers() {
    // 0. SonarPing
    m_buffers[static_cast<std::size_t>(SoundEffectId::SonarPing)] = makeBuffer(
        0.48f,
        [](float t, std::uint32_t) {
            const float env = std::exp(-t * 6.8f);
            const float f = 590.0f - t * 240.0f;
            const float s = std::sin(kTwoPi * f * t)
                          + 0.35f * std::sin(kTwoPi * (f * 1.5f) * t)
                          + 0.15f * std::sin(kTwoPi * (f * 2.0f) * t);
            return s * env * 0.55f;
        }
    );

    // 1. RadialPulse
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

    // 2. FootstepSneak
    m_buffers[static_cast<std::size_t>(SoundEffectId::FootstepSneak)] = makeBuffer(
        0.11f,
        [](float t, std::uint32_t idx) {
            const float env = std::exp(-t * 32.0f);
            const float thump = std::sin(kTwoPi * 72.0f * t);
            const float grit = hashNoise(idx) * 0.22f;
            return (thump * 0.7f + grit) * env * 0.28f;
        }
    );

    // 3. FootstepStone (crisp slate heel tap)
    m_buffers[static_cast<std::size_t>(SoundEffectId::FootstepStone)] = makeBuffer(
        0.15f,
        [](float t, std::uint32_t idx) {
            const float env = std::exp(-t * 25.0f);
            const float thump = std::sin(kTwoPi * (135.0f - t * 320.0f) * t);
            const float grit = hashNoise(idx + 1024u) * 0.38f;
            return (thump * 0.75f + grit) * env * 0.50f;
        }
    );

    // 4. FootstepConcrete (gritty dry thud)
    m_buffers[static_cast<std::size_t>(SoundEffectId::FootstepConcrete)] = makeBuffer(
        0.16f,
        [](float t, std::uint32_t idx) {
            const float env = std::exp(-t * 22.0f);
            const float body = std::sin(kTwoPi * (105.0f - t * 240.0f) * t);
            const float scrape = hashNoise(idx + 2048u) * 0.52f * std::exp(-t * 28.0f);
            return (body * 0.65f + scrape) * env * 0.52f;
        }
    );

    // 5. FootstepMetal (hollow steel clang + high ring)
    m_buffers[static_cast<std::size_t>(SoundEffectId::FootstepMetal)] = makeBuffer(
        0.22f,
        [](float t, std::uint32_t idx) {
            const float env = std::exp(-t * 16.0f);
            const float clang = std::sin(kTwoPi * 310.0f * t) + 0.6f * std::sin(kTwoPi * 745.0f * t);
            const float transient = hashNoise(idx + 3072u) * std::exp(-t * 40.0f) * 0.45f;
            return (clang * 0.45f + transient) * env * 0.60f;
        }
    );

    // 6. FootstepWater (filtered splash & ripple)
    m_buffers[static_cast<std::size_t>(SoundEffectId::FootstepWater)] = makeBuffer(
        0.26f,
        [](float t, std::uint32_t idx) {
            const float env = std::sin(std::min(1.0f, t * 22.0f) * 1.570796f) * std::exp(-t * 11.5f);
            const float bubble = std::sin(kTwoPi * (280.0f + std::sin(t * 95.0f) * 120.0f) * t);
            const float splash = hashNoise(idx + 5120u) * 0.65f;
            return (bubble * 0.45f + splash * 0.55f) * env * 0.55f;
        }
    );

    // 7. FootstepMoss (soft organic brush)
    m_buffers[static_cast<std::size_t>(SoundEffectId::FootstepMoss)] = makeBuffer(
        0.14f,
        [](float t, std::uint32_t idx) {
            const float env = std::exp(-t * 20.0f);
            const float low = std::sin(kTwoPi * 68.0f * t);
            const float rustle = hashNoise(idx + 6144u) * 0.35f;
            return (low * 0.6f + rustle * 0.4f) * env * 0.36f;
        }
    );

    // 8. FootstepSprint
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

    // 9. PlayerBreath (exhausted respirator inhale/exhale)
    m_buffers[static_cast<std::size_t>(SoundEffectId::PlayerBreath)] = makeBuffer(
        0.55f,
        [](float t, std::uint32_t idx) {
            const float env = std::sin(std::min(1.0f, t / 0.55f) * kPi);
            const float air = hashNoise(idx + 8192u) * 0.25f;
            const float resonator = std::sin(kTwoPi * 195.0f * t) * 0.12f;
            return (air + resonator) * env * 0.42f;
        }
    );

    // 10. DecoyImpact
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

    // 11. SwitchClunk
    m_buffers[static_cast<std::size_t>(SoundEffectId::SwitchClunk)] = makeBuffer(
        0.72f,
        [](float t, std::uint32_t idx) {
            const float clunk = std::exp(-t * 22.0f) * (std::sin(kTwoPi * 140.0f * t) + hashNoise(idx) * 0.4f);
            const float chordEnv = (t > 0.12f) ? std::exp(-(t - 0.12f) * 4.5f) : 0.0f;
            const float chord = std::sin(kTwoPi * 330.0f * t) + 0.7f * std::sin(kTwoPi * 495.0f * t);
            return (clunk * 0.6f + chord * chordEnv * 0.45f) * 0.75f;
        }
    );

    // 12. GateSlideOpen (heavy pneumatic bulkhead retraction)
    m_buffers[static_cast<std::size_t>(SoundEffectId::GateSlideOpen)] = makeBuffer(
        0.92f,
        [](float t, std::uint32_t idx) {
            const float env = std::sin(std::min(1.0f, t / 0.92f) * kPi);
            const float rumble = std::sin(kTwoPi * (68.0f + t * 22.0f) * t);
            const float hiss = hashNoise(idx + 9000u) * std::exp(-t * 4.0f) * 0.35f;
            const float lock = (t > 0.75f) ? std::sin(kTwoPi * 240.0f * (t - 0.75f)) * std::exp(-(t - 0.75f) * 18.0f) : 0.0f;
            return (rumble * 0.55f + hiss + lock * 0.5f) * env * 0.78f;
        }
    );

    // 13. ResonatorHum
    m_buffers[static_cast<std::size_t>(SoundEffectId::ResonatorHum)] = makeBuffer(
        0.88f,
        [](float t, std::uint32_t) {
            const float env = std::exp(-t * 3.2f);
            const float c1 = std::sin(kTwoPi * 440.0f * t);
            const float c2 = std::sin(kTwoPi * 660.0f * t);
            const float c3 = std::sin(kTwoPi * 880.0f * t);
            return (c1 * 0.5f + c2 * 0.35f + c3 * 0.25f) * env * 0.70f;
        }
    );

    // 14. ShaleCrack
    m_buffers[static_cast<std::size_t>(SoundEffectId::ShaleCrack)] = makeBuffer(
        0.45f,
        [](float t, std::uint32_t idx) {
            const float snap1 = std::exp(-t * 28.0f) * (std::sin(kTwoPi * 520.0f * t) + hashNoise(idx) * 0.8f);
            const float snap2 = (t > 0.06f)
                ? std::exp(-(t - 0.06f) * 20.0f) * (std::sin(kTwoPi * 390.0f * (t - 0.06f)) + hashNoise(idx + 77u) * 0.6f)
                : 0.0f;
            return (snap1 * 0.6f + snap2 * 0.5f) * 0.80f;
        }
    );

    // 15. LorePickup
    m_buffers[static_cast<std::size_t>(SoundEffectId::LorePickup)] = makeBuffer(
        0.55f,
        [](float t, std::uint32_t) {
            const float f = (t < 0.14f) ? 528.0f : 660.0f;
            const float env = std::exp(-t * 5.0f);
            return std::sin(kTwoPi * f * t) * env * 0.5f;
        }
    );

    // 16. EnemyAlert
    m_buffers[static_cast<std::size_t>(SoundEffectId::EnemyAlert)] = makeBuffer(
        0.65f,
        [](float t, std::uint32_t) {
            const float env = std::exp(-t * 4.2f);
            const float s1 = std::sin(kTwoPi * (190.0f + t * 80.0f) * t);
            const float s2 = std::sin(kTwoPi * (268.0f - t * 40.0f) * t);
            return (s1 + s2) * 0.4f * env;
        }
    );

    // 17. EnemyTremor
    m_buffers[static_cast<std::size_t>(SoundEffectId::EnemyTremor)] = makeBuffer(
        0.32f,
        [](float t, std::uint32_t) {
            const float env = std::exp(-t * 10.5f);
            return std::sin(kTwoPi * (64.0f - t * 80.0f) * t) * env * 0.65f;
        }
    );

    // 18. EnemySearchClick (echolocation chitin click)
    m_buffers[static_cast<std::size_t>(SoundEffectId::EnemySearchClick)] = makeBuffer(
        0.18f,
        [](float t, std::uint32_t idx) {
            const float click1 = std::exp(-t * 45.0f) * std::sin(kTwoPi * 1150.0f * t);
            const float click2 = (t > 0.055f)
                ? std::exp(-(t - 0.055f) * 45.0f) * std::sin(kTwoPi * 980.0f * (t - 0.055f))
                : 0.0f;
            return (click1 + click2 + hashNoise(idx) * 0.15f * std::exp(-t * 40.0f)) * 0.55f;
        }
    );

    // 19. EnemyHuntRoar (predatory low-mid growl)
    m_buffers[static_cast<std::size_t>(SoundEffectId::EnemyHuntRoar)] = makeBuffer(
        0.75f,
        [](float t, std::uint32_t idx) {
            const float env = std::exp(-t * 3.5f);
            const float growl = std::sin(kTwoPi * (112.0f - t * 35.0f) * t)
                              + 0.6f * std::sin(kTwoPi * (168.0f + std::sin(t * 45.0f) * 18.0f) * t);
            const float rasp = hashNoise(idx + 3333u) * 0.35f;
            return (growl * 0.5f + rasp) * env * 0.72f;
        }
    );

    // 20. PlayerCaught
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

    // 21. DistantMetalClang (environmental ambience)
    m_buffers[static_cast<std::size_t>(SoundEffectId::DistantMetalClang)] = makeBuffer(
        1.10f,
        [](float t, std::uint32_t) {
            const float env = std::exp(-t * 3.4f);
            const float m1 = std::sin(kTwoPi * 184.0f * t);
            const float m2 = std::sin(kTwoPi * 297.0f * t) * std::exp(-t * 5.0f);
            const float m3 = std::sin(kTwoPi * 512.0f * t) * std::exp(-t * 8.0f);
            return (m1 * 0.5f + m2 * 0.35f + m3 * 0.2f) * env * 0.40f;
        }
    );

    // 22. StructuralCreak
    m_buffers[static_cast<std::size_t>(SoundEffectId::StructuralCreak)] = makeBuffer(
        0.85f,
        [](float t, std::uint32_t) {
            const float env = std::sin(std::min(1.0f, t / 0.85f) * kPi);
            const float f = 95.0f + std::sin(t * 14.0f) * 18.0f;
            return (std::sin(kTwoPi * f * t) + 0.4f * std::sin(kTwoPi * (f * 2.05f) * t)) * env * 0.32f;
        }
    );

    // 23. WaterDrip
    m_buffers[static_cast<std::size_t>(SoundEffectId::WaterDrip)] = makeBuffer(
        0.16f,
        [](float t, std::uint32_t) {
            const float env = std::exp(-t * 26.0f);
            const float pitch = 680.0f + t * 1400.0f;
            return std::sin(kTwoPi * pitch * t) * env * 0.42f;
        }
    );

    // 24. SteamHiss
    m_buffers[static_cast<std::size_t>(SoundEffectId::SteamHiss)] = makeBuffer(
        0.52f,
        [](float t, std::uint32_t idx) {
            const float env = std::sin(std::min(1.0f, t / 0.52f) * kPi) * std::exp(-t * 3.5f);
            return hashNoise(idx + 12345u) * env * 0.36f;
        }
    );

    // 25. ElectricalBuzz
    m_buffers[static_cast<std::size_t>(SoundEffectId::ElectricalBuzz)] = makeBuffer(
        0.35f,
        [](float t, std::uint32_t idx) {
            const float env = std::exp(-t * 8.0f);
            const float hum = std::sin(kTwoPi * 120.0f * t) + 0.5f * std::sin(kTwoPi * 240.0f * t);
            const float spark = hashNoise(idx + 999u) * 0.35f;
            return (hum * 0.4f + spark) * env * 0.32f;
        }
    );

    // 26. UiMove
    m_buffers[static_cast<std::size_t>(SoundEffectId::UiMove)] = makeBuffer(
        0.07f,
        [](float t, std::uint32_t) {
            return std::sin(kTwoPi * 420.0f * t) * std::exp(-t * 40.0f) * 0.35f;
        }
    );

    // 27. UiSelect
    m_buffers[static_cast<std::size_t>(SoundEffectId::UiSelect)] = makeBuffer(
        0.16f,
        [](float t, std::uint32_t) {
            const float f = (t < 0.05f) ? 480.0f : 720.0f;
            return std::sin(kTwoPi * f * t) * std::exp(-t * 16.0f) * 0.45f;
        }
    );

    // 28. VictoryChord
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
}

void AudioManager::playEffect(SoundEffectId id, float volumeScale, float pitch) {
    if (!m_initialized || m_muted) return;
    sf::Sound& voice = m_voices[m_voiceCursor];
    m_voiceCursor = (m_voiceCursor + 1) % m_voices.size();

    voice.stop();
    voice.setBuffer(m_buffers[static_cast<std::size_t>(id)]);
    voice.setRelativeToListener(true);
    voice.setPosition(0.0f, 0.0f, 0.0f);
    voice.setPitch(pitch);
    const float gain = (m_masterVolume / 100.0f) * (m_sfxVolume / 100.0f) * volumeScale * 100.0f;
    voice.setVolume(std::clamp(gain, 0.0f, 100.0f));
    voice.play();
}

void AudioManager::playSpatial(
    SoundEffectId id,
    sf::Vector2f sourcePos,
    float occlusionFactor,
    float volumeScale,
    float basePitch
) {
    if (!m_initialized || m_muted) return;
    sf::Sound& voice = m_voices[m_voiceCursor];
    m_voiceCursor = (m_voiceCursor + 1) % m_voices.size();

    const float dx = sourcePos.x - m_listenerPos.x;
    const float dy = sourcePos.y - m_listenerPos.y;
    const float dist = std::sqrt(dx * dx + dy * dy);
    const float distAtten = std::clamp(1.0f - (dist / 920.0f), 0.08f, 1.0f);
    const float occ = std::clamp(occlusionFactor, 0.10f, 1.0f);

    voice.stop();
    voice.setBuffer(m_buffers[static_cast<std::size_t>(id)]);
    voice.setRelativeToListener(false);
    voice.setPosition(sourcePos.x, 0.0f, sourcePos.y);
    voice.setMinDistance(140.0f);
    voice.setAttenuation(1.8f);
    const float occludedPitch = basePitch * (0.88f + 0.12f * occ) * nextPitchVariation(0.04f);
    voice.setPitch(std::clamp(occludedPitch, 0.5f, 2.0f));

    const float gain = (m_masterVolume / 100.0f) * (m_sfxVolume / 100.0f) * volumeScale * distAtten * occ * 100.0f;
    voice.setVolume(std::clamp(gain, 0.0f, 100.0f));
    voice.play();
}

void AudioManager::playMaterialFootstep(SurfaceMaterial mat, bool isSneak, bool isSprint, sf::Vector2f pos) {
    if (isSneak) {
        playSpatial(SoundEffectId::FootstepSneak, pos, 1.0f, 0.55f, nextPitchVariation(0.05f));
        return;
    }
    if (isSprint) {
        playSpatial(SoundEffectId::FootstepSprint, pos, 1.0f, 0.95f, nextPitchVariation(0.06f));
        return;
    }

    SoundEffectId id = SoundEffectId::FootstepStone;
    float matPitch = 1.0f;
    switch (mat) {
        case SurfaceMaterial::Stone:         id = SoundEffectId::FootstepStone;    matPitch = 1.00f; break;
        case SurfaceMaterial::Concrete:      id = SoundEffectId::FootstepConcrete; matPitch = 0.94f; break;
        case SurfaceMaterial::Metal:         id = SoundEffectId::FootstepMetal;    matPitch = 1.08f; break;
        case SurfaceMaterial::Water:         id = SoundEffectId::FootstepWater;    matPitch = 1.02f; break;
        case SurfaceMaterial::DampMoss:      id = SoundEffectId::FootstepMoss;     matPitch = 0.88f; break;
        case SurfaceMaterial::ResonanceDoor: id = SoundEffectId::FootstepMetal;    matPitch = 0.84f; break;
        case SurfaceMaterial::Glass:         id = SoundEffectId::FootstepMetal;    matPitch = 1.38f; break;
        case SurfaceMaterial::EchoCrystal:   id = SoundEffectId::ResonatorHum;     matPitch = 1.46f; break;
        case SurfaceMaterial::Count:         id = SoundEffectId::FootstepStone;    matPitch = 1.00f; break;
    }
    playSpatial(id, pos, 1.0f, 0.78f, matPitch * nextPitchVariation(0.05f));
}

void AudioManager::playWaveSignature(WaveSignature sig) {
    switch (sig) {
        case WaveSignature::FootstepSneak:
            playEffect(SoundEffectId::FootstepSneak, 0.55f, nextPitchVariation(0.05f));
            break;
        case WaveSignature::FootstepWalk:
            playEffect(SoundEffectId::FootstepStone, 0.75f, nextPitchVariation(0.05f));
            break;
        case WaveSignature::FootstepSprint:
            playEffect(SoundEffectId::FootstepSprint, 0.95f, nextPitchVariation(0.05f));
            break;
        case WaveSignature::DirectionalPing:
            playEffect(SoundEffectId::SonarPing, 0.85f, nextPitchVariation(0.02f));
            break;
        case WaveSignature::RadialPulse:
            playEffect(SoundEffectId::RadialPulse, 1.0f, 1.0f);
            break;
        case WaveSignature::DecoyImpact:
            playEffect(SoundEffectId::DecoyImpact, 0.95f, nextPitchVariation(0.04f));
            break;
        case WaveSignature::MechanismClunk:
            playEffect(SoundEffectId::SwitchClunk, 0.95f, 1.0f);
            playEffect(SoundEffectId::GateSlideOpen, 0.85f, 1.0f);
            break;
        case WaveSignature::EnemyTremor:
            playEffect(SoundEffectId::EnemyTremor, 0.65f, nextPitchVariation(0.04f));
            break;
        case WaveSignature::AmbientDrip:
            playEffect(SoundEffectId::WaterDrip, 0.35f, nextPitchVariation(0.08f));
            break;
        case WaveSignature::ShaleCrack:
            playEffect(SoundEffectId::ShaleCrack, 0.92f, nextPitchVariation(0.05f));
            break;
        case WaveSignature::ResonatorHum:
            playEffect(SoundEffectId::ResonatorHum, 0.95f, 1.0f);
            playEffect(SoundEffectId::GateSlideOpen, 0.80f, 1.05f);
            break;
        case WaveSignature::WhistleBeam:
            playEffect(SoundEffectId::SonarPing, 0.60f, 1.55f);
            break;
        case WaveSignature::StompWave:
            playEffect(SoundEffectId::RadialPulse, 1.0f, 0.68f);
            playEffect(SoundEffectId::EnemyTremor, 0.90f, 0.82f);
            break;
        case WaveSignature::EchoTrapPulse:
            playEffect(SoundEffectId::ResonatorHum, 0.90f, 1.32f);
            playEffect(SoundEffectId::SonarPing, 0.80f, 1.25f);
            break;
        case WaveSignature::GlassShatterWave:
            playEffect(SoundEffectId::ShaleCrack, 1.0f, 1.48f);
            playEffect(SoundEffectId::DistantMetalClang, 0.85f, 1.65f);
            break;
    }
}

void AudioManager::syncLoopLayerVolumes() noexcept {
    if (!m_initialized) return;
    if (m_muted) {
        m_loopExplore.setVolume(0.0f);
        m_loopTension.setVolume(0.0f);
        m_loopPursuit.setVolume(0.0f);
        m_loopSanctum.setVolume(0.0f);
        m_loopBossChoir.setVolume(0.0f);
        return;
    }

    const float master = m_masterVolume / 100.0f;
    const float music = m_musicVolume / 100.0f;
    const float amb = m_ambientVolume / 100.0f;

    const float exploreDuck = 1.0f - 0.55f * std::max(m_bossBlend, m_pursuitBlend);
    m_loopExplore.setVolume(std::clamp(master * amb * 42.0f * exploreDuck, 0.0f, 100.0f));
    m_loopTension.setVolume(std::clamp(master * music * m_tensionBlend * 56.0f, 0.0f, 100.0f));
    m_loopPursuit.setVolume(std::clamp(master * music * m_pursuitBlend * 72.0f, 0.0f, 100.0f));
    m_loopSanctum.setVolume(std::clamp(master * music * m_sanctumBlend * 50.0f, 0.0f, 100.0f));
    m_loopBossChoir.setVolume(std::clamp(master * music * m_bossBlend * 78.0f, 0.0f, 100.0f));

    float bossPitch = 1.0f;
    if (m_currentBossPhase == 2) bossPitch = 1.059463f;
    else if (m_currentBossPhase >= 3) bossPitch = 1.122462f;
    m_loopBossChoir.setPitch(bossPitch);
}

void AudioManager::setMasterVolume(float volume0to100) noexcept {
    m_masterVolume = std::clamp(volume0to100, 0.0f, 100.0f);
    syncLoopLayerVolumes();
}

void AudioManager::setMusicVolume(float volume0to100) noexcept {
    m_musicVolume = std::clamp(volume0to100, 0.0f, 100.0f);
    syncLoopLayerVolumes();
}

void AudioManager::setSfxVolume(float volume0to100) noexcept {
    m_sfxVolume = std::clamp(volume0to100, 0.0f, 100.0f);
}

void AudioManager::setAmbientVolume(float volume0to100) noexcept {
    m_ambientVolume = std::clamp(volume0to100, 0.0f, 100.0f);
    syncLoopLayerVolumes();
}

void AudioManager::setMuted(bool muted) noexcept {
    m_muted = muted;
    syncLoopLayerVolumes();
}

void AudioManager::startAmbientDrone() {
    if (!m_initialized) initialize();
    if (m_loopExplore.getStatus() != sf::Sound::Playing) m_loopExplore.play();
    if (m_loopTension.getStatus() != sf::Sound::Playing) m_loopTension.play();
    if (m_loopPursuit.getStatus() != sf::Sound::Playing) m_loopPursuit.play();
    if (m_loopSanctum.getStatus() != sf::Sound::Playing) m_loopSanctum.play();
    if (m_loopBossChoir.getStatus() != sf::Sound::Playing) m_loopBossChoir.play();
    syncLoopLayerVolumes();
}

void AudioManager::stopAmbientDrone() {
    if (!m_initialized) return;
    m_loopExplore.stop();
    m_loopTension.stop();
    m_loopPursuit.stop();
    m_loopSanctum.stop();
    m_loopBossChoir.stop();
}

AdaptiveStemWeights AudioManager::computeAdaptiveStemWeights(
    float dangerLevel0to1,
    bool isPursuing,
    bool isSanctumCheckpoint,
    bool isBossChamber,
    int bossPhase,
    BiomeId biome
) noexcept {
    AdaptiveStemWeights w{};
    const float clampedDanger = std::clamp(dangerLevel0to1, 0.0f, 1.0f);
    w.bossChoir = isBossChamber ? 1.0f : 0.0f;
    w.tensionStrings = std::clamp((clampedDanger - 0.12f) / 0.75f, 0.0f, 1.0f) * (isBossChamber ? 0.45f : 1.0f);
    w.pursuitPercussion = isPursuing ? (isBossChamber ? 0.55f : 1.0f) : 0.0f;
    w.sanctumPad = (!isBossChamber && isSanctumCheckpoint && !isPursuing && clampedDanger < 0.25f) ? 1.0f : 0.0f;
    w.subDrone = std::clamp(1.0f - 0.55f * std::max(w.bossChoir, w.pursuitPercussion), 0.25f, 1.0f);

    // Biome-specific Sonar Arp & DSP parameters (Low-Pass Cutoff Hz & Reverb Wet Mix)
    switch (biome) {
        case BiomeId::DrownedGalleries:
            w.sonarArp = 0.48f;
            w.lowPassCutoffHz = 2800.0f; // Underwater hydro-acoustic muffling
            w.reverbWetMix = 0.42f;
            break;
        case BiomeId::CrystalVaults:
            w.sonarArp = 0.82f;
            w.lowPassCutoffHz = 16500.0f; // Crisp crystalline high-harmonic ring
            w.reverbWetMix = 0.58f;
            break;
        case BiomeId::IronFoundry:
            w.sonarArp = 0.60f;
            w.lowPassCutoffHz = 9500.0f;  // Metallic industrial resonance
            w.reverbWetMix = 0.46f;
            break;
        case BiomeId::LivingMycelium:
        default:
            w.sonarArp = 0.30f;
            w.lowPassCutoffHz = 1850.0f;  // Heavy organic spore absorption
            w.reverbWetMix = 0.14f;
            break;
    }

    if (bossPhase >= 3) {
        w.lowPassCutoffHz *= 0.75f;
        w.reverbWetMix = std::min(0.75f, w.reverbWetMix + 0.18f);
    }
    return w;
}

void AudioManager::applyLowPassFilter(
    std::vector<sf::Int16>& pcmSamples,
    float cutoffHz,
    unsigned int sampleRate
) noexcept {
    if (pcmSamples.empty() || sampleRate == 0) return;
    const float clampedCutoff = std::clamp(cutoffHz, 80.0f, static_cast<float>(sampleRate) * 0.48f);
    const float rc = 1.0f / (2.0f * kPi * clampedCutoff);
    const float dt = 1.0f / static_cast<float>(sampleRate);
    const float alpha = std::clamp(dt / (rc + dt), 0.005f, 1.0f);

    float prev = static_cast<float>(pcmSamples[0]);
    for (std::size_t i = 0; i < pcmSamples.size(); ++i) {
        const float x = static_cast<float>(pcmSamples[i]);
        prev = prev + alpha * (x - prev);
        pcmSamples[i] = static_cast<sf::Int16>(std::clamp(prev, -32767.0f, 32767.0f));
    }
}

void AudioManager::applyReverbTail(
    std::vector<sf::Int16>& pcmSamples,
    float delaySec,
    float feedbackGain,
    float wetMix,
    unsigned int sampleRate
) {
    if (pcmSamples.empty() || sampleRate == 0) return;
    const std::size_t delaySamples = std::max<std::size_t>(
        64,
        static_cast<std::size_t>(std::clamp(delaySec, 0.01f, 0.85f) * static_cast<float>(sampleRate))
    );
    const float fb = std::clamp(feedbackGain, 0.0f, 0.88f);
    const float wet = std::clamp(wetMix, 0.0f, 0.85f);
    const float dry = 1.0f - wet * 0.45f;

    std::vector<float> combLine(delaySamples, 0.0f);
    std::size_t cursor = 0;

    for (std::size_t i = 0; i < pcmSamples.size(); ++i) {
        const float inSample = static_cast<float>(pcmSamples[i]);
        const float delayed = combLine[cursor];
        const float nextVal = inSample + delayed * fb;
        combLine[cursor] = nextVal;
        cursor = (cursor + 1) % delaySamples;

        const float mixed = inSample * dry + delayed * wet;
        pcmSamples[i] = static_cast<sf::Int16>(std::clamp(mixed, -32767.0f, 32767.0f));
    }
}

void AudioManager::updateAdaptiveAudio(
    float dt,
    float dangerLevel0to1,
    bool isPursuing,
    bool isSanctumCheckpoint,
    float playerStamina0to100,
    bool isBossChamber,
    int bossPhase
) {
    m_currentBossPhase = std::clamp(bossPhase, 1, 3);
    const AdaptiveStemWeights targets = computeAdaptiveStemWeights(
        dangerLevel0to1,
        isPursuing,
        isSanctumCheckpoint,
        isBossChamber,
        m_currentBossPhase,
        m_currentBiome
    );

    const float blendRate = std::min(1.0f, dt * 2.2f);
    m_tensionBlend += (targets.tensionStrings - m_tensionBlend) * blendRate;
    m_pursuitBlend += (targets.pursuitPercussion - m_pursuitBlend) * blendRate;
    m_sanctumBlend += (targets.sanctumPad - m_sanctumBlend) * blendRate;
    m_bossBlend    += (targets.bossChoir - m_bossBlend) * blendRate;

    m_stemWeights.subDrone = targets.subDrone;
    m_stemWeights.sonarArp += (targets.sonarArp - m_stemWeights.sonarArp) * blendRate;
    m_stemWeights.tensionStrings = m_tensionBlend;
    m_stemWeights.pursuitPercussion = m_pursuitBlend;
    m_stemWeights.sanctumPad = m_sanctumBlend;
    m_stemWeights.bossChoir = m_bossBlend;
    m_stemWeights.lowPassCutoffHz = targets.lowPassCutoffHz;
    m_stemWeights.reverbWetMix = targets.reverbWetMix;

    syncLoopLayerVolumes();

    if (dangerLevel0to1 > 0.15f) {
        m_heartbeatTimer += dt;
        const float interval = std::clamp(1.05f - dangerLevel0to1 * 0.62f, 0.36f, 1.05f);
        if (m_heartbeatTimer >= interval) {
            m_heartbeatTimer = 0.0f;
            playEffect(SoundEffectId::EnemyTremor, 0.30f + dangerLevel0to1 * 0.45f, 1.15f);
        }
    } else {
        m_heartbeatTimer = 0.0f;
    }

    if (playerStamina0to100 < 38.0f) {
        m_breathTimer += dt;
        if (m_breathTimer >= 1.45f) {
            m_breathTimer = 0.0f;
            const float intensity = std::clamp((38.0f - playerStamina0to100) / 38.0f, 0.2f, 0.85f);
            playEffect(SoundEffectId::PlayerBreath, intensity, nextPitchVariation(0.04f));
        }
    } else {
        m_breathTimer = 0.0f;
    }

    m_randomAmbienceTimer += dt;
    if (m_randomAmbienceTimer >= 7.5f) {
        m_randomAmbienceTimer = 0.0f;
        const float r = nextPitchVariation(1.0f);
        const sf::Vector2f offset{
            std::cos(r * kPi) * 320.0f,
            std::sin(r * kPi) * 240.0f
        };
        const SoundEffectId ambId = (r > 1.0f)
            ? SoundEffectId::DistantMetalClang
            : SoundEffectId::StructuralCreak;
        playSpatial(
            ambId,
            m_listenerPos + offset,
            0.65f,
            (m_ambientVolume / 100.0f) * 0.45f,
            nextPitchVariation(0.08f)
        );
    }
}

} // namespace echo
