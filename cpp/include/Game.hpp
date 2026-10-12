#pragma once

#include "AcousticSystem.hpp"
#include "AssetManager.hpp"
#include "AudioManager.hpp"
#include "Config.hpp"
#include "EnemyAI.hpp"
#include "Labyrinth.hpp"
#include "Player.hpp"
#include "PostProcessor.hpp"
#include "SaveSystem.hpp"
#include "UiRenderer.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

namespace echo {

struct DustParticle {
    sf::Vector2f pos;
    sf::Vector2f vel;
    float size{1.5f};
};

struct MenuButton {
    std::string label;
    sf::Vector2f center;
    sf::Vector2f size{360.0f, 44.0f};
    bool enabled{true};
};

struct BalanceReportSummary {
    DifficultyMode difficulty{DifficultyMode::Surveyor};
    int totalRuns{1000};
    int victories{0};
    float winRatePct{0.0f};
    float avgDurationSec{0.0f};
    float avgRoomsCleared{0.0f};
    float avgGhostClears{0.0f};
    std::array<int, static_cast<std::size_t>(RelicId::Count)> relicPicks{};
    std::array<int, static_cast<std::size_t>(RelicId::Count)> relicWins{};
};

struct BenchmarkReportSummary {
    int totalFrames{1200};
    float totalElapsedMs{0.0f};
    float avgFrameMs{0.0f};
    float p99FrameMs{0.0f};
    float estimatedFps{0.0f};
    std::size_t peakActiveRays{0};
    int crystalSplitsTriggered{0};
};

class Game {
public:
    explicit Game(const std::string& exeDir = "");

    int run();

    // Runs deterministic headless verification of all C++17 roguelike, noise, acoustic, AI, save, and 3D asset systems
    static int runSelfTest(const std::string& exeDir = "");

    // Stage 1 Verification Suite (Hit-Stop, Telegraphing, 4 Difficulties, 3-Room Tutorial, Save V6, Balance Report)
    static int runStage1SelfTest(const std::string& exeDir = "");

    // Stage 2 Verification Suite (4 Biomes, EchoCrystal Ray Split, Leech, Banshee, TuningForkConstruct, Room 16 Conductor)
    static int runStage2SelfTest(const std::string& exeDir = "");

    // Stage 3 Verification Suite (GLSL PostFX Shaders, 6-Stem Adaptive Audio + Low-Pass/Reverb DSP, 4 Colorblind Modes, Closed Captions)
    static int runStage3SelfTest(const std::string& exeDir = "");

    // Stage 4 Verification Suite (Procedural 64x64 Window Icon, --benchmark 1200-Frame Stress Test, Release Asset & CPack Manifest Integrity)
    static int runStage4SelfTest(const std::string& exeDir = "");

    // Stage 4: Runs a deterministic high-load acoustic/AI/shader benchmark and prints frame-time telemetry
    static BenchmarkReportSummary runBenchmark(
        std::ostream& out,
        int numFrames = 1200,
        const std::string& exeDir = ""
    );

    // Stage 1.2: Simulates 1000 headless bot runs and prints balance statistics & Top-5 relics
    static BalanceReportSummary simulateBalanceRuns(
        int numRuns = 1000,
        DifficultyMode difficulty = DifficultyMode::Surveyor,
        std::uint32_t baseSeed = 0xEC402026u
    );
    static int runBalanceReport(
        std::ostream& out,
        int numRuns = 1000,
        DifficultyMode difficulty = DifficultyMode::Surveyor
    );

    void setDifficulty(DifficultyMode mode) noexcept;
    DifficultyMode difficulty() const noexcept { return m_difficulty; }
    void cycleDifficulty(int delta = 1) noexcept;

    bool isTutorialDone() const noexcept { return m_tutorialDone; }
    void setTutorialDone(bool done) noexcept { m_tutorialDone = done; }
    int tutorialRoomStage() const noexcept { return m_tutorialRoomStage; }
    float hitStopTimer() const noexcept { return m_hitStopTimer; }
    NoiseAction lastNoiseAction() const noexcept { return m_lastNoiseAction; }
    float lastNoiseDelta() const noexcept { return m_lastNoiseDelta; }
    bool isFoundryHammerMaskingActive() const noexcept { return m_foundryHammerMaskTimer > 0.0f; }

