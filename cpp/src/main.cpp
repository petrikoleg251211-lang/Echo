#include "AcousticSystem.hpp"
#include "Config.hpp"
#include "EnemyAI.hpp"
#include "Labyrinth.hpp"
#include "Player.hpp"
#include "SaveSystem.hpp"

#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <cmath>
#include <vector>

namespace echo {

// Procedural PCM audio buffer generator so the SFML binary works without external .wav files
sf::SoundBuffer synthesizeSonarPingBuffer(float frequencyHz, float durationSec) {
    constexpr unsigned int sampleRate = 44100;
    const std::size_t sampleCount = static_cast<std::size_t>(sampleRate * durationSec);
    std::vector<sf::Int16> samples(sampleCount, 0);

    for (std::size_t i = 0; i < sampleCount; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(sampleRate);
        const float envelope = std::exp(-t * 6.5f);
        const float wave = std::sin(2.0f * 3.14159265f * frequencyHz * t)
                         + 0.35f * std::sin(2.0f * 3.14159265f * (frequencyHz * 1.5f) * t);
        samples[i] = static_cast<sf::Int16>(wave * envelope * 14000.0f);
    }

    sf::SoundBuffer buffer;
    buffer.loadFromSamples(samples.data(), samples.size(), 1, sampleRate);
    return buffer;
}

} // namespace echo

int main() {
    using namespace echo;

    sf::RenderWindow window(
        sf::VideoMode(cfg::kWindowWidth, cfg::kWindowHeight),
        "EchoLabyrinth_SFML — Acoustic Horror Labyrinth",
        sf::Style::Titlebar | sf::Style::Close
    );
    window.setFramerateLimit(120);

    AcousticSystem acoustics;
    Labyrinth labyrinth;
    Player player;
    EnemyAI enemy;

    auto enterRoom = [&](int roomId, sf::Vector2f spawnPos) {
        labyrinth.setCurrentRoom(roomId);
        acoustics.clear();
        player.reset(spawnPos);
        enemy.configurePatrol(labyrinth.currentRoom().enemyPatrolNodes);
        acoustics.emitPulse(spawnPos, 360.0f, 420.0f, 0.9f, 90, WaveSignature::RadialPulse);
    };

    enterRoom(0, labyrinth.currentRoom().playerDefaultSpawn);

    const sf::SoundBuffer pingBuf = synthesizeSonarPingBuffer(528.0f, 0.45f);
    const sf::SoundBuffer pulseBuf = synthesizeSonarPingBuffer(264.0f, 0.75f);
    sf::Sound pingSound(pingBuf);
    sf::Sound pulseSound(pulseBuf);

    sf::Clock clock;
    float accumulator = 0.0f;
    bool paused = false;

    while (window.isOpen()) {
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            } else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) {
                    paused = !paused;
                } else if (event.key.code == sf::Keyboard::Space && !paused) {
                    if (player.triggerRadialPulse(acoustics)) {
                        pulseSound.play();
                    }
                } else if (event.key.code == sf::Keyboard::F && !paused) {
                    auto& room = labyrinth.currentRoomMut();
                    for (auto& mech : room.mechanisms) {
                        const sf::Vector2f d = mech.position - player.position();
                        if ((d.x * d.x + d.y * d.y) <= 48.0f * 48.0f && !mech.activated) {
                            labyrinth.activateSwitch(mech.id);
                            acoustics.emitPulse(mech.position, 420.0f, 620.0f, 1.25f, 100, WaveSignature::MechanismClunk);
                        }
                    }
                } else if (event.key.code == sf::Keyboard::F5) {
                    SaveData data{labyrinth.currentRoomId(), player.decoyCount(), {}, labyrinth.collectedLoreIds()};
                    SaveSystem::saveToFile("echolabyrinth_save.dat", data);
                } else if (event.key.code == sf::Keyboard::F9) {
                    SaveData data{};
                    if (SaveSystem::loadFromFile("echolabyrinth_save.dat", data)) {
                        enterRoom(data.checkpointRoomId, labyrinth.currentRoom().playerDefaultSpawn);
                    }
                }
            } else if (event.type == sf::Event::MouseButtonPressed && !paused) {
                const sf::Vector2f mouseWorld = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                if (event.mouseButton.button == sf::Mouse::Left) {
                    if (player.triggerDirectionalPing(mouseWorld, acoustics)) {
                        pingSound.play();
                    }
                } else if (event.mouseButton.button == sf::Mouse::Right) {
                    if (player.throwDecoy(mouseWorld, acoustics)) {
                        pulseSound.play();
                    }
                }
            }
        }

        const float frameDt = std::min(clock.restart().asSeconds(), 0.1f);
        if (!paused) {
            accumulator += frameDt;
            while (accumulator >= cfg::kFixedTimeStep) {
                sf::Vector2f moveInput{0.0f, 0.0f};
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) moveInput.y -= 1.0f;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) moveInput.y += 1.0f;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) moveInput.x -= 1.0f;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) moveInput.x += 1.0f;

                MovementStance stance = MovementStance::Walk;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) stance = MovementStance::Sneak;
                else if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) stance = MovementStance::Sprint;

                const auto& room = labyrinth.currentRoom();
                player.update(cfg::kFixedTimeStep, moveInput, stance, room.walls, acoustics);
                enemy.update(cfg::kFixedTimeStep, player.position(), room.walls, acoustics);
                acoustics.update(cfg::kFixedTimeStep, room.walls);
                acoustics.clearRecentEvents();

                // Check room portals
                for (const auto& portal : room.portals) {
                    if (portal.requiredSwitchId != -1 && !labyrinth.isSwitchActivated(portal.requiredSwitchId)) {
                        continue;
                    }
                    const sf::Vector2f d = portal.position - player.position();
                    if ((d.x * d.x + d.y * d.y) <= portal.radius * portal.radius) {
                        enterRoom(portal.targetRoomId, portal.targetSpawnPos);
                        break;
                    }
                }

                if (enemy.hasCaughtPlayer()) {
                    enterRoom(labyrinth.currentRoomId(), labyrinth.currentRoom().playerDefaultSpawn);
                }

                accumulator -= cfg::kFixedTimeStep;
            }
        }

        // Pitch-black atmospheric clear
        window.clear(sf::Color(6, 8, 12));

        // Render propagating sound waves & wall phosphor echoes
        acoustics.render(window);

        // Render subtle player core ring
        sf::CircleShape playerMarker(cfg::kPlayerRadius);
        playerMarker.setOrigin(cfg::kPlayerRadius, cfg::kPlayerRadius);
        playerMarker.setPosition(player.position());
        playerMarker.setFillColor(sf::Color(15, 23, 42, 200));
        playerMarker.setOutlineThickness(1.5f);
        playerMarker.setOutlineColor(sf::Color(56, 189, 248, 210));
        window.draw(playerMarker);

        window.display();
    }

    return 0;
}
