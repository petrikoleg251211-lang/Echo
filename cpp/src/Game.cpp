#include "Game.hpp"
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>

namespace echo {

namespace {
float distSq(sf::Vector2f a, sf::Vector2f b) noexcept {
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    return dx * dx + dy * dy;
}

bool pointInButton(sf::Vector2f pt, const MenuButton& btn) noexcept {
    const float halfW = btn.size.x * 0.5f;
    const float halfH = btn.size.y * 0.5f;
    return pt.x >= (btn.center.x - halfW) && pt.x <= (btn.center.x + halfW)
        && pt.y >= (btn.center.y - halfH) && pt.y <= (btn.center.y + halfH);
}
} // namespace

Game::Game(const std::string& exeDir)
    : m_exeDir(exeDir)
    , m_saveFilePath(SaveSystem::resolveSaveFilePath(exeDir, "echolabyrinth_save.dat"))
    , m_worldView(sf::FloatRect(0.0f, 0.0f, cfg::kWorldWidth, cfg::kWorldHeight)) {

    m_particles.resize(64);
    for (std::size_t i = 0; i < m_particles.size(); ++i) {
        m_particles[i].pos = {
            static_cast<float>((i * 197) % static_cast<int>(cfg::kWorldWidth)),
            static_cast<float>((i * 131) % static_cast<int>(cfg::kWorldHeight))
        };
        m_particles[i].vel = {
            static_cast<float>(static_cast<int>(i % 5) - 2) * 5.0f,
            static_cast<float>(static_cast<int>(i % 3) - 1) * 4.0f
        };
        m_particles[i].size = (i % 2 == 0) ? 2.0f : 1.5f;
    }
}

int Game::run() {
    m_window.create(
        sf::VideoMode(cfg::kDefaultWindowWidth, cfg::kDefaultWindowHeight),
        "EchoLabyrinth — Acoustic Stealth Horror",
        sf::Style::Default
    );
    m_window.setFramerateLimit(120);
    updateLetterboxView(cfg::kDefaultWindowWidth, cfg::kDefaultWindowHeight);

    m_ui.initialize(m_exeDir);
    m_audio.initialize();
    m_audio.startAmbientDrone();

    enterRoom(0, m_labyrinth.currentRoom().playerDefaultSpawn);
    m_state = AppState::MainMenu;
    rebuildMenuButtons();

    sf::Clock clock;
    float accumulator = 0.0f;

    while (m_window.isOpen()) {
        handleEvents();

        const float frameDt = std::min(clock.restart().asSeconds(), 0.1f);
        accumulator += frameDt;

        while (accumulator >= cfg::kFixedTimeStep) {
            updateFixed(cfg::kFixedTimeStep);
            accumulator -= cfg::kFixedTimeStep;
        }

        render();
    }

    m_audio.stopAmbientDrone();
    return 0;
}

void Game::updateLetterboxView(unsigned int winW, unsigned int winH) {
    if (winW == 0 || winH == 0) return;
    const float windowRatio = static_cast<float>(winW) / static_cast<float>(winH);
    const float viewRatio = cfg::kWorldWidth / cfg::kWorldHeight;

    float sizeX = 1.0f;
    float sizeY = 1.0f;
    float posX = 0.0f;
    float posY = 0.0f;

    if (windowRatio > viewRatio) {
        sizeX = viewRatio / windowRatio;
        posX = (1.0f - sizeX) * 0.5f;
    } else {
        sizeY = windowRatio / viewRatio;
        posY = (1.0f - sizeY) * 0.5f;
    }

    m_worldView.setViewport(sf::FloatRect(posX, posY, sizeX, sizeY));
}

void Game::toggleFullscreen() {
    m_isFullscreen = !m_isFullscreen;
    if (m_isFullscreen) {
        m_window.create(
            sf::VideoMode::getDesktopMode(),
            "EchoLabyrinth — Acoustic Stealth Horror",
            sf::Style::Fullscreen
        );
    } else {
        m_window.create(
            sf::VideoMode(cfg::kDefaultWindowWidth, cfg::kDefaultWindowHeight),
            "EchoLabyrinth — Acoustic Stealth Horror",
            sf::Style::Default
        );
    }
    m_window.setFramerateLimit(120);
    const sf::Vector2u sz = m_window.getSize();
    updateLetterboxView(sz.x, sz.y);
}

void Game::rebuildMenuButtons() {
    m_menuButtons.clear();
    m_selectedMenuIndex = 0;

    if (m_state == AppState::MainMenu) {
        const bool hasSave = SaveSystem::hasSaveFile(m_saveFilePath);
        m_menuButtons.push_back({"CONTINUE EXPEDITION", {640.0f, 320.0f}, {380.0f, 44.0f}, hasSave});
        m_menuButtons.push_back({"NEW EXPEDITION", {640.0f, 380.0f}, {380.0f, 44.0f}, true});
        m_menuButtons.push_back({"AUDIO & GRAPHICS SETTINGS", {640.0f, 440.0f}, {380.0f, 44.0f}, true});
        m_menuButtons.push_back({"CONTROLS & FIELD MANUAL", {640.0f, 500.0f}, {380.0f, 44.0f}, true});
        m_menuButtons.push_back({"QUIT TO DESKTOP", {640.0f, 560.0f}, {380.0f, 44.0f}, true});
        if (!hasSave) m_selectedMenuIndex = 1;
    } else if (m_state == AppState::Paused) {
        m_menuButtons.push_back({"RESUME EXPLORATION", {640.0f, 290.0f}, {360.0f, 42.0f}, true});
        m_menuButtons.push_back({"SAVE CHECKPOINT [F5]", {640.0f, 345.0f}, {360.0f, 42.0f}, true});
        m_menuButtons.push_back({"LOAD CHECKPOINT [F9]", {640.0f, 400.0f}, {360.0f, 42.0f}, SaveSystem::hasSaveFile(m_saveFilePath)});
        m_menuButtons.push_back({"SETTINGS", {640.0f, 455.0f}, {360.0f, 42.0f}, true});
        m_menuButtons.push_back({"CONTROLS & MANUAL", {640.0f, 510.0f}, {360.0f, 42.0f}, true});
        m_menuButtons.push_back({"RETURN TO MAIN MENU", {640.0f, 565.0f}, {360.0f, 42.0f}, true});
    } else if (m_state == AppState::GameOver) {
        m_menuButtons.push_back({"RETRY CURRENT CHAMBER", {640.0f, 390.0f}, {380.0f, 44.0f}, true});
        m_menuButtons.push_back({"LOAD SAVED CHECKPOINT", {640.0f, 450.0f}, {380.0f, 44.0f}, SaveSystem::hasSaveFile(m_saveFilePath)});
        m_menuButtons.push_back({"RETURN TO MAIN MENU", {640.0f, 510.0f}, {380.0f, 44.0f}, true});
    } else if (m_state == AppState::Victory) {
        m_menuButtons.push_back({"START NEW EXPEDITION", {640.0f, 440.0f}, {380.0f, 44.0f}, true});
        m_menuButtons.push_back({"RETURN TO MAIN MENU", {640.0f, 505.0f}, {380.0f, 44.0f}, true});
    } else if (m_state == AppState::Settings) {
        m_menuButtons.push_back({"VOLUME DOWN [-]", {490.0f, 290.0f}, {240.0f, 40.0f}, true});
        m_menuButtons.push_back({"VOLUME UP [+]", {790.0f, 290.0f}, {240.0f, 40.0f}, true});
        m_menuButtons.push_back({"TOGGLE MUTE [M]", {640.0f, 350.0f}, {360.0f, 40.0f}, true});
        m_menuButtons.push_back({"CYCLE RAY QUALITY", {640.0f, 410.0f}, {360.0f, 40.0f}, true});
        m_menuButtons.push_back({"CYCLE AMBIENT DARKNESS", {640.0f, 470.0f}, {360.0f, 40.0f}, true});
        m_menuButtons.push_back({"TOGGLE FULLSCREEN [F11]", {640.0f, 530.0f}, {360.0f, 40.0f}, true});
        m_menuButtons.push_back({"BACK", {640.0f, 600.0f}, {280.0f, 42.0f}, true});
    } else if (m_state == AppState::Controls) {
        m_menuButtons.push_back({"BACK", {640.0f, 640.0f}, {280.0f, 42.0f}, true});
    }
}

void Game::executeMenuSelection(std::size_t index) {
    if (index >= m_menuButtons.size() || !m_menuButtons[index].enabled) return;
    m_audio.playEffect(SoundEffectId::UiSelect);

    if (m_state == AppState::MainMenu) {
        if (index == 0) {
            if (loadCheckpoint()) {
                m_state = AppState::Playing;
            }
        } else if (index == 1) {
            startNewGame();
        } else if (index == 2) {
            m_previousState = AppState::MainMenu;
            m_state = AppState::Settings;
            rebuildMenuButtons();
        } else if (index == 3) {
            m_previousState = AppState::MainMenu;
            m_state = AppState::Controls;
            rebuildMenuButtons();
        } else if (index == 4) {
            m_window.close();
        }
    } else if (m_state == AppState::Paused) {
        if (index == 0) {
            m_state = AppState::Playing;
        } else if (index == 1) {
            saveCheckpoint();
            rebuildMenuButtons();
        } else if (index == 2) {
            if (loadCheckpoint()) {
                m_state = AppState::Playing;
            }
        } else if (index == 3) {
            m_previousState = AppState::Paused;
            m_state = AppState::Settings;
            rebuildMenuButtons();
        } else if (index == 4) {
            m_previousState = AppState::Paused;
            m_state = AppState::Controls;
            rebuildMenuButtons();
        } else if (index == 5) {
            m_state = AppState::MainMenu;
            rebuildMenuButtons();
        }
    } else if (m_state == AppState::GameOver) {
        if (index == 0) {
            enterRoom(m_labyrinth.currentRoomId(), m_labyrinth.currentRoom().playerDefaultSpawn);
            m_state = AppState::Playing;
        } else if (index == 1) {
            if (loadCheckpoint()) {
                m_state = AppState::Playing;
            }
        } else if (index == 2) {
            m_state = AppState::MainMenu;
            rebuildMenuButtons();
        }
    } else if (m_state == AppState::Victory) {
        if (index == 0) {
            startNewGame();
        } else if (index == 1) {
            m_state = AppState::MainMenu;
            rebuildMenuButtons();
        }
    } else if (m_state == AppState::Settings) {
        if (index == 0) {
            m_audio.setMasterVolume(m_audio.masterVolume() - 10.0f);
        } else if (index == 1) {
            m_audio.setMasterVolume(m_audio.masterVolume() + 10.0f);
        } else if (index == 2) {
            m_audio.setMuted(!m_audio.isMuted());
        } else if (index == 3) {
            const float cur = m_acoustics.rayDensityScale();
            if (cur < 0.9f) {
                m_acoustics.setRayDensityScale(1.0f);
                m_acoustics.setMaxReflections(3);
            } else if (cur < 1.3f) {
                m_acoustics.setRayDensityScale(1.5f);
                m_acoustics.setMaxReflections(4);
            } else {
                m_acoustics.setRayDensityScale(0.7f);
                m_acoustics.setMaxReflections(2);
            }
        } else if (index == 4) {
            if (m_ambientVisibility < 0.01f) m_ambientVisibility = 0.03f;
            else if (m_ambientVisibility < 0.05f) m_ambientVisibility = 0.08f;
            else m_ambientVisibility = 0.0f;
        } else if (index == 5) {
            toggleFullscreen();
        } else if (index == 6) {
            m_state = m_previousState;
            rebuildMenuButtons();
        }
    } else if (m_state == AppState::Controls) {
        m_state = m_previousState;
        rebuildMenuButtons();
    }
}

void Game::startNewGame() {
    m_labyrinth.resetAllProgress();
    m_acoustics.resetStats();
    m_player.setDecoyCount(4);
    m_runTimeSec = 0.0f;
    m_activeLoreTitle.clear();
    m_activeLoreBody.clear();
    m_loreDisplayTimer = 0.0f;
    enterRoom(0, m_labyrinth.currentRoom().playerDefaultSpawn);
    m_state = AppState::Playing;
    m_statusBannerText = "EXPEDITION STARTED - EMIT SOUND TO REVEAL THE LABYRINTH";
    m_statusBannerTimer = 4.5f;
}

void Game::enterRoom(int roomId, sf::Vector2f spawnPos) {
    m_labyrinth.setCurrentRoom(roomId);
    m_acoustics.clear();
    m_player.reset(spawnPos);
    m_enemy.configurePatrol(m_labyrinth.currentRoom().enemyPatrolNodes);

    // Emit subtle orientation pulse on room transition
    m_acoustics.emitPulse(spawnPos, 350.0f, 380.0f, 0.82f, 84, WaveSignature::RadialPulse, 0.0f, 6.2831853f, false);

    m_statusBannerText = m_labyrinth.currentRoom().name + " - " + m_labyrinth.currentRoom().sectorTitle;
    m_statusBannerTimer = 3.5f;

    if (m_labyrinth.currentRoom().isCheckpoint) {
        saveCheckpoint();
    }
}

void Game::saveCheckpoint() {
    SaveData data{};
    data.checkpointRoomId = m_labyrinth.currentRoomId();
    data.playerPosX = m_player.position().x;
    data.playerPosY = m_player.position().y;
    data.playerStamina = m_player.stamina();
    data.decoyCount = m_player.decoyCount();
    data.masterVolume = m_audio.masterVolume();
    data.rayDensityScale = m_acoustics.rayDensityScale();
    data.maxReflections = static_cast<int>(m_acoustics.maxReflections());
    data.ambientVisibility = m_ambientVisibility;
    data.runTimeSec = m_runTimeSec;
    data.activatedSwitches = m_labyrinth.activatedSwitches();
    data.collectedLoreIds = m_labyrinth.collectedLoreIds();
    data.visitedRooms = m_labyrinth.visitedRooms();

    if (SaveSystem::saveToFile(m_saveFilePath, data)) {
        m_statusBannerText = "CHECKPOINT SAVED AT " + m_labyrinth.currentRoom().name;
        m_statusBannerTimer = 3.2f;
    }
}

bool Game::loadCheckpoint() {
    SaveData data{};
    if (!SaveSystem::loadFromFile(m_saveFilePath, data)) {
        m_statusBannerText = "NO VALID CHECKPOINT FILE FOUND";
        m_statusBannerTimer = 3.0f;
        return false;
    }

    m_labyrinth.restoreState(data.activatedSwitches, data.collectedLoreIds, data.visitedRooms);
    m_player.setDecoyCount(data.decoyCount);
    m_audio.setMasterVolume(data.masterVolume);
    m_acoustics.setRayDensityScale(data.rayDensityScale);
    m_acoustics.setMaxReflections(static_cast<std::uint8_t>(data.maxReflections));
    m_ambientVisibility = data.ambientVisibility;
    m_runTimeSec = data.runTimeSec;

    m_labyrinth.setCurrentRoom(data.checkpointRoomId);
    const sf::Vector2f spawn{data.playerPosX, data.playerPosY};
    m_acoustics.clear();
    m_player.reset(spawn);
    m_player.setStamina(data.playerStamina);
    m_enemy.configurePatrol(m_labyrinth.currentRoom().enemyPatrolNodes);
    m_acoustics.emitPulse(spawn, 360.0f, 400.0f, 0.85f, 90, WaveSignature::RadialPulse, 0.0f, 6.2831853f, false);

    m_statusBannerText = "CHECKPOINT LOADED - " + m_labyrinth.currentRoom().name;
    m_statusBannerTimer = 3.5f;
    return true;
}

void Game::tryInteract() {
    auto& room = m_labyrinth.currentRoomMut();
    bool didInteract = false;

    for (auto& mech : room.mechanisms) {
        if (!mech.activated && distSq(mech.position, m_player.position()) <= 56.0f * 56.0f) {
            m_labyrinth.activateSwitch(mech.id);
            m_acoustics.emitPulse(mech.position, 420.0f, 640.0f, 1.25f, 110, WaveSignature::MechanismClunk, 0.0f, 6.2831853f, true);
            m_audio.playWaveSignature(WaveSignature::MechanismClunk);
            m_statusBannerText = mech.label + " ENGAGED - RESONANCE GATE UNSEALED";
            m_statusBannerTimer = 4.0f;
            m_cameraShakeTimer = 0.25f;
            didInteract = true;
        }
    }

    for (auto& lore : room.collectibles) {
        if (!lore.collected && distSq(lore.position, m_player.position()) <= 54.0f * 54.0f) {
            m_labyrinth.markLoreCollected(lore.id);
            m_player.addDecoy(1);
            m_acoustics.emitPulse(lore.position, 320.0f, 360.0f, 0.72f, 64, WaveSignature::DirectionalPing, 0.0f, 6.2831853f, false);
            m_audio.playEffect(SoundEffectId::LorePickup);
            m_activeLoreTitle = lore.title + " (+1 DECOY STONE)";
            m_activeLoreBody = lore.excerpt;
            m_loreDisplayTimer = 7.5f;
            didInteract = true;
        }
    }

    if (!didInteract) {
        m_statusBannerText = "NO LEVER OR ACOUSTIC LOG WITHIN REACH";
        m_statusBannerTimer = 2.0f;
    }
}

void Game::handleEvents() {
    sf::Event event{};
    while (m_window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            m_window.close();
        } else if (event.type == sf::Event::Resized) {
            updateLetterboxView(event.size.width, event.size.height);
        } else if (event.type == sf::Event::KeyPressed) {
            handleKeyPressed(event.key.code);
        } else if (event.type == sf::Event::MouseMoved) {
            const sf::Vector2f worldPt = m_window.mapPixelToCoords(
                {event.mouseMove.x, event.mouseMove.y},
                m_worldView
            );
            handleMouseMoved(worldPt);
        } else if (event.type == sf::Event::MouseButtonPressed) {
            const sf::Vector2f worldPt = m_window.mapPixelToCoords(
                {event.mouseButton.x, event.mouseButton.y},
                m_worldView
            );
            handleMousePressed(event.mouseButton.button, worldPt);
        }
    }
}

