#include "Game.hpp"
#include <SFML/Graphics/PrimitiveType.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace echo {

void Game::renderNoiseWaveMeter(sf::Vector2f topLeft, sf::Vector2f size) {
    if (!m_window) return;
    auto& win = *m_window;

    // Background panel for the Oscilloscope Wave Noise Meter (0..100)
    m_ui.drawPanel(
        win,
        topLeft,
        size,
        sf::Color(10, 18, 32, 230),
        sf::Color(45, 72, 108, 210),
        1.2f
    );

    const float ratio = std::clamp(m_noiseLevel / cfg::noise::kMaxNoise, 0.0f, 1.0f);
    const NoiseTier tier = classifyNoiseTier(m_noiseLevel);

    sf::Color waveCol(52, 211, 153); // Quiet [0.0, 30.0]
    if (tier == NoiseTier::Audible) {
        waveCol = sf::Color(56, 189, 248); // Audible (30.0, 60.0)
    } else if (tier == NoiseTier::Loud) {
        waveCol = sf::Color(250, 204, 21); // Loud [60.0, 85.0)
    } else if (tier == NoiseTier::Scream) {
        waveCol = sf::Color(244, 63, 94); // Scream [85.0, 100.0]
    }

    // Subtle fill behind active noise portion
    if (ratio > 0.01f) {
        sf::RectangleShape activeBg({(size.x - 4.0f) * ratio, size.y - 4.0f});
        activeBg.setPosition(topLeft.x + 2.0f, topLeft.y + 2.0f);
        activeBg.setFillColor(sf::Color(waveCol.r, waveCol.g, waveCol.b, 55));
        win.draw(activeBg);
    }

    // Threshold markers at 30% (Audible), 60% (Loud), and 85% (Scream)
    const float thresholds[3] = {0.30f, 0.60f, 0.85f};
    sf::VertexArray ticks(sf::Lines, 6);
    for (int i = 0; i < 3; ++i) {
        const float tx = topLeft.x + size.x * thresholds[i];
        ticks[static_cast<std::size_t>(i * 2)] = sf::Vertex({tx, topLeft.y + 1.0f}, sf::Color(148, 163, 184, 150));
        ticks[static_cast<std::size_t>(i * 2 + 1)] = sf::Vertex({tx, topLeft.y + size.y - 1.0f}, sf::Color(148, 163, 184, 150));
    }
    win.draw(ticks);

    // Sinusoidal acoustic waveform across the bar width
    constexpr int kSamples = 48;
    sf::VertexArray wave(sf::LineStrip, static_cast<std::size_t>(kSamples + 1));
    const float midY = topLeft.y + size.y * 0.5f;
    const float maxAmp = (size.y * 0.42f) * (0.15f + 0.85f * ratio);
    const float freq = 4.0f + ratio * 14.0f;

    for (int s = 0; s <= kSamples; ++s) {
        const float u = static_cast<float>(s) / static_cast<float>(kSamples);
        const float x = topLeft.x + 2.0f + u * (size.x - 4.0f);
        const bool inActiveRegion = (u <= ratio + 0.02f);
        const float envelope = inActiveRegion ? std::sin(u * 3.1415926f) : 0.12f;
        const float yOffset = std::sin(u * freq * 3.1415926f - m_runTimeSec * (8.0f + ratio * 14.0f)) * maxAmp * envelope;
        const sf::Color ptCol = inActiveRegion
            ? waveCol
            : sf::Color(71, 85, 105, 110);
        wave[static_cast<std::size_t>(s)] = sf::Vertex({x, midY + yOffset}, ptCol);
    }
    win.draw(wave);
}

