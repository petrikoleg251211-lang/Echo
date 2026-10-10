#include "Labyrinth.hpp"
#include <algorithm>
#include <cmath>
#include <queue>

namespace echo {

namespace {
sf::Vector2f closestPointOnSegment(sf::Vector2f p, sf::Vector2f a, sf::Vector2f b) noexcept {
    const sf::Vector2f ab = b - a;
    const float lenSq = ab.x * ab.x + ab.y * ab.y;
    if (lenSq < 1e-6f) return a;
    const float t = std::clamp(((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / lenSq, 0.0f, 1.0f);
    return a + ab * t;
}

float distSq(sf::Vector2f a, sf::Vector2f b) noexcept {
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    return dx * dx + dy * dy;
}

void addRectObstacle(
    std::vector<WallSegment>& walls,
    float x, float y, float w, float h,
    SurfaceMaterial mat
) {
    walls.push_back({{x, y}, {x + w, y}, mat, -1, false});
    walls.push_back({{x + w, y}, {x + w, y + h}, mat, -1, false});
    walls.push_back({{x + w, y + h}, {x, y + h}, mat, -1, false});
    walls.push_back({{x, y + h}, {x, y}, mat, -1, false});
}

void addRoomPerimeterWithVestibules(
    std::vector<WallSegment>& walls,
    SurfaceMaterial mat,
    bool hasWestTunnel,
    bool hasEastTunnel
) {
    constexpr float left = 90.0f;
    constexpr float top = 80.0f;
    constexpr float right = 1190.0f;
    constexpr float bottom = 720.0f;

    walls.push_back({{left, top}, {right, top}, mat, -1, false});
    walls.push_back({{right, bottom}, {left, bottom}, mat, -1, false});

    if (hasWestTunnel) {
        walls.push_back({{left, top}, {left, 335.0f}, mat, -1, false});
        walls.push_back({{left, 335.0f}, {45.0f, 335.0f}, mat, -1, false});
        walls.push_back({{45.0f, 335.0f}, {45.0f, 465.0f}, mat, -1, false});
        walls.push_back({{45.0f, 465.0f}, {left, 465.0f}, mat, -1, false});
        walls.push_back({{left, 465.0f}, {left, bottom}, mat, -1, false});
    } else {
        walls.push_back({{left, bottom}, {left, top}, mat, -1, false});
    }

    if (hasEastTunnel) {
        walls.push_back({{right, top}, {right, 335.0f}, mat, -1, false});
        walls.push_back({{right, 335.0f}, {1235.0f, 335.0f}, mat, -1, false});
        walls.push_back({{1235.0f, 335.0f}, {1235.0f, 465.0f}, mat, -1, false});
        walls.push_back({{1235.0f, 465.0f}, {right, 465.0f}, mat, -1, false});
        walls.push_back({{right, 465.0f}, {right, bottom}, mat, -1, false});
    } else {
        walls.push_back({{right, top}, {right, bottom}, mat, -1, false});
    }
}

void addSealedEastBulkhead(
    std::vector<WallSegment>& walls,
    int doorSwitchId,
    SurfaceMaterial bulkheadMat
) {
    constexpr float bx = 1045.0f;
    walls.push_back({{bx, 80.0f}, {bx, 325.0f}, bulkheadMat, -1, false});
    walls.push_back({{bx, 475.0f}, {bx, 720.0f}, bulkheadMat, -1, false});
    walls.push_back({{bx, 325.0f}, {bx, 475.0f}, SurfaceMaterial::ResonanceDoor, doorSwitchId, false});
}
} // namespace

Labyrinth::Labyrinth() {
    buildAllRooms();
}

void Labyrinth::resetAllProgress() {
    m_activatedSwitches.clear();
    m_collectedLoreIds.clear();
    m_visitedRooms.clear();
    m_currentRoomIndex = 0;
    buildAllRooms();
    markRoomVisited(0);
}

void Labyrinth::buildAllRooms() {
    m_rooms.clear();
    m_rooms.reserve(15);

    struct RoomBlueprint {
        const char* name;
        const char* sector;
        const char* objective;
        SurfaceMaterial primaryMat;
        SurfaceMaterial secondaryMat;
        int archetype; // 0..5
        bool hasEnemy;
        bool isCheckpoint;
        const char* loreTitle;
        const char* loreBody;
    };

    const RoomBlueprint blueprints[15] = {
        {
            "01. ENTRY CISTERN",
            "UPPER DRAINAGE",
            "CLICK [LMB] FOR SONAR PING OR [SPACE] FOR 360 PULSE. REACH THE EAST CONDUIT.",
            SurfaceMaterial::Stone, SurfaceMaterial::Water, 0, false, true,
            "LOG 01 - CALIBRATION",
            "OPTICAL LAMPS FAIL IN THIS VAPOR. USE DIRECTIONAL ACOUSTIC PINGS TO MAP MASONRY."
        },
        {
            "02. ECHOING GALLERY",
            "UPPER DRAINAGE",
            "ENERGIZE THE TUNING CRYSTAL WITH SONAR RAYS [LMB / SPACE] TO UNSEAL THE EAST GATE.",
            SurfaceMaterial::Stone, SurfaceMaterial::Metal, 1, false, false,
            nullptr, nullptr
        },
        {
            "03. FLOODED SLUICE",
            "AQUEDUCT SECTOR",
            "ENGAGE THE HARMONIC LEVER [F] TO UNSEAL THE EAST RESONANCE GATE. AVOID THE LISTENER.",
            SurfaceMaterial::Water, SurfaceMaterial::Stone, 2, true, false,
            "LOG 02 - THE LISTENERS",
            "THEY ARE BLIND BUT HUNT VIBRATIONS. HOLD [LSHIFT] TO SNEAK QUIETLY PAST PATROLS."
        },
        {
            "04. IRON FOUNDRY",
            "INDUSTRIAL CORE",
            "SNEAK [LSHIFT] OVER BRITTLE SHALE ZONES SO THEY DO NOT CRACK AND ALERT THE HUNTER.",
            SurfaceMaterial::Metal, SurfaceMaterial::Concrete, 3, true, false,
            nullptr, nullptr
        },
        {
            "05. WHISPERING OSSUARY",
            "CATACOMB TIER",
            "DAMP MOSS ABSORBS ECHOES QUICKLY. CHECKPOINT ACTIVE - PRESS [F5] TO SAVE.",
            SurfaceMaterial::DampMoss, SurfaceMaterial::Stone, 4, false, true,
            "LOG 03 - MOSS DAMPENING",
            "OVERGROWN MOSS ABSORBS 64 PERCENT OF WAVE ENERGY, HIDING WALLS BUT MUFFLING STEPS."
        },
        {
            "06. COLLAPSED VAULT",
            "CATACOMB TIER",
            "THROW A DECOY STONE [RMB / G] TO LURE THE GUARDIAN AWAY FROM THE HARMONIC LEVER [F].",
            SurfaceMaterial::Stone, SurfaceMaterial::DampMoss, 2, true, false,
            nullptr, nullptr
        },
        {
            "07. RESONANCE CHAMBER",
            "ACOUSTIC SANCTUM",
            "BOUNCE SONAR WAVES OFF THE 45-DEGREE STEEL MIRRORS TO CHARGE THE RESONATOR CRYSTAL.",
            SurfaceMaterial::Metal, SurfaceMaterial::Stone, 1, false, false,
            "LOG 04 - HARMONIC CRYSTALS",
            "CRYSTAL RESONATORS ABSORB COHERENT SONAR RAYS AND UNSEAL PAIRED HARMONIC BARRIERS."
        },
        {
            "08. WARDEN WATCHPOST",
            "SECURITY RING",
            "NAVIGATE THE REINFORCED CONCRETE CHICANES AND SNEAK ACROSS LOOSE SHALE.",
            SurfaceMaterial::Concrete, SurfaceMaterial::Metal, 3, true, false,
            nullptr, nullptr
        },
        {
            "09. SUBTERRANEAN CANAL",
            "AQUEDUCT SECTOR",
            "UNLOCK THE CANAL GATE VIA THE NORTHERN LEVER [F]. CHECKPOINT REACHED.",
            SurfaceMaterial::Water, SurfaceMaterial::Metal, 2, true, true,
            "LOG 05 - HYDRO ACOUSTICS",
            "STEAM VENTS MASK NEARBY FOOTSTEPS WHILE PULSING. USE DECOY STONES WHEN CORNERED."
        },
        {
            "10. RITUAL ATRIUM",
            "DEEP VAULTS",
            "SLIP PAST THE MONOLITH RING WHILE THE LISTENER CIRCLES THE CENTRAL SANCTUM.",
            SurfaceMaterial::Stone, SurfaceMaterial::Concrete, 4, true, false,
            nullptr, nullptr
        },
        {
            "11. RUST CORRIDOR",
            "INDUSTRIAL CORE",
            "NARROW IRON BULKHEADS TRANSMIT 72% OF SOUND THROUGH WALLS. DO NOT SPRINT.",
            SurfaceMaterial::Metal, SurfaceMaterial::DampMoss, 3, true, false,
            "LOG 06 - IRON RESONANCE",
            "FORGED STEEL BULKHEADS TRANSMIT VIBRATIONS EVEN THROUGH SOLID PARTITIONS."
        },
        {
            "12. GENERATOR HALL",
            "INDUSTRIAL CORE",
            "ACTIVATE THE TURBINE BYPASS LEVER [F] TO OPEN THE EAST BULKHEAD. CHECKPOINT ACTIVE.",
            SurfaceMaterial::Concrete, SurfaceMaterial::Metal, 2, true, true,
            nullptr, nullptr
        },
        {
            "13. SILENT ARCHIVE",
            "DEEP VAULTS",
            "MOSS-COVERED ARCHIVE STACKS DEADEN SONAR. ADVANCE CAREFULLY BETWEEN AISLES.",
            SurfaceMaterial::DampMoss, SurfaceMaterial::Stone, 0, true, false,
            "LOG 07 - FINAL SHAFT",
            "ONLY THE WARDEN SANCTUM STANDS BETWEEN US AND THE SURFACE ASCENT LIFT."
        },
        {
            "14. WARDEN SANCTUM",
            "ABYSSAL THRESHOLD",
            "LURE THE SANCTUM GUARDIAN AWAY AND PULL THE CONDUIT LEVER [F] TO UNSEAL THE LIFT.",
            SurfaceMaterial::Stone, SurfaceMaterial::Metal, 2, true, false,
            nullptr, nullptr
        },
        {
            "15. ASCENT GATE",
            "SURFACE CONDUIT",
            "ENGAGE THE FINAL LIFT LOCK [F] AND STEP INTO THE EASTERN SURFACE LIFT TO ESCAPE.",
            SurfaceMaterial::Metal, SurfaceMaterial::Stone, 5, false, true,
            "LOG 08 - EMERGENCE",
            "THE SURFACE SHAFT IS OPEN. STEP THROUGH THE EASTERN PORTAL TO COMPLETE THE ASCENT."
        }
    };

    for (int i = 0; i < 15; ++i) {
        const auto& bp = blueprints[i];
        RoomDefinition room;
        room.id = i;
        room.name = bp.name;
        room.sectorTitle = bp.sector;
        room.objectiveText = bp.objective;
        room.isCheckpoint = bp.isCheckpoint;
        room.isExitRoom = (i == 14);
        room.playerDefaultSpawn = {145.0f, 400.0f};

        const bool hasWest = (i > 0);
        const bool hasEast = true;
        addRoomPerimeterWithVestibules(room.walls, bp.primaryMat, hasWest, hasEast);

        int reqSwitch = -1;

        if (bp.archetype == 0) {
            addRectObstacle(room.walls, 280.0f, 160.0f, 105.0f, 165.0f, bp.secondaryMat);
            addRectObstacle(room.walls, 280.0f, 475.0f, 105.0f, 165.0f, bp.primaryMat);
            addRectObstacle(room.walls, 560.0f, 275.0f, 130.0f, 250.0f, bp.secondaryMat);
            addRectObstacle(room.walls, 845.0f, 160.0f, 100.0f, 165.0f, bp.primaryMat);
            addRectObstacle(room.walls, 845.0f, 475.0f, 100.0f, 165.0f, bp.secondaryMat);
            room.ambientEmitters.push_back({{625.0f, 225.0f}, 3.6f, 1.0f, true});
        } else if (bp.archetype == 1) {
            // Acoustic Mirror Gallery + Resonator Crystal lock
            reqSwitch = 100 + i;
            addRectObstacle(room.walls, 330.0f, 230.0f, 95.0f, 340.0f, bp.primaryMat);
            addRectObstacle(room.walls, 670.0f, 140.0f, 110.0f, 185.0f, bp.secondaryMat);
            addRectObstacle(room.walls, 670.0f, 475.0f, 110.0f, 185.0f, bp.secondaryMat);
            // Angled mirror plates
            room.walls.push_back({{490.0f, 140.0f}, {600.0f, 240.0f}, SurfaceMaterial::Metal, -1, false});
            room.walls.push_back({{490.0f, 660.0f}, {600.0f, 560.0f}, SurfaceMaterial::Metal, -1, false});
            room.walls.push_back({{870.0f, 230.0f}, {965.0f, 325.0f}, SurfaceMaterial::Metal, -1, false});
            room.walls.push_back({{870.0f, 570.0f}, {965.0f, 475.0f}, SurfaceMaterial::Metal, -1, false});

            addSealedEastBulkhead(room.walls, reqSwitch, SurfaceMaterial::Metal);
            room.resonators.push_back({
                reqSwitch,
                {825.0f, 400.0f},
                18.0f,
                0.0f,
                false,
                reqSwitch,
                "HARMONIC RESONATOR CRYSTAL"
            });
        } else if (bp.archetype == 2) {
            reqSwitch = 100 + i;
            addRectObstacle(room.walls, 285.0f, 170.0f, 115.0f, 175.0f, bp.secondaryMat);
            addRectObstacle(room.walls, 285.0f, 460.0f, 115.0f, 175.0f, bp.primaryMat);
            addRectObstacle(room.walls, 550.0f, 300.0f, 135.0f, 200.0f, bp.primaryMat);
            addRectObstacle(room.walls, 790.0f, 165.0f, 100.0f, 175.0f, bp.secondaryMat);

            addSealedEastBulkhead(room.walls, reqSwitch, SurfaceMaterial::Metal);

            room.mechanisms.push_back({
                reqSwitch,
                {615.0f, 185.0f},
                false,
                "HARMONIC CONDUIT LEVER",
                reqSwitch
            });
            room.ambientEmitters.push_back({{920.0f, 610.0f}, 4.0f, 0.5f, true});
            room.hazards.push_back({{475.0f, 235.0f}, 42.0f, "BRITTLE SHALE"});
        } else if (bp.archetype == 3) {
            addRectObstacle(room.walls, 275.0f, 80.0f, 90.0f, 350.0f, bp.secondaryMat);
            addRectObstacle(room.walls, 505.0f, 370.0f, 90.0f, 350.0f, bp.primaryMat);
            addRectObstacle(room.walls, 735.0f, 80.0f, 90.0f, 350.0f, bp.secondaryMat);
            addRectObstacle(room.walls, 935.0f, 410.0f, 75.0f, 195.0f, bp.primaryMat);

            room.hazards.push_back({{425.0f, 520.0f}, 45.0f, "BRITTLE SHALE"});
            room.hazards.push_back({{660.0f, 240.0f}, 45.0f, "BRITTLE SHALE"});
        } else if (bp.archetype == 4) {
            addRectObstacle(room.walls, 380.0f, 185.0f, 120.0f, 120.0f, bp.secondaryMat);
            addRectObstacle(room.walls, 380.0f, 495.0f, 120.0f, 120.0f, bp.secondaryMat);
            addRectObstacle(room.walls, 740.0f, 185.0f, 120.0f, 120.0f, bp.secondaryMat);
            addRectObstacle(room.walls, 740.0f, 495.0f, 120.0f, 120.0f, bp.secondaryMat);
            addRectObstacle(room.walls, 575.0f, 345.0f, 110.0f, 110.0f, SurfaceMaterial::Metal);
            room.ambientEmitters.push_back({{630.0f, 260.0f}, 3.2f, 0.2f, true});
        } else {
            // Archetype 5: Room 15 Final Ascent Gate
            reqSwitch = 100 + i;
            addRectObstacle(room.walls, 320.0f, 150.0f, 120.0f, 180.0f, SurfaceMaterial::Metal);
            addRectObstacle(room.walls, 320.0f, 470.0f, 120.0f, 180.0f, SurfaceMaterial::Metal);
            addSealedEastBulkhead(room.walls, reqSwitch, SurfaceMaterial::Metal);
            room.mechanisms.push_back({
                reqSwitch,
                {680.0f, 400.0f},
                false,
                "ASCENT SHAFT INTERLOCK",
                reqSwitch
            });
        }

        if (hasWest) {
            // Notice: when returning West into room (i-1), spawn at x = 985.0f (inside the main chamber west of any 1045 bulkhead!)
            room.portals.push_back({
                {70.0f, 400.0f},
                28.0f,
                i - 1,
                {985.0f, 400.0f},
                "WEST TUNNEL",
                -1,
                false
            });
        }

        room.portals.push_back({
            {1210.0f, 400.0f},
            30.0f,
            (i < 14) ? (i + 1) : 14,
            {145.0f, 400.0f},
            (i < 14) ? "EAST CONDUIT" : "SURFACE ASCENT LIFT",
            reqSwitch,
            (i == 14)
        });

        if (bp.loreTitle != nullptr && bp.loreBody != nullptr) {
            room.collectibles.push_back({
                200 + i,
                {450.0f, 645.0f},
                false,
                bp.loreTitle,
                bp.loreBody
            });
        }

        if (bp.hasEnemy) {
            if (bp.archetype == 3) {
                room.enemyPatrolNodes = {
                    {420.0f, 220.0f},
                    {645.0f, 220.0f},
                    {645.0f, 580.0f},
                    {865.0f, 580.0f},
                    {865.0f, 240.0f}
                };
            } else {
                room.enemyPatrolNodes = {
                    {730.0f, 145.0f},
                    {975.0f, 185.0f},
                    {975.0f, 625.0f},
                    {470.0f, 625.0f}
                };
            }
        }

        m_rooms.push_back(std::move(room));
    }

    if (m_visitedRooms.empty()) {
        m_visitedRooms.push_back(0);
    }
}

void Labyrinth::setCurrentRoom(int roomId) noexcept {
    if (roomId >= 0 && static_cast<std::size_t>(roomId) < m_rooms.size()) {
        m_currentRoomIndex = static_cast<std::size_t>(roomId);
        markRoomVisited(roomId);
    }
}

void Labyrinth::activateSwitch(int switchId) noexcept {
    if (!isSwitchActivated(switchId)) {
        m_activatedSwitches.push_back(switchId);
    }
    for (auto& room : m_rooms) {
        for (auto& mech : room.mechanisms) {
            if (mech.id == switchId) mech.activated = true;
        }
        for (auto& res : room.resonators) {
            if (res.id == switchId) {
                res.activated = true;
                res.energy = 1.0f;
            }
        }
        for (auto& wall : room.walls) {
            if (wall.doorId == switchId) wall.isOpen = true;
        }
    }
}

bool Labyrinth::isSwitchActivated(int switchId) const noexcept {
    return std::find(m_activatedSwitches.begin(), m_activatedSwitches.end(), switchId) != m_activatedSwitches.end();
}

void Labyrinth::markLoreCollected(int loreId) noexcept {
    if (!isLoreCollected(loreId)) {
        m_collectedLoreIds.push_back(loreId);
    }
    for (auto& room : m_rooms) {
        for (auto& lore : room.collectibles) {
            if (lore.id == loreId) lore.collected = true;
        }
    }
}

bool Labyrinth::isLoreCollected(int loreId) const noexcept {
    return std::find(m_collectedLoreIds.begin(), m_collectedLoreIds.end(), loreId) != m_collectedLoreIds.end();
}

void Labyrinth::markRoomVisited(int roomId) noexcept {
    if (!isRoomVisited(roomId)) {
        m_visitedRooms.push_back(roomId);
    }
}

bool Labyrinth::isRoomVisited(int roomId) const noexcept {
    return std::find(m_visitedRooms.begin(), m_visitedRooms.end(), roomId) != m_visitedRooms.end();
}

void Labyrinth::restoreState(
    const std::vector<int>& switches,
    const std::vector<int>& loreIds,
    const std::vector<int>& visitedRooms
) {
    buildAllRooms();
    m_activatedSwitches.clear();
    m_collectedLoreIds.clear();
    m_visitedRooms.clear();

    for (int sw : switches) activateSwitch(sw);
    for (int lore : loreIds) markLoreCollected(lore);
    for (int rm : visitedRooms) {
        if (rm >= 0 && static_cast<std::size_t>(rm) < m_rooms.size()) {
            markRoomVisited(rm);
        }
    }
    if (m_visitedRooms.empty()) {
        markRoomVisited(0);
    }
}

bool Labyrinth::verifyRoomTraversability(int roomId, std::string& outError) const {
    if (roomId < 0 || static_cast<std::size_t>(roomId) >= m_rooms.size()) {
        outError = "Invalid room ID";
        return false;
    }

    const RoomDefinition& room = m_rooms[static_cast<std::size_t>(roomId)];
    constexpr float kStep = 10.0f;
    constexpr float kMinX = 45.0f;
    constexpr float kMaxX = 1235.0f;
    constexpr float kMinY = 80.0f;
    constexpr float kMaxY = 720.0f;
    constexpr int kCols = static_cast<int>((kMaxX - kMinX) / kStep) + 1;
    constexpr int kRows = static_cast<int>((kMaxY - kMinY) / kStep) + 1;
    constexpr float kClearance = cfg::kPlayerRadius + 2.0f;

    auto runBfs = [&](bool gatesOpen) {
        std::vector<bool> reachable(static_cast<std::size_t>(kCols * kRows), false);

        auto isWalkable = [&](float wx, float wy) {
            for (const auto& w : room.walls) {
                if (w.isOpen || (gatesOpen && w.doorId != -1)) continue;
                const sf::Vector2f c = closestPointOnSegment({wx, wy}, w.a, w.b);
                if (distSq({wx, wy}, c) < kClearance * kClearance) {
                    return false;
                }
            }
            return true;
        };

        const int startC = std::clamp(static_cast<int>(std::round((room.playerDefaultSpawn.x - kMinX) / kStep)), 0, kCols - 1);
        const int startR = std::clamp(static_cast<int>(std::round((room.playerDefaultSpawn.y - kMinY) / kStep)), 0, kRows - 1);

        std::queue<std::pair<int, int>> q;
        if (isWalkable(kMinX + static_cast<float>(startC) * kStep, kMinY + static_cast<float>(startR) * kStep)) {
            reachable[static_cast<std::size_t>(startR * kCols + startC)] = true;
            q.push({startC, startR});
        }

        const int dc[4] = {1, -1, 0, 0};
        const int dr[4] = {0, 0, 1, -1};

        while (!q.empty()) {
            const auto [c, r] = q.front();
            q.pop();
            for (int d = 0; d < 4; ++d) {
                const int nc = c + dc[d];
                const int nr = r + dr[d];
                if (nc < 0 || nc >= kCols || nr < 0 || nr >= kRows) continue;
                const std::size_t idx = static_cast<std::size_t>(nr * kCols + nc);
                if (reachable[idx]) continue;
                const float wx = kMinX + static_cast<float>(nc) * kStep;
                const float wy = kMinY + static_cast<float>(nr) * kStep;
                if (isWalkable(wx, wy)) {
                    reachable[idx] = true;
                    q.push({nc, nr});
                }
            }
        }
        return reachable;
    };

    auto isPointReachableWithin = [&](const std::vector<bool>& grid, sf::Vector2f target, float interactRadius) {
        for (int r = 0; r < kRows; ++r) {
            for (int c = 0; c < kCols; ++c) {
                if (!grid[static_cast<std::size_t>(r * kCols + c)]) continue;
                const sf::Vector2f pt{kMinX + static_cast<float>(c) * kStep, kMinY + static_cast<float>(r) * kStep};
                if (distSq(pt, target) <= interactRadius * interactRadius) {
                    return true;
                }
            }
        }
        return false;
    };

    // 1. Check with gates CLOSED
    const auto closedGrid = runBfs(false);
    for (const auto& mech : room.mechanisms) {
        if (!isPointReachableWithin(closedGrid, mech.position, 48.0f)) {
            outError = "Mechanism unreachable in " + room.name;
            return false;
        }
    }
    for (const auto& lore : room.collectibles) {
        if (!isPointReachableWithin(closedGrid, lore.position, 48.0f)) {
            outError = "Lore collectible unreachable in " + room.name;
            return false;
        }
    }
    for (const auto& portal : room.portals) {
        if (portal.requiredSwitchId != -1) {
            // Must NOT be reachable while gate is closed
            if (isPointReachableWithin(closedGrid, portal.position, portal.radius)) {
                outError = "Locked portal can be bypassed without switch in " + room.name;
                return false;
            }
        }
    }

    // 2. Check with gates OPEN
    const auto openGrid = runBfs(true);
    for (const auto& portal : room.portals) {
        if (!isPointReachableWithin(openGrid, portal.position, portal.radius)) {
            outError = "Portal (" + portal.label + ") unreachable even with gate open in " + room.name;
            return false;
        }
    }

    return true;
}

} // namespace echo