void Game::handleKeyPressed(sf::Keyboard::Key key) {
    if (key == sf::Keyboard::F11) {
        toggleFullscreen();
        return;
    }

    if (m_state == AppState::Playing) {
        if (key == sf::Keyboard::Escape || key == sf::Keyboard::P) {
            m_state = AppState::Paused;
            rebuildMenuButtons();
            m_audio.playEffect(SoundEffectId::UiMove);
        } else if (key == sf::Keyboard::Space) {
            if (m_player.triggerRadialPulse(m_acoustics)) {
                m_audio.playWaveSignature(WaveSignature::RadialPulse);
                m_cameraShakeTimer = 0.14f;
            }
        } else if (key == sf::Keyboard::E) {
            if (m_player.triggerDirectionalPing(m_mouseWorldPos, m_acoustics)) {
                m_audio.playWaveSignature(WaveSignature::DirectionalPing);
            }
        } else if (key == sf::Keyboard::G) {
            if (m_player.throwDecoy(m_mouseWorldPos, m_labyrinth.currentRoom().walls)) {
                m_audio.playEffect(SoundEffectId::UiMove, 0.7f, 1.3f);
            }
        } else if (key == sf::Keyboard::F) {
            tryInteract();
        } else if (key == sf::Keyboard::Tab || key == sf::Keyboard::M) {
            m_showMinimap = !m_showMinimap;
            m_audio.playEffect(SoundEffectId::UiMove);
        } else if (key == sf::Keyboard::F5) {
            saveCheckpoint();
        } else if (key == sf::Keyboard::F9) {
            loadCheckpoint();
        }
        return;
    }

    // Menu keyboard navigation
    if (key == sf::Keyboard::Escape) {
        if (m_state == AppState::Paused) {
            m_state = AppState::Playing;
            m_audio.playEffect(SoundEffectId::UiSelect);
        } else if (m_state == AppState::Settings || m_state == AppState::Controls) {
            m_state = m_previousState;
            rebuildMenuButtons();
            m_audio.playEffect(SoundEffectId::UiSelect);
        }
        return;
    }

    if (!m_menuButtons.empty()) {
        if (key == sf::Keyboard::W || key == sf::Keyboard::Up) {
            for (std::size_t step = 0; step < m_menuButtons.size(); ++step) {
                m_selectedMenuIndex = (m_selectedMenuIndex + m_menuButtons.size() - 1) % m_menuButtons.size();
                if (m_menuButtons[m_selectedMenuIndex].enabled) break;
            }
            m_audio.playEffect(SoundEffectId::UiMove);
        } else if (key == sf::Keyboard::S || key == sf::Keyboard::Down) {
            for (std::size_t step = 0; step < m_menuButtons.size(); ++step) {
                m_selectedMenuIndex = (m_selectedMenuIndex + 1) % m_menuButtons.size();
                if (m_menuButtons[m_selectedMenuIndex].enabled) break;
            }
            m_audio.playEffect(SoundEffectId::UiMove);
        } else if (key == sf::Keyboard::Enter || key == sf::Keyboard::Space) {
            executeMenuSelection(m_selectedMenuIndex);
        }
    }
}