void Game::renderHud() {
    if (!m_window) return;
    auto& win = *m_window;
    const auto& room = m_labyrinth.currentRoom();

    // Top-left Chamber & Objective Banner
    m_ui.drawPanel(win, {16.0f, 14.0f}, {690.0f, 62.0f}, sf::Color(6, 12, 24, 215), sf::Color(30, 58, 95, 190));
    const std::string header = room.name + "  [" + std::string(Labyrinth::roomTypeName(room.type)) + "]   //   SEED #" + std::to_string(m_labyrinth.runSeed());
    m_ui.drawText(win, header, {28.0f, 22.0f}, 12.5f, sf::Color(56, 189, 248), TextAlign::Left, true);
    m_ui.drawText(win, room.objectiveText, {28.0f, 48.0f}, 10.5f, sf::Color(203, 213, 225));

    // Bottom-left Player Vitality, Stamina, Shards, & Relics Panel
    m_ui.drawPanel(win, {16.0f, 704.0f}, {520.0f, 82.0f}, sf::Color(6, 12, 24, 215), sf::Color(30, 58, 95, 190));

    // Hull Integrity pips
    m_ui.drawText(win, "HULL INTEGRITY:", {28.0f, 712.0f}, 10.5f, sf::Color(148, 163, 184));
    for (int i = 0; i < m_player.maxHealth(); ++i) {
        const bool filled = (i < m_player.health());
        const sf::Vector2f pipPos{28.0f + static_cast<float>(i) * 20.0f, 728.0f};
        m_ui.drawPanel(
            win,
            pipPos,
            {15.0f, 12.0f},
            filled ? sf::Color(56, 189, 248, 235) : sf::Color(15, 23, 42, 200),
            filled ? sf::Color(186, 230, 253) : sf::Color(51, 65, 85),
            1.0f
        );
    }

    // Stamina Bar
    m_ui.drawText(win, "STAMINA", {28.0f, 748.0f}, 10.0f, sf::Color(148, 163, 184));
    m_ui.drawProgressBar(
        win,
        {28.0f, 764.0f},
        {165.0f, 10.0f},
        m_player.stamina() / 100.0f,
        sf::Color(15, 23, 42, 220),
        m_player.stamina() < 25.0f ? sf::Color(244, 63, 94) : sf::Color(34, 211, 238),
        sf::Color(51, 65, 85, 220)
    );

    // Echo Shards, Decoys, Echo Traps, Frequency & Relics counters
    m_ui.drawText(
        win,
        "SHARDS: " + std::to_string(m_player.echoShards()) + "   |   RELICS [TAB]: " + std::to_string(m_player.relics().size()) + "/" + std::to_string(static_cast<int>(RelicId::Count)),
        {212.0f, 714.0f},
        10.5f,
        sf::Color(250, 204, 21),
        TextAlign::Left,
        true
    );
    m_ui.drawText(
        win,
        "DECOYS [RMB]: " + std::to_string(m_player.decoyCount()) + "   |   ECHO TRAPS [R]: " + std::to_string(m_player.echoTrapCount()),
        {212.0f, 736.0f},
        10.5f,
        sf::Color(167, 139, 250)
    );
    m_ui.drawText(
        win,
        std::string("FREQ: ") + frequencyName(m_player.activeFrequency()) + "   |   [Q] WHISTLE  [C] STOMP",
        {212.0f, 758.0f},
        10.0f,
        sf::Color(52, 211, 153)
    );

    // Bottom-Center Block 1: Oscilloscope Wave Noise Meter (0..100) & Ghost Clear Status
    m_ui.drawPanel(win, {548.0f, 704.0f}, {352.0f, 82.0f}, sf::Color(6, 12, 24, 215), sf::Color(30, 58, 95, 190));
    const NoiseTier nTier = classifyNoiseTier(m_noiseLevel);
    sf::Color tierCol(52, 211, 153);
    if (nTier == NoiseTier::Audible) tierCol = sf::Color(56, 189, 248);
    else if (nTier == NoiseTier::Loud) tierCol = sf::Color(250, 204, 21);
    else if (nTier == NoiseTier::Scream) tierCol = sf::Color(244, 63, 94);

    char noiseHeader[64];
    std::snprintf(
        noiseHeader,
        sizeof(noiseHeader),
        "NOISE METER: %d/100 [%s]",
        static_cast<int>(std::round(m_noiseLevel)),
        noiseTierName(nTier)
    );
    m_ui.drawText(win, noiseHeader, {562.0f, 712.0f}, 11.0f, tierCol, TextAlign::Left, true);

    renderNoiseWaveMeter({562.0f, 732.0f}, {324.0f, 20.0f});

    const bool ghostEligible = (m_roomPeakNoise <= cfg::noise::kThresholdAudible);
    const std::string ghostLine = ghostEligible
        ? "GHOST CLEAR READY (PEAK <= 30 -> x1.5 SHARDS)"
        : ("PEAK NOISE: " + std::to_string(static_cast<int>(std::round(m_roomPeakNoise))) + "/100 (GHOST CLEAR LOST)");
    m_ui.drawText(
        win,
        ghostLine,
        {562.0f, 758.0f},
        9.5f,
        ghostEligible ? sf::Color(110, 231, 183) : sf::Color(148, 163, 184)
    );

    // Bottom-right Acoustic Threat & Boss Bar Panel
    m_ui.drawPanel(win, {910.0f, 704.0f}, {352.0f, 82.0f}, sf::Color(6, 12, 24, 215), sf::Color(30, 58, 95, 190));

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
            win,
            "BOSS: THE CHOIR SOVEREIGN (PHASE " + std::to_string(bossPtr->bossPhase()) + ")",
            {924.0f, 714.0f},
            11.5f,
            sf::Color(244, 63, 94),
            TextAlign::Left,
            true
        );
        m_ui.drawProgressBar(
            win,
            {924.0f, 738.0f},
            {324.0f, 12.0f},
            bossPtr->health() / std::max(1.0f, bossPtr->maxHealth()),
            sf::Color(25, 12, 22, 230),
            sf::Color(244, 63, 94),
            sf::Color(159, 18, 57, 230)
        );
        m_ui.drawText(
            win,
            "PING [LMB] & PULSE [SPACE] TO SHATTER",
            {924.0f, 758.0f},
            10.0f,
            sf::Color(203, 213, 225)
        );
    } else if (activeEnemyCount > 0) {
        const std::string threatLine = "HUNTERS ACTIVE: " + std::to_string(activeEnemyCount)
            + " (WAVE " + std::to_string(m_currentWaveIndex + 1) + "/" + std::to_string(room.totalWaves) + ")";
        m_ui.drawText(win, threatLine, {924.0f, 714.0f}, 11.5f, sf::Color(248, 113, 113), TextAlign::Left, true);
        m_ui.drawProgressBar(
            win,
            {924.0f, 738.0f},
            {324.0f, 10.0f},
            highestSuspicion / 100.0f,
            sf::Color(15, 23, 42, 220),
            highestSuspicion > 65.0f ? sf::Color(239, 68, 68) : sf::Color(250, 204, 21),
            sf::Color(51, 65, 85, 220)
        );
        m_ui.drawText(
            win,
            "SONAR PING [LMB] DEALS HARMONIC DAMAGE",
            {924.0f, 756.0f},
            10.0f,
            sf::Color(148, 163, 184)
        );
    } else {
        m_ui.drawText(win, "CHAMBER CLEAR — NO ACOUSTIC THREATS", {924.0f, 722.0f}, 12.0f, sf::Color(52, 211, 153), TextAlign::Left, true);
        m_ui.drawText(win, "[TAB/M] MAP & RELICS   [F5] SAVE", {924.0f, 750.0f}, 10.5f, sf::Color(148, 163, 184));
    }

    if (m_statusBannerTimer > 0.0f) {
        m_ui.drawPanel(win, {250.0f, 88.0f}, {780.0f, 32.0f}, sf::Color(8, 22, 42, 225), sf::Color(56, 189, 248, 200));
        m_ui.drawText(win, m_statusBannerText, {640.0f, 97.0f}, 11.5f, sf::Color(186, 230, 253), TextAlign::Center, true);
    }

    if (m_loreDisplayTimer > 0.0f) {
        m_ui.drawPanel(win, {240.0f, 608.0f}, {800.0f, 82.0f}, sf::Color(10, 20, 38, 235), sf::Color(56, 189, 248, 220), 2.0f);
        m_ui.drawText(win, m_activeLoreTitle, {260.0f, 618.0f}, 13.0f, sf::Color(56, 189, 248), TextAlign::Left, true);
        m_ui.drawText(win, m_activeLoreBody, {260.0f, 646.0f}, 10.5f, sf::Color(226, 232, 240));
    }
}

