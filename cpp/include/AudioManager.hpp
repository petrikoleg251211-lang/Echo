#pragma once

#include "Config.hpp"
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/System/Vector2.hpp>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace echo {

enum class SoundEffectId : std::uint8_t {
    SonarPing = 0,
    RadialPulse = 1,
    FootstepSneak = 2,
    FootstepStone = 3,
    FootstepConcrete = 4,
    FootstepMetal = 5,
    FootstepWater = 6,
    FootstepMoss = 7,
    FootstepSprint = 8,
    PlayerBreath = 9,
    DecoyImpact = 10,
    SwitchClunk = 11,
    GateSlideOpen = 12,
    ResonatorHum = 13,
    ShaleCrack = 14,
    LorePickup = 15,
    EnemyAlert = 16,
    EnemyTremor = 17,
    EnemySearchClick = 18,
    EnemyHuntRoar = 19,
    PlayerCaught = 20,
    DistantMetalClang = 21,
    StructuralCreak = 22,
    WaterDrip = 23,
    SteamHiss = 24,
    ElectricalBuzz = 25,
    UiMove = 26,
    UiSelect = 27,
    VictoryChord = 28,
    Count = 29
};

enum class SoundtrackLayerId : std::uint8_t {
    SubterraneanSurvey = 0, // Exploration dark ambient & cavern bells (D Dorian, 16s)
    FracturedEchoes = 1,    // Suspicion & tension tritone strings + chitin clicks (16s)
    BlindPursuit = 2,       // Active hunt 120 BPM industrial darksynth & sub-kick (8s)
    ResonanceSanctum = 3,   // Checkpoint & harmonic crystal sanctuary pad + harp (16s)
    ChoirSovereign = 4,     // Boss arena gothic organ, abyssal vowel choir & war drums (16s)
    Count = 5
};

using MusicTrackId = SoundtrackLayerId;
constexpr std::size_t kMusicTrackCount = static_cast<std::size_t>(SoundtrackLayerId::Count);

struct AdaptiveStemWeights {
    float subDrone{0.85f};          // Stem 1: Deep Subterranean Sub-Drone
    float sonarArp{0.40f};          // Stem 2: Biome-tuned Harmonic Sonar Arpeggio
    float tensionStrings{0.0f};     // Stem 3: Suspicion Tritone Strings & Chitin Clicks
    float pursuitPercussion{0.0f};  // Stem 4: 120 BPM Industrial Pursuit Percussion
    float sanctumPad{0.0f};         // Stem 5: Harmonic Sanctuary Pad & Harp
    float bossChoir{0.0f};          // Stem 6: Abyssal Boss Organ & Choir
    float lowPassCutoffHz{14000.0f}; // Biome & occlusion Low-Pass Filter cutoff frequency
    float reverbWetMix{0.28f};      // Biome acoustic reverb wet mix [0..1]
};

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    void initialize(const std::string& assetRootDir = "");

    // Updates OpenAL 3D listener position and orientation from player world state
    void updateListener(sf::Vector2f playerPos, float facingAngleRad) noexcept;

    // Non-positional UI / 2D sound effect
    void playEffect(SoundEffectId id, float volumeScale = 1.0f, float pitch = 1.0f);

    // 3D OpenAL spatial sound effect with distance attenuation & wall occlusion factor [0.08..1.0]
    void playSpatial(
        SoundEffectId id,
        sf::Vector2f sourcePos,
        float occlusionFactor = 1.0f,
        float volumeScale = 1.0f,
        float basePitch = 1.0f
    );

    // Plays material-specific player footstep with organic pitch variation
    void playMaterialFootstep(SurfaceMaterial mat, bool isSneak, bool isSprint, sf::Vector2f pos);

    void playWaveSignature(WaveSignature sig);

    // 4-Channel Mixer Controls [0..100]
    void setMasterVolume(float volume0to100) noexcept;
    float masterVolume() const noexcept { return m_masterVolume; }

    void setMusicVolume(float volume0to100) noexcept;
    float musicVolume() const noexcept { return m_musicVolume; }

    void setSfxVolume(float volume0to100) noexcept;
    float sfxVolume() const noexcept { return m_sfxVolume; }

    void setAmbientVolume(float volume0to100) noexcept;
    float ambientVolume() const noexcept { return m_ambientVolume; }

    void setMuted(bool muted) noexcept;
    bool isMuted() const noexcept { return m_muted; }

    void startAmbientDrone();
    void stopAmbientDrone();

    // Stage 3: Computes target 6-stem adaptive weights & biome DSP parameters
    static AdaptiveStemWeights computeAdaptiveStemWeights(
        float dangerLevel0to1,
        bool isPursuing,
        bool isSanctumCheckpoint,
        bool isBossChamber = false,
        int bossPhase = 1,
        BiomeId biome = BiomeId::DrownedGalleries
    ) noexcept;

    // Stage 3: Procedural DSP One-Pole Low-Pass Filter & Schroeder Comb Reverb
    static void applyLowPassFilter(
        std::vector<sf::Int16>& pcmSamples,
        float cutoffHz,
        unsigned int sampleRate = 44100
    ) noexcept;

    static void applyReverbTail(
        std::vector<sf::Int16>& pcmSamples,
        float delaySec,
        float feedbackGain,
        float wetMix,
        unsigned int sampleRate = 44100
    );

    void setCurrentBiome(BiomeId biome) noexcept { m_currentBiome = biome; }
    BiomeId currentBiome() const noexcept { return m_currentBiome; }
    const AdaptiveStemWeights& activeStemWeights() const noexcept { return m_stemWeights; }

    // Updates 6-stem adaptive soundtrack crossfades and physiological heartbeat/breathing
    void updateAdaptiveAudio(
        float dt,
        float dangerLevel0to1,
        bool isPursuing,
        bool isSanctumCheckpoint,
        float playerStamina0to100,
        bool isBossChamber = false,
        int bossPhase = 1
    );

    // Exports all 5 synthesized OST tracks as standard 16-bit 44.1 kHz PCM .wav files
    bool exportSoundtrackWavFiles(const std::string& outputDir);

    // Accessors for deterministic self-testing & OST metadata
    const sf::SoundBuffer& soundtrackBuffer(SoundtrackLayerId layer) const noexcept;
    static const char* trackFilename(SoundtrackLayerId layer) noexcept;
    static const char* trackTitle(SoundtrackLayerId layer) noexcept;
    float tensionBlend() const noexcept { return m_tensionBlend; }
    float pursuitBlend() const noexcept { return m_pursuitBlend; }
    float sanctumBlend() const noexcept { return m_sanctumBlend; }
    float bossBlend() const noexcept { return m_bossBlend; }

private:
    void buildProceduralBuffers();
    void buildProceduralSoundtracks(const std::string& assetRootDir);
    void syncLoopLayerVolumes() noexcept;
    float nextPitchVariation(float spread = 0.05f) noexcept;

    std::array<sf::SoundBuffer, static_cast<std::size_t>(SoundEffectId::Count)> m_buffers;

    // 5 adaptive music & ambience loop buffers
    sf::SoundBuffer m_bufExploreDrone;
    sf::SoundBuffer m_bufTensionLayer;
    sf::SoundBuffer m_bufPursuitPulse;
    sf::SoundBuffer m_bufSanctumTheme;
    sf::SoundBuffer m_bufBossChoir;

    // 20-voice polyphonic pool for simultaneous spatial footsteps, echoes, and environment sounds
    std::array<sf::Sound, 20> m_voices;
    std::size_t m_voiceCursor{0};

    sf::Sound m_loopExplore;
    sf::Sound m_loopTension;
    sf::Sound m_loopPursuit;
    sf::Sound m_loopSanctum;
    sf::Sound m_loopBossChoir;

    float m_masterVolume{80.0f};
    float m_musicVolume{75.0f};
    float m_sfxVolume{85.0f};
    float m_ambientVolume{80.0f};

    float m_tensionBlend{0.0f};
    float m_pursuitBlend{0.0f};
    float m_sanctumBlend{0.0f};
    float m_bossBlend{0.0f};
    int m_currentBossPhase{1};
    BiomeId m_currentBiome{BiomeId::DrownedGalleries};
    AdaptiveStemWeights m_stemWeights{};

    float m_heartbeatTimer{0.0f};
    float m_breathTimer{0.0f};
    float m_randomAmbienceTimer{0.0f};
    std::uint32_t m_rngState{0x9E3779B9u};

    sf::Vector2f m_listenerPos{640.0f, 400.0f};
    bool m_muted{false};
    bool m_initialized{false};
};

} // namespace echo
