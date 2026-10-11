#include "Game.hpp"
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <algorithm>
#include <cmath>
#include <string>

namespace echo {

namespace {
float distSqRender(sf::Vector2f a, sf::Vector2f b) noexcept {
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    return dx * dx + dy * dy;
}
} // namespace

void Game::render() {
    if (!m_window) return;
    auto& win = *m_window;
    win.clear(sf::Color(3, 6, 12));

    sf::View activeView = m_worldView;
    if (m_cameraShakeTimer > 0.0f) {
        const float mag = m_cameraShakeTimer * 12.0f;
        activeView.move(
            std::sin(m_runTimeSec * 55.0f) * mag,
            std::cos(m_runTimeSec * 48.0f) * mag
        );
    }
    win.setView(activeView);

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

    win.display();
}

void Game::renderWorld() {
    if (!m_window) return;
    auto& win = *m_window;
    const auto& room = m_labyrinth.currentRoom();

    renderSubterraneanProps();
    m_acoustics.render(win, m_ambientVisibility, &room.walls);
    renderInteractiveObjects();

    if (m_state == AppState::Playing || m_state == AppState::Paused || m_state == AppState::GameOver) {
        for (const auto& enemy : m_enemies) {
            if (!enemy.isActive()) continue;
            const float enemyVis = m_acoustics.queryIlluminationAt(enemy.position(), m_player.position());
            enemy.render(win, enemyVis, m_player.position(), &m_assets);
        }
        m_player.render(win, &m_assets);
    }

    renderVignetteAndDust();
}

void Game::renderSubterraneanProps() {
    if (!m_window) return;
    auto& win = *m_window;
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
            win.draw(pipe);
        } else if (prop.type == PropType::WaterPuddle) {
            sf::CircleShape puddle(prop.scale);
            puddle.setOrigin(prop.scale, prop.scale);
            puddle.setPosition(prop.position);
            puddle.setScale(1.35f, 0.75f);
            puddle.setFillColor(sf::Color(12, 42, 54, static_cast<sf::Uint8>(alpha * 0.45f)));
            puddle.setOutlineThickness(1.0f);
            puddle.setOutlineColor(sf::Color(45, 212, 191, static_cast<sf::Uint8>(alpha * 0.65f)));
            win.draw(puddle);
        } else if (prop.type == PropType::VentTurbine) {
            const float rad = prop.rotationDeg * 0.01745329f;
            m_assets.renderMesh25D(
                win,
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
            win.draw(glow);
        }
    }
}

void Game::renderInteractiveObjects() {
    if (!m_window) return;
    auto& win = *m_window;
    const auto& room = m_labyrinth.currentRoom();
    const sf::Vector2f pPos = m_player.position();
    const float visorBoost = m_player.hasRelic(RelicId::SubharmonicVisor) ? 0.10f : 0.0f;

    // 1. Render N/E/S/W Portals
    for (const auto& portal : room.portals) {
        const bool unlocked = (portal.requiredSwitchId == -1) || m_labyrinth.isSwitchActivated(portal.requiredSwitchId);
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
        win.draw(ring);

        if (vis > 0.35f) {
            const float labelY = (portal.direction == CardinalDir::North)
                ? (portal.position.y + portal.radius + 8.0f)
                : (portal.position.y - portal.radius - 18.0f);
            m_ui.drawText(
                win,
                unlocked ? portal.label : ("SEALED: " + portal.label),
                {portal.position.x, labelY},
                10.0f,
                col,
                TextAlign::Center
            );
        }
    }

    // 2. Render Relic Pedestals
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
        win.draw(aura);

        m_assets.renderMesh25D(
            win,
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
        m_ui.drawText(win, label, {ped.position.x, ped.position.y - 34.0f}, 10.5f, spec.tint, TextAlign::Center, true);
        if (distSqRender(pPos, ped.position) <= 85.0f * 85.0f) {
            m_ui.drawText(win, spec.effectSummary, {ped.position.x, ped.position.y + 24.0f}, 9.5f, sf::Color(224, 242, 254), TextAlign::Center);
        }
    }

    // 3. Render Quartermaster Shop Offers
    for (const auto& offer : room.shopOffers) {
        if (offer.purchased) continue;
        const float vis = std::max(0.55f, m_acoustics.queryIlluminationAt(offer.position, pPos));
        const auto alpha = static_cast<sf::Uint8>(vis * 240.0f);
        const sf::Color col(250, 204, 21, alpha);

        m_assets.renderMesh25D(
            win,
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
        m_ui.drawText(win, tag, {offer.position.x, offer.position.y - 30.0f}, 10.0f, col, TextAlign::Center);
    }

    // 4. Render Interactive Mechanisms
    for (const auto& mech : room.mechanisms) {
        const float vis = std::max(0.28f + visorBoost, m_acoustics.queryIlluminationAt(mech.position, pPos));
        if (vis <= 0.05f) continue;

        const auto alpha = static_cast<sf::Uint8>(vis * 240.0f);
        const sf::Color col = mech.activated
            ? sf::Color(52, 211, 153, alpha)
            : sf::Color(250, 204, 21, alpha);

        const float leverTilt = (mech.animProgress - 0.5f) * 0.55f;
        m_assets.renderMesh25D(
            win,
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

        if (distSqRender(pPos, mech.position) <= 68.0f * 68.0f || vis > 0.45f) {
            m_ui.drawText(
                win,
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
        win.draw(ring);

        m_assets.renderMesh25D(
            win,
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
            m_ui.drawText(win, status, {res.position.x, res.position.y - 28.0f}, 10.5f, col, TextAlign::Center);
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
        win.draw(zone);
    }

    // 7. Render Surveyor Log Terminals
    for (const auto& lore : room.collectibles) {
        if (lore.collected) continue;
        const float vis = std::max(0.26f + visorBoost, m_acoustics.queryIlluminationAt(lore.position, pPos));
        if (vis <= 0.05f) continue;

        const auto alpha = static_cast<sf::Uint8>(vis * 240.0f);
        const sf::Color col(56, 189, 248, alpha);

        m_assets.renderMesh25D(
            win,
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

        if (distSqRender(pPos, lore.position) <= 68.0f * 68.0f || vis > 0.45f) {
            m_ui.drawText(
                win,
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
    if (!m_window) return;
    auto& win = *m_window;
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
    win.draw(dust);
}

} // namespace echo