void Game::renderCornerMinimap() {
    if (!m_window) return;
    auto& win = *m_window;
    const sf::Vector2f boxPos{1072.0f, 14.0f};
    const sf::Vector2f boxSize{190.0f, 134.0f};
    m_ui.drawPanel(win, boxPos, boxSize, sf::Color(6, 12, 24, 215), sf::Color(30, 58, 95, 190));

    const auto& cur = m_labyrinth.currentRoom();
    const sf::Vector2f center{boxPos.x + boxSize.x * 0.5f, boxPos.y + boxSize.y * 0.5f + 6.0f};
    constexpr float cellStep = 24.0f;
    constexpr float cellW = 18.0f;
    constexpr float cellH = 14.0f;

    m_ui.drawText(win, "SONAR GRID [TAB]", {center.x, boxPos.y + 6.0f}, 9.5f, sf::Color(125, 211, 252), TextAlign::Center, true);

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

        m_ui.drawPanel(win, pos, {cellW, cellH}, fill, outline, 1.2f);
        const std::string iconStr(1, Labyrinth::roomTypeIcon(rm.type));
        m_ui.drawText(win, iconStr, {pos.x + cellW * 0.5f, pos.y + 2.0f}, 8.5f, outline, TextAlign::Center, true);
    }
}

void Game::renderMinimapOverlay() {
    if (!m_window) return;
    auto& win = *m_window;
    m_ui.drawPanel(win, {110.0f, 75.0f}, {1060.0f, 650.0f}, sf::Color(4, 10, 22, 244), sf::Color(56, 189, 248, 220), 2.0f);
    m_ui.drawText(
        win,
        "PROCEDURAL LABYRINTH TOPOLOGY & RELIC LOADOUT",
        {640.0f, 94.0f},
        18.0f,
        sf::Color(56, 189, 248),
        TextAlign::Center,
        true
    );

    const std::string subHeader = "SEED #" + std::to_string(m_labyrinth.runSeed())
        + "   |   ROOMS CLEARED: " + std::to_string(m_labyrinth.clearedRooms().size()) + "/" + std::to_string(m_labyrinth.roomCount())
        + "   |   GHOST CLEARS: " + std::to_string(m_ghostClearsThisRun)
        + "   |   META VICTORIES: " + std::to_string(m_metaTotalVictories);
    m_ui.drawText(win, subHeader, {640.0f, 122.0f}, 11.5f, sf::Color(148, 163, 184), TextAlign::Center);

    const auto& cur = m_labyrinth.currentRoom();
    const sf::Vector2f mapOrigin{410.0f, 395.0f};
    constexpr float stepX = 42.0f;
    constexpr float stepY = 34.0f;
    constexpr float boxW = 32.0f;
    constexpr float boxH = 24.0f;

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
    win.draw(links);

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

        m_ui.drawPanel(win, pos, {boxW, boxH}, fill, outline, 1.5f);
        const std::string label(1, Labyrinth::roomTypeIcon(rm.type));
        m_ui.drawText(win, label, {pos.x + boxW * 0.5f, pos.y + 5.0f}, 11.0f, outline, TextAlign::Center, true);
    }

    // Right Panel: Collected Relics & Synergies
    m_ui.drawPanel(win, {720.0f, 160.0f}, {420.0f, 480.0f}, sf::Color(8, 18, 34, 230), sf::Color(30, 58, 95, 200));
    m_ui.drawText(win, "EQUIPPED ACOUSTIC RELICS", {930.0f, 174.0f}, 13.5f, sf::Color(52, 211, 153), TextAlign::Center, true);

    float ry = 208.0f;
    if (m_player.relics().empty()) {
        m_ui.drawText(win, "NO RELICS COLLECTED YET.", {930.0f, 250.0f}, 11.5f, sf::Color(148, 163, 184), TextAlign::Center);
        m_ui.drawText(win, "EXPLORE TREASURE [T], SHOP [$],", {930.0f, 275.0f}, 10.5f, sf::Color(148, 163, 184), TextAlign::Center);
        m_ui.drawText(win, "SECRET [?], OR GHOST CLEAR CHAMBERS.", {930.0f, 296.0f}, 10.5f, sf::Color(148, 163, 184), TextAlign::Center);
    } else {
        for (RelicId rId : m_player.relics()) {
            const RelicSpec spec = getRelicSpec(rId);
            m_ui.drawText(win, "* " + std::string(spec.name), {738.0f, ry}, 11.5f, spec.tint, TextAlign::Left, true);
            m_ui.drawText(win, spec.effectSummary, {750.0f, ry + 16.0f}, 9.5f, sf::Color(203, 213, 225));
            ry += 42.0f;
        }
    }

    m_ui.drawText(
        win,
        "LEGEND: [S] START  [C] COMBAT  [T] RELIC  [$] SHOP  [+] SANCTUARY  [!] ELITE  [?] SECRET  [B] BOSS  [^] EXIT",
        {640.0f, 692.0f},
        10.0f,
        sf::Color(148, 163, 184),
        TextAlign::Center
    );
}

