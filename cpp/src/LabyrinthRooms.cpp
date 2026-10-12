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
} // namespace

const char* Labyrinth::roomTypeName(RoomType type) noexcept {
    switch (type) {
        case RoomType::Start:     return "ENTRY CISTERN";
        case RoomType::Combat:    return "HUNTING CHAMBER";
        case RoomType::Treasure:  return "RELIC VAULT";
        case RoomType::Secret:    return "CONCEALED CAVERN";
        case RoomType::Puzzle:    return "RESONANCE PUZZLE";
        case RoomType::Sanctuary: return "ACOUSTIC SANCTUM";
        case RoomType::Shop:      return "QUARTERMASTER";
        case RoomType::Elite:     return "WARDEN GARRISON";
        case RoomType::Boss:      return "CHOIR SOVEREIGN";
        case RoomType::FinalExit: return "ASCENT CONDUIT";
    }
    return "CHAMBER";
}

char Labyrinth::roomTypeIcon(RoomType type) noexcept {
    switch (type) {
        case RoomType::Start:     return 'S';
        case RoomType::Combat:    return 'C';
        case RoomType::Treasure:  return 'T';
        case RoomType::Secret:    return '?';
        case RoomType::Puzzle:    return 'P';
        case RoomType::Sanctuary: return '+';
        case RoomType::Shop:      return '$';
        case RoomType::Elite:     return '!';
        case RoomType::Boss:      return 'B';
        case RoomType::FinalExit: return '^';
    }
    return '.';
}

sf::Vector2f Labyrinth::spawnPositionForEntry(CardinalDir enteredFromDir) noexcept {
    switch (enteredFromDir) {
        case CardinalDir::North: return {640.0f, 152.0f};
        case CardinalDir::South: return {640.0f, 648.0f};
        case CardinalDir::West:  return {162.0f, 400.0f};
        case CardinalDir::East:  return {1118.0f, 400.0f};
        default:                 return {640.0f, 400.0f};
    }
}