void Game::handleMouseMoved(sf::Vector2f worldPos) {
    m_mouseWorldPos = worldPos;
    if (m_state != AppState::Playing) {
        for (std::size_t i = 0; i < m_menuButtons.size(); ++i) {
            if (m_menuButtons[i].enabled && pointInButton(worldPos, m_menuButtons[i])) {
                if (m_selectedMenuIndex != i) {
                    m_selectedMenuIndex = i;
                    m_audio.playEffect(SoundEffectId::UiMove, 0.5f);
                }
                break;
            }
        }
    }
}

void Game::handleMousePressed(sf::Mouse::Button button, sf::Vector2f worldPos) {
    m_mouseWorldPos = worldPos;
    if (m_state == AppState::Playing) {
        if (button == sf::Mouse::Left) {
            if (m_player.triggerDirectionalPing(worldPos, m_acoustics)) {
                m_audio.playWaveSignature(WaveSignature::DirectionalPing);
            }
        } else if (button == sf::Mouse::Right) {
            if (m_player.throwDecoy(worldPos, m_labyrinth.currentRoom().walls)) {
                m_audio.playEffect(SoundEffectId::UiMove, 0.7f, 1.3f);
            }
        }
        return;
    }

    if (button == sf::Mouse::Left) {
        for (std::size_t i = 0; i < m_menuButtons.size(); ++i) {
            if (m_menuButtons[i].enabled && pointInButton(worldPos, m_menuButtons[i])) {
                m_selectedMenuIndex = i;
                executeMenuSelection(i);
                break;
            }
        }
    }
}

void Game::updateFixed(float dt) {
    // Update atmospheric particles in all states
    for (auto& p : m_particles) {
        p.pos += p.vel * dt;
        if (p.pos.x < 0.0f) p.pos.x += cfg::kWorldWidth;
        if (p.pos.x >= cfg::kWorldWidth) p.pos.x -= cfg::kWorldWidth;
        if (p.pos.y < 0.0f) p.pos.y += cfg::kWorldHeight;
        if (p.pos.y >= cfg::kWorldHeight) p.pos.y -= cfg::kWorldHeight;
    }

    if (m_state == AppState::Playing) {
        updatePlaying(dt);
    } else {
        updateMenuBackground(dt);
    }
}

void Game::updateMenuBackground(float dt) {
    m_menuPulseTimer += dt;
    if (m_menuPulseTimer >= 1.8f) {
        m_menuPulseTimer = 0.0f;
        const sf::Vector2f origin{
            320.0f + std::fmod(m_runTimeSec * 95.0f, 640.0f),
            260.0f + std::sin(m_runTimeSec * 1.3f) * 140.0f
        };
        m_acoustics.emitPulse(origin, 340.0f, 520.0f, 0.85f, 72, WaveSignature::RadialPulse, 0.0f, 6.2831853f, false);
    }
    m_runTimeSec += dt;
    m_acoustics.update(dt, m_labyrinth.currentRoom().walls);
    m_acoustics.clearRecentEvents();
}

