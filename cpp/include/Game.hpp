#pragma once

#include "AcousticSystem.hpp"
#include "AssetManager.hpp"
#include "AudioManager.hpp"
#include "Config.hpp"
#include "EnemyAI.hpp"
#include "Labyrinth.hpp"
#include "Player.hpp"
#include "SaveSystem.hpp"
#include "UiRenderer.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
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

class Game {
public:
    explicit Game(const std::string& exeDir = "");

    int run();

    // Runs deterministic headless verification of all core C++ gameplay systems & 3D assets
    static int runSelfTest(const std::string& exeDir = "");

private:
    void handleEvents();
    void handleKeyPressed(sf::Keyboard::Key key);
    void handleMousePressed(sf::Mouse::Button button, sf::Vector2f worldPos);
    void handleMouseMoved(sf::Vector2f worldPos);

    void updateFixed(float dt);
    void updatePlaying(float dt);
    void updateMenuBackground(float dt);

    void render();
    void renderWorld();
    void renderSubterraneanProps();
    void renderInteractiveObjects();
    void renderVignetteAndDust();
    void renderHud();
    void renderMinimapOverlay();
    void renderMenuOverlay();
    void renderSettingsOverlay();
    void renderControlsOverlay();

    void enterRoom(int roomId, sf::Vector2f spawnPos);
    void startNewGame();
    void saveCheckpoint();
    bool loadCheckpoint();
    void tryInteract();
    void rebuildMenuButtons();
    void executeMenuSelection(std::size_t index);
    void updateLetterboxView(unsigned int winW, unsigned int winH);
    void toggleFullscreen();

    std::string m_exeDir;
    std::string m_saveFilePath;

    sf::RenderWindow m_window;
    sf::View m_worldView;

    AssetManager m_assets;
    AcousticSystem m_acoustics;
    AudioManager m_audio;
    UiRenderer m_ui;
    Labyrinth m_labyrinth;
    Player m_player;
    EnemyAI m_enemy;

    AppState m_state{AppState::MainMenu};
    AppState m_previousState{AppState::MainMenu};

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
    float m_cameraShakeTimer{0.0f};
    float m_shaleCrackCooldown{0.0f};
    bool m_showMinimap{false};
    bool m_isFullscreen{false};
};

} // namespace echo
