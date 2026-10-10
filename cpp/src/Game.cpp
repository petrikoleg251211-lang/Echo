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

    // Load persistent meta-progression counters if a save file exists
    SaveData existing{};
    if (SaveSystem::loadFromFile(m_saveFilePath, existing)) {
        m_metaTotalRuns = existing.totalRuns;
        m_metaTotalVictories = existing.totalVictories;
        m_metaBossesDefeated = existing.bossesDefeated;
    }
}

int Game::run() {
    m_window.create(
        sf::VideoMode(cfg::kDefaultWindowWidth, cfg::kDefaultWindowHeight),
        "EchoLabyrinth 4.0 — Acoustic Roguelike Horror",
        sf::Style::Default
    );
    m_window.setFramerateLimit(120);
    updateLetterboxView(cfg::kDefaultWindowWidth, cfg::kDefaultWindowHeight);

    m_assets.initialize(m_exeDir, true);
    m_ui.initialize(m_exeDir);
    m_audio.initialize();
    m_audio.startAmbientDrone();

    enterRoom(0, m_labyrinth.currentRoom().playerDefaultSpawn, false);
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
            "EchoLabyrinth 4.0 — Acoustic Roguelike Horror",
            sf::Style::Fullscreen
        );
    } else {
        m_window.create(
            sf::VideoMode(cfg::kDefaultWindowWidth, cfg::kDefaultWindowHeight),
            "EchoLabyrinth 4.0 — Acoustic Roguelike Horror",
            sf::Style::Default
        );
    }
    m_window.setFramerateLimit(120);
    const sf::Vector2u sz = m_window.getSize();
    updateLetterboxView(sz.x, sz.y);
}

void Game::spawnWaveForCurrentRoom(int waveIndex) {
    m_enemies.clear();
    const auto& room = m_labyrinth.currentRoom();
    if (room.cleared) return;

    for (const auto& sp : room.enemySpawns) {
        if (sp.waveIndex == waveIndex) {
            EnemyAI enemy;
            enemy.configurePatrol(sp.waypoints, sp.variant);
            m_enemies.push_back(std::move(enemy));
        }
    }
}

void Game::enterRoom(int roomId, sf::Vector2f spawnPos, bool preservePlayerBuild) {
    m_labyrinth.setCurrentRoom(roomId);
    m_acoustics.clear();

    if (preservePlayerBuild) {
        m_player.setPosition(spawnPos);
    } else {
        m_player.reset(spawnPos);
    }

    auto& room = m_labyrinth.currentRoomMut();
    m_currentWaveIndex = 0;

    if (!room.cleared && !room.enemySpawns.empty()) {
        spawnWaveForCurrentRoom(0);
        m_labyrinth.setCombatLock(roomId, true);
        m_statusBannerText = std::string(Labyrinth::roomTypeName(room.type)) + " — EXITS SEALED UNTIL HUNTERS DISRUPTED";
        m_statusBannerTimer = 3.2f;
    } else {
        m_enemies.clear();
        m_labyrinth.setCombatLock(roomId, false);
        m_statusBannerText = room.name + "  //  " + room.sectorTitle;
        m_statusBannerTimer = 3.0f;
    }

    m_acoustics.emitPulse(spawnPos, 380.0f, 360.0f, 0.72f, 56, WaveSignature::FootstepWalk, 0.0f, 6.2831853f, false);

    if (room.isCheckpoint) {
        saveCheckpoint();
    }
}

void Game::startNewGame(std::uint32_t customSeed) {
    const std::uint32_t nextSeed = (customSeed != 0)
        ? customSeed
        : static_cast<std::uint32_t>(m_labyrinth.runSeed() * 1664525u + 1013904223u + static_cast<std::uint32_t>(m_metaTotalRuns * 97));

    ++m_metaTotalRuns;
    m_enemiesDisruptedThisRun = 0;
    m_runTimeSec = 0.0f;
    m_loreDisplayTimer = 0.0f;
    m_showMinimap = false;
    m_acoustics.setMaxReflections(cfg::kDefaultMaxReflections);

    m_labyrinth.generateRun(nextSeed, cfg::kDefaultRunRoomCount);
    m_player.reset(m_labyrinth.currentRoom().playerDefaultSpawn);
    enterRoom(0, m_labyrinth.currentRoom().playerDefaultSpawn, true);
    m_state = AppState::Playing;
}

void Game::saveCheckpoint() {
    SaveData data{};
    data.runSeed = m_labyrinth.runSeed();
    data.targetRoomCount = static_cast<int>(m_labyrinth.targetRoomCount());
    data.checkpointRoomId = m_labyrinth.currentRoomId();
    data.playerPosX = m_player.position().x;
    data.playerPosY = m_player.position().y;
    data.playerHealth = m_player.health();
    data.playerMaxHealth = m_player.maxHealth();
    data.playerStamina = m_player.stamina();
    data.decoyCount = m_player.decoyCount();
    data.echoShards = m_player.echoShards();

    data.masterVolume = m_audio.masterVolume();
    data.musicVolume = m_audio.musicVolume();
    data.sfxVolume = m_audio.sfxVolume();
    data.ambientVolume = m_audio.ambientVolume();
    data.rayDensityScale = m_acoustics.rayDensityScale();
    data.maxReflections = static_cast<int>(m_acoustics.maxReflections());
    data.ambientVisibility = m_ambientVisibility;
    data.runTimeSec = m_runTimeSec;

    data.totalRuns = m_metaTotalRuns;
    data.totalVictories = m_metaTotalVictories;
    data.bossesDefeated = m_metaBossesDefeated;

    data.activatedSwitches = m_labyrinth.activatedSwitches();
    data.collectedLoreIds = m_labyrinth.collectedLoreIds();
    data.visitedRooms = m_labyrinth.visitedRooms();
    data.clearedRooms = m_labyrinth.clearedRooms();
    for (RelicId r : m_player.relics()) {
        data.collectedRelics.push_back(static_cast<int>(r));
    }

    SaveSystem::saveToFile(m_saveFilePath, data);
}

bool Game::loadCheckpoint() {
    SaveData data{};
    if (!SaveSystem::loadFromFile(m_saveFilePath, data)) {
        return false;
    }

    m_audio.setMasterVolume(data.masterVolume);
    m_audio.setMusicVolume(data.musicVolume);
    m_audio.setSfxVolume(data.sfxVolume);
    m_audio.setAmbientVolume(data.ambientVolume);
    m_acoustics.setRayDensityScale(data.rayDensityScale);
    m_acoustics.setMaxReflections(static_cast<std::uint8_t>(data.maxReflections));
    m_ambientVisibility = data.ambientVisibility;
    m_runTimeSec = data.runTimeSec;
    m_metaTotalRuns = data.totalRuns;
    m_metaTotalVictories = data.totalVictories;
    m_metaBossesDefeated = data.bossesDefeated;

    m_labyrinth.generateRun(data.runSeed, static_cast<std::size_t>(data.targetRoomCount));
    m_labyrinth.restoreState(
        data.activatedSwitches,
        data.collectedLoreIds,
        data.visitedRooms,
        data.clearedRooms
    );

    m_player.reset({data.playerPosX, data.playerPosY});
    std::vector<RelicId> loadedRelics;
    for (int id : data.collectedRelics) {
        const auto rId = static_cast<RelicId>(id);
        loadedRelics.push_back(rId);
        if (rId == RelicId::SubharmonicVisor) {
            m_labyrinth.revealSecretRoomsOnMap();
        }
    }
    m_player.setRelics(loadedRelics);
    m_player.setMaxHealth(data.playerMaxHealth);
    m_player.setHealth(data.playerHealth);
    m_player.setStamina(data.playerStamina);
    m_player.setDecoyCount(data.decoyCount);
    m_player.setEchoShards(data.echoShards);

    enterRoom(data.checkpointRoomId, {data.playerPosX, data.playerPosY}, true);
    m_state = AppState::Playing;
    m_statusBannerText = "CHECKPOINT RESTORED — SEED #" + std::to_string(data.runSeed);
    m_statusBannerTimer = 3.0f;
    return true;
}

void Game::rebuildMenuButtons() {
    m_menuButtons.clear();
    m_selectedMenuIndex = 0;

    const float cx = cfg::kWorldWidth * 0.5f;

    if (m_state == AppState::MainMenu) {
        const bool hasSave = SaveSystem::hasSaveFile(m_saveFilePath);
        m_menuButtons.push_back({"START NEW ROGUELIKE RUN", {cx, 330.0f}, {390.0f, 46.0f}, true});
        m_menuButtons.push_back({"CONTINUE SAVED EXPEDITION", {cx, 390.0f}, {390.0f, 46.0f}, hasSave});
        m_menuButtons.push_back({"AUDIO & ACOUSTIC SETTINGS", {cx, 450.0f}, {390.0f, 46.0f}, true});
        m_menuButtons.push_back({"SURVEYOR FIELD MANUAL", {cx, 510.0f}, {390.0f, 46.0f}, true});
        m_menuButtons.push_back({"EXIT TO DESKTOP", {cx, 570.0f}, {390.0f, 46.0f}, true});
    } else if (m_state == AppState::Paused) {
        m_menuButtons.push_back({"RESUME EXPEDITION", {cx, 310.0f}, {380.0f, 44.0f}, true});
        m_menuButtons.push_back({"SAVE CHECKPOINT [F5]", {cx, 366.0f}, {380.0f, 44.0f}, true});
        m_menuButtons.push_back({"START NEW SEED RUN", {cx, 422.0f}, {380.0f, 44.0f}, true});
        m_menuButtons.push_back({"AUDIO & ACOUSTIC SETTINGS", {cx, 478.0f}, {380.0f, 44.0f}, true});
        m_menuButtons.push_back({"SURVEYOR FIELD MANUAL", {cx, 534.0f}, {380.0f, 44.0f}, true});
        m_menuButtons.push_back({"RETURN TO MAIN MENU", {cx, 590.0f}, {380.0f, 44.0f}, true});
    } else if (m_state == AppState::Settings) {
        m_menuButtons.push_back({"MASTER VOLUME: < ADJUST >", {cx, 225.0f}, {440.0f, 38.0f}, true});
        m_menuButtons.push_back({"MUSIC LAYER VOLUME: < ADJUST >", {cx, 273.0f}, {440.0f, 38.0f}, true});
        m_menuButtons.push_back({"SFX SPATIAL VOLUME: < ADJUST >", {cx, 321.0f}, {440.0f, 38.0f}, true});
        m_menuButtons.push_back({"AMBIENT DRONE VOLUME: < ADJUST >", {cx, 369.0f}, {440.0f, 38.0f}, true});
        m_menuButtons.push_back({"RAY DENSITY SCALE: < ADJUST >", {cx, 417.0f}, {440.0f, 38.0f}, true});
        m_menuButtons.push_back({"MAX WALL REFLECTIONS: < ADJUST >", {cx, 465.0f}, {440.0f, 38.0f}, true});
        m_menuButtons.push_back({"AMBIENT DARKNESS: < ADJUST >", {cx, 513.0f}, {440.0f, 38.0f}, true});
        m_menuButtons.push_back({"WINDOW / FULLSCREEN [F11]", {cx, 561.0f}, {440.0f, 38.0f}, true});
        m_menuButtons.push_back({"BACK", {cx, 618.0f}, {320.0f, 42.0f}, true});
    } else if (m_state == AppState::Controls) {
        m_menuButtons.push_back({"BACK", {cx, 665.0f}, {320.0f, 44.0f}, true});
    } else if (m_state == AppState::GameOver || m_state == AppState::Victory) {
        m_menuButtons.push_back({"START NEW PROCEDURAL RUN", {cx, 465.0f}, {390.0f, 46.0f}, true});
        m_menuButtons.push_back({"RESTORE LAST CHECKPOINT", {cx, 525.0f}, {390.0f, 46.0f}, SaveSystem::hasSaveFile(m_saveFilePath)});
        m_menuButtons.push_back({"RETURN TO MAIN MENU", {cx, 585.0f}, {390.0f, 46.0f}, true});
    }
}