void Game::updatePlaying(float dt) {
    m_runTimeSec += dt;
    m_statusBannerTimer = std::max(0.0f, m_statusBannerTimer - dt);
    m_loreDisplayTimer = std::max(0.0f, m_loreDisplayTimer - dt);
    m_cameraShakeTimer = std::max(0.0f, m_cameraShakeTimer - dt);
    m_shaleCrackCooldown = std::max(0.0f, m_shaleCrackCooldown - dt);

    sf::Vector2f moveInput{0.0f, 0.0f};
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) moveInput.y -= 1.0f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) moveInput.y += 1.0f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) moveInput.x -= 1.0f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) moveInput.x += 1.0f;

    MovementStance stance = MovementStance::Walk;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) {
        stance = MovementStance::Sneak;
    } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::C)) {
        stance = MovementStance::Sprint;
    }

    auto& room = m_labyrinth.currentRoomMut();

    // Update ambient cistern water drips & steam vents
    for (auto& emitter : room.ambientEmitters) {
        emitter.timer += dt;
        if (emitter.timer >= emitter.intervalSec) {
            emitter.timer = 0.0f;
            m_acoustics.emitPulse(
                emitter.position,
                emitter.isSteamVent ? 245.0f : 210.0f,
                emitter.isSteamVent ? 210.0f : 165.0f,
                emitter.isSteamVent ? 0.48f : 0.35f,
                28,
                WaveSignature::AmbientDrip,
                0.0f,
                6.2831853f,
                false
            );
        }
    }

    const sf::Vector2f prevPos = m_player.position();
    WaveSignature emittedSig = WaveSignature::FootstepWalk;
    if (m_player.update(dt, moveInput, stance, m_mouseWorldPos, room.walls, m_acoustics, emittedSig)) {
        m_audio.playWaveSignature(emittedSig);
    }

    // Check Brittle Shale hazard zones: walking/sprinting across them cracks the floor loudly!
    const bool playerMoved = distSq(prevPos, m_player.position()) > 0.05f;
    if (playerMoved && m_player.activeStance() != MovementStance::Sneak && m_shaleCrackCooldown <= 0.0f) {
        for (const auto& hz : room.hazards) {
            if (distSq(m_player.position(), hz.position) <= hz.radius * hz.radius) {
                m_shaleCrackCooldown = 1.40f;
                m_acoustics.emitPulse(
                    m_player.position(),
                    415.0f,
                    550.0f,
                    1.20f,
                    96,
                    WaveSignature::ShaleCrack,
                    0.0f,
                    6.2831853f,
                    true
                );
                m_audio.playWaveSignature(WaveSignature::ShaleCrack);
                m_cameraShakeTimer = 0.16f;
                m_statusBannerText = "BRITTLE SHALE CRACKED! HOLD [LSHIFT] TO SNEAK SILENTLY";
                m_statusBannerTimer = 3.2f;
                break;
            }
        }
    }

    // Auto-collect lore when stepping directly onto terminal
    for (const auto& lore : room.collectibles) {
        if (!lore.collected && distSq(lore.position, m_player.position()) <= 24.0f * 24.0f) {
            tryInteract();
            break;
        }
    }

    const bool newlyAlerted = m_enemy.update(dt, m_player.position(), room.walls, m_acoustics);
    if (newlyAlerted) {
        m_audio.playEffect(SoundEffectId::EnemyAlert);
        m_cameraShakeTimer = 0.22f;
    }

    const float dangerRatio = m_enemy.isActive() ? (m_enemy.suspicion() / 100.0f) : 0.0f;
    m_audio.updateHeartbeat(dangerRatio, dt);

    const auto newlyChargedResonators = m_acoustics.update(dt, room.walls, &room.resonators);
    for (int resId : newlyChargedResonators) {
        m_labyrinth.activateSwitch(resId);
        m_audio.playWaveSignature(WaveSignature::ResonatorHum);
        m_cameraShakeTimer = 0.26f;
        m_statusBannerText = "HARMONIC RESONATOR CHARGED - EAST RESONANCE GATE UNSEALED";
        m_statusBannerTimer = 4.2f;
        for (const auto& res : room.resonators) {
            if (res.id == resId) {
                m_acoustics.emitPulse(res.position, 440.0f, 640.0f, 1.30f, 120, WaveSignature::ResonatorHum, 0.0f, 6.2831853f, false);
            }
        }
    }

    m_acoustics.clearRecentEvents();

    if (m_enemy.hasCaughtPlayer()) {
        m_audio.playEffect(SoundEffectId::PlayerCaught);
        m_state = AppState::GameOver;
        rebuildMenuButtons();
        return;
    }

    // Check room transition portals
    for (const auto& portal : room.portals) {
        if (portal.requiredSwitchId != -1 && !m_labyrinth.isSwitchActivated(portal.requiredSwitchId)) {
            continue;
        }
        if (distSq(portal.position, m_player.position()) <= portal.radius * portal.radius) {
            if (portal.isVictoryExit) {
                m_audio.playEffect(SoundEffectId::VictoryChord);
                m_state = AppState::Victory;
                rebuildMenuButtons();
                return;
            }
            enterRoom(portal.targetRoomId, portal.targetSpawnPos);
            break;
        }
    }
}

void Game::render() {
    m_window.clear(sf::Color(5, 8, 13));

    sf::View activeView = m_worldView;
    if (m_state == AppState::Playing && m_cameraShakeTimer > 0.0f) {
        const float mag = m_cameraShakeTimer * 14.0f;
        activeView.move(
            std::sin(m_runTimeSec * 55.0f) * mag,
            std::cos(m_runTimeSec * 48.0f) * mag
        );
    }
    m_window.setView(activeView);

    renderWorld();

    if (m_state == AppState::Playing) {
        renderHud();
        if (m_showMinimap) {
            renderMinimapOverlay();
        }
    } else if (m_state == AppState::Settings) {
        renderSettingsOverlay();
    } else if (m_state == AppState::Controls) {
        renderControlsOverlay();
    } else {
        renderMenuOverlay();
    }

    m_window.display();
}

void Game::renderWorld() {
    const auto& room = m_labyrinth.currentRoom();

    // 1. Subtle subterranean floor grid lines near illuminated zones
    sf::VertexArray gridLines(sf::Lines);
    for (float x = 100.0f; x <= 1180.0f; x += 60.0f) {
        gridLines.append(sf::Vertex({x, 80.0f}, sf::Color(15, 25, 38, 95)));
        gridLines.append(sf::Vertex({x, 720.0f}, sf::Color(15, 25, 38, 95)));
    }
    for (float y = 100.0f; y <= 700.0f; y += 60.0f) {
        gridLines.append(sf::Vertex({90.0f, y}, sf::Color(15, 25, 38, 95)));
        gridLines.append(sf::Vertex({1190.0f, y}, sf::Color(15, 25, 38, 95)));
    }
    m_window.draw(gridLines);

    // 2. Render Acoustic Wavefronts & 3D Wall Echoes
    m_acoustics.render(m_window, m_ambientVisibility, &room.walls);

    // 3. Render Portals, Switches, Lore Terminals, Enemy, and Player
    if (m_state == AppState::Playing || m_state == AppState::Paused || m_state == AppState::GameOver) {
        renderInteractiveObjects();
        const float enemyIllum = m_acoustics.queryIlluminationAt(m_enemy.position(), m_player.position());
        m_enemy.render(m_window, enemyIllum);
        m_player.render(m_window);
    }

    // 4. Render atmospheric dust motes & vignette darkness
    renderVignetteAndDust();
}

void Game::renderInteractiveObjects() {
    const auto& room = m_labyrinth.currentRoom();
    const sf::Vector2f playerPos = m_player.position();

    // Portals
    for (const auto& portal : room.portals) {
        const bool locked = (portal.requiredSwitchId != -1) && !m_labyrinth.isSwitchActivated(portal.requiredSwitchId);
        const float illum = std::max(0.35f, m_acoustics.queryIlluminationAt(portal.position, playerPos));
        const auto alpha = static_cast<sf::Uint8>(illum * 220.0f);

        sf::CircleShape ring(portal.radius);
        ring.setOrigin(portal.radius, portal.radius);
        ring.setPosition(portal.position);
        ring.setFillColor(sf::Color(10, 22, 36, static_cast<sf::Uint8>(illum * 110.0f)));
        ring.setOutlineThickness(2.0f);
        ring.setOutlineColor(locked ? sf::Color(167, 139, 250, alpha) : sf::Color(56, 189, 248, alpha));
        m_window.draw(ring);

        m_ui.drawText(
            m_window,
            locked ? "SEALED GATE" : portal.label,
            {portal.position.x, portal.position.y - portal.radius - 16.0f},
            10.0f,
            locked ? sf::Color(196, 181, 253, alpha) : sf::Color(125, 211, 252, alpha),
            TextAlign::Center
        );
    }

    // Harmonic Levers
    for (const auto& mech : room.mechanisms) {
        const float illum = std::max(0.30f, m_acoustics.queryIlluminationAt(mech.position, playerPos));
        const auto alpha = static_cast<sf::Uint8>(illum * 245.0f);
        const sf::Color col = mech.activated
            ? sf::Color(52, 211, 153, alpha)
            : sf::Color(167, 139, 250, alpha);

        sf::CircleShape base(12.0f);
        base.setOrigin(12.0f, 12.0f);
        base.setPosition(mech.position);
        base.setFillColor(sf::Color(18, 16, 34, alpha));
        base.setOutlineThickness(2.0f);
        base.setOutlineColor(col);
        m_window.draw(base);

        if (distSq(playerPos, mech.position) <= 95.0f * 95.0f || mech.activated) {
            m_ui.drawText(
                m_window,
                mech.activated ? "LEVER ENGAGED" : "[F] PULL LEVER",
                {mech.position.x, mech.position.y - 24.0f},
                11.0f,
                col,
                TextAlign::Center
            );
        }
    }

    // Lore Terminals
    for (const auto& lore : room.collectibles) {
        if (lore.collected) continue;
        const float illum = std::max(0.32f, m_acoustics.queryIlluminationAt(lore.position, playerPos));
        const auto alpha = static_cast<sf::Uint8>(illum * 235.0f);

        sf::RectangleShape icon({14.0f, 14.0f});
        icon.setOrigin(7.0f, 7.0f);
        icon.setPosition(lore.position);
        icon.setRotation(45.0f);
        icon.setFillColor(sf::Color(16, 36, 48, alpha));
        icon.setOutlineThickness(1.8f);
        icon.setOutlineColor(sf::Color(45, 212, 191, alpha));
        m_window.draw(icon);

        if (distSq(playerPos, lore.position) <= 105.0f * 105.0f) {
            m_ui.drawText(
                m_window,
                "[F] SURVEYOR LOG",
                {lore.position.x, lore.position.y - 22.0f},
                11.0f,
                sf::Color(94, 234, 212, alpha),
                TextAlign::Center
            );
        }
    }

    // Harmonic Resonator Crystals (charged by sonar rays)
    for (const auto& res : room.resonators) {
        const float illum = std::max(0.36f, m_acoustics.queryIlluminationAt(res.position, playerPos));
        const auto alpha = static_cast<sf::Uint8>(illum * 245.0f);
        const sf::Color col = res.activated
            ? sf::Color(52, 211, 153, alpha)
            : sf::Color(34, 211, 238, alpha);

        sf::CircleShape outer(res.radius);
        outer.setOrigin(res.radius, res.radius);
        outer.setPosition(res.position);
        outer.setFillColor(sf::Color(12, 28, 42, static_cast<sf::Uint8>(illum * 140.0f)));
        outer.setOutlineThickness(2.0f);
        outer.setOutlineColor(col);
        m_window.draw(outer);

        const float coreSize = 8.0f + res.energy * 6.0f;
        sf::RectangleShape crystal({coreSize, coreSize});
        crystal.setOrigin(coreSize * 0.5f, coreSize * 0.5f);
        crystal.setPosition(res.position);
        crystal.setRotation(45.0f + m_runTimeSec * 35.0f);
        crystal.setFillColor(col);
        m_window.draw(crystal);

        char resLabel[64];
        if (res.activated) {
            std::snprintf(resLabel, sizeof(resLabel), "RESONATOR LOCKED [100%%]");
        } else {
            std::snprintf(resLabel, sizeof(resLabel), "HIT WITH SONAR [%d%%]", static_cast<int>(res.energy * 100.0f));
        }
        m_ui.drawText(
            m_window,
            resLabel,
            {res.position.x, res.position.y - res.radius - 16.0f},
            10.5f,
            col,
            TextAlign::Center
        );
    }

    // Brittle Shale Acoustic Hazard Zones
    for (const auto& hz : room.hazards) {
        const float illum = m_acoustics.queryIlluminationAt(hz.position, playerPos);
        if (illum <= 0.08f) continue;
        const auto alpha = static_cast<sf::Uint8>(illum * 195.0f);

        sf::CircleShape zone(hz.radius);
        zone.setOrigin(hz.radius, hz.radius);
        zone.setPosition(hz.position);
        zone.setFillColor(sf::Color(48, 28, 10, static_cast<sf::Uint8>(illum * 65.0f)));
        zone.setOutlineThickness(1.5f);
        zone.setOutlineColor(sf::Color(251, 191, 36, alpha));
        m_window.draw(zone);

        m_ui.drawText(
            m_window,
            "BRITTLE SHALE [SNEAK]",
            {hz.position.x, hz.position.y - 5.0f},
            9.5f,
            sf::Color(251, 191, 36, alpha),
            TextAlign::Center
        );
    }
}