void Game::renderMenuOverlay() {
    if (!m_window) return;
    auto& win = *m_window;
    m_ui.drawPanel(win, {320.0f, 120.0f}, {640.0f, 550.0f}, sf::Color(5, 11, 22, 238), sf::Color(56, 189, 248, 210), 2.0f);

    std::string title = "ECHOLABYRINTH 5.0 — RESONANCE";
    std::string subtitle = "SOUND IS SIGHT, WEAPON, & RISK  //  NATIVE C++17 & SFML";
    sf::Color titleCol(56, 189, 248);

    if (m_state == AppState::Paused) {
        title = "EXPEDITION PAUSED";
        subtitle = m_labyrinth.currentRoom().name + "  //  SEED #" + std::to_string(m_labyrinth.runSeed());
    } else if (m_state == AppState::GameOver) {
        title = "HULL INTEGRITY DEPLETED";
        subtitle = "CHAMBERS CLEARED: " + std::to_string(m_labyrinth.clearedRooms().size())
            + "  |  GHOST CLEARS: " + std::to_string(m_ghostClearsThisRun)
            + "  |  RELICS: " + std::to_string(m_player.relics().size());
        titleCol = sf::Color(239, 68, 68);
    } else if (m_state == AppState::Victory) {
        title = "SURFACE ASCENT COMPLETE";
        subtitle = "SEED #" + std::to_string(m_labyrinth.runSeed())
            + " CLEARED IN " + std::to_string(static_cast<int>(m_runTimeSec)) + "S"
            + "  |  GHOST CLEARS: " + std::to_string(m_ghostClearsThisRun);
        titleCol = sf::Color(52, 211, 153);
    }

    m_ui.drawText(win, title, {640.0f, 160.0f}, 24.0f, titleCol, TextAlign::Center, true);
    m_ui.drawText(win, subtitle, {640.0f, 202.0f}, 11.5f, sf::Color(148, 163, 184), TextAlign::Center);

    const std::string metaLine = "META-PROGRESSION: RUNS #" + std::to_string(m_metaTotalRuns)
        + "  |  VICTORIES: " + std::to_string(m_metaTotalVictories)
        + "  |  BOSSES SHATTERED: " + std::to_string(m_metaBossesDefeated);
    m_ui.drawText(win, metaLine, {640.0f, 232.0f}, 10.5f, sf::Color(250, 204, 21), TextAlign::Center);

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

        m_ui.drawPanel(win, topLeft, btn.size, fill, outline, selected ? 2.0f : 1.0f);
        m_ui.drawText(win, btn.label, {btn.center.x, btn.center.y - 7.0f}, 13.5f, textCol, TextAlign::Center, selected);
    }

    m_ui.drawText(
        win,
        "LMB/E: SONAR PING (+12 NOISE) | SPACE: PULSE (+30 NOISE) | RMB/G: DECOY (+15) | KEEP NOISE <= 30 FOR GHOST CLEAR",
        {640.0f, 642.0f},
        9.5f,
        sf::Color(100, 116, 139),
        TextAlign::Center
    );
}