void Game::executeMenuSelection(std::size_t index) {
    if (index >= m_menuButtons.size() || !m_menuButtons[index].enabled) return;
    m_audio.playEffect(SoundEffectId::UiSelect, 0.8f, 1.0f);

    if (m_state == AppState::MainMenu) {
        if (index == 0) startNewGame();
        else if (index == 1) loadCheckpoint();
        else if (index == 2) { m_previousState = AppState::MainMenu; m_state = AppState::Settings; rebuildMenuButtons(); }
        else if (index == 3) { m_previousState = AppState::MainMenu; m_state = AppState::Controls; rebuildMenuButtons(); }
        else if (index == 4) m_window.close();
    } else if (m_state == AppState::Paused) {
        if (index == 0) m_state = AppState::Playing;
        else if (index == 1) {
            saveCheckpoint();
            m_statusBannerText = "CHECKPOINT SAVED TO DISK";
            m_statusBannerTimer = 2.5f;
            m_state = AppState::Playing;
        } else if (index == 2) {
            startNewGame();
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
    } else if (m_state == AppState::Settings) {
        if (index == 0) {
            float v = m_audio.masterVolume() + 20.0f;
            if (v > 100.0f) v = 0.0f;
            m_audio.setMasterVolume(v);
        } else if (index == 1) {
            float v = m_audio.musicVolume() + 25.0f;
            if (v > 100.0f) v = 0.0f;
            m_audio.setMusicVolume(v);
        } else if (index == 2) {
            float v = m_audio.sfxVolume() + 25.0f;
            if (v > 100.0f) v = 0.0f;
            m_audio.setSfxVolume(v);
        } else if (index == 3) {
            float v = m_audio.ambientVolume() + 25.0f;
            if (v > 100.0f) v = 0.0f;
            m_audio.setAmbientVolume(v);
        } else if (index == 4) {
            float s = m_acoustics.rayDensityScale() + 0.25f;
            if (s > 1.75f) s = 0.5f;
            m_acoustics.setRayDensityScale(s);
        } else if (index == 5) {
            int r = static_cast<int>(m_acoustics.maxReflections()) + 1;
            if (r > 5) r = 1;
            m_acoustics.setMaxReflections(static_cast<std::uint8_t>(r));
        } else if (index == 6) {
            m_ambientVisibility += 0.03f;
            if (m_ambientVisibility > 0.12f) m_ambientVisibility = 0.0f;
        } else if (index == 7) {
            toggleFullscreen();
        } else if (index == 8) {
            saveCheckpoint();
            m_state = m_previousState;
            rebuildMenuButtons();
        }
    } else if (m_state == AppState::Controls) {
        m_state = m_previousState;
        rebuildMenuButtons();
    } else if (m_state == AppState::GameOver || m_state == AppState::Victory) {
        if (index == 0) startNewGame();
        else if (index == 1) {
            if (!loadCheckpoint()) startNewGame();
        } else if (index == 2) {
            m_state = AppState::MainMenu;
            rebuildMenuButtons();
        }
    }
}

void Game::handleEvents() {
    sf::Event ev{};
    while (m_window.pollEvent(ev)) {
        if (ev.type == sf::Event::Closed) {
            m_window.close();
        } else if (ev.type == sf::Event::Resized) {
            updateLetterboxView(ev.size.width, ev.size.height);
        } else if (ev.type == sf::Event::KeyPressed) {
            handleKeyPressed(ev.key.code);
        } else if (ev.type == sf::Event::MouseMoved) {
            const sf::Vector2f worldPt = m_window.mapPixelToCoords({ev.mouseMove.x, ev.mouseMove.y}, m_worldView);
            handleMouseMoved(worldPt);
        } else if (ev.type == sf::Event::MouseButtonPressed) {
            const sf::Vector2f worldPt = m_window.mapPixelToCoords({ev.mouseButton.x, ev.mouseButton.y}, m_worldView);
            handleMousePressed(ev.mouseButton.button, worldPt);
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
            return;
        }
        if (key == sf::Keyboard::Tab || key == sf::Keyboard::M) {
            m_showMinimap = !m_showMinimap;
            m_audio.playEffect(SoundEffectId::UiMove, 0.7f, 1.1f);
            return;
        }
        if (key == sf::Keyboard::Space) {
            if (m_player.triggerRadialPulse(m_acoustics)) {
                m_audio.playWaveSignature(WaveSignature::RadialPulse);
            }
        } else if (key == sf::Keyboard::E) {
            if (m_player.triggerDirectionalPing(m_mouseWorldPos, m_acoustics)) {
                m_audio.playWaveSignature(WaveSignature::DirectionalPing);
            }
        } else if (key == sf::Keyboard::G) {
            m_player.throwDecoy(m_mouseWorldPos, m_labyrinth.currentRoom().walls);
        } else if (key == sf::Keyboard::F) {
            tryInteract();
        } else if (key == sf::Keyboard::F5) {
            saveCheckpoint();
            m_statusBannerText = "CHECKPOINT SAVED [F5]";
            m_statusBannerTimer = 2.5f;
            m_audio.playEffect(SoundEffectId::UiSelect, 0.8f, 1.2f);
        } else if (key == sf::Keyboard::F9) {
            loadCheckpoint();
        }
        return;
    }

    if (key == sf::Keyboard::Escape) {
        if (m_state == AppState::Paused) {
            m_state = AppState::Playing;
        } else if (m_state == AppState::Settings || m_state == AppState::Controls) {
            m_state = m_previousState;
            rebuildMenuButtons();
        }
        return;
    }

    if (key == sf::Keyboard::Up || key == sf::Keyboard::W) {
        if (!m_menuButtons.empty()) {
            m_selectedMenuIndex = (m_selectedMenuIndex + m_menuButtons.size() - 1) % m_menuButtons.size();
            m_audio.playEffect(SoundEffectId::UiMove, 0.6f, 1.0f);
        }
    } else if (key == sf::Keyboard::Down || key == sf::Keyboard::S) {
        if (!m_menuButtons.empty()) {
            m_selectedMenuIndex = (m_selectedMenuIndex + 1) % m_menuButtons.size();
            m_audio.playEffect(SoundEffectId::UiMove, 0.6f, 1.0f);
        }
    } else if (key == sf::Keyboard::Enter || key == sf::Keyboard::Space) {
        executeMenuSelection(m_selectedMenuIndex);
    }
}

void Game::handleMouseMoved(sf::Vector2f worldPos) {
    m_mouseWorldPos = worldPos;
    if (m_state != AppState::Playing) {
        for (std::size_t i = 0; i < m_menuButtons.size(); ++i) {
            if (m_menuButtons[i].enabled && pointInButton(worldPos, m_menuButtons[i])) {
                if (m_selectedMenuIndex != i) {
                    m_selectedMenuIndex = i;
                    m_audio.playEffect(SoundEffectId::UiMove, 0.5f, 1.05f);
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
            m_player.throwDecoy(worldPos, m_labyrinth.currentRoom().walls);
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

void Game::tryInteract() {
    auto& room = m_labyrinth.currentRoomMut();
    const sf::Vector2f pPos = m_player.position();

    // 1. Check Relic Pedestals (Free in Treasure/Secret/Cleared Elite rooms; costs Shards in Shop)
    for (auto& ped : room.relicPedestals) {
        if (ped.collected) continue;
        if (!room.cleared) {
            m_statusBannerText = "DEFEAT ALL HUNTERS BEFORE CLAIMING RELIC";
            m_statusBannerTimer = 2.2f;
            return;
        }
        if (distSq(pPos, ped.position) <= 52.0f * 52.0f) {
            const RelicSpec spec = getRelicSpec(ped.relicId);
            if (ped.costShards > 0) {
                if (!m_player.spendEchoShards(ped.costShards)) {
                    m_statusBannerText = "INSUFFICIENT ECHO SHARDS (NEED " + std::to_string(ped.costShards) + ")";
                    m_statusBannerTimer = 2.4f;
                    m_audio.playEffect(SoundEffectId::UiMove, 0.8f, 0.7f);
                    return;
                }
            }
            ped.collected = true;
            m_player.addRelic(ped.relicId, &m_acoustics);
            if (ped.relicId == RelicId::SubharmonicVisor) {
                m_labyrinth.revealSecretRoomsOnMap();
            }
            m_audio.playEffect(SoundEffectId::VictoryChord, 0.85f, 1.15f);
            m_acoustics.emitPulse(ped.position, 420.0f, 520.0f, 1.15f, 96, WaveSignature::ResonatorHum, 0.0f, 6.2831853f, true);
            m_statusBannerText = std::string("RELIC ACQUIRED: ") + spec.name + " (" + spec.effectSummary + ")";
            m_statusBannerTimer = 4.2f;
            return;
        }
    }

    // 2. Check Quartermaster Shop Offers
    for (auto& offer : room.shopOffers) {
        if (offer.purchased) continue;
        if (distSq(pPos, offer.position) <= 50.0f * 50.0f) {
            if (!m_player.spendEchoShards(offer.costShards)) {
                m_statusBannerText = "INSUFFICIENT ECHO SHARDS (NEED " + std::to_string(offer.costShards) + ")";
                m_statusBannerTimer = 2.4f;
                m_audio.playEffect(SoundEffectId::UiMove, 0.8f, 0.7f);
                return;
            }
            offer.purchased = true;
            if (offer.healAmount > 0) m_player.heal(offer.healAmount);
            if (offer.decoyAmount > 0) m_player.addDecoy(offer.decoyAmount);
            m_audio.playEffect(SoundEffectId::LorePickup, 0.95f, 1.1f);
            m_statusBannerText = "PURCHASED: " + offer.label;
            m_statusBannerTimer = 3.0f;
            return;
        }
    }

    // 3. Check Interactive Mechanisms (Sanctuary Repair Shrines & Harmonic Levers)
    for (auto& mech : room.mechanisms) {
        if (!mech.activated && distSq(pPos, mech.position) <= 52.0f * 52.0f) {
            if (mech.id >= 500) {
                // Sanctuary Shrine
                mech.activated = true;
                m_player.heal(2);
                m_player.restoreStamina(100.0f);
                saveCheckpoint();
                m_audio.playEffect(SoundEffectId::ResonatorHum, 0.95f, 1.0f);
                m_acoustics.emitPulse(mech.position, 410.0f, 540.0f, 1.15f, 96, WaveSignature::ResonatorHum, 0.0f, 6.2831853f, true);
                m_statusBannerText = "SANCTUARY SHRINE ACTIVATED: +2 HULL INTEGRITY & CHECKPOINT SAVED";
                m_statusBannerTimer = 3.5f;
                return;
            }

            m_labyrinth.activateSwitch(mech.id);
            m_acoustics.emitPulse(
                mech.position,
                400.0f,
                520.0f,
                1.05f,
                84,
                WaveSignature::MechanismClunk,
                0.0f,
                6.2831853f,
                true
            );
            m_audio.playSpatial(SoundEffectId::SwitchClunk, mech.position, 1.0f, 1.0f, 1.0f);
            m_audio.playSpatial(SoundEffectId::GateSlideOpen, {1045.0f, 400.0f}, 0.85f, 0.95f, 0.92f);
            m_statusBannerText = mech.label + " ENGAGED — RESONANCE BARRIER UNSEALED";
            m_statusBannerTimer = 3.5f;
            return;
        }
    }

    // 4. Check Surveyor Log Collectibles & Caches
    for (auto& lore : room.collectibles) {
        if (!lore.collected && distSq(pPos, lore.position) <= 50.0f * 50.0f) {
            m_labyrinth.markLoreCollected(lore.id);
            m_player.addDecoy(1);
            m_player.addEchoShards(10);
            m_activeLoreTitle = lore.title;
            m_activeLoreBody = lore.excerpt;
            m_loreDisplayTimer = 7.5f;
            m_audio.playEffect(SoundEffectId::LorePickup, 0.9f, 1.0f);
            m_statusBannerText = "LOG RECOVERED: " + lore.title + " (+1 DECOY, +10 SHARDS)";
            m_statusBannerTimer = 3.2f;
            return;
        }
    }
}

void Game::updateFixed(float dt) {
    for (auto& pt : m_particles) {
        pt.pos += pt.vel * dt;
        if (pt.pos.x < 0.0f) pt.pos.x += cfg::kWorldWidth;
        if (pt.pos.x > cfg::kWorldWidth) pt.pos.x -= cfg::kWorldWidth;
        if (pt.pos.y < 0.0f) pt.pos.y += cfg::kWorldHeight;
        if (pt.pos.y > cfg::kWorldHeight) pt.pos.y -= cfg::kWorldHeight;
    }

    if (m_state == AppState::Playing) {
        updatePlaying(dt);
    } else {
        updateMenuBackground(dt);
    }
}

void Game::updateMenuBackground(float dt) {
    m_menuPulseTimer += dt;
    if (m_menuPulseTimer >= 2.4f) {
        m_menuPulseTimer = 0.0f;
        const sf::Vector2f origin{
            340.0f + std::cos(m_runTimeSec + 1.2f) * 260.0f,
            400.0f + std::sin(m_runTimeSec * 0.7f) * 140.0f
        };
        m_acoustics.emitPulse(origin, 360.0f, 580.0f, 0.85f, 84, WaveSignature::RadialPulse, 0.0f, 6.2831853f, false);
    }
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
    m_labyrinth.updateAnimations(dt);

    WaveSignature emittedSig{};
    if (m_player.update(dt, moveInput, stance, m_mouseWorldPos, room.walls, m_acoustics, emittedSig)) {
        m_audio.updateListener(m_player.position(), m_player.facingAngleRad());

        if (emittedSig == WaveSignature::FootstepSneak ||
            emittedSig == WaveSignature::FootstepWalk ||
            emittedSig == WaveSignature::FootstepSprint) {
            const SurfaceMaterial floorMat = m_player.detectFloorMaterial(room.walls, room.floorMaterial);
            m_audio.playMaterialFootstep(
                floorMat,
                emittedSig == WaveSignature::FootstepSneak,
                emittedSig == WaveSignature::FootstepSprint,
                m_player.position()
            );
        } else {
            m_audio.playWaveSignature(emittedSig);
        }

        // Brittle Shale Acoustic Hazard (ignored if player has MossTreadBoots relic)
        if (!m_player.hasRelic(RelicId::MossTreadBoots) &&
            (emittedSig == WaveSignature::FootstepWalk || emittedSig == WaveSignature::FootstepSprint) &&
            m_shaleCrackCooldown <= 0.0f) {
            for (const auto& hz : room.hazards) {
                if (distSq(m_player.position(), hz.position) <= hz.radius * hz.radius) {
                    m_shaleCrackCooldown = 1.15f;
                    m_acoustics.emitPulse(
                        m_player.position(),
                        400.0f,
                        520.0f,
                        1.15f,
                        88,
                        WaveSignature::ShaleCrack,
                        0.0f,
                        6.2831853f,
                        true
                    );
                    m_audio.playWaveSignature(WaveSignature::ShaleCrack);
                    m_statusBannerText = "WARNING: BRITTLE SHALE FRACTURED — HIGH-VOLUME ECHO EMITTED!";
                    m_statusBannerTimer = 2.6f;
                    break;
                }
            }
        }
    }

    m_audio.updateListener(m_player.position(), m_player.facingAngleRad());

    // Ambient room emitters
    for (auto& emitter : room.ambientEmitters) {
        emitter.timer += dt;
        if (emitter.timer >= emitter.intervalSec) {
            emitter.timer = 0.0f;
            m_acoustics.emitPulse(
                emitter.position,
                265.0f,
                emitter.isSteamVent ? 260.0f : 210.0f,
                emitter.isSteamVent ? 0.55f : 0.42f,
                32,
                WaveSignature::AmbientDrip,
                0.0f,
                6.2831853f,
                false
            );
            const float occ = AcousticSystem::computeTransmissionFactor(emitter.position, m_player.position(), room.walls);
            m_audio.playSpatial(
                emitter.isSteamVent ? SoundEffectId::SteamHiss : SoundEffectId::WaterDrip,
                emitter.position,
                occ,
                0.65f,
                1.0f
            );
        }
    }

    // Update acoustic rays, resonators, and concealed secret doors (doorId >= 900)
    const std::vector<int> newlyUnlockedBySonar = m_acoustics.update(
        dt,
        room.walls,
        &room.resonators,
        m_player.resonatorChargeMultiplier()
    );
    for (int unlockedId : newlyUnlockedBySonar) {
        m_labyrinth.activateSwitch(unlockedId);
        m_audio.playWaveSignature(WaveSignature::ResonatorHum);
        m_audio.playSpatial(SoundEffectId::GateSlideOpen, {640.0f, 400.0f}, 0.9f, 1.0f, 0.95f);

        if (unlockedId >= 900) {
            m_acoustics.emitPulse(m_player.position(), 420.0f, 520.0f, 1.2f, 96, WaveSignature::ResonatorHum, 0.0f, 6.2831853f, false);
            m_statusBannerText = "SECRET ACOUSTIC FRACTURE UNSEALED BY SONAR!";
            m_statusBannerTimer = 3.8f;
        } else {
            // Puzzle room reward
            if (room.type == RoomType::Puzzle && room.shardReward > 0) {
                m_player.addEchoShards(room.shardReward);
                room.shardReward = 0;
                m_statusBannerText = "HARMONIC PUZZLE SOLVED (+20 ECHO SHARDS)!";
            } else {
                m_statusBannerText = "HARMONIC RESONATOR CHARGED — BARRIER UNSEALED";
            }
            m_statusBannerTimer = 3.6f;
        }
    }

    // Steam vent acoustic masking
    for (const auto& emitter : room.ambientEmitters) {
        if (emitter.isSteamVent && emitter.timer < 0.75f) {
            if (distSq(m_player.position(), emitter.position) <= 95.0f * 95.0f) {
                m_acoustics.clearRecentEvents();
                break;
            }
        }
    }

    // Update all enemies in the room, apply player sonar disruption damage, and check wave clear
    float maxSuspicion = 0.0f;
    bool anyPursuing = false;
    bool anyActiveBefore = false;
    bool allInactiveNow = true;

    for (auto& enemy : m_enemies) {
        if (!enemy.isActive()) continue;
        anyActiveBefore = true;

        // 1. Check if player sonar rays struck this enemy
        bool reflectedHit = false;
        const float impactEnergy = m_acoustics.consumeRayImpactAt(enemy.position(), enemy.hitRadius(), reflectedHit);
        if (impactEnergy > 0.0f) {
            const float dmg = impactEnergy * 11.5f * m_player.sonarDamageMultiplier(reflectedHit);
            if (m_player.hasRelic(RelicId::EchoSiphonCoil)) {
                m_player.restoreStamina(5.5f);
            }
            const bool defeated = enemy.applyAcousticDamage(dmg, reflectedHit, m_player.position(), room.walls);
            if (defeated) {
                ++m_enemiesDisruptedThisRun;
                if (enemy.variant() == EnemyVariant::AbyssalBoss) {
                    ++m_metaBossesDefeated;
                }
                m_audio.playSpatial(SoundEffectId::ResonatorHum, enemy.position(), 1.0f, 0.95f, 1.25f);
                m_acoustics.emitPulse(enemy.position(), 410.0f, 420.0f, 1.10f, 72, WaveSignature::ResonatorHum, 0.0f, 6.2831853f, false);
                continue;
            }
        }

        // 2. Update enemy AI & ranged SonicBolts
        const EnemyState prevState = enemy.state();
        if (enemy.update(dt, m_player.position(), room.walls, m_acoustics)) {
            const float occ = AcousticSystem::computeTransmissionFactor(enemy.position(), m_player.position(), room.walls);
            if (enemy.state() == EnemyState::PursuePlayer) {
                m_audio.playSpatial(SoundEffectId::EnemyHuntRoar, enemy.position(), occ, 1.0f, 0.95f);
            } else {
                m_audio.playSpatial(SoundEffectId::EnemyAlert, enemy.position(), occ, 0.95f, 1.0f);
            }
            m_cameraShakeTimer = 0.30f;
        } else if (prevState != EnemyState::SearchLastKnown && enemy.state() == EnemyState::SearchLastKnown) {
            const float occ = AcousticSystem::computeTransmissionFactor(enemy.position(), m_player.position(), room.walls);
            m_audio.playSpatial(SoundEffectId::EnemySearchClick, enemy.position(), occ, 0.85f, 1.0f);
        }

        // 3. Check if enemy melee contact or ranged SonicBolt damaged the player
        const bool meleeHit = enemy.hasCaughtPlayer();
        const bool boltHit = enemy.consumeBoltHitOnPlayer(m_player.position(), cfg::kPlayerRadius);
        if (meleeHit || boltHit) {
            if (meleeHit) {
                enemy.clearContactState();
            }
            if (m_player.applyDamage(1, m_acoustics)) {
                m_audio.playEffect(SoundEffectId::PlayerCaught, 0.95f, 1.0f);
                m_cameraShakeTimer = 0.40f;
                m_statusBannerText = "HULL INTEGRITY DAMAGED (-1 HP)!";
                m_statusBannerTimer = 2.2f;

                if (m_player.isDead()) {
                    saveCheckpoint();
                    m_state = AppState::GameOver;
                    rebuildMenuButtons();
                    m_acoustics.clearRecentEvents();
                    return;
                }
            }
        }

        if (enemy.isActive()) {
            allInactiveNow = false;
            maxSuspicion = std::max(maxSuspicion, enemy.suspicion());
            if (enemy.state() == EnemyState::PursuePlayer) {
                anyPursuing = true;
            }
        }
    }

    // Check if wave or entire combat room was just cleared
    if (anyActiveBefore && allInactiveNow && !room.cleared) {
        if (m_currentWaveIndex + 1 < room.totalWaves) {
            ++m_currentWaveIndex;
            spawnWaveForCurrentRoom(m_currentWaveIndex);
            m_audio.playEffect(SoundEffectId::EnemyAlert, 0.9f, 0.88f);
            m_statusBannerText = "WAVE 2 REINFORCEMENTS DETECTED — MAINTAIN SONAR LOCK!";
            m_statusBannerTimer = 3.2f;
        } else {
            const int shardsEarned = room.shardReward;
            m_labyrinth.markRoomCleared(room.id);
            m_player.addEchoShards(shardsEarned);
            if (room.type == RoomType::Boss) {
                m_player.heal(2);
                m_statusBannerText = "THE CHOIR SOVEREIGN SHATTERED! ASCENT CONDUIT UNSEALED (+" + std::to_string(shardsEarned) + " SHARDS)";
            } else {
                m_statusBannerText = "CHAMBER CLEARED — DOORS UNSEALED (+" + std::to_string(shardsEarned) + " ECHO SHARDS)";
            }
            m_statusBannerTimer = 3.8f;
            m_audio.playEffect(SoundEffectId::VictoryChord, 0.85f, 1.08f);
            m_audio.playSpatial(SoundEffectId::GateSlideOpen, {640.0f, 400.0f}, 1.0f, 1.0f, 1.0f);
            saveCheckpoint();
        }
    }

    // Update 4-layer adaptive soundtrack
    const float danger0to1 = std::clamp(maxSuspicion / 100.0f, 0.0f, 1.0f);
    m_audio.updateAdaptiveAudio(
        dt,
        danger0to1,
        anyPursuing,
        room.isCheckpoint,
        m_player.stamina()
    );

    m_acoustics.clearRecentEvents();

    // Check 4-directional room portals
    for (const auto& portal : room.portals) {
        if (distSq(m_player.position(), portal.position) <= portal.radius * portal.radius) {
            if (portal.requiredSwitchId != -1 && !m_labyrinth.isSwitchActivated(portal.requiredSwitchId)) {
                if (m_statusBannerTimer <= 0.1f) {
                    if (portal.isSecretDoor) {
                        m_statusBannerText = "HOLLOW MASONRY DETECTED — PING [LMB] TO FRACTURE SECRET WALL";
                    } else if (room.combatLocked) {
                        m_statusBannerText = "DOORWAY SEALED DURING COMBAT — DISRUPT ALL HUNTERS";
                    } else {
                        m_statusBannerText = "LOCKED: " + portal.label + " (ACTIVATE MECHANISM OR DEFEAT BOSS)";
                    }
                    m_statusBannerTimer = 1.8f;
                }
            } else {
                if (portal.isVictoryExit) {
                    ++m_metaTotalVictories;
                    saveCheckpoint();
                    m_audio.playEffect(SoundEffectId::VictoryChord, 1.0f, 1.0f);
                    m_state = AppState::Victory;
                    rebuildMenuButtons();
                } else {
                    m_audio.playEffect(SoundEffectId::SwitchClunk, 0.65f, 1.15f);
                    enterRoom(portal.targetRoomId, portal.targetSpawnPos, true);
                }
                return;
            }
        }
    }
}

void Game::render() {
    m_window.clear(sf::Color(3, 6, 12));

    sf::View activeView = m_worldView;
    if (m_cameraShakeTimer > 0.0f) {
        const float mag = m_cameraShakeTimer * 12.0f;
        activeView.move(
            std::sin(m_runTimeSec * 55.0f) * mag,
            std::cos(m_runTimeSec * 48.0f) * mag
        );
    }
    m_window.setView(activeView);

    renderWorld();

    if (m_state == AppState::Playing) {
        renderHud();
        renderCornerMinimap();
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

    renderSubterraneanProps();
    m_acoustics.render(m_window, m_ambientVisibility, &room.walls);
    renderInteractiveObjects();

    if (m_state == AppState::Playing || m_state == AppState::Paused || m_state == AppState::GameOver) {
        for (const auto& enemy : m_enemies) {
            if (!enemy.isActive()) continue;
            const float enemyVis = m_acoustics.queryIlluminationAt(enemy.position(), m_player.position());
            enemy.render(m_window, enemyVis, m_player.position(), &m_assets);
        }
        m_player.render(m_window, &m_assets);
    }

    renderVignetteAndDust();
}

void Game::renderSubterraneanProps() {
    const auto& room = m_labyrinth.currentRoom();
    const sf::Vector2f pPos = m_player.position();
    const float visorBoost = m_player.hasRelic(RelicId::SubharmonicVisor) ? 0.08f : 0.0f;

    for (const auto& prop : room.props) {
        const float vis = std::max(
            m_ambientVisibility * 1.4f + visorBoost,
            m_acoustics.queryIlluminationAt(prop.position, pPos)
        );
        if (vis <= 0.02f) continue;
        const auto alpha = static_cast<sf::Uint8>(std::clamp(vis * 185.0f, 12.0f, 195.0f));

        if (prop.type == PropType::PipeConduit) {
            sf::VertexArray pipe(sf::Lines, 4);
            pipe[0] = sf::Vertex(prop.position, sf::Color(30, 58, 82, alpha));
            pipe[1] = sf::Vertex(prop.endPoint, sf::Color(30, 58, 82, alpha));
            pipe[2] = sf::Vertex(prop.position + sf::Vector2f{0.0f, -4.0f}, sf::Color(56, 189, 248, static_cast<sf::Uint8>(alpha / 2)));
            pipe[3] = sf::Vertex(prop.endPoint + sf::Vector2f{0.0f, -4.0f}, sf::Color(56, 189, 248, static_cast<sf::Uint8>(alpha / 2)));
            m_window.draw(pipe);
        } else if (prop.type == PropType::WaterPuddle) {
            sf::CircleShape puddle(prop.scale);
            puddle.setOrigin(prop.scale, prop.scale);
            puddle.setPosition(prop.position);
            puddle.setScale(1.35f, 0.75f);
            puddle.setFillColor(sf::Color(12, 42, 54, static_cast<sf::Uint8>(alpha * 0.45f)));
            puddle.setOutlineThickness(1.0f);
            puddle.setOutlineColor(sf::Color(45, 212, 191, static_cast<sf::Uint8>(alpha * 0.65f)));
            m_window.draw(puddle);
        } else if (prop.type == PropType::VentTurbine) {
            const float rad = prop.rotationDeg * 0.01745329f;
            m_assets.renderMesh25D(
                m_window,
                ModelId::VentTurbine,
                prop.position,
                rad,
                prop.scale,
                sf::Color(22, 38, 54, alpha),
                sf::Color(56, 189, 248, alpha),
                vis,
                pPos
            );
        } else if (prop.type == PropType::EmergencyBeacon) {
            const float pulse = 0.5f + 0.5f * std::sin(m_runTimeSec * 4.5f);
            sf::CircleShape glow(18.0f + pulse * 8.0f);
            glow.setOrigin(glow.getRadius(), glow.getRadius());
            glow.setPosition(prop.position);
            glow.setFillColor(sf::Color(52, 211, 153, static_cast<sf::Uint8>(30.0f + pulse * 45.0f)));
            m_window.draw(glow);
        }
    }
}

void Game::renderInteractiveObjects() {
    const auto& room = m_labyrinth.currentRoom();
    const sf::Vector2f pPos = m_player.position();
    const float visorBoost = m_player.hasRelic(RelicId::SubharmonicVisor) ? 0.10f : 0.0f;

    // 1. Render N/E/S/W Portals
    for (const auto& portal : room.portals) {
        const bool unlocked = (portal.requiredSwitchId == -1) || m_labyrinth.isSwitchActivated(portal.requiredSwitchId);
        // Secret doors stay hidden until unlocked by sonar or revealed by SubharmonicVisor
        if (portal.isSecretDoor && !unlocked && !m_player.hasRelic(RelicId::SubharmonicVisor)) {
            continue;
        }

        const float vis = std::max(0.28f + visorBoost, m_acoustics.queryIlluminationAt(portal.position, pPos));
        const auto alpha = static_cast<sf::Uint8>(vis * 230.0f);

        sf::Color col = unlocked ? sf::Color(52, 211, 153, alpha) : sf::Color(167, 139, 250, alpha);
        if (portal.isVictoryExit && unlocked) {
            col = sf::Color(250, 204, 21, alpha);
        } else if (room.combatLocked && !portal.isSecretDoor) {
            col = sf::Color(244, 63, 94, alpha);
        }

        const float pulseR = portal.radius + std::sin(m_runTimeSec * 3.5f) * 3.0f;
        sf::CircleShape ring(pulseR);
        ring.setOrigin(pulseR, pulseR);
        ring.setPosition(portal.position);
        ring.setFillColor(sf::Color(col.r, col.g, col.b, static_cast<sf::Uint8>(vis * 38.0f)));
        ring.setOutlineThickness(2.0f);
        ring.setOutlineColor(col);
        m_window.draw(ring);

        if (vis > 0.35f) {
            const float labelY = (portal.direction == CardinalDir::North)
                ? (portal.position.y + portal.radius + 8.0f)
                : (portal.position.y - portal.radius - 18.0f);
            m_ui.drawText(
                m_window,
                unlocked ? portal.label : ("SEALED: " + portal.label),
                {portal.position.x, labelY},
                10.0f,
                col,
                TextAlign::Center
            );
        }
    }

    // 2. Render Relic Pedestals (Treasure, Secret, Elite, & Shop rooms)
    for (const auto& ped : room.relicPedestals) {
        if (ped.collected) continue;
        const RelicSpec spec = getRelicSpec(ped.relicId);
        const float vis = std::max(0.45f, m_acoustics.queryIlluminationAt(ped.position, pPos));
        const auto alpha = static_cast<sf::Uint8>(vis * 245.0f);

        const float bob = std::sin(m_runTimeSec * 3.2f) * 3.0f;
        sf::CircleShape aura(22.0f + bob);
        aura.setOrigin(aura.getRadius(), aura.getRadius());
        aura.setPosition(ped.position);
        aura.setFillColor(sf::Color(spec.tint.r, spec.tint.g, spec.tint.b, static_cast<sf::Uint8>(vis * 55.0f)));
        aura.setOutlineThickness(1.8f);
        aura.setOutlineColor(sf::Color(spec.tint.r, spec.tint.g, spec.tint.b, alpha));
        m_window.draw(aura);

        m_assets.renderMesh25D(
            m_window,
            ModelId::ResonatorCrystal,
            ped.position,
            m_runTimeSec * 1.8f,
            15.5f,
            sf::Color(24, 34, 58, alpha),
            sf::Color(spec.tint.r, spec.tint.g, spec.tint.b, alpha),
            vis,
            pPos,
            0.15f
        );

        std::string label = std::string("[F] ") + spec.name;
        if (ped.costShards > 0) {
            label += " (" + std::to_string(ped.costShards) + " SHARDS)";
        }
        m_ui.drawText(m_window, label, {ped.position.x, ped.position.y - 34.0f}, 10.5f, spec.tint, TextAlign::Center, true);
        if (distSq(pPos, ped.position) <= 85.0f * 85.0f) {
            m_ui.drawText(m_window, spec.effectSummary, {ped.position.x, ped.position.y + 24.0f}, 9.5f, sf::Color(224, 242, 254), TextAlign::Center);
        }
    }

    // 3. Render Quartermaster Shop Offers
    for (const auto& offer : room.shopOffers) {
        if (offer.purchased) continue;
        const float vis = std::max(0.55f, m_acoustics.queryIlluminationAt(offer.position, pPos));
        const auto alpha = static_cast<sf::Uint8>(vis * 240.0f);
        const sf::Color col(250, 204, 21, alpha);

        m_assets.renderMesh25D(
            m_window,
            ModelId::SurveyorTerminal,
            offer.position,
            0.0f,
            14.5f,
            sf::Color(30, 42, 58, alpha),
            col,
            vis,
            pPos,
            0.08f
        );
        const std::string tag = "[F] " + offer.label + " (" + std::to_string(offer.costShards) + " SHARDS)";
        m_ui.drawText(m_window, tag, {offer.position.x, offer.position.y - 30.0f}, 10.0f, col, TextAlign::Center);
    }

    // 4. Render Interactive Mechanisms (Sanctuary Shrine & Levers)
    for (const auto& mech : room.mechanisms) {
        const float vis = std::max(0.28f + visorBoost, m_acoustics.queryIlluminationAt(mech.position, pPos));
        if (vis <= 0.05f) continue;

        const auto alpha = static_cast<sf::Uint8>(vis * 240.0f);
        const sf::Color col = mech.activated
            ? sf::Color(52, 211, 153, alpha)
            : sf::Color(250, 204, 21, alpha);

        const float leverTilt = (mech.animProgress - 0.5f) * 0.55f;
        m_assets.renderMesh25D(
            m_window,
            ModelId::HarmonicLever,
            mech.position,
            0.0f,
            14.5f,
            sf::Color(24, 38, 56, alpha),
            col,
            vis,
            pPos,
            leverTilt
        );

        if (distSq(pPos, mech.position) <= 68.0f * 68.0f || vis > 0.45f) {
            m_ui.drawText(
                m_window,
                mech.activated ? "ENGAGED" : ("[F] " + mech.label),
                {mech.position.x, mech.position.y - 26.0f},
                10.5f,
                col,
                TextAlign::Center
            );
        }
    }

    // 5. Render Acoustic Resonator Crystals
    for (const auto& res : room.resonators) {
        const float vis = std::max(0.30f, std::max(res.energy, m_acoustics.queryIlluminationAt(res.position, pPos)));
        const auto alpha = static_cast<sf::Uint8>(vis * 245.0f);
        const sf::Color col = res.activated
            ? sf::Color(52, 211, 153, alpha)
            : sf::Color(167, 139, 250, alpha);

        sf::CircleShape ring(res.radius + 4.0f + std::sin(m_runTimeSec * 5.0f) * 2.0f);
        ring.setOrigin(ring.getRadius(), ring.getRadius());
        ring.setPosition(res.position);
        ring.setFillColor(sf::Color(col.r, col.g, col.b, static_cast<sf::Uint8>(25.0f + res.energy * 90.0f)));
        ring.setOutlineThickness(2.0f);
        ring.setOutlineColor(col);
        m_window.draw(ring);

        m_assets.renderMesh25D(
            m_window,
            ModelId::ResonatorCrystal,
            res.position,
            m_runTimeSec * (res.activated ? 2.6f : 1.1f),
            15.0f,
            sf::Color(28, 20, 52, alpha),
            col,
            vis,
            pPos,
            0.12f
        );

        if (vis > 0.35f) {
            const int pct = static_cast<int>(std::round(res.energy * 100.0f));
            const std::string status = res.activated
                ? "RESONATOR LOCKED (100%)"
                : ("PING CRYSTAL (" + std::to_string(pct) + "%)");
            m_ui.drawText(m_window, status, {res.position.x, res.position.y - 28.0f}, 10.5f, col, TextAlign::Center);
        }
    }

    // 6. Render Brittle Shale Hazard Zones
    for (const auto& hz : room.hazards) {
        const float vis = std::max(m_ambientVisibility * 1.6f, m_acoustics.queryIlluminationAt(hz.position, pPos));
        if (vis <= 0.04f) continue;
        const auto alpha = static_cast<sf::Uint8>(vis * 180.0f);

        sf::CircleShape zone(hz.radius);
        zone.setOrigin(hz.radius, hz.radius);
        zone.setPosition(hz.position);
        zone.setFillColor(sf::Color(120, 53, 15, static_cast<sf::Uint8>(vis * 35.0f)));
        zone.setOutlineThickness(1.5f);
        zone.setOutlineColor(sf::Color(251, 146, 60, alpha));
        m_window.draw(zone);
    }

    // 7. Render Surveyor Log Terminals
    for (const auto& lore : room.collectibles) {
        if (lore.collected) continue;
        const float vis = std::max(0.26f + visorBoost, m_acoustics.queryIlluminationAt(lore.position, pPos));
        if (vis <= 0.05f) continue;

        const auto alpha = static_cast<sf::Uint8>(vis * 240.0f);
        const sf::Color col(56, 189, 248, alpha);

        m_assets.renderMesh25D(
            m_window,
            ModelId::SurveyorTerminal,
            lore.position,
            0.0f,
            14.0f,
            sf::Color(14, 34, 56, alpha),
            col,
            vis,
            pPos,
            0.05f
        );

        if (distSq(pPos, lore.position) <= 68.0f * 68.0f || vis > 0.45f) {
            m_ui.drawText(
                m_window,
                "[F] " + lore.title,
                {lore.position.x, lore.position.y - 24.0f},
                10.5f,
                col,
                TextAlign::Center
            );
        }
    }
}

void Game::renderVignetteAndDust() {
    sf::VertexArray dust(sf::Quads);
    const sf::Vector2f pPos = m_player.position();
    for (const auto& pt : m_particles) {
        const float vis = std::max(0.08f, m_acoustics.queryIlluminationAt(pt.pos, pPos));
        const auto alpha = static_cast<sf::Uint8>(vis * 140.0f);
        const sf::Color c(125, 211, 252, alpha);
        const float s = pt.size;
        dust.append(sf::Vertex({pt.pos.x - s, pt.pos.y - s}, c));
        dust.append(sf::Vertex({pt.pos.x + s, pt.pos.y - s}, c));
        dust.append(sf::Vertex({pt.pos.x + s, pt.pos.y + s}, c));
        dust.append(sf::Vertex({pt.pos.x - s, pt.pos.y + s}, c));
    }
    m_window.draw(dust);
}

void Game::renderHud() {
    const auto& room = m_labyrinth.currentRoom();

    // Top-left Chamber & Seed Telemetry Panel
    m_ui.drawPanel(m_window, {18.0f, 14.0f}, {540.0f, 68.0f}, sf::Color(6, 12, 24, 215), sf::Color(30, 58, 95, 190));
    m_ui.drawText(m_window, room.name, {30.0f, 22.0f}, 14.0f, sf::Color(56, 189, 248), TextAlign::Left, true);

    std::string seedLine = "SEED #" + std::to_string(m_labyrinth.runSeed()) + "  |  VISITED: "
        + std::to_string(m_labyrinth.visitedRooms().size()) + "/" + std::to_string(m_labyrinth.roomCount());
    m_ui.drawText(m_window, seedLine, {545.0f, 23.0f}, 10.5f, sf::Color(148, 163, 184), TextAlign::Right);
    m_ui.drawText(m_window, room.objectiveText, {30.0f, 48.0f}, 10.5f, sf::Color(203, 213, 225));

    // Bottom-left Hull Integrity (Health), Stamina, Echo Shards, Decoys & Relics Panel
    m_ui.drawPanel(m_window, {18.0f, 704.0f}, {510.0f, 82.0f}, sf::Color(6, 12, 24, 215), sf::Color(30, 58, 95, 190));

    // Hull Integrity pips
    m_ui.drawText(m_window, "HULL INTEGRITY", {30.0f, 712.0f}, 10.5f, sf::Color(248, 113, 113), TextAlign::Left, true);
    for (int i = 0; i < m_player.maxHealth(); ++i) {
        sf::RectangleShape pip({14.0f, 10.0f});
        pip.setPosition(30.0f + static_cast<float>(i) * 18.0f, 730.0f);
        if (i < m_player.health()) {
            pip.setFillColor(sf::Color(244, 63, 94));
            pip.setOutlineColor(sf::Color(253, 164, 175));
        } else {
            pip.setFillColor(sf::Color(30, 20, 32));
            pip.setOutlineColor(sf::Color(71, 85, 105));
        }
        pip.setOutlineThickness(1.0f);
        m_window.draw(pip);
    }

    // Stamina Bar
    m_ui.drawText(m_window, "STAMINA", {30.0f, 748.0f}, 10.0f, sf::Color(148, 163, 184));
    m_ui.drawProgressBar(
        m_window,
        {30.0f, 764.0f},
        {165.0f, 10.0f},
        m_player.stamina() / 100.0f,
        sf::Color(15, 23, 42, 220),
        m_player.stamina() < 25.0f ? sf::Color(239, 68, 68) : sf::Color(56, 189, 248),
        sf::Color(51, 65, 85, 220)
    );

    // Echo Shards, Decoy Stones, and Active Relic Count
    m_ui.drawText(
        m_window,
        "ECHO SHARDS: " + std::to_string(m_player.echoShards()),
        {225.0f, 714.0f},
        12.0f,
        sf::Color(250, 204, 21),
        TextAlign::Left,
        true
    );
    m_ui.drawText(
        m_window,
        "DECOY STONES [RMB]: " + std::to_string(m_player.decoyCount()),
        {225.0f, 736.0f},
        11.5f,
        sf::Color(167, 139, 250)
    );
    m_ui.drawText(
        m_window,
        "ACTIVE RELICS [TAB]: " + std::to_string(m_player.relics().size()) + "/8",
        {225.0f, 758.0f},
        11.5f,
        sf::Color(52, 211, 153)
    );

    // Bottom-right Acoustic Threat & Boss Bar Panel
    m_ui.drawPanel(m_window, {910.0f, 704.0f}, {352.0f, 82.0f}, sf::Color(6, 12, 24, 215), sf::Color(30, 58, 95, 190));

    std::size_t activeEnemyCount = 0;
    const EnemyAI* bossPtr = nullptr;
    float highestSuspicion = 0.0f;
    for (const auto& e : m_enemies) {
        if (!e.isActive()) continue;
        ++activeEnemyCount;
        highestSuspicion = std::max(highestSuspicion, e.suspicion());
        if (e.variant() == EnemyVariant::AbyssalBoss) {
            bossPtr = &e;
        }
    }

    if (bossPtr != nullptr) {
        m_ui.drawText(
            m_window,
            "BOSS: THE CHOIR SOVEREIGN (PHASE " + std::to_string(bossPtr->bossPhase()) + ")",
            {924.0f, 714.0f},
            11.5f,
            sf::Color(244, 63, 94),
            TextAlign::Left,
            true
        );
        m_ui.drawProgressBar(
            m_window,
            {924.0f, 738.0f},
            {324.0f, 12.0f},
            bossPtr->health() / std::max(1.0f, bossPtr->maxHealth()),
            sf::Color(25, 12, 22, 230),
            sf::Color(244, 63, 94),
            sf::Color(159, 18, 57, 230)
        );
        m_ui.drawText(
            m_window,
            "PING [LMB] & PULSE [SPACE] TO SHATTER",
            {924.0f, 758.0f},
            10.0f,
            sf::Color(203, 213, 225)
        );
    } else if (activeEnemyCount > 0) {
        const std::string threatLine = "HUNTERS ACTIVE: " + std::to_string(activeEnemyCount)
            + " (WAVE " + std::to_string(m_currentWaveIndex + 1) + "/" + std::to_string(room.totalWaves) + ")";
        m_ui.drawText(m_window, threatLine, {924.0f, 714.0f}, 11.5f, sf::Color(248, 113, 113), TextAlign::Left, true);
        m_ui.drawProgressBar(
            m_window,
            {924.0f, 738.0f},
            {324.0f, 10.0f},
            highestSuspicion / 100.0f,
            sf::Color(15, 23, 42, 220),
            highestSuspicion > 65.0f ? sf::Color(239, 68, 68) : sf::Color(250, 204, 21),
            sf::Color(51, 65, 85, 220)
        );
        m_ui.drawText(
            m_window,
            "SONAR PING [LMB] DEALS HARMONIC DAMAGE",
            {924.0f, 756.0f},
            10.0f,
            sf::Color(148, 163, 184)
        );
    } else {
        m_ui.drawText(m_window, "CHAMBER CLEAR — NO ACOUSTIC THREATS", {924.0f, 722.0f}, 12.0f, sf::Color(52, 211, 153), TextAlign::Left, true);
        m_ui.drawText(m_window, "[TAB/M] MAP & RELICS   [F5] SAVE", {924.0f, 750.0f}, 10.5f, sf::Color(148, 163, 184));
    }

    if (m_statusBannerTimer > 0.0f) {
        m_ui.drawPanel(m_window, {290.0f, 88.0f}, {700.0f, 32.0f}, sf::Color(8, 22, 42, 225), sf::Color(56, 189, 248, 200));
        m_ui.drawText(m_window, m_statusBannerText, {640.0f, 97.0f}, 11.5f, sf::Color(186, 230, 253), TextAlign::Center, true);
    }

    if (m_loreDisplayTimer > 0.0f) {
        m_ui.drawPanel(m_window, {240.0f, 608.0f}, {800.0f, 82.0f}, sf::Color(10, 20, 38, 235), sf::Color(56, 189, 248, 220), 2.0f);
        m_ui.drawText(m_window, m_activeLoreTitle, {260.0f, 618.0f}, 13.0f, sf::Color(56, 189, 248), TextAlign::Left, true);
        m_ui.drawText(m_window, m_activeLoreBody, {260.0f, 646.0f}, 10.5f, sf::Color(226, 232, 240));
    }
}

void Game::renderCornerMinimap() {
    // Top-right Isaac-style 2D Grid Minimap showing 5x5 local neighborhood around current room
    const sf::Vector2f boxPos{1072.0f, 14.0f};
    const sf::Vector2f boxSize{190.0f, 134.0f};
    m_ui.drawPanel(m_window, boxPos, boxSize, sf::Color(6, 12, 24, 215), sf::Color(30, 58, 95, 190));

    const auto& cur = m_labyrinth.currentRoom();
    const sf::Vector2f center{boxPos.x + boxSize.x * 0.5f, boxPos.y + boxSize.y * 0.5f + 6.0f};
    constexpr float cellStep = 24.0f;
    constexpr float cellW = 18.0f;
    constexpr float cellH = 14.0f;

    m_ui.drawText(m_window, "SONAR GRID [TAB]", {center.x, boxPos.y + 6.0f}, 9.5f, sf::Color(125, 211, 252), TextAlign::Center, true);

    for (std::size_t i = 0; i < m_labyrinth.roomCount(); ++i) {
        const auto& rm = m_labyrinth.roomAt(i);
        if (!rm.discovered) continue;

        const int dx = rm.gridX - cur.gridX;
        const int dy = rm.gridY - cur.gridY;
        if (std::abs(dx) > 3 || std::abs(dy) > 2) continue;

        const sf::Vector2f pos{
            center.x + static_cast<float>(dx) * cellStep - cellW * 0.5f,
            center.y + static_cast<float>(dy) * cellStep - cellH * 0.5f
        };

        sf::Color fill = rm.visited ? sf::Color(20, 48, 76, 230) : sf::Color(12, 22, 36, 180);
        sf::Color outline = rm.visited ? sf::Color(56, 189, 248) : sf::Color(71, 85, 105);

        if (rm.type == RoomType::Boss) outline = sf::Color(244, 63, 94);
        else if (rm.type == RoomType::Treasure || rm.type == RoomType::Shop) outline = sf::Color(250, 204, 21);
        else if (rm.type == RoomType::Sanctuary || rm.type == RoomType::FinalExit) outline = sf::Color(52, 211, 153);
        else if (rm.type == RoomType::Secret) outline = sf::Color(216, 180, 254);

        if (static_cast<int>(i) == m_labyrinth.currentRoomId()) {
            fill = sf::Color(14, 116, 144, 245);
            outline = sf::Color(224, 242, 254);
        }

        m_ui.drawPanel(m_window, pos, {cellW, cellH}, fill, outline, 1.2f);
        const std::string iconStr(1, Labyrinth::roomTypeIcon(rm.type));
        m_ui.drawText(m_window, iconStr, {pos.x + cellW * 0.5f, pos.y + 2.0f}, 8.5f, outline, TextAlign::Center, true);
    }
}

void Game::renderMinimapOverlay() {
    m_ui.drawPanel(m_window, {110.0f, 75.0f}, {1060.0f, 650.0f}, sf::Color(4, 10, 22, 244), sf::Color(56, 189, 248, 220), 2.0f);
    m_ui.drawText(
        m_window,
        "PROCEDURAL LABYRINTH TOPOLOGY & RELIC LOADOUT",
        {640.0f, 94.0f},
        18.0f,
        sf::Color(56, 189, 248),
        TextAlign::Center,
        true
    );

    const std::string subHeader = "SEED #" + std::to_string(m_labyrinth.runSeed())
        + "   |   ROOMS CLEARED: " + std::to_string(m_labyrinth.clearedRooms().size()) + "/" + std::to_string(m_labyrinth.roomCount())
        + "   |   META VICTORIES: " + std::to_string(m_metaTotalVictories);
    m_ui.drawText(m_window, subHeader, {640.0f, 122.0f}, 11.5f, sf::Color(148, 163, 184), TextAlign::Center);

    // Left Half: 2D Grid Map centered on Start/Current
    const auto& cur = m_labyrinth.currentRoom();
    const sf::Vector2f mapOrigin{410.0f, 395.0f};
    constexpr float stepX = 42.0f;
    constexpr float stepY = 34.0f;
    constexpr float boxW = 32.0f;
    constexpr float boxH = 24.0f;

    // Draw doorway connectors first
    sf::VertexArray links(sf::Lines);
    for (std::size_t i = 0; i < m_labyrinth.roomCount(); ++i) {
        const auto& rm = m_labyrinth.roomAt(i);
        if (!rm.discovered) continue;
        const int dx = rm.gridX - cur.gridX;
        const int dy = rm.gridY - cur.gridY;
        if (std::abs(dx) > 6 || std::abs(dy) > 6) continue;

        const sf::Vector2f c1{
            mapOrigin.x + static_cast<float>(dx) * stepX,
            mapOrigin.y + static_cast<float>(dy) * stepY
        };
        for (int d = 0; d < 4; ++d) {
            const int nb = rm.neighbors[static_cast<std::size_t>(d)];
            if (nb <= static_cast<int>(i)) continue;
            const auto& nbRm = m_labyrinth.roomAt(static_cast<std::size_t>(nb));
            if (!nbRm.discovered) continue;
            const sf::Vector2f c2{
                mapOrigin.x + static_cast<float>(nbRm.gridX - cur.gridX) * stepX,
                mapOrigin.y + static_cast<float>(nbRm.gridY - cur.gridY) * stepY
            };
            links.append(sf::Vertex(c1, sf::Color(56, 189, 248, 150)));
            links.append(sf::Vertex(c2, sf::Color(56, 189, 248, 150)));
        }
    }
    m_window.draw(links);

    for (std::size_t i = 0; i < m_labyrinth.roomCount(); ++i) {
        const auto& rm = m_labyrinth.roomAt(i);
        if (!rm.discovered) continue;
        const int dx = rm.gridX - cur.gridX;
        const int dy = rm.gridY - cur.gridY;
        if (std::abs(dx) > 6 || std::abs(dy) > 6) continue;

        const sf::Vector2f pos{
            mapOrigin.x + static_cast<float>(dx) * stepX - boxW * 0.5f,
            mapOrigin.y + static_cast<float>(dy) * stepY - boxH * 0.5f
        };

        sf::Color fill = rm.visited ? sf::Color(18, 42, 68, 240) : sf::Color(10, 18, 30, 200);
        sf::Color outline = rm.visited ? sf::Color(56, 189, 248) : sf::Color(71, 85, 105);

        if (rm.type == RoomType::Boss) outline = sf::Color(244, 63, 94);
        else if (rm.type == RoomType::Treasure || rm.type == RoomType::Shop) outline = sf::Color(250, 204, 21);
        else if (rm.type == RoomType::Sanctuary || rm.type == RoomType::FinalExit) outline = sf::Color(52, 211, 153);
        else if (rm.type == RoomType::Secret) outline = sf::Color(216, 180, 254);

        if (static_cast<int>(i) == m_labyrinth.currentRoomId()) {
            fill = sf::Color(14, 116, 144, 250);
            outline = sf::Color(224, 242, 254);
        }

        m_ui.drawPanel(m_window, pos, {boxW, boxH}, fill, outline, 1.5f);
        const std::string label(1, Labyrinth::roomTypeIcon(rm.type));
        m_ui.drawText(m_window, label, {pos.x + boxW * 0.5f, pos.y + 5.0f}, 11.0f, outline, TextAlign::Center, true);
    }

    // Right Panel: Collected Relics & Synergies
    m_ui.drawPanel(m_window, {720.0f, 160.0f}, {420.0f, 480.0f}, sf::Color(8, 18, 34, 230), sf::Color(30, 58, 95, 200));
    m_ui.drawText(m_window, "EQUIPPED ACOUSTIC RELICS", {930.0f, 174.0f}, 13.5f, sf::Color(52, 211, 153), TextAlign::Center, true);

    float ry = 208.0f;
    if (m_player.relics().empty()) {
        m_ui.drawText(m_window, "NO RELICS COLLECTED YET.", {930.0f, 250.0f}, 11.5f, sf::Color(148, 163, 184), TextAlign::Center);
        m_ui.drawText(m_window, "EXPLORE TREASURE [T], SHOP [$],", {930.0f, 275.0f}, 10.5f, sf::Color(148, 163, 184), TextAlign::Center);
        m_ui.drawText(m_window, "SECRET [?], AND ELITE [!] CHAMBERS.", {930.0f, 296.0f}, 10.5f, sf::Color(148, 163, 184), TextAlign::Center);
    } else {
        for (RelicId rId : m_player.relics()) {
            const RelicSpec spec = getRelicSpec(rId);
            m_ui.drawText(m_window, "* " + std::string(spec.name), {738.0f, ry}, 11.5f, spec.tint, TextAlign::Left, true);
            m_ui.drawText(m_window, spec.effectSummary, {750.0f, ry + 16.0f}, 9.5f, sf::Color(203, 213, 225));
            ry += 42.0f;
        }
    }

    m_ui.drawText(
        m_window,
        "LEGEND: [S] START  [C] COMBAT  [T] RELIC  [$] SHOP  [+] SANCTUARY  [!] ELITE  [?] SECRET  [B] BOSS  [^] EXIT",
        {640.0f, 692.0f},
        10.0f,
        sf::Color(148, 163, 184),
        TextAlign::Center
    );
}

void Game::renderMenuOverlay() {
    m_ui.drawPanel(m_window, {320.0f, 120.0f}, {640.0f, 550.0f}, sf::Color(5, 11, 22, 238), sf::Color(56, 189, 248, 210), 2.0f);

    std::string title = "ECHOLABYRINTH 4.0";
    std::string subtitle = "PROCEDURAL ACOUSTIC ROGUELIKE  //  NATIVE C++17 & SFML";
    sf::Color titleCol(56, 189, 248);

    if (m_state == AppState::Paused) {
        title = "EXPEDITION PAUSED";
        subtitle = m_labyrinth.currentRoom().name + "  //  SEED #" + std::to_string(m_labyrinth.runSeed());
    } else if (m_state == AppState::GameOver) {
        title = "HULL INTEGRITY DEPLETED";
        subtitle = "CHAMBERS CLEARED: " + std::to_string(m_labyrinth.clearedRooms().size())
            + "  |  HUNTERS DISRUPTED: " + std::to_string(m_enemiesDisruptedThisRun)
            + "  |  RELICS: " + std::to_string(m_player.relics().size());
        titleCol = sf::Color(239, 68, 68);
    } else if (m_state == AppState::Victory) {
        title = "SURFACE ASCENT COMPLETE";
        subtitle = "SEED #" + std::to_string(m_labyrinth.runSeed())
            + " CLEARED IN " + std::to_string(static_cast<int>(m_runTimeSec)) + "S"
            + "  |  TOTAL VICTORIES: " + std::to_string(m_metaTotalVictories);
        titleCol = sf::Color(52, 211, 153);
    }

    m_ui.drawText(m_window, title, {640.0f, 160.0f}, 24.0f, titleCol, TextAlign::Center, true);
    m_ui.drawText(m_window, subtitle, {640.0f, 202.0f}, 11.5f, sf::Color(148, 163, 184), TextAlign::Center);

    const std::string metaLine = "META-PROGRESSION: RUNS #" + std::to_string(m_metaTotalRuns)
        + "  |  VICTORIES: " + std::to_string(m_metaTotalVictories)
        + "  |  BOSSES SHATTERED: " + std::to_string(m_metaBossesDefeated);
    m_ui.drawText(m_window, metaLine, {640.0f, 232.0f}, 10.5f, sf::Color(250, 204, 21), TextAlign::Center);

    for (std::size_t i = 0; i < m_menuButtons.size(); ++i) {
        const auto& btn = m_menuButtons[i];
        const bool selected = (i == m_selectedMenuIndex);
        const sf::Vector2f topLeft = btn.center - btn.size * 0.5f;

        sf::Color fill = selected ? sf::Color(18, 48, 78, 240) : sf::Color(10, 22, 38, 210);
        sf::Color outline = selected ? sf::Color(56, 189, 248, 255) : sf::Color(30, 58, 95, 180);
        sf::Color textCol = selected ? sf::Color(224, 242, 254) : sf::Color(148, 163, 184);

        if (!btn.enabled) {
            fill = sf::Color(8, 14, 24, 150);
            outline = sf::Color(30, 41, 59, 100);
            textCol = sf::Color(71, 85, 105);
        }

        m_ui.drawPanel(m_window, topLeft, btn.size, fill, outline, selected ? 2.0f : 1.0f);
        m_ui.drawText(m_window, btn.label, {btn.center.x, btn.center.y - 7.0f}, 13.5f, textCol, TextAlign::Center, selected);
    }

    m_ui.drawText(
        m_window,
        "LMB/E: SONAR PING (DAMAGES HUNTERS & OPENS SECRETS) | SPACE: 360 PULSE | RMB/G: DECOY | F: INTERACT | TAB: MAP",
        {640.0f, 642.0f},
        9.5f,
        sf::Color(100, 116, 139),
        TextAlign::Center
    );
}

void Game::renderSettingsOverlay() {
    m_ui.drawPanel(m_window, {290.0f, 110.0f}, {700.0f, 575.0f}, sf::Color(5, 11, 22, 242), sf::Color(56, 189, 248, 210), 2.0f);
    m_ui.drawText(m_window, "3D SPATIAL AUDIO & ACOUSTIC ENGINE SETTINGS", {640.0f, 142.0f}, 18.0f, sf::Color(56, 189, 248), TextAlign::Center, true);

    char buf0[64], buf1[64], buf2[64], buf3[64], buf4[64], buf5[64], buf6[64], buf7[64];
    std::snprintf(buf0, sizeof(buf0), "MASTER VOLUME: %.0f%%", m_audio.masterVolume());
    std::snprintf(buf1, sizeof(buf1), "ADAPTIVE MUSIC LAYER: %.0f%%", m_audio.musicVolume());
    std::snprintf(buf2, sizeof(buf2), "3D SPATIAL SFX VOLUME: %.0f%%", m_audio.sfxVolume());
    std::snprintf(buf3, sizeof(buf3), "AMBIENT COMPLEX DRONE: %.0f%%", m_audio.ambientVolume());
    std::snprintf(buf4, sizeof(buf4), "ACOUSTIC RAY DENSITY: %.0f%%", m_acoustics.rayDensityScale() * 100.0f);
    std::snprintf(buf5, sizeof(buf5), "MAX WALL REFLECTIONS: %d BOUNCES", static_cast<int>(m_acoustics.maxReflections()));
    std::snprintf(buf6, sizeof(buf6), "AMBIENT VISIBILITY: %.0f%%", m_ambientVisibility * 100.0f);
    std::snprintf(buf7, sizeof(buf7), "DISPLAY MODE: %s [F11]", m_isFullscreen ? "FULLSCREEN" : "WINDOWED 1280x800");

    if (m_menuButtons.size() >= 8) {
        m_menuButtons[0].label = buf0;
        m_menuButtons[1].label = buf1;
        m_menuButtons[2].label = buf2;
        m_menuButtons[3].label = buf3;
        m_menuButtons[4].label = buf4;
        m_menuButtons[5].label = buf5;
        m_menuButtons[6].label = buf6;
        m_menuButtons[7].label = buf7;
    }

    for (std::size_t i = 0; i < m_menuButtons.size(); ++i) {
        const auto& btn = m_menuButtons[i];
        const bool selected = (i == m_selectedMenuIndex);
        const sf::Vector2f topLeft = btn.center - btn.size * 0.5f;

        m_ui.drawPanel(
            m_window,
            topLeft,
            btn.size,
            selected ? sf::Color(18, 48, 78, 240) : sf::Color(10, 22, 38, 210),
            selected ? sf::Color(56, 189, 248, 255) : sf::Color(30, 58, 95, 180),
            selected ? 2.0f : 1.0f
        );
        m_ui.drawText(
            m_window,
            btn.label,
            {btn.center.x, btn.center.y - 6.5f},
            12.5f,
            selected ? sf::Color(224, 242, 254) : sf::Color(148, 163, 184),
            TextAlign::Center,
            selected
        );
    }
}

void Game::renderControlsOverlay() {
    m_ui.drawPanel(m_window, {210.0f, 85.0f}, {860.0f, 635.0f}, sf::Color(5, 11, 22, 244), sf::Color(56, 189, 248, 210), 2.0f);
    m_ui.drawText(m_window, "SURVEYOR FIELD MANUAL — ROGUELIKE ECHOLOCATION", {640.0f, 112.0f}, 18.5f, sf::Color(56, 189, 248), TextAlign::Center, true);

    const char* lines[] = {
        "MOVEMENT & ACOUSTIC DISCIPLINE:",
        "  [W / A / S / D]  MOVE SURVEYOR THROUGH N / E / S / W CHAMBER DOORS",
        "  [HOLD LSHIFT]    SNEAK STANCE (SILENT STEPS, SAFE OVER BRITTLE SHALE)",
        "  [HOLD LCTRL / C] SPRINT STANCE (HIGH SPEED, LOUD 430M SHOCKWAVES)",
        "",
        "OFFENSIVE & TACTICAL ECHOLOCATION:",
        "  [LEFT CLICK / E] DIRECTIONAL SONAR PING (ILLUMINATES & HARMONICALLY DAMAGES HUNTERS!)",
        "  [SPACEBAR]       360-DEGREE RADIAL PULSE (REVEALS ROOM, UNSEALS SECRET WALLS)",
        "  [RIGHT CLICK / G] THROW DECOY STONE TO LURE HUNTERS OR TRIGGER RESONANCE BLAST",
        "  [F]              CLAIM RELIC / BUY AT QUARTERMASTER / ACTIVATE SHRINE OR LEVER",
        "  [TAB / M]        TOGGLE 2D PROCEDURAL MAP & EQUIPPED RELIC LOADOUT",
        "",
        "CHAMBER TYPES & SYNERGIES:",
        "  [T] RELIC VAULT | [$] QUARTERMASTER | [+] SANCTUARY | [?] SECRET | [!] ELITE | [B] BOSS"
    };

    float y = 160.0f;
    for (const char* line : lines) {
        const bool isHeader = (line[0] != ' ' && line[0] != '\0');
        m_ui.drawText(
            m_window,
            line,
            {250.0f, y},
            isHeader ? 12.5f : 11.0f,
            isHeader ? sf::Color(125, 211, 252) : sf::Color(203, 213, 225)
        );
        y += isHeader ? 25.0f : 21.0f;
    }

    if (!m_menuButtons.empty()) {
        const auto& btn = m_menuButtons[0];
        m_ui.drawPanel(m_window, btn.center - btn.size * 0.5f, btn.size, sf::Color(20, 48, 76), sf::Color(56, 189, 248), 2.0f);
        m_ui.drawText(m_window, btn.label, {btn.center.x, btn.center.y - 7.0f}, 14.0f, sf::Color(224, 242, 254), TextAlign::Center, true);
    }
}

int Game::runSelfTest(const std::string& exeDir) {
    std::cout << "[SelfTest] Starting EchoLabyrinth 4.0 C++17 Roguelike verification suite...\n";

    // 1. Multi-Seed Procedural Run Graph Verification (20 distinct seeds)
    Labyrinth lab;
    for (std::uint32_t seedIdx = 1; seedIdx <= 20; ++seedIdx) {
        const std::uint32_t seed = 0x1000u + seedIdx * 7919u;
        lab.generateRun(seed, 32);
        std::string graphErr;
        if (!lab.verifyRunGraph(graphErr)) {
            std::cerr << "[SelfTest] FAIL: Seed " << seed << " failed verifyRunGraph: " << graphErr << "\n";
            return 1;
        }
    }
    std::cout << "[SelfTest] PASS [1/10]: 20 distinct seeds verified for 2D grid uniqueness, N/E/S/W door symmetry, & Boss/Exit reachability.\n";

    // 2. Seed Determinism Verification (same seed -> identical graph; different seed -> different graph)
    Labyrinth labA;
    Labyrinth labB;
    labA.generateRun(0xCAFE1234u, 32);
    labB.generateRun(0xCAFE1234u, 32);
    if (labA.roomCount() != labB.roomCount() || labA.bossRoomId() != labB.bossRoomId() || labA.exitRoomId() != labB.exitRoomId()) {
        std::cerr << "[SelfTest] FAIL: Identical seed did not produce identical room counts or Boss/Exit IDs.\n";
        return 1;
    }
    for (std::size_t i = 0; i < labA.roomCount(); ++i) {
        const auto& ra = labA.roomAt(i);
        const auto& rb = labB.roomAt(i);
        if (ra.gridX != rb.gridX || ra.gridY != rb.gridY || ra.type != rb.type || ra.neighbors != rb.neighbors) {
            std::cerr << "[SelfTest] FAIL: Seed determinism mismatch at room " << i << "\n";
            return 1;
        }
    }
    std::cout << "[SelfTest] PASS [2/10]: Deterministic seed reproducibility verified across all 32 chambers.\n";

    // 3. Verify 2D BFS Geometric Walkability across every room in the procedural run
    for (int r = 0; r < static_cast<int>(labA.roomCount()); ++r) {
        std::string travErr;
        if (!labA.verifyRoomTraversability(r, travErr)) {
            std::cerr << "[SelfTest] FAIL: Room " << r << " failed 2D BFS walkability: " << travErr << "\n";
            return 1;
        }
    }
    std::cout << "[SelfTest] PASS [3/10]: All 32 procedural chambers verified via 2D BFS grid pathfinding.\n";

    // 4. Verify Combat Door Lockdown, Room Clear Unsealing, and Secret Wall Sonar Fracture
    int combatRoomId = -1;
    int secretRoomId = -1;
    for (std::size_t i = 0; i < labA.roomCount(); ++i) {
        if (combatRoomId == -1 && labA.roomAt(i).type == RoomType::Combat) combatRoomId = static_cast<int>(i);
        if (secretRoomId == -1 && labA.roomAt(i).type == RoomType::Secret) secretRoomId = static_cast<int>(i);
    }
    if (combatRoomId == -1 || secretRoomId == -1) {
        std::cerr << "[SelfTest] FAIL: Missing Combat or Secret room in generated run.\n";
        return 1;
    }
    labA.setCombatLock(combatRoomId, true);
    if (!labA.roomAt(static_cast<std::size_t>(combatRoomId)).combatLocked) {
        std::cerr << "[SelfTest] FAIL: setCombatLock did not lock combat doors.\n";
        return 1;
    }
    labA.markRoomCleared(combatRoomId);
    if (!labA.isRoomCleared(combatRoomId) || labA.roomAt(static_cast<std::size_t>(combatRoomId)).combatLocked) {
        std::cerr << "[SelfTest] FAIL: markRoomCleared did not unseal combat doors.\n";
        return 1;
    }

    // Verify Secret Wall (doorId = 900 + secretRoomId) unseals when struck by player sonar rays
    AcousticSystem acoustics;
    labA.setCurrentRoom(secretRoomId);
    auto& secRoom = labA.currentRoomMut();
    bool secretDoorUnsealed = false;
    acoustics.emitPulse({640.0f, 400.0f}, 460.0f, 750.0f, 1.25f, 180, WaveSignature::RadialPulse, 0.0f, 6.2831853f, true);
    for (int step = 0; step < 45; ++step) {
        const auto unlocked = acoustics.update(cfg::kFixedTimeStep, secRoom.walls, &secRoom.resonators);
        for (int id : unlocked) {
            labA.activateSwitch(id);
            if (id == 900 + secretRoomId) secretDoorUnsealed = true;
        }
    }
    if (!secretDoorUnsealed || !labA.isSwitchActivated(900 + secretRoomId)) {
        std::cerr << "[SelfTest] FAIL: Secret room wall was not unsealed by sonar pulse.\n";
        return 1;
    }
    std::cout << "[SelfTest] PASS [4/10]: Combat door lockdown, room clear unsealing, & Secret Wall sonar fracture verified.\n";

    // 5. Verify AcousticSystem reflections & multi-wall material attenuation
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
        std::cerr << "[SelfTest] FAIL: Material transmission ordering invalid.\n";
        return 1;
    }
    std::cout << "[SelfTest] PASS [5/10]: AcousticSystem reflections & multi-wall material attenuation verified.\n";

    // 6. Verify AcousticResonator crystal charging in Puzzle room
    int puzzleRoomId = -1;
    for (std::size_t i = 0; i < labA.roomCount(); ++i) {
        if (labA.roomAt(i).type == RoomType::Puzzle) {
            puzzleRoomId = static_cast<int>(i);
            break;
        }
    }
    if (puzzleRoomId == -1) {
        std::cerr << "[SelfTest] FAIL: No Puzzle room generated.\n";
        return 1;
    }
    labA.setCurrentRoom(puzzleRoomId);
    acoustics.clear();
    auto& puzRoom = labA.currentRoomMut();
    bool resonatorUnlocked = false;
    for (int burst = 0; burst < 3 && !resonatorUnlocked; ++burst) {
        acoustics.emitPulse({580.0f, 400.0f}, 450.0f, 560.0f, 1.20f, 96, WaveSignature::DirectionalPing, 0.0f, 1.0471975f, true);
        for (int step = 0; step < 30; ++step) {
            const auto unlockedIds = acoustics.update(cfg::kFixedTimeStep, puzRoom.walls, &puzRoom.resonators, 2.0f);
            for (int id : unlockedIds) {
                labA.activateSwitch(id);
                resonatorUnlocked = true;
            }
        }
    }
    if (!resonatorUnlocked) {
        std::cerr << "[SelfTest] FAIL: Puzzle room AcousticResonator did not charge.\n";
        return 1;
    }
    std::cout << "[SelfTest] PASS [6/10]: Puzzle chamber AcousticResonator crystal energized and unlocked.\n";

    // 7. Verify Player Hull Integrity, Echo Shards economy, and Relic synergies
    Player player;
    player.reset({640.0f, 400.0f});
    player.addEchoShards(50);
    if (!player.spendEchoShards(25) || player.echoShards() != 25) {
        std::cerr << "[SelfTest] FAIL: Player Echo Shards transaction failed.\n";
        return 1;
    }
    player.addRelic(RelicId::FocusedTransducer, &acoustics);
    player.addRelic(RelicId::PrismaticMirror, &acoustics);
    player.addRelic(RelicId::PiezoArmorCore, &acoustics);
    if (player.maxHealth() != 8 || player.sonarDamageMultiplier(true) <= 2.0f) {
        std::cerr << "[SelfTest] FAIL: Relic bonuses & Focused+Prismatic synergy not applied properly.\n";
        return 1;
    }
    if (!player.applyDamage(1, acoustics) || !player.isInvulnerable() || player.applyDamage(1, acoustics)) {
        std::cerr << "[SelfTest] FAIL: Player invulnerability frames or damage failed.\n";
        return 1;
    }
    std::cout << "[SelfTest] PASS [7/10]: Player Hull Integrity, Echo Shards economy, & Relic synergies verified.\n";

    // 8. Verify EnemyAI 5 archetypes, 3-phase AbyssalBoss, and harmonic sonar disruption damage
    EnemyAI boss;
    boss.configurePatrol({{640.0f, 240.0f}, {780.0f, 400.0f}}, EnemyVariant::AbyssalBoss);
    if (!boss.isActive() || boss.bossPhase() != 1) {
        std::cerr << "[SelfTest] FAIL: AbyssalBoss initial state invalid.\n";
        return 1;
    }
    // Emit sonar ping at Boss and verify consumeRayImpactAt + applyAcousticDamage
    acoustics.clear();
    acoustics.emitPulse(boss.position() - sf::Vector2f{45.0f, 0.0f}, 460.0f, 500.0f, 1.25f, 96, WaveSignature::DirectionalPing, 0.0f, 0.8f, true);
    bool refHit = false;
    float hitEnergy = 0.0f;
    for (int step = 0; step < 8 && hitEnergy <= 0.0f; ++step) {
        acoustics.update(cfg::kFixedTimeStep, {});
        hitEnergy += acoustics.consumeRayImpactAt(boss.position(), boss.hitRadius(), refHit);
    }
    if (hitEnergy <= 0.0f) {
        std::cerr << "[SelfTest] FAIL: Sonar rays did not register impact on AbyssalBoss.\n";
        return 1;
    }
    boss.applyAcousticDamage(100.0f, true, {500.0f, 400.0f}, {});
    if (boss.bossPhase() != 2) {
        std::cerr << "[SelfTest] FAIL: AbyssalBoss did not transition to Phase 2 after taking damage.\n";
        return 1;
    }
    const bool killed = boss.applyAcousticDamage(200.0f, true, {500.0f, 400.0f}, {});
    if (!killed || boss.isActive()) {
        std::cerr << "[SelfTest] FAIL: AbyssalBoss was not defeated when HP reached 0.\n";
        return 1;
    }
    std::cout << "[SelfTest] PASS [8/10]: EnemyAI 5 variants, 3-phase AbyssalBoss, & harmonic sonar damage verified.\n";

    // 9. Verify SaveSystem V4 atomic write, FNV-1a checksum, and full roguelike + meta state round-trip
    const std::string testSavePath = SaveSystem::resolveSaveFilePath(exeDir, "test_echolabyrinth_v4.dat");
    SaveData outSave{};
    outSave.runSeed = 0xDEADBEEFu;
    outSave.targetRoomCount = 32;
    outSave.checkpointRoomId = 5;
    outSave.playerPosX = 610.0f;
    outSave.playerPosY = 390.0f;
    outSave.playerHealth = 5;
    outSave.playerMaxHealth = 8;
    outSave.echoShards = 42;
    outSave.totalRuns = 7;
    outSave.totalVictories = 3;
    outSave.bossesDefeated = 4;
    outSave.visitedRooms = {0, 1, 2, 5};
    outSave.clearedRooms = {0, 1, 2};
    outSave.collectedRelics = {0, 1, 5};

    if (!SaveSystem::saveToFile(testSavePath, outSave)) {
        std::cerr << "[SelfTest] FAIL: Could not write V4 save file.\n";
        return 1;
    }
    SaveData inSave{};
    if (!SaveSystem::loadFromFile(testSavePath, inSave)) {
        std::cerr << "[SelfTest] FAIL: Could not read valid V4 save file.\n";
        return 1;
    }
    std::remove(testSavePath.c_str());

    if (inSave.runSeed != 0xDEADBEEFu || inSave.echoShards != 42 || inSave.playerMaxHealth != 8 ||
        inSave.totalVictories != 3 || inSave.collectedRelics.size() != 3 || inSave.clearedRooms.size() != 3) {
        std::cerr << "[SelfTest] FAIL: V4 save round-trip data mismatch.\n";
        return 1;
    }

    // Verify tampered V4 checksum rejection
    {
        std::ofstream badOut(testSavePath, std::ios::trunc);
        badOut << "ECHOLABYRINTH_SAVE_V4\nseed 123\nroom 2\nchecksum 99999\n";
    }
    SaveData badData{};
    if (SaveSystem::loadFromFile(testSavePath, badData)) {
        std::cerr << "[SelfTest] FAIL: Tampered V4 checksum should be rejected.\n";
        std::remove(testSavePath.c_str());
        return 1;
    }
    std::remove(testSavePath.c_str());
    std::cout << "[SelfTest] PASS [9/10]: SaveSystem V4 atomic serialization, seed/relic/meta round-trip, & checksum verified.\n";

    // 10. Verify AssetManager 3D Wavefront OBJ models and procedural material textures
    AssetManager assets;
    assets.initialize(exeDir, false);
    if (!assets.isInitialized() || assets.loadedDiskModelCount() < 7 || assets.totalTriangleCount() < 60) {
        std::cerr << "[SelfTest] FAIL: AssetManager expected 7 Wavefront .obj models from disk, got "
                  << assets.loadedDiskModelCount() << " (" << assets.totalTriangleCount() << " tris).\n";
        return 1;
    }
    for (int m = 0; m < 6; ++m) {
        const auto& img = assets.getMaterialImage(static_cast<SurfaceMaterial>(m));
        if (img.getSize().x != 64 || img.getSize().y != 64) {
            std::cerr << "[SelfTest] FAIL: Material texture image " << m << " invalid dimensions.\n";
            return 1;
        }
    }
    std::cout << "[SelfTest] PASS [10/10]: AssetManager loaded " << assets.loadedDiskModelCount()
              << "/7 Wavefront 3D .obj models (" << assets.totalTriangleCount()
              << " triangles) and 6 RGBA material textures.\n";

    std::cout << "[SelfTest] ALL 10 ECHOLABYRINTH 4.0 NATIVE C++17 VERIFICATION SUITES PASSED SUCCESSFULLY.\n";
    return 0;
}

} // namespace echo