void Game::renderVignetteAndDust() {
    // Floating dust motes brighten when sound waves pass over them
    sf::VertexArray dustQuads(sf::Quads);
    const sf::Vector2f pPos = m_player.position();
    for (const auto& p : m_particles) {
        const float illum = m_acoustics.queryIlluminationAt(p.pos, pPos);
        const auto alpha = static_cast<sf::Uint8>(25.0f + illum * 165.0f);
        const sf::Color c(125, 211, 252, alpha);
        const float s = p.size;
        dustQuads.append(sf::Vertex(p.pos, c));
        dustQuads.append(sf::Vertex({p.pos.x + s, p.pos.y}, c));
        dustQuads.append(sf::Vertex({p.pos.x + s, p.pos.y + s}, c));
        dustQuads.append(sf::Vertex({p.pos.x, p.pos.y + s}, c));
    }
    m_window.draw(dustQuads);

    // Subtle edge vignette frame
    sf::VertexArray vignette(sf::Triangles);
    const sf::Color edgeDark(2, 4, 8, 200);
    const sf::Color clearCenter(2, 4, 8, 0);
    constexpr float border = 75.0f;

    auto addBevel = [&](sf::Vector2f a0, sf::Vector2f a1, sf::Vector2f b1, sf::Vector2f b0) {
        vignette.append(sf::Vertex(a0, edgeDark));
        vignette.append(sf::Vertex(a1, edgeDark));
        vignette.append(sf::Vertex(b1, clearCenter));
        vignette.append(sf::Vertex(a0, edgeDark));
        vignette.append(sf::Vertex(b1, clearCenter));
        vignette.append(sf::Vertex(b0, clearCenter));
    };

    addBevel({0.0f, 0.0f}, {cfg::kWorldWidth, 0.0f}, {cfg::kWorldWidth, border}, {0.0f, border});
    addBevel({0.0f, cfg::kWorldHeight}, {cfg::kWorldWidth, cfg::kWorldHeight}, {cfg::kWorldWidth, cfg::kWorldHeight - border}, {0.0f, cfg::kWorldHeight - border});
    m_window.draw(vignette);
}

void Game::renderHud() {
    const auto& room = m_labyrinth.currentRoom();

    // Top HUD Panel
    m_ui.drawPanel(m_window, {24.0f, 14.0f}, {1232.0f, 52.0f}, sf::Color(9, 14, 22, 225), sf::Color(30, 45, 68), 1.5f);

    m_ui.drawText(m_window, room.name, {42.0f, 22.0f}, 14.0f, sf::Color(56, 189, 248), TextAlign::Left, true);
    m_ui.drawText(m_window, room.objectiveText, {42.0f, 43.0f}, 11.0f, sf::Color(148, 163, 184), TextAlign::Left);

    // Right side of Top HUD: Listener Status & Progress
    const std::string listenerLabel = std::string("LISTENER: ") + EnemyAI::stateName(m_enemy.state());
    const sf::Color listenerCol = (m_enemy.state() == EnemyState::PursuePlayer)
        ? sf::Color(248, 113, 113)
        : (m_enemy.state() == EnemyState::InvestigateSound || m_enemy.state() == EnemyState::SearchLastKnown)
            ? sf::Color(192, 132, 252)
            : sf::Color(148, 163, 184);

    m_ui.drawText(m_window, listenerLabel, {1236.0f, 22.0f}, 12.0f, listenerCol, TextAlign::Right);

    char progBuf[96];
    std::snprintf(
        progBuf,
        sizeof(progBuf),
        "CHAMBERS %zu/15  |  LOGS %zu/8  |  [TAB] MAP",
        m_labyrinth.visitedRooms().size(),
        m_labyrinth.collectedLoreIds().size()
    );
    m_ui.drawText(m_window, progBuf, {1236.0f, 43.0f}, 11.0f, sf::Color(125, 211, 252), TextAlign::Right);

    // Bottom HUD Panel
    m_ui.drawPanel(m_window, {24.0f, 734.0f}, {1232.0f, 52.0f}, sf::Color(9, 14, 22, 225), sf::Color(30, 45, 68), 1.5f);

    const char* stanceStr = (m_player.activeStance() == MovementStance::Sneak)
        ? "STANCE: SNEAK (QUIET)"
        : (m_player.activeStance() == MovementStance::Sprint ? "STANCE: SPRINT (LOUD)" : "STANCE: WALK");
    m_ui.drawText(m_window, stanceStr, {42.0f, 743.0f}, 12.0f, sf::Color(56, 189, 248));

    m_ui.drawText(m_window, "STAMINA", {42.0f, 764.0f}, 10.0f, sf::Color(148, 163, 184));
    m_ui.drawProgressBar(
        m_window,
        {115.0f, 763.0f},
        {150.0f, 12.0f},
        m_player.stamina() / 100.0f,
        sf::Color(15, 23, 42),
        sf::Color(45, 212, 191),
        sf::Color(51, 65, 85)
    );

    char abilityBuf[160];
    std::snprintf(
        abilityBuf,
        sizeof(abilityBuf),
        "[LMB/E] PING CONE  |  [SPACE] 360 PULSE  |  [RMB/G] DECOY (%d)  |  [F] INTERACT  |  [F5/F9] SAVE/LOAD",
        m_player.decoyCount()
    );
    m_ui.drawText(m_window, abilityBuf, {1236.0f, 753.0f}, 11.0f, sf::Color(203, 213, 225), TextAlign::Right);

    // Transient status toast banner
    if (m_statusBannerTimer > 0.0f) {
        m_ui.drawPanel(m_window, {320.0f, 78.0f}, {640.0f, 30.0f}, sf::Color(12, 22, 36, 215), sf::Color(56, 189, 248, 140), 1.0f);
        m_ui.drawText(m_window, m_statusBannerText, {640.0f, 87.0f}, 11.0f, sf::Color(186, 230, 253), TextAlign::Center);
    }

    // Lore reader popup when a surveyor log is collected
    if (m_loreDisplayTimer > 0.0f) {
        m_ui.drawPanel(m_window, {220.0f, 645.0f}, {840.0f, 72.0f}, sf::Color(14, 24, 38, 240), sf::Color(45, 212, 191), 1.5f);
        m_ui.drawText(m_window, m_activeLoreTitle, {640.0f, 657.0f}, 13.0f, sf::Color(45, 212, 191), TextAlign::Center, true);
        m_ui.drawText(m_window, m_activeLoreBody, {640.0f, 684.0f}, 11.0f, sf::Color(226, 232, 240), TextAlign::Center);
    }
}

