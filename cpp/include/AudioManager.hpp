#pragma once

#include "Config.hpp"
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/System/Vector2.hpp>
#include <array>
#include <cstdint>

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

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    void initialize();

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

    // Updates 4-layer adaptive soundtrack (Exploration, Tension, Pursuit, Sanctum) and heartbeat
    void updateAdaptiveAudio(
        float dt,
        float dangerLevel0to1,
        bool isPursuing,
        bool isSanctumCheckpoint,
        float playerStamina0to100
    );

private:
    void buildProceduralBuffers();
    void syncLoopLayerVolumes() noexcept;
    float nextPitchVariation(float spread = 0.05f) noexcept;

    std::array<sf::SoundBuffer, static_cast<std::size_t>(SoundEffectId::Count)> m_buffers;

    // 4 adaptive music & ambience loop buffers
    sf::SoundBuffer m_bufExploreDrone;
    sf::SoundBuffer m_bufTensionLayer;
    sf::SoundBuffer m_bufPursuitPulse;
    sf::SoundBuffer m_bufSanctumTheme;

    // 20-voice polyphonic pool for simultaneous spatial footsteps, echoes, and environment sounds
    std::array<sf::Sound, 20> m_voices;
    std::size_t m_voiceCursor{0};

    sf::Sound m_loopExplore;
    sf::Sound m_loopTension;
    sf::Sound m_loopPursuit;
    sf::Sound m_loopSanctum;

    float m_masterVolume{80.0f};
    float m_musicVolume{75.0f};
    float m_sfxVolume{85.0f};
    float m_ambientVolume{80.0f};

    float m_tensionBlend{0.0f};
    float m_pursuitBlend{0.0f};
    float m_sanctumBlend{0.0f};

    float m_heartbeatTimer{0.0f};
    float m_breathTimer{0.0f};
    float m_randomAmbienceTimer{0.0f};
    std::uint32_t m_rngState{0x9E3779B9u};

    sf::Vector2f m_listenerPos{640.0f, 400.0f};
    bool m_muted{false};
    bool m_initialized{false};
};

} // namespace echo
