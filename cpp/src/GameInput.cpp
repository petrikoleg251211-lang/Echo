#include "Game.hpp"
#include <SFML/Window/Event.hpp>
#include <algorithm>
#include <string>

namespace echo {

namespace {
float distSqInput(sf::Vector2f a, sf::Vector2f b) noexcept {
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

void Game::rebuildMenuButtons() {
    m_menuButtons.clear();
    m_selectedMenuIndex = 0;

    const float cx = cfg::kWorldWidth * 0.5f;

    if (m_state == AppState::MainMenu) {
        const bool hasSave = SaveSystem::hasSaveFile(m_saveFilePath);
        m_menuButtons.push_back({"START NEW ROGUELIKE RUN", {cx, 310.0f}, {390.0f, 42.0f}, true});
        m_menuButtons.push_back({"CONTINUE SAVED EXPEDITION", {cx, 362.0f}, {390.0f, 42.0f}, hasSave});
        m_menuButtons.push_back({"AUDIO-LORE CODEX [J]", {cx, 414.0f}, {390.0f, 42.0f}, true});
        m_menuButtons.push_back({"AUDIO & ACOUSTIC SETTINGS", {cx, 466.0f}, {390.0f, 42.0f}, true});
        m_menuButtons.push_back({"SURVEYOR FIELD MANUAL", {cx, 518.0f}, {390.0f, 42.0f}, true});
        m_menuButtons.push_back({"EXIT TO DESKTOP", {cx, 570.0f}, {390.0f, 42.0f}, true});
    } else if (m_state == AppState::Paused) {
        m_menuButtons.push_back({"RESUME EXPEDITION", {cx, 290.0f}, {380.0f, 40.0f}, true});
        m_menuButtons.push_back({"SAVE CHECKPOINT [F5]", {cx, 340.0f}, {380.0f, 40.0f}, true});
        m_menuButtons.push_back({"AUDIO-LORE CODEX [J]", {cx, 390.0f}, {380.0f, 40.0f}, true});
        m_menuButtons.push_back({"START NEW SEED RUN", {cx, 440.0f}, {380.0f, 40.0f}, true});
        m_menuButtons.push_back({"AUDIO & ACOUSTIC SETTINGS", {cx, 490.0f}, {380.0f, 40.0f}, true});
        m_menuButtons.push_back({"SURVEYOR FIELD MANUAL", {cx, 540.0f}, {380.0f, 40.0f}, true});
        m_menuButtons.push_back({"RETURN TO MAIN MENU", {cx, 590.0f}, {380.0f, 40.0f}, true});
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
    } else if (m_state == AppState::Controls || m_state == AppState::Codex) {
        m_menuButtons.push_back({"BACK", {cx, 665.0f}, {320.0f, 44.0f}, true});
    } else if (m_state == AppState::GameOver || m_state == AppState::Victory) {
        m_menuButtons.push_back({"START NEW PROCEDURAL RUN", {cx, 465.0f}, {390.0f, 46.0f}, true});
        m_menuButtons.push_back({"RESTORE LAST CHECKPOINT", {cx, 525.0f}, {390.0f, 46.0f}, SaveSystem::hasSaveFile(m_saveFilePath)});
        m_menuButtons.push_back({"RETURN TO MAIN MENU", {cx, 585.0f}, {390.0f, 46.0f}, true});
    }
}

void Game::executeMenuSelection(std::size_t index) {
    if (index >= m_menuButtons.size() || !m_menuButtons[index].enabled) return;
    if (m_audio) m_audio->playEffect(SoundEffectId::UiSelect, 0.8f, 1.0f);

    if (m_state == AppState::MainMenu) {
        if (index == 0) startNewGame();
        else if (index == 1) loadCheckpoint();
        else if (index == 2) { m_previousState = AppState::MainMenu; m_state = AppState::Codex; rebuildMenuButtons(); }
        else if (index == 3) { m_previousState = AppState::MainMenu; m_state = AppState::Settings; rebuildMenuButtons(); }
        else if (index == 4) { m_previousState = AppState::MainMenu; m_state = AppState::Controls; rebuildMenuButtons(); }
        else if (index == 5) { if (m_window) m_window->close(); }
    } else if (m_state == AppState::Paused) {
        if (index == 0) m_state = AppState::Playing;
        else if (index == 1) {
            saveCheckpoint();
            m_statusBannerText = "CHECKPOINT SAVED TO DISK";
            m_statusBannerTimer = 2.5f;
            m_state = AppState::Playing;
        } else if (index == 2) {
            m_previousState = AppState::Paused;
            m_state = AppState::Codex;
            rebuildMenuButtons();
        } else if (index == 3) {
            startNewGame();
        } else if (index == 4) {
            m_previousState = AppState::Paused;
            m_state = AppState::Settings;
            rebuildMenuButtons();
        } else if (index == 5) {
            m_previousState = AppState::Paused;
            m_state = AppState::Controls;
            rebuildMenuButtons();
        } else if (index == 6) {
            m_state = AppState::MainMenu;
            rebuildMenuButtons();
        }
    } else if (m_state == AppState::Settings) {
        if (index == 0 && m_audio) {
            float v = m_audio->masterVolume() + 20.0f;
            if (v > 100.0f) v = 0.0f;
            m_audio->setMasterVolume(v);
        } else if (index == 1 && m_audio) {
            float v = m_audio->musicVolume() + 25.0f;
            if (v > 100.0f) v = 0.0f;
            m_audio->setMusicVolume(v);
        } else if (index == 2 && m_audio) {
            float v = m_audio->sfxVolume() + 25.0f;
            if (v > 100.0f) v = 0.0f;
            m_audio->setSfxVolume(v);
        } else if (index == 3 && m_audio) {
            float v = m_audio->ambientVolume() + 25.0f;
            if (v > 100.0f) v = 0.0f;
            m_audio->setAmbientVolume(v);
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
    } else if (m_state == AppState::Controls || m_state == AppState::Codex) {
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
    if (!m_window) return;
    sf::Event ev{};
    while (m_window->pollEvent(ev)) {
        if (ev.type == sf::Event::Closed) {
            m_window->close();
        } else if (ev.type == sf::Event::Resized) {
            updateLetterboxView(ev.size.width, ev.size.height);
        } else if (ev.type == sf::Event::KeyPressed) {
            handleKeyPressed(ev.key.code);
        } else if (ev.type == sf::Event::MouseMoved) {
            const sf::Vector2f worldPt = m_window->mapPixelToCoords({ev.mouseMove.x, ev.mouseMove.y}, m_worldView);
            handleMouseMoved(worldPt);
        } else if (ev.type == sf::Event::MouseButtonPressed) {
            const sf::Vector2f worldPt = m_window->mapPixelToCoords({ev.mouseButton.x, ev.mouseButton.y}, m_worldView);
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
        if (key == sf::Keyboard::J) {
            m_previousState = AppState::Playing;
            m_state = AppState::Codex;
            rebuildMenuButtons();
            if (m_audio) m_audio->playEffect(SoundEffectId::UiSelect, 0.7f, 1.15f);
            return;
        }
        if (key == sf::Keyboard::Tab || key == sf::Keyboard::M) {
            m_showMinimap = !m_showMinimap;
            if (m_audio) m_audio->playEffect(SoundEffectId::UiMove, 0.7f, 1.1f);
            return;
        }
        if (key == sf::Keyboard::Space) {
            if (m_player.triggerRadialPulse(m_acoustics)) {
                if (m_audio) m_audio->playWaveSignature(WaveSignature::RadialPulse);
                registerNoiseAction(NoiseAction::RadialPulse, 1.0f, m_player.position());
            }
        } else if (key == sf::Keyboard::E) {
            if (m_player.triggerDirectionalPing(m_mouseWorldPos, m_acoustics)) {
                if (m_audio) m_audio->playWaveSignature(WaveSignature::DirectionalPing);
                registerNoiseAction(NoiseAction::DirectionalPing, 1.0f, m_player.position());
            }
        } else if (key == sf::Keyboard::Q) {
            // Block 3.1: Narrow 15-deg Whistle beam (long range, only +5 noise)
            if (m_player.triggerWhistle(m_mouseWorldPos, m_acoustics)) {
                if (m_audio) m_audio->playWaveSignature(WaveSignature::WhistleBeam);
                registerNoiseAction(NoiseAction::WhistlePing, 1.0f, m_player.position());
            }
        } else if (key == sf::Keyboard::C || key == sf::Keyboard::V) {
            // Block 3.2: Wide Stomp shockwave (+35 noise, pushes enemies, shatters Glass/Shale)
            executePlayerStomp();
        } else if (key == sf::Keyboard::R || key == sf::Keyboard::X) {
            // Block 3.3: Delayed Echo Trap / Beacon Mine (3.0s countdown)
            if (m_player.placeEchoTrap()) {
                if (m_audio) m_audio->playEffect(SoundEffectId::SwitchClunk, 0.75f, 1.35f);
                m_statusBannerText = "ECHO BEACON MINE ARMED (3.0S DELAY -> 3 SONAR LURE BURSTS)";
                m_statusBannerTimer = 2.6f;
            }
        } else if (key == sf::Keyboard::Num1) {
            m_player.setFrequency(AcousticFrequency::Low, m_acoustics);
            if (m_audio) m_audio->playEffect(SoundEffectId::UiMove, 0.7f, 0.80f);
            m_statusBannerText = "SONAR TUNED: LOW FREQUENCY [1] (WALL PENETRATION & LONG PERSISTENCE)";
            m_statusBannerTimer = 2.2f;
        } else if (key == sf::Keyboard::Num2) {
            m_player.setFrequency(AcousticFrequency::Mid, m_acoustics);
            if (m_audio) m_audio->playEffect(SoundEffectId::UiMove, 0.7f, 1.0f);
            m_statusBannerText = "SONAR TUNED: MID FREQUENCY [2] (BALANCED SPECTRUM)";
            m_statusBannerTimer = 2.2f;
        } else if (key == sf::Keyboard::Num3) {
            m_player.setFrequency(AcousticFrequency::High, m_acoustics);
            if (m_audio) m_audio->playEffect(SoundEffectId::UiMove, 0.7f, 1.30f);
            m_statusBannerText = "SONAR TUNED: HIGH FREQUENCY [3] (+1 METAL BOUNCE, UNMASKS MIMICS & SWARMS)";
            m_statusBannerTimer = 2.2f;
        } else if (key == sf::Keyboard::G) {
            m_player.throwDecoy(m_mouseWorldPos, m_labyrinth.currentRoom().walls);
        } else if (key == sf::Keyboard::F) {
            tryInteract();
        } else if (key == sf::Keyboard::F5) {
            saveCheckpoint();
            m_statusBannerText = "CHECKPOINT SAVED [F5]";
            m_statusBannerTimer = 2.5f;
            if (m_audio) m_audio->playEffect(SoundEffectId::UiSelect, 0.8f, 1.2f);
        } else if (key == sf::Keyboard::F9) {
            loadCheckpoint();
        }
        return;
    }

    if (key == sf::Keyboard::Escape || (m_state == AppState::Codex && key == sf::Keyboard::J)) {
        if (m_state == AppState::Paused) {
            m_state = AppState::Playing;
        } else if (m_state == AppState::Settings || m_state == AppState::Controls || m_state == AppState::Codex) {
            m_state = m_previousState;
            rebuildMenuButtons();
        }
        return;
    }

    if (key == sf::Keyboard::Up || key == sf::Keyboard::W) {
        if (!m_menuButtons.empty()) {
            m_selectedMenuIndex = (m_selectedMenuIndex + m_menuButtons.size() - 1) % m_menuButtons.size();
            if (m_audio) m_audio->playEffect(SoundEffectId::UiMove, 0.6f, 1.0f);
        }
    } else if (key == sf::Keyboard::Down || key == sf::Keyboard::S) {
        if (!m_menuButtons.empty()) {
            m_selectedMenuIndex = (m_selectedMenuIndex + 1) % m_menuButtons.size();
            if (m_audio) m_audio->playEffect(SoundEffectId::UiMove, 0.6f, 1.0f);
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
                    if (m_audio) m_audio->playEffect(SoundEffectId::UiMove, 0.5f, 1.05f);
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
                if (m_audio) m_audio->playWaveSignature(WaveSignature::DirectionalPing);
                registerNoiseAction(NoiseAction::DirectionalPing, 1.0f, m_player.position());
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

    // 1. Check Relic Pedestals
    for (auto& ped : room.relicPedestals) {
        if (ped.collected) continue;
        if (!room.cleared) {
            m_statusBannerText = "DEFEAT ALL HUNTERS BEFORE CLAIMING RELIC";
            m_statusBannerTimer = 2.2f;
            return;
        }
        if (distSqInput(pPos, ped.position) <= 52.0f * 52.0f) {
            const RelicSpec spec = getRelicSpec(ped.relicId);
            if (ped.costShards > 0) {
                if (!m_player.spendEchoShards(ped.costShards)) {
                    m_statusBannerText = "INSUFFICIENT ECHO SHARDS (NEED " + std::to_string(ped.costShards) + ")";
                    m_statusBannerTimer = 2.4f;
                    if (m_audio) m_audio->playEffect(SoundEffectId::UiMove, 0.8f, 0.7f);
                    return;
                }
            }
            ped.collected = true;
            m_player.addRelic(ped.relicId, &m_acoustics);
            if (ped.relicId == RelicId::SubharmonicVisor) {
                m_labyrinth.revealSecretRoomsOnMap();
            }
            if (m_audio) m_audio->playEffect(SoundEffectId::VictoryChord, 0.85f, 1.15f);
            m_acoustics.emitPulse(ped.position, 420.0f, 520.0f, 1.15f, 96, WaveSignature::ResonatorHum, 0.0f, 6.2831853f, true);
            m_statusBannerText = std::string("RELIC ACQUIRED: ") + spec.name + " (" + spec.effectSummary + ")";
            m_statusBannerTimer = 4.2f;
            return;
        }
    }

    // 2. Check Quartermaster Shop Offers
    for (auto& offer : room.shopOffers) {
        if (offer.purchased) continue;
        if (distSqInput(pPos, offer.position) <= 50.0f * 50.0f) {
            if (!m_player.spendEchoShards(offer.costShards)) {
                m_statusBannerText = "INSUFFICIENT ECHO SHARDS (NEED " + std::to_string(offer.costShards) + ")";
                m_statusBannerTimer = 2.4f;
                if (m_audio) m_audio->playEffect(SoundEffectId::UiMove, 0.8f, 0.7f);
                return;
            }
            offer.purchased = true;
            if (offer.healAmount > 0) m_player.heal(offer.healAmount);
            if (offer.decoyAmount > 0) {
                m_player.addDecoy(offer.decoyAmount);
                m_player.addEchoTrap(2);
            }
            if (m_audio) m_audio->playEffect(SoundEffectId::LorePickup, 0.95f, 1.1f);
            m_statusBannerText = "PURCHASED: " + offer.label;
            m_statusBannerTimer = 3.0f;
            return;
        }
    }

    // 3. Check Interactive Mechanisms (Sanctuary Repair Shrines & Harmonic Levers)
    for (auto& mech : room.mechanisms) {
        if (!mech.activated && distSqInput(pPos, mech.position) <= 52.0f * 52.0f) {
            if (mech.id >= 500) {
                mech.activated = true;
                m_player.heal(2);
                m_player.restoreStamina(100.0f);
                m_noiseLevel = 0.0f;
                saveCheckpoint();
                if (m_audio) m_audio->playEffect(SoundEffectId::ResonatorHum, 0.95f, 1.0f);
                m_acoustics.emitPulse(mech.position, 410.0f, 540.0f, 1.15f, 96, WaveSignature::ResonatorHum, 0.0f, 6.2831853f, true);
                m_statusBannerText = "SANCTUARY SHRINE ACTIVATED: +2 HULL, NOISE RESET, & CHECKPOINT SAVED";
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
            if (m_audio) {
                m_audio->playSpatial(SoundEffectId::SwitchClunk, mech.position, 1.0f, 1.0f, 1.0f);
                m_audio->playSpatial(SoundEffectId::GateSlideOpen, {1045.0f, 400.0f}, 0.85f, 0.95f, 0.92f);
            }
            m_statusBannerText = mech.label + " ENGAGED — RESONANCE BARRIER UNSEALED";
            m_statusBannerTimer = 3.5f;
            return;
        }
    }

    // 4. Check Surveyor Log Collectibles & Caches (Block 5.3 Audio-Lore Codex)
    for (auto& lore : room.collectibles) {
        if (!lore.collected && distSqInput(pPos, lore.position) <= 50.0f * 50.0f) {
            m_labyrinth.markLoreCollected(lore.id);
            if (std::find(m_discoveredCodexIds.begin(), m_discoveredCodexIds.end(), lore.id) == m_discoveredCodexIds.end()) {
                m_discoveredCodexIds.push_back(lore.id);
            }
            m_player.addDecoy(1);
            m_player.addEchoShards(10);
            m_activeLoreTitle = lore.title;
            m_activeLoreBody = lore.excerpt;
            m_loreDisplayTimer = 7.5f;
            if (m_audio) m_audio->playEffect(SoundEffectId::LorePickup, 0.9f, 1.0f);
            m_statusBannerText = "CODEX LOG RECOVERED: " + lore.title + " [PRESS J FOR CODEX] (+1 DECOY, +10 SHARDS)";
            m_statusBannerTimer = 3.6f;
            return;
        }
    }
}

} // namespace echo