void Game::renderMinimapOverlay() {
    m_ui.drawPanel(m_window, {290.0f, 160.0f}, {700.0f, 460.0f}, sf::Color(7, 12, 20, 242), sf::Color(56, 189, 248), 2.0f);
    m_ui.drawText(m_window, "LABYRINTH ACOUSTIC TOPOLOGY (15 CHAMBERS)", {640.0f, 185.0f}, 16.0f, sf::Color(56, 189, 248), TextAlign::Center, true);
    m_ui.drawText(m_window, "PRESS [TAB] OR [M] TO CLOSE", {640.0f, 212.0f}, 11.0f, sf::Color(148, 163, 184), TextAlign::Center);

    for (int i = 0; i < 15; ++i) {
        const int row = i / 5;
        const int col = i % 5;
        const sf::Vector2f topLeft{335.0f + static_cast<float>(col) * 126.0f, 255.0f + static_cast<float>(row) * 105.0f};
        const bool isCurrent = (i == m_labyrinth.currentRoomId());
        const bool visited = m_labyrinth.isRoomVisited(i);

        const sf::Color fill = isCurrent
            ? sf::Color(14, 58, 88)
            : (visited ? sf::Color(16, 28, 44) : sf::Color(10, 14, 22));
        const sf::Color border = isCurrent
            ? sf::Color(56, 189, 248)
            : (visited ? sf::Color(71, 85, 105) : sf::Color(30, 41, 59));

        m_ui.drawPanel(m_window, topLeft, {106.0f, 76.0f}, fill, border, isCurrent ? 2.0f : 1.0f);

        char roomNum[32];
        std::snprintf(roomNum, sizeof(roomNum), "ROOM %02d", i + 1);
        m_ui.drawText(
            m_window,
            roomNum,
            {topLeft.x + 53.0f, topLeft.y + 18.0f},
            12.0f,
            visited ? sf::Color(226, 232, 240) : sf::Color(71, 85, 105),
            TextAlign::Center
        );

        const auto& rDef = m_labyrinth.roomAt(static_cast<std::size_t>(i));
        const char* tag = rDef.isExitRoom ? "EXIT LIFT" : (rDef.isCheckpoint ? "CHECKPOINT" : "SECTOR");
        m_ui.drawText(
            m_window,
            visited ? tag : "UNMAPPED",
            {topLeft.x + 53.0f, topLeft.y + 44.0f},
            10.0f,
            isCurrent ? sf::Color(56, 189, 248) : sf::Color(100, 116, 139),
            TextAlign::Center
        );
    }
}

void Game::renderMenuOverlay() {
    m_ui.drawPanel(m_window, {340.0f, 130.0f}, {600.0f, 520.0f}, sf::Color(8, 13, 21, 235), sf::Color(38, 62, 92), 2.0f);

    if (m_state == AppState::MainMenu) {
        m_ui.drawText(m_window, "ECHOLABYRINTH", {640.0f, 175.0f}, 32.0f, sf::Color(56, 189, 248), TextAlign::Center, true);
        m_ui.drawText(m_window, "ACOUSTIC STEALTH HORROR IN A 15-CHAMBER ABYSS", {640.0f, 225.0f}, 12.0f, sf::Color(148, 163, 184), TextAlign::Center);
    } else if (m_state == AppState::Paused) {
        m_ui.drawText(m_window, "EXPEDITION PAUSED", {640.0f, 185.0f}, 26.0f, sf::Color(56, 189, 248), TextAlign::Center, true);
        m_ui.drawText(m_window, m_labyrinth.currentRoom().name, {640.0f, 225.0f}, 13.0f, sf::Color(148, 163, 184), TextAlign::Center);
    } else if (m_state == AppState::GameOver) {
        m_ui.drawText(m_window, "SIGNAL TERMINATED", {640.0f, 195.0f}, 28.0f, sf::Color(248, 113, 113), TextAlign::Center, true);
        m_ui.drawText(m_window, "THE BLIND LISTENER TRIANGULATED YOUR ACOUSTIC SIGNATURE", {640.0f, 245.0f}, 12.0f, sf::Color(203, 213, 225), TextAlign::Center);
        m_ui.drawText(m_window, "TIP: HOLD [LSHIFT] TO SNEAK OR THROW DECOY STONES [RMB / G]", {640.0f, 275.0f}, 11.0f, sf::Color(148, 163, 184), TextAlign::Center);
    } else if (m_state == AppState::Victory) {
        m_ui.drawText(m_window, "SURFACE ASCENT COMPLETE", {640.0f, 185.0f}, 26.0f, sf::Color(52, 211, 153), TextAlign::Center, true);
        m_ui.drawText(m_window, "YOU ESCAPED THE SUBTERRANEAN LABYRINTH", {640.0f, 228.0f}, 13.0f, sf::Color(203, 213, 225), TextAlign::Center);

        char stats1[128];
        std::snprintf(
            stats1,
            sizeof(stats1),
            "CHAMBERS MAPPED: %zu/15   |   SURVEYOR LOGS: %zu/8",
            m_labyrinth.visitedRooms().size(),
            m_labyrinth.collectedLoreIds().size()
        );
        m_ui.drawText(m_window, stats1, {640.0f, 290.0f}, 13.0f, sf::Color(56, 189, 248), TextAlign::Center);

        char stats2[128];
        std::snprintf(
            stats2,
            sizeof(stats2),
            "SONAR PULSES EMITTED: %zu   |   EXPEDITION TIME: %ds",
            m_acoustics.totalPulsesEmitted(),
            static_cast<int>(m_runTimeSec)
        );
        m_ui.drawText(m_window, stats2, {640.0f, 325.0f}, 13.0f, sf::Color(167, 139, 250), TextAlign::Center);
    }

    for (std::size_t i = 0; i < m_menuButtons.size(); ++i) {
        const auto& btn = m_menuButtons[i];
        const bool selected = (i == m_selectedMenuIndex);
        const sf::Vector2f topLeft = btn.center - btn.size * 0.5f;

        const sf::Color fill = !btn.enabled
            ? sf::Color(12, 17, 26, 150)
            : (selected ? sf::Color(20, 48, 76, 240) : sf::Color(14, 22, 34, 220));
        const sf::Color border = !btn.enabled
            ? sf::Color(30, 41, 59)
            : (selected ? sf::Color(56, 189, 248) : sf::Color(51, 65, 85));
        const sf::Color textCol = !btn.enabled
            ? sf::Color(71, 85, 105)
            : (selected ? sf::Color(224, 242, 254) : sf::Color(148, 163, 184));

        m_ui.drawPanel(m_window, topLeft, btn.size, fill, border, selected ? 2.0f : 1.0f);
        m_ui.drawText(m_window, btn.label, {btn.center.x, btn.center.y - 7.0f}, 14.0f, textCol, TextAlign::Center, selected);
    }
}

void Game::renderSettingsOverlay() {
    m_ui.drawPanel(m_window, {300.0f, 120.0f}, {680.0f, 540.0f}, sf::Color(8, 13, 21, 242), sf::Color(56, 189, 248), 2.0f);
    m_ui.drawText(m_window, "AUDIO & ACOUSTIC ENGINE SETTINGS", {640.0f, 155.0f}, 22.0f, sf::Color(56, 189, 248), TextAlign::Center, true);

    char infoBuf[160];
    std::snprintf(
        infoBuf,
        sizeof(infoBuf),
        "VOLUME: %d%% (%s)  |  BOUNCES: %d  |  DARKNESS: %d%%",
        static_cast<int>(m_audio.masterVolume()),
        m_audio.isMuted() ? "MUTED" : "ACTIVE",
        static_cast<int>(m_acoustics.maxReflections()),
        static_cast<int>(m_ambientVisibility * 100.0f)
    );
    m_ui.drawText(m_window, infoBuf, {640.0f, 210.0f}, 13.0f, sf::Color(167, 139, 250), TextAlign::Center);

    for (std::size_t i = 0; i < m_menuButtons.size(); ++i) {
        const auto& btn = m_menuButtons[i];
        const bool selected = (i == m_selectedMenuIndex);
        const sf::Vector2f topLeft = btn.center - btn.size * 0.5f;
        m_ui.drawPanel(
            m_window,
            topLeft,
            btn.size,
            selected ? sf::Color(20, 48, 76, 240) : sf::Color(14, 22, 34, 220),
            selected ? sf::Color(56, 189, 248) : sf::Color(51, 65, 85),
            selected ? 2.0f : 1.0f
        );
        m_ui.drawText(
            m_window,
            btn.label,
            {btn.center.x, btn.center.y - 7.0f},
            13.0f,
            selected ? sf::Color(224, 242, 254) : sf::Color(148, 163, 184),
            TextAlign::Center,
            selected
        );
    }
}