    // Stage 3: Accessibility & Post-Processing Controls
    void setColorblindMode(ColorblindMode mode) noexcept;
    ColorblindMode colorblindMode() const noexcept { return m_colorblindMode; }
    void cycleColorblindMode() noexcept;

    void setSubtitlesEnabled(bool enabled) noexcept { m_subtitlesEnabled = enabled; }
    bool areSubtitlesEnabled() const noexcept { return m_subtitlesEnabled; }

    void setShakeIntensityScale(float scale0to1) noexcept { m_shakeIntensityScale = std::clamp(scale0to1, 0.0f, 1.0f); }
    float shakeIntensityScale() const noexcept { return m_shakeIntensityScale; }

    void setReduceFlashes(bool reduce) noexcept { m_reduceFlashes = reduce; }
    bool reduceFlashes() const noexcept { return m_reduceFlashes; }

    void setPostFxEnabled(bool enabled) noexcept {
        m_postFxEnabled = enabled;
        m_postProcessor.setEnabled(enabled);
    }
    bool isPostFxEnabled() const noexcept { return m_postFxEnabled; }

    void pushSoundCaption(
        const std::string& description,
        sf::Vector2f origin,
        sf::Color color = sf::Color(125, 211, 252),
        float durationSec = 2.8f
    );
    const std::vector<SoundCaptionEvent>& activeSoundCaptions() const noexcept { return m_soundCaptions; }

private:
    // Input & Menu Navigation (GameInput.cpp)
    void handleEvents();
    void handleKeyPressed(sf::Keyboard::Key key);
    void handleMousePressed(sf::Mouse::Button button, sf::Vector2f worldPos);
    void handleMouseMoved(sf::Vector2f worldPos);
    void tryInteract();
    void rebuildMenuButtons();
    void executeMenuSelection(std::size_t index);

    // Core Simulation & Noise System (Game.cpp & GameNoise.cpp)
    void updateFixed(float dt);
    void updatePlaying(float dt);
    void updateMenuBackground(float dt);
    void updateTutorialProgression();
    void triggerCameraShake(float amplitude, float durationSec = 0.32f) noexcept;
    void registerNoiseAction(NoiseAction action, float dt = 1.0f, sf::Vector2f stimulusOrigin = {640.0f, 400.0f});
    void updateNoiseSystem(float dt);
    void executePlayerStomp();

    // World Rendering (GameWorldRender.cpp)
    void render();
    void renderWorld(sf::RenderTarget& target);
    void renderSubterraneanProps(sf::RenderTarget& target);
    void renderInteractiveObjects(sf::RenderTarget& target);
    void renderVignetteAndDust(sf::RenderTarget& target);

    // HUD, Wave Noise Meter, Threat Arrows, Tutorial, Closed Captions & Overlays (GameHudRender.cpp)
    void renderHud();
    void renderNoiseWaveMeter(sf::Vector2f topLeft, sf::Vector2f size);
    void renderThreatDirectionIndicators();
    void renderTutorialBanner();
    void renderSoundCaptions();
    void renderCornerMinimap();
    void renderMinimapOverlay();
    void renderMenuOverlay();
    void renderSettingsOverlay();
    void renderControlsOverlay();
    void renderCodexOverlay();

    // Room Transitions & Persistence (GameRun.cpp)
    void enterRoom(int roomId, sf::Vector2f spawnPos, bool preservePlayerBuild = true);
    void spawnWaveForCurrentRoom(int waveIndex);
    void startNewGame(std::uint32_t customSeed = 0);
    void saveCheckpoint();
    bool loadCheckpoint();
    void updateLetterboxView(unsigned int winW, unsigned int winH);
    void toggleFullscreen();

    std::string m_exeDir;
    std::string m_saveFilePath;

