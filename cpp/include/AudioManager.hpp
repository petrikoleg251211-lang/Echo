#pragma once

#include "Config.hpp"
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <array>
#include <vector>

namespace echo {

enum class SoundEffectId : std::uint8_t {
    SonarPing = 0,
    RadialPulse = 1,
    FootstepSneak = 2,
    FootstepWalk = 3,
    FootstepSprint = 4,
    DecoyImpact = 5,
    SwitchClunk = 6,
    LorePickup = 7,
    EnemyAlert = 8,
    EnemyTremor = 9,
    UiMove = 10,
    UiSelect = 11,
    PlayerCaught = 12,
    VictoryChord = 13,
    Count = 14
};

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    void initialize();

    void playEffect(SoundEffectId id, float volumeScale = 1.0f, float pitch = 1.0f);
    void playWaveSignature(WaveSignature sig);

    void setMasterVolume(float volume0to100) noexcept;
    float masterVolume() const noexcept { return m_masterVolume; }

    void setMuted(bool muted) noexcept;
    bool isMuted() const noexcept { return m_muted; }

    void startAmbientDrone();
    void stopAmbientDrone();
    void updateHeartbeat(float dangerLevel0to1, float dt);

private:
    void buildProceduralBuffers();

    std::array<sf::SoundBuffer, static_cast<std::size_t>(SoundEffectId::Count)> m_buffers;
    sf::SoundBuffer m_ambientBuffer;

    // Polyphonic voice pool so rapid footsteps and echoes never cut each other off
    std::array<sf::Sound, 16> m_voices;
    std::size_t m_voiceCursor{0};

    sf::Sound m_ambientSound;
    float m_masterVolume{80.0f};
    float m_heartbeatTimer{0.0f};
    bool m_muted{false};
    bool m_initialized{false};
};

} // namespace echo