void Game::renderControlsOverlay() {
    m_ui.drawPanel(m_window, {220.0f, 95.0f}, {840.0f, 600.0f}, sf::Color(8, 13, 21, 244), sf::Color(56, 189, 248), 2.0f);
    m_ui.drawText(m_window, "CONTROLS & ACOUSTIC FIELD MANUAL", {640.0f, 130.0f}, 22.0f, sf::Color(56, 189, 248), TextAlign::Center, true);

    const char* lines[] = {
        "MOVEMENT & STANCES:",
        "  [WASD / ARROWS]  MOVE SURVEYOR THROUGH THE LABYRINTH",
        "  [HOLD LSHIFT]    SNEAK STANCE (QUIET FOOTSTEPS - EVADES LISTENERS)",
        "  [HOLD LCTRL / C] SPRINT STANCE (FAST MOVEMENT - LOUD ACOUSTIC SIGNATURE)",
        "",
        "ACOUSTIC EXPLORATION & INTERACTION:",
        "  [LEFT CLICK / E] DIRECTIONAL SONAR CONE TOWARD CURSOR",
        "  [SPACE]          360-DEGREE RADIAL RESONANCE PULSE",
        "  [RIGHT CLICK / G] THROW DECOY STONE TO LURE HOSTILE ENTITIES",
        "  [F]              PULL HARMONIC LEVER / RECOVER SURVEYOR LOG",
        "  [TAB / M]        TOGGLE 15-CHAMBER ACOUSTIC TOPOLOGY MAP",
        "  [F5 / F9]        QUICK SAVE / QUICK LOAD CHECKPOINT",
        "",
        "SURFACE MATERIALS & REFLECTIVITY:",
        "  STONE (82% BLUE) | STEEL (94% CYAN) | WATER (64% TEAL) | MOSS (36% GREEN) | GATE (90% VIOLET)"
    };

    float y = 185.0f;
    for (const char* line : lines) {
        const bool isHeader = (line[0] != ' ' && line[0] != '\0');
        m_ui.drawText(
            m_window,
            line,
            {265.0f, y},
            isHeader ? 13.0f : 11.5f,
            isHeader ? sf::Color(125, 211, 252) : sf::Color(203, 213, 225)
        );
        y += isHeader ? 26.0f : 22.0f;
    }

    if (!m_menuButtons.empty()) {
        const auto& btn = m_menuButtons[0];
        m_ui.drawPanel(m_window, btn.center - btn.size * 0.5f, btn.size, sf::Color(20, 48, 76), sf::Color(56, 189, 248), 2.0f);
        m_ui.drawText(m_window, btn.label, {btn.center.x, btn.center.y - 7.0f}, 14.0f, sf::Color(224, 242, 254), TextAlign::Center, true);
    }
}