int Labyrinth::findRoomAtGrid(int gx, int gy) const noexcept {
    for (std::size_t i = 0; i < m_rooms.size(); ++i) {
        if (m_rooms[i].gridX == gx && m_rooms[i].gridY == gy) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void Labyrinth::updateAnimations(float dt) noexcept {
    if (m_rooms.empty()) return;
    auto& room = m_rooms[m_currentRoomIndex];
    for (auto& mech : room.mechanisms) {
        const float target = mech.activated ? 1.0f : 0.0f;
        mech.animProgress += (target - mech.animProgress) * std::min(1.0f, dt * 7.5f);
    }
    for (auto& prop : room.props) {
        if (prop.type == PropType::VentTurbine) {
            prop.rotationDeg = std::fmod(prop.rotationDeg + dt * 65.0f, 360.0f);
        }
    }
}

void Labyrinth::setCurrentRoom(int roomId) noexcept {
    if (roomId >= 0 && static_cast<std::size_t>(roomId) < m_rooms.size()) {
        m_currentRoomIndex = static_cast<std::size_t>(roomId);
        markRoomVisited(roomId);
    }
}

void Labyrinth::setCombatLock(int roomId, bool locked) noexcept {
    if (roomId < 0 || static_cast<std::size_t>(roomId) >= m_rooms.size()) return;
    auto& room = m_rooms[static_cast<std::size_t>(roomId)];
    room.combatLocked = locked;
    const int gateId = 800 + roomId;
    for (auto& w : room.walls) {
        if (w.doorId == gateId) {
            w.isOpen = !locked;
        }
    }
    for (auto& p : room.portals) {
        if (!p.isSecretDoor && !p.isVictoryExit) {
            if (locked) {
                p.requiredSwitchId = gateId;
            } else if (p.requiredSwitchId == gateId) {
                p.requiredSwitchId = -1;
            }
        }
    }
}

void Labyrinth::markRoomCleared(int roomId) noexcept {
    if (roomId < 0 || static_cast<std::size_t>(roomId) >= m_rooms.size()) return;
    auto& room = m_rooms[static_cast<std::size_t>(roomId)];
    room.cleared = true;
    room.combatLocked = false;
    room.noiseIntrusionActive = false;
    room.pendingNoiseAlertLevel = 0.0f;
    if (std::find(m_clearedRooms.begin(), m_clearedRooms.end(), roomId) == m_clearedRooms.end()) {
        m_clearedRooms.push_back(roomId);
    }
    activateSwitch(800 + roomId);
}

bool Labyrinth::isRoomCleared(int roomId) const noexcept {
    if (roomId < 0 || static_cast<std::size_t>(roomId) >= m_rooms.size()) return false;
    return m_rooms[static_cast<std::size_t>(roomId)].cleared;
}

bool Labyrinth::isDoorOpenTowards(int roomId, CardinalDir dir) const noexcept {
    if (roomId < 0 || static_cast<std::size_t>(roomId) >= m_rooms.size()) return false;
    const auto& room = m_rooms[static_cast<std::size_t>(roomId)];
    const int nbId = room.neighbors[static_cast<std::size_t>(dir)];
    if (nbId < 0 || static_cast<std::size_t>(nbId) >= m_rooms.size()) return false;

    const auto& nbRoom = m_rooms[static_cast<std::size_t>(nbId)];
    if (room.combatLocked || nbRoom.combatLocked) {
        return false;
    }

    for (const auto& portal : room.portals) {
        if (portal.isVictoryExit) continue;
        if (portal.direction == dir && portal.targetRoomId == nbId) {
            if (portal.requiredSwitchId != -1 && !isSwitchActivated(portal.requiredSwitchId)) {
                return false;
            }
        }
    }
    return true;
}

std::vector<RoomNoiseLeak> Labyrinth::propagateNoiseFromRoom(
    int sourceRoomId,
    float sourceNoise,
    int maxGridSteps
) const {
    std::vector<RoomNoiseLeak> leaks;
    if (sourceRoomId < 0 || static_cast<std::size_t>(sourceRoomId) >= m_rooms.size() || sourceNoise <= 0.0f || maxGridSteps <= 0) {
        return leaks;
    }

    struct QueueEntry {
        int roomId;
        int dist;
        float noise;
    };

    std::vector<int> bestDist(m_rooms.size(), 9999);
    std::queue<QueueEntry> q;
    bestDist[static_cast<std::size_t>(sourceRoomId)] = 0;
    q.push({sourceRoomId, 0, sourceNoise});

    while (!q.empty()) {
        const QueueEntry cur = q.front();
        q.pop();

        if (cur.dist >= maxGridSteps) continue;

        for (int d = 0; d < 4; ++d) {
            const auto dir = static_cast<CardinalDir>(d);
            if (!isDoorOpenTowards(cur.roomId, dir)) {
                continue;
            }
            const int nb = m_rooms[static_cast<std::size_t>(cur.roomId)].neighbors[static_cast<std::size_t>(d)];
            if (nb < 0 || static_cast<std::size_t>(nb) >= m_rooms.size()) continue;

            const int nextDist = cur.dist + 1;
            if (nextDist < bestDist[static_cast<std::size_t>(nb)]) {
                bestDist[static_cast<std::size_t>(nb)] = nextDist;
                const float nextNoise = cur.noise * cfg::noise::kCrossRoomAttenuation;
                leaks.push_back({nb, nextDist, nextNoise, oppositeDir(dir)});
                q.push({nb, nextDist, nextNoise});
            }
        }
    }

    return leaks;
}

bool Labyrinth::reopenRoomForNoiseIntrusion(int roomId) noexcept {
    if (roomId < 0 || static_cast<std::size_t>(roomId) >= m_rooms.size()) return false;
    auto& room = m_rooms[static_cast<std::size_t>(roomId)];
    if (room.noiseIntrusionConsumed || room.noiseIntrusionActive || !room.cleared) {
        return false;
    }
    room.cleared = false;
    room.noiseIntrusionActive = true;
    room.noiseIntrusionConsumed = true;
    m_clearedRooms.erase(
        std::remove(m_clearedRooms.begin(), m_clearedRooms.end(), roomId),
        m_clearedRooms.end()
    );
    setCombatLock(roomId, true);
    return true;
}

int Labyrinth::computeClearShardReward(
    int baseShards,
    float roomPeakNoise,
    bool& outIsGhostClear,
    RoomEvent roomEvent
) noexcept {
    if (baseShards <= 0) {
        outIsGhostClear = false;
        return 0;
    }
    float mult = 1.0f;
    if (roomPeakNoise <= cfg::noise::kThresholdAudible) {
        outIsGhostClear = true;
        mult *= cfg::noise::kGhostClearShardMultiplier;
    } else {
        outIsGhostClear = false;
    }
    if (roomEvent == RoomEvent::AcousticBlackout) {
        mult *= 1.50f;
    }
    return static_cast<int>(std::round(static_cast<float>(baseShards) * mult));
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
        for (auto& portal : room.portals) {
            if (portal.requiredSwitchId == switchId && !portal.isVictoryExit) {
                portal.requiredSwitchId = -1;
            }
        }
    }
    if (switchId >= 900 && static_cast<std::size_t>(switchId - 900) < m_rooms.size()) {
        m_rooms[static_cast<std::size_t>(switchId - 900)].discovered = true;
    }
}

bool Labyrinth::isSwitchActivated(int switchId) const noexcept {
    return std::find(m_activatedSwitches.begin(), m_activatedSwitches.end(), switchId) != m_activatedSwitches.end();
}

void Labyrinth::markRoomVisited(int roomId) noexcept {
    if (roomId < 0 || static_cast<std::size_t>(roomId) >= m_rooms.size()) return;
    if (!isRoomVisited(roomId)) {
        m_visitedRooms.push_back(roomId);
    }
    auto& room = m_rooms[static_cast<std::size_t>(roomId)];
    room.visited = true;
    room.discovered = true;

    for (int d = 0; d < 4; ++d) {
        const int nb = room.neighbors[static_cast<std::size_t>(d)];
        if (nb >= 0 && static_cast<std::size_t>(nb) < m_rooms.size()) {
            if (m_rooms[static_cast<std::size_t>(nb)].type != RoomType::Secret) {
                m_rooms[static_cast<std::size_t>(nb)].discovered = true;
            }
        }
    }
}

bool Labyrinth::isRoomVisited(int roomId) const noexcept {
    return std::find(m_visitedRooms.begin(), m_visitedRooms.end(), roomId) != m_visitedRooms.end();
}

void Labyrinth::revealAllRoomsOnMap() noexcept {
    for (auto& r : m_rooms) r.discovered = true;
}

void Labyrinth::revealSecretRoomsOnMap() noexcept {
    for (auto& r : m_rooms) {
        if (r.type == RoomType::Secret) r.discovered = true;
    }
}

void Labyrinth::restoreState(
    const std::vector<int>& switches,
    const std::vector<int>& loreIds,
    const std::vector<int>& visitedRooms,
    const std::vector<int>& clearedRooms
) {
    m_activatedSwitches.clear();
    m_collectedLoreIds.clear();
    m_visitedRooms.clear();
    m_clearedRooms.clear();

    for (int sw : switches) activateSwitch(sw);
    for (int lore : loreIds) markLoreCollected(lore);
    for (int rm : visitedRooms) {
        if (rm >= 0 && static_cast<std::size_t>(rm) < m_rooms.size()) {
            markRoomVisited(rm);
        }
    }
    for (int cr : clearedRooms) {
        if (cr >= 0 && static_cast<std::size_t>(cr) < m_rooms.size()) {
            markRoomCleared(cr);
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
    constexpr float kMinY = 42.0f;
    constexpr float kMaxY = 758.0f;
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

    const auto openGrid = runBfs(true);
    for (const auto& mech : room.mechanisms) {
        if (!isPointReachableWithin(openGrid, mech.position, 48.0f)) {
            outError = "Mechanism unreachable in " + room.name;
            return false;
        }
    }
    for (const auto& lore : room.collectibles) {
        if (!isPointReachableWithin(openGrid, lore.position, 48.0f)) {
            outError = "Lore collectible unreachable in " + room.name;
            return false;
        }
    }
    for (const auto& ped : room.relicPedestals) {
        if (!isPointReachableWithin(openGrid, ped.position, 48.0f)) {
            outError = "Relic pedestal unreachable in " + room.name;
            return false;
        }
    }
    for (const auto& portal : room.portals) {
        if (!isPointReachableWithin(openGrid, portal.position, portal.radius + 6.0f)) {
            outError = "Portal (" + portal.label + ") unreachable in " + room.name;
            return false;
        }
    }

    return true;
}

} // namespace echo