void Game::renderSettingsOverlay() {
    if (!m_window) return;
    auto& win = *m_window;
    m_ui.drawPanel(win, {290.0f, 110.0f}, {700.0f, 575.0f}, sf::Color(5, 11, 22, 242), sf::Color(56, 189, 248, 210), 2.0f);
    m_ui.drawText(win, "3D SPATIAL AUDIO & ACOUSTIC ENGINE SETTINGS", {640.0f, 142.0f}, 18.0f, sf::Color(56, 189, 248), TextAlign::Center, true);

    char buf0[64], buf1[64], buf2[64], buf3[64], buf4[64], buf5[64], buf6[64], buf7[64];
    std::snprintf(buf0, sizeof(buf0), "MASTER VOLUME: %.0f%%", m_audio ? m_audio->masterVolume() : 80.0f);
    std::snprintf(buf1, sizeof(buf1), "ADAPTIVE MUSIC LAYER: %.0f%%", m_audio ? m_audio->musicVolume() : 75.0f);
    std::snprintf(buf2, sizeof(buf2), "3D SPATIAL SFX VOLUME: %.0f%%", m_audio ? m_audio->sfxVolume() : 85.0f);
    std::snprintf(buf3, sizeof(buf3), "AMBIENT COMPLEX DRONE: %.0f%%", m_audio ? m_audio->ambientVolume() : 80.0f);
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
            win,
            topLeft,
            btn.size,
            selected ? sf::Color(18, 48, 78, 240) : sf::Color(10, 22, 38, 210),
            selected ? sf::Color(56, 189, 248, 255) : sf::Color(30, 58, 95, 180),
            selected ? 2.0f : 1.0f
        );
        m_ui.drawText(
            win,
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
    if (!m_window) return;
    auto& win = *m_window;
    m_ui.drawPanel(win, {210.0f, 85.0f}, {860.0f, 635.0f}, sf::Color(5, 11, 22, 244), sf::Color(56, 189, 248, 210), 2.0f);
    m_ui.drawText(win, "SURVEYOR FIELD MANUAL — RESONANCE & NOISE DISCIPLINE", {640.0f, 112.0f}, 18.5f, sf::Color(56, 189, 248), TextAlign::Center, true);

    const char* lines[] = {
        "NOISE METER (0-100) & MATERIAL ACOUSTICS (BLOCKS 1 & 2):",
        "  [0-30 QUIET]     PATROL ONLY; CLEAR CHAMBER <= 30 FOR GHOST CLEAR (x1.5 SHARDS & SILENT STEP)",
        "  [DAMP MOSS]      ABSORBS 80% ECHO; STANDING IN MOSS HALVES STEP NOISE & DOUBLES SILENCE DECAY",
        "  [METAL / GLASS]  METAL GRANTS +2 BOUNCES; GLASS SHATTERS ON HIGH PULSE/STOMP (+40 NOISE)",
        "  [WATER / PIPES]  WATER DOUBLES SOUND RANGE (+15 SPLASH NOISE); PIPE COLUMNS HUM 4S LURE",
        "",
        "EXTENDED ACOUSTIC ARSENAL & TUNING (BLOCKS 3 & 4):",
        "  [Q] WHISTLE      15-DEG NARROW BEAM (+5 NOISE, LONG RANGE, UNMASKS MIMICS SAFELY)",
        "  [LMB / E] PING   DIRECTIONAL SONAR PING (+12 NOISE, DAMAGES HUNTERS, +10 HIT NOISE)",
        "  [SPACE] PULSE    360-DEG RADIAL PULSE (+30 NOISE, REVEALS ROOM & SHATTERS GLASS)",
        "  [C / V] STOMP    SEISMIC SHOCKWAVE (+35 NOISE, KNOCKS BACK HUNTERS & SHATTERS GLASS)",
        "  [R / X] TRAP     3.0S DELAYED ECHO BEACON MINE (EMITS 3 SONAR LURES FROM FLOOR)",
        "  [1 / 2 / 3]      TUNE SONAR FREQUENCY: LOW [1] PENETRATES, MID [2], HIGH [3] ANTI-SWARM/MIMIC"
    };

    float y = 160.0f;
    for (const char* line : lines) {
        const bool isHeader = (line[0] != ' ' && line[0] != '\0');
        m_ui.drawText(
            win,
            line,
            {250.0f, y},
            isHeader ? 12.5f : 11.0f,
            isHeader ? sf::Color(125, 211, 252) : sf::Color(203, 213, 225)
        );
        y += isHeader ? 25.0f : 21.0f;
    }

    if (!m_menuButtons.empty()) {
        const auto& btn = m_menuButtons[0];
        m_ui.drawPanel(win, btn.center - btn.size * 0.5f, btn.size, sf::Color(20, 48, 76), sf::Color(56, 189, 248), 2.0f);
        m_ui.drawText(win, btn.label, {btn.center.x, btn.center.y - 7.0f}, 14.0f, sf::Color(224, 242, 254), TextAlign::Center, true);
    }
}

} // namespace echo
