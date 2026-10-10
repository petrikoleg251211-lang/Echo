#include "Labyrinth.hpp"
#include <algorithm>

namespace echo {

namespace {
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

void addOuterBoundary(std::vector<WallSegment>& walls, SurfaceMaterial mat = SurfaceMaterial::Stone) {
    constexpr float left = 80.0f;
    constexpr float top = 80.0f;
    constexpr float right = 1200.0f;
    constexpr float bottom = 720.0f;
    walls.push_back({{left, top}, {right, top}, mat, -1, false});
    walls.push_back({{right, top}, {right, bottom}, mat, -1, false});
    walls.push_back({{right, bottom}, {left, bottom}, mat, -1, false});
    walls.push_back({{left, bottom}, {left, top}, mat, -1, false});
}
} // namespace

Labyrinth::Labyrinth() {
    buildAllRooms();
}

void Labyrinth::buildAllRooms() {
    m_rooms.clear();
    m_rooms.reserve(15);

    struct RoomBlueprint {
        const char* name;
        const char* sector;
        const char* hint;
        SurfaceMaterial primaryMat;
        SurfaceMaterial obstacleMat;
        bool hasEnemy;
        bool isCheckpoint;
    };

    const RoomBlueprint blueprints[15] = {
        {"01. Entry Cistern", "Upper Drainage", "Emit a directional ping (E / Click) or radial pulse (Space) to reveal stone masonry.", SurfaceMaterial::Stone, SurfaceMaterial::Water, false, true},
        {"02. Echoing Gallery", "Upper Drainage", "Forged iron pillars reflect 94% of acoustic energy with long amber persistence.", SurfaceMaterial::Stone, SurfaceMaterial::Metal, false, false},
        {"03. Flooded Sluice", "Aqueduct Sector", "Subterranean water scatters waves with teal ripples. A Listener patrols the far bank.", SurfaceMaterial::Water, SurfaceMaterial::Stone, true, false},
        {"04. Iron Foundry", "Industrial Core", "High metallic resonance amplifies footsteps. Hold Shift to sneak past the Listener.", SurfaceMaterial::Metal, SurfaceMaterial::Metal, true, false},
        {"05. Whispering Ossuary", "Catacomb Tier", "Damp moss absorbs 65% of sound waves, shortening echo visibility.", SurfaceMaterial::DampMoss, SurfaceMaterial::Stone, false, true},
        {"06. Collapsed Vault", "Catacomb Tier", "Throw a Decoy Stone (G / Right-Click) to lure the Listener away from the Resonance Gate.", SurfaceMaterial::Stone, SurfaceMaterial::DampMoss, true, false},
        {"07. Resonance Chamber", "Acoustic Sanctum", "Activate the harmonic lever (F) to unlock the sealed conduit.", SurfaceMaterial::Metal, SurfaceMaterial::Stone, false, false},
        {"08. Warden's Watchpost", "Security Ring", "Narrow stone baffles create multi-bounce reflections around blind corners.", SurfaceMaterial::Stone, SurfaceMaterial::Metal, true, false},
        {"09. Subterranean Canal", "Aqueduct Sector", "Long flooded causeways carry acoustic waves across open water.", SurfaceMaterial::Water, SurfaceMaterial::Metal, true, true},
        {"10. Ritual Atrium", "Deep Vaults", "Concentric pillars form an acoustic focal ring in the center.", SurfaceMaterial::Stone, SurfaceMaterial::Stone, true, false},
        {"11. Rust Corridor", "Industrial Core", "Corroded iron bulkheads channel sound directly toward hunting entities.", SurfaceMaterial::Metal, SurfaceMaterial::DampMoss, true, false},
        {"12. Generator Hall", "Industrial Core", "Heavy turbine housings block line-of-sight; use wall bounces to scout ahead.", SurfaceMaterial::Metal, SurfaceMaterial::Metal, true, true},
        {"13. Silent Archive", "Deep Vaults", "Moss-choked shelves deaden echoes. Navigate carefully between narrow aisles.", SurfaceMaterial::DampMoss, SurfaceMaterial::Stone, true, false},
        {"14. Warden's Sanctum", "Abyssal Threshold", "Dual patrol paths converge around the final resonance lock.", SurfaceMaterial::Stone, SurfaceMaterial::Metal, true, false},
        {"15. Ascent Gate", "Surface Conduit", "Align the final harmonic switch and step into the surface lift.", SurfaceMaterial::Metal, SurfaceMaterial::Stone, false, true}
    };

    for (int i = 0; i < 15; ++i) {
        RoomDefinition room;
        room.id = i;
        room.name = blueprints[i].name;
        room.sectorTitle = blueprints[i].sector;
        room.acousticHint = blueprints[i].hint;
        room.isCheckpoint = blueprints[i].isCheckpoint;
        room.isExitRoom = (i == 14);
        room.playerDefaultSpawn = {150.0f, 400.0f};

        addOuterBoundary(room.walls, blueprints[i].primaryMat);

        // Internal architectural baffles & pillars per room index
        const float offset = static_cast<float>((i % 3) * 45);
        addRectObstacle(room.walls, 340.0f, 180.0f + offset, 110.0f, 170.0f, blueprints[i].obstacleMat);
        addRectObstacle(room.walls, 610.0f, 420.0f - offset, 130.0f, 180.0f, blueprints[i].obstacleMat);
        addRectObstacle(room.walls, 880.0f, 200.0f + offset * 0.5f, 95.0f, 220.0f, blueprints[i].primaryMat);

        // Diagonal acoustic reflector wall
        room.walls.push_back({
            {480.0f, 260.0f},
            {580.0f, 350.0f},
            SurfaceMaterial::Metal,
            -1,
            false
        });

        // Switch & Resonance Door on every 3rd room
        int reqSwitch = -1;
        if (i % 3 == 2) {
            reqSwitch = 100 + i;
            room.mechanisms.push_back({
                reqSwitch,
                {640.0f, 210.0f},
                false,
                "Harmonic Conduit Lever",
                reqSwitch
            });
            // Resonance Gate blocking east portal until switch activated
            room.walls.push_back({
                {1080.0f, 310.0f},
                {1080.0f, 490.0f},
                SurfaceMaterial::ResonanceDoor,
                reqSwitch,
                false
            });
        }

        // West portal (back to previous room)
        if (i > 0) {
            room.portals.push_back({
                {110.0f, 400.0f},
                28.0f,
                i - 1,
                {1090.0f, 400.0f},
                -1
            });
        }

        // East portal (forward to next room)
        if (i < 14) {
            room.portals.push_back({
                {1145.0f, 400.0f},
                30.0f,
                i + 1,
                {155.0f, 400.0f},
                reqSwitch
            });
        }

        // Collectibles in rooms 0, 2, 4, 6, 8, 10, 12, 14
        if (i % 2 == 0) {
            room.collectibles.push_back({
                200 + i,
                {520.0f, 560.0f},
                false,
                "Acoustic Log #" + std::to_string(i / 2 + 1),
                "Surveyor Note: The stone remembers the pitch of every footstep. Never strike iron twice when the Listener is near."
            });
        }

        if (blueprints[i].hasEnemy) {
            room.enemyPatrolNodes = {
                {780.0f, 200.0f},
                {1010.0f, 240.0f},
                {1010.0f, 590.0f},
                {520.0f, 600.0f}
            };
        }

        m_rooms.push_back(std::move(room));
    }
}

void Labyrinth::setCurrentRoom(int roomId) noexcept {
    if (roomId >= 0 && static_cast<std::size_t>(roomId) < m_rooms.size()) {
        m_currentRoomIndex = static_cast<std::size_t>(roomId);
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
        for (auto& wall : room.walls) {
            if (wall.doorId == switchId) wall.isOpen = true;
        }
    }
}

bool Labyrinth::isSwitchActivated(int switchId) const noexcept {
    return std::find(m_activatedSwitches.begin(), m_activatedSwitches.end(), switchId) != m_activatedSwitches.end();
}

void Labyrinth::markLoreCollected(int loreId) noexcept {
    if (std::find(m_collectedLoreIds.begin(), m_collectedLoreIds.end(), loreId) == m_collectedLoreIds.end()) {
        m_collectedLoreIds.push_back(loreId);
    }
    for (auto& room : m_rooms) {
        for (auto& lore : room.collectibles) {
            if (lore.id == loreId) lore.collected = true;
        }
    }
}

} // namespace echo