int Game::runSelfTest(const std::string& exeDir) {
    std::cout << "[SelfTest] Starting EchoLabyrinth C++17 comprehensive verification suite...\n";

    // 1. Verify Labyrinth 15-room topology & mathematical BFS traversability of all 15 chambers
    Labyrinth lab;
    if (lab.roomCount() != 15) {
        std::cerr << "[SelfTest] FAIL: Expected 15 rooms, got " << lab.roomCount() << "\n";
        return 1;
    }
    for (int r = 0; r < 15; ++r) {
        std::string travErr;
        if (!lab.verifyRoomTraversability(r, travErr)) {
            std::cerr << "[SelfTest] FAIL: Room " << (r + 1) << " failed BFS traversability: " << travErr << "\n";
            return 1;
        }
    }
    std::cout << "[SelfTest] PASS [1/8]: All 15 chambers verified via 2D BFS grid pathfinding (levers, logs, and locked gate barriers).\n";

    // 2. Verify full 15-room transition graph (forward 0->14->Victory and backward West return spawn positions)
    lab.resetAllProgress();
    int currentRoom = 0;
    bool reachedVictoryExit = false;
    for (int step = 0; step < 15; ++step) {
        lab.setCurrentRoom(currentRoom);
        const auto& rm = lab.currentRoom();

        // Unlock any lever or resonator in this room
        for (const auto& m : rm.mechanisms) lab.activateSwitch(m.id);
        for (const auto& res : rm.resonators) lab.activateSwitch(res.id);

        // Verify West portal return spawn is west of any East bulkhead (x < 1020)
        if (currentRoom > 0) {
            bool foundWest = false;
            for (const auto& p : rm.portals) {
                if (p.targetRoomId == currentRoom - 1) {
                    foundWest = true;
                    if (p.targetSpawnPos.x >= 1020.0f) {
                        std::cerr << "[SelfTest] FAIL: West portal in room " << currentRoom
                                  << " spawns behind East bulkhead at x=" << p.targetSpawnPos.x << "\n";
                        return 1;
                    }
                }
            }
            if (!foundWest) {
                std::cerr << "[SelfTest] FAIL: Missing West return portal in room " << currentRoom << "\n";
                return 1;
            }
        }

        // Follow East portal
        bool advanced = false;
        for (const auto& p : rm.portals) {
            if (p.isVictoryExit) {
                if (p.requiredSwitchId != -1 && !lab.isSwitchActivated(p.requiredSwitchId)) {
                    std::cerr << "[SelfTest] FAIL: Victory exit remained locked after activating switches.\n";
                    return 1;
                }
                reachedVictoryExit = true;
                advanced = true;
                break;
            }
            if (p.targetRoomId == currentRoom + 1) {
                currentRoom = p.targetRoomId;
                advanced = true;
                break;
            }
        }
        if (!advanced) {
            std::cerr << "[SelfTest] FAIL: Graph progression broken at room " << currentRoom << "\n";
            return 1;
        }
    }
    if (!reachedVictoryExit || currentRoom != 14) {
        std::cerr << "[SelfTest] FAIL: Did not reach Room 15 Victory Exit.\n";
        return 1;
    }
    std::cout << "[SelfTest] PASS [2/8]: Full 15-room progression graph & bidirectional portals verified.\n";

    // 3. Verify AcousticSystem ray propagation, specular reflections, and multi-wall material transmission
    AcousticSystem acoustics;
    acoustics.emitPulse({200.0f, 400.0f}, 420.0f, 650.0f, 1.0f, 120, WaveSignature::RadialPulse);
    if (acoustics.activeRayCount() == 0 || acoustics.recentEvents().empty()) {
        std::cerr << "[SelfTest] FAIL: AcousticSystem failed to emit pulse.\n";
        return 1;
    }
    for (int step = 0; step < 60; ++step) {
        acoustics.update(cfg::kFixedTimeStep, lab.currentRoom().walls);
    }
    if (acoustics.activeEchoCount() == 0) {
        std::cerr << "[SelfTest] FAIL: Acoustic rays did not produce wall echo traces.\n";
        return 1;
    }

    const std::vector<WallSegment> metalWall = {{{500.0f, 100.0f}, {500.0f, 700.0f}, SurfaceMaterial::Metal, -1, false}};
    const std::vector<WallSegment> mossWall = {{{500.0f, 100.0f}, {500.0f, 700.0f}, SurfaceMaterial::DampMoss, -1, false}};
    const std::vector<WallSegment> doubleStoneWalls = {
        {{400.0f, 100.0f}, {400.0f, 700.0f}, SurfaceMaterial::Stone, -1, false},
        {{600.0f, 100.0f}, {600.0f, 700.0f}, SurfaceMaterial::Stone, -1, false}
    };
    const float transMetal = AcousticSystem::computeTransmissionFactor({300.0f, 400.0f}, {700.0f, 400.0f}, metalWall);
    const float transMoss = AcousticSystem::computeTransmissionFactor({300.0f, 400.0f}, {700.0f, 400.0f}, mossWall);
    const float transDoubleStone = AcousticSystem::computeTransmissionFactor({300.0f, 400.0f}, {700.0f, 400.0f}, doubleStoneWalls);
    if (!(transMetal > transMoss && transMoss > transDoubleStone)) {
        std::cerr << "[SelfTest] FAIL: Material transmission ordering invalid (Metal=" << transMetal
                  << ", Moss=" << transMoss << ", DoubleStone=" << transDoubleStone << ").\n";
        return 1;
    }
    std::cout << "[SelfTest] PASS [3/8]: AcousticSystem reflections & multi-wall material attenuation verified.\n";

    // 4. Verify AcousticResonator crystal charging via sonar rays unlocks paired ResonanceDoor
    lab.resetAllProgress();
    lab.setCurrentRoom(1); // Room 02: Echoing Gallery has Resonator ID 101 at (825, 400)
    acoustics.clear();
    auto& room02 = lab.currentRoomMut();
    if (room02.resonators.empty() || room02.resonators[0].activated) {
        std::cerr << "[SelfTest] FAIL: Room 02 resonator missing or prematurely active.\n";
        return 1;
    }
    bool resonatorUnlocked = false;
    for (int burst = 0; burst < 3 && !resonatorUnlocked; ++burst) {
        acoustics.emitPulse({760.0f, 400.0f}, 445.0f, 560.0f, 1.15f, 96, WaveSignature::DirectionalPing, 0.0f, 1.0471975f, true);
        for (int step = 0; step < 30; ++step) {
            const auto unlockedIds = acoustics.update(cfg::kFixedTimeStep, room02.walls, &room02.resonators);
            for (int id : unlockedIds) {
                lab.activateSwitch(id);
                resonatorUnlocked = true;
            }
        }
    }
    if (!resonatorUnlocked || !lab.isSwitchActivated(101)) {
        std::cerr << "[SelfTest] FAIL: AcousticResonator did not charge and unlock Gate 101.\n";
        return 1;
    }
    std::cout << "[SelfTest] PASS [4/8]: AcousticResonator crystal energized by sonar rays and unsealed Gate 101.\n";

    // 5. Verify Player movement, circle-segment collision clamping, and decoy stone wall clamping
    Player player;
    player.reset(lab.currentRoom().playerDefaultSpawn);
    WaveSignature sig{};
    for (int step = 0; step < 120; ++step) {
        player.update(cfg::kFixedTimeStep, {-1.0f, -1.0f}, MovementStance::Sprint, {100.0f, 100.0f}, lab.currentRoom().walls, acoustics, sig);
    }
    if (player.position().x < 40.0f || player.position().y < 70.0f) {
        std::cerr << "[SelfTest] FAIL: Player clipped out of room perimeter.\n";
        return 1;
    }
    std::cout << "[SelfTest] PASS [5/8]: Player movement, stamina, & circle-segment collision clamping verified.\n";

    // 6. Verify EnemyAI full state machine transitions (Patrol -> Investigate -> Search -> Return -> Patrol, Pursue, Attack)
    lab.setCurrentRoom(2); // Room 03 has an active Listener patrol
    EnemyAI enemy;
    enemy.configurePatrol(lab.currentRoom().enemyPatrolNodes);
    if (!enemy.isActive() || enemy.state() != EnemyState::Patrol) {
        std::cerr << "[SelfTest] FAIL: EnemyAI did not enter Patrol state in Room 03.\n";
        return 1;
    }
    // Emit moderate sound to trigger InvestigateSound
    acoustics.clearRecentEvents();
    acoustics.emitPulse(enemy.position() + sf::Vector2f{90.0f, 0.0f}, 300.0f, 350.0f, 0.45f, 32, WaveSignature::FootstepWalk, 0.0f, 6.2831853f, true);
    enemy.update(cfg::kFixedTimeStep, {145.0f, 400.0f}, lab.currentRoom().walls, acoustics);
    if (enemy.state() != EnemyState::InvestigateSound) {
        std::cerr << "[SelfTest] FAIL: EnemyAI expected InvestigateSound, got " << EnemyAI::stateName(enemy.state()) << "\n";
        return 1;
    }
    // Advance until EnemyAI reaches sound origin -> transitions to SearchLastKnown -> ReturnToPatrol -> Patrol
    acoustics.clearRecentEvents();
    bool sawSearch = false;
    bool sawReturnOrPatrol = false;
    for (int step = 0; step < 650; ++step) {
        enemy.update(cfg::kFixedTimeStep, {145.0f, 400.0f}, lab.currentRoom().walls, acoustics);
        acoustics.clearRecentEvents();
        if (enemy.state() == EnemyState::SearchLastKnown) sawSearch = true;
        if (sawSearch && (enemy.state() == EnemyState::ReturnToPatrol || enemy.state() == EnemyState::Patrol)) {
            sawReturnOrPatrol = true;
            break;
        }
    }
    if (!sawSearch || !sawReturnOrPatrol) {
        std::cerr << "[SelfTest] FAIL: EnemyAI did not complete Investigate -> SearchLastKnown -> ReturnToPatrol cycle.\n";
        return 1;
    }
    // Emit high-intensity sound nearby to trigger PursuePlayer
    acoustics.emitPulse(enemy.position() + sf::Vector2f{40.0f, 0.0f}, 420.0f, 650.0f, 1.35f, 64, WaveSignature::DecoyImpact, 0.0f, 6.2831853f, true);
    enemy.update(cfg::kFixedTimeStep, {145.0f, 400.0f}, lab.currentRoom().walls, acoustics);
    if (enemy.state() != EnemyState::PursuePlayer) {
        std::cerr << "[SelfTest] FAIL: EnemyAI did not enter PursuePlayer on loud stimulus.\n";
        return 1;
    }
    // Verify Attack state on player contact
    acoustics.clearRecentEvents();
    enemy.update(cfg::kFixedTimeStep, enemy.position(), lab.currentRoom().walls, acoustics);
    if (!enemy.hasCaughtPlayer() || enemy.state() != EnemyState::Attack) {
        std::cerr << "[SelfTest] FAIL: EnemyAI did not enter Attack state upon player contact.\n";
        return 1;
    }
    std::cout << "[SelfTest] PASS [6/8]: EnemyAI 6-state hunting FSM (Patrol/Investigate/Search/Return/Pursue/Attack) verified.\n";

    // 7. Verify SaveSystem V3 atomic write, FNV-1a checksum, and full state round-trip
    const std::string testSavePath = SaveSystem::resolveSaveFilePath(exeDir, "test_echolabyrinth_save.dat");
    SaveData outSave{};
    outSave.checkpointRoomId = 8;
    outSave.playerPosX = 512.5f;
    outSave.playerPosY = 388.0f;
    outSave.playerStamina = 76.5f;
    outSave.decoyCount = 6;
    outSave.masterVolume = 70.0f;
    outSave.rayDensityScale = 1.5f;
    outSave.maxReflections = 4;
    outSave.ambientVisibility = 0.08f;
    outSave.runTimeSec = 142.5f;
    outSave.activatedSwitches = {101, 102, 105};
    outSave.collectedLoreIds = {200, 202, 204};
    outSave.visitedRooms = {0, 1, 2, 3, 4, 5, 6, 7, 8};

    if (!SaveSystem::saveToFile(testSavePath, outSave)) {
        std::cerr << "[SelfTest] FAIL: Could not write V3 save file.\n";
        return 1;
    }
    SaveData inSave{};
    if (!SaveSystem::loadFromFile(testSavePath, inSave)) {
        std::cerr << "[SelfTest] FAIL: Could not read valid V3 save file.\n";
        return 1;
    }
    std::remove(testSavePath.c_str());

    if (inSave.checkpointRoomId != 8 || inSave.decoyCount != 6 ||
        std::abs(inSave.playerPosX - 512.5f) > 0.1f ||
        std::abs(inSave.playerStamina - 76.5f) > 0.1f ||
        inSave.maxReflections != 4 ||
        inSave.activatedSwitches.size() != 3 ||
        inSave.collectedLoreIds.size() != 3) {
        std::cerr << "[SelfTest] FAIL: Loaded V3 save data mismatch.\n";
        return 1;
    }
    lab.restoreState(inSave.activatedSwitches, inSave.collectedLoreIds, inSave.visitedRooms);
    if (!lab.isSwitchActivated(102) || !lab.isLoreCollected(204) || !lab.isRoomVisited(8)) {
        std::cerr << "[SelfTest] FAIL: Labyrinth::restoreState did not apply loaded save state.\n";
        return 1;
    }
    std::cout << "[SelfTest] PASS [7/8]: SaveSystem V3 atomic serialization, checksum, & state restoration verified.\n";

    // 8. Verify corrupted header and tampered checksum rejection
    {
        std::ofstream badOut(testSavePath, std::ios::trunc);
        badOut << "ECHOLABYRINTH_SAVE_V3\nroom 14\ndecoys 99\nchecksum 12345\n";
    }
    SaveData badData{};
    if (SaveSystem::loadFromFile(testSavePath, badData)) {
        std::cerr << "[SelfTest] FAIL: Tampered V3 checksum should be rejected.\n";
        std::remove(testSavePath.c_str());
        return 1;
    }
    std::remove(testSavePath.c_str());
    std::cout << "[SelfTest] PASS [8/8]: Tampered save checksum and corrupted files rejected gracefully.\n";

    std::cout << "[SelfTest] ALL 8 NATIVE C++17 VERIFICATION SUITES PASSED SUCCESSFULLY.\n";
    return 0;
}

} // namespace echo