    std::unique_ptr<sf::RenderWindow> m_window;
    sf::View m_worldView;

    AssetManager m_assets;
    AcousticSystem m_acoustics;
    std::unique_ptr<AudioManager> m_audio;
    PostProcessor m_postProcessor;
    UiRenderer m_ui;
    Labyrinth m_labyrinth;
    Player m_player;
    std::vector<EnemyAI> m_enemies;
    int m_currentWaveIndex{0};

    AppState m_state{AppState::MainMenu};
    AppState m_previousState{AppState::MainMenu};
    DifficultyMode m_difficulty{DifficultyMode::Surveyor};

    std::vector<MenuButton> m_menuButtons;
    std::size_t m_selectedMenuIndex{0};

    std::vector<DustParticle> m_particles;

    sf::Vector2f m_mouseWorldPos{640.0f, 400.0f};
    float m_ambientVisibility{0.03f};
    float m_runTimeSec{0.0f};
    float m_statusBannerTimer{0.0f};
    std::string m_statusBannerText;
    std::string m_activeLoreTitle;
    std::string m_activeLoreBody;
    float m_loreDisplayTimer{0.0f};
    float m_menuPulseTimer{0.0f};

    // Stage 1.1: Hit-stop & Exponential Screen Shake
    float m_hitStopTimer{0.0f};
    float m_cameraShakeTimer{0.0f};
    float m_cameraShakeElapsed{0.0f};
    float m_cameraShakeAmplitude{0.0f};
    float m_shaleCrackCooldown{0.0f};

    // Block 1 & Stage 1.5: Sound as Risk Resource — Noise Meter, Silence System & Last Noise Cause HUD
    float m_noiseLevel{0.0f};
    float m_roomPeakNoise{0.0f};
    float m_noiseSilenceGraceTimer{0.0f};
    float m_noiseIntrusionCooldown{0.0f};
    int m_ghostClearsThisRun{0};
    NoiseAction m_lastNoiseAction{NoiseAction::SilenceDecay};
    float m_lastNoiseDelta{0.0f};
    sf::Vector2f m_lastNoiseOrigin{640.0f, 400.0f};
    float m_lastNoiseSourceTimer{0.0f};

    // Stage 1.4: Interactive 3-Room Tutorial State & Persistence Flag
    bool m_tutorialDone{false};
    int m_tutorialRoomStage{0}; // 0 = Room 1 (Move+Sneak), 1 = Room 2 (Sonar Ping/Whistle), 2 = Room 3 (Noise & EchoTrap/Decoy), 3 = Complete
    bool m_tutStepMoved{false};
    bool m_tutStepSneaked{false};
    bool m_tutStepPinged{false};
    bool m_tutStepTrappedOrDecoy{false};

    // Block 5 & Stage 2: Dynamic Room Events, Biome Timers, Boss Arena Phases & Persistent Audio-Lore Codex
    float m_echoStormTimer{0.0f};
    float m_bossDissonanceTimer{0.0f};
    float m_foundryHammerTimer{0.0f};
    float m_foundryHammerMaskTimer{0.0f};
    float m_leechSprintDetachTimer{0.0f};
    int m_metaTotalGhostClears{0};
    std::vector<int> m_discoveredCodexIds;

    // Stage 3: Accessibility & PostFX State
    ColorblindMode m_colorblindMode{ColorblindMode::Normal};
    bool m_subtitlesEnabled{true};
    float m_shakeIntensityScale{1.0f};
    bool m_reduceFlashes{false};
    bool m_postFxEnabled{true};
    float m_postFxShockwaveTimer{0.0f};
    sf::Vector2f m_postFxShockwaveOrigin{640.0f, 400.0f};
    std::vector<SoundCaptionEvent> m_soundCaptions;

    // Roguelike Run & Meta-Progression Counters
    int m_enemiesDisruptedThisRun{0};
    int m_metaTotalRuns{1};
    int m_metaTotalVictories{0};
    int m_metaBossesDefeated{0};

    bool m_showMinimap{false};
    bool m_isFullscreen{false};
};

} // namespace echo
