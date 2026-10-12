#include "Config.hpp"
#include "Labyrinth.hpp"
#include <algorithm>

namespace echo {

CodexEntrySpec getCodexEntrySpec(int index) noexcept {
    const int normalizedIdx = ((index % static_cast<int>(kTotalCodexFragments)) + static_cast<int>(kTotalCodexFragments))
                            % static_cast<int>(kTotalCodexFragments);
    switch (normalizedIdx) {
        case 0:
            return {200, "LOG 01: RESONANCE PROTOCOL", "EXPEDITION DIRECTIVE",
                    "SOUND IS SIGHT, WEAPON, AND RISK. KEEP CHAMBER PEAK NOISE <= 30 TO ACHIEVE GHOST CLEAR."};
        case 1:
            return {201, "LOG 02: THE BLIND ECOSYSTEM", "XENO-BIOLOGY",
                    "SIGHTLESS STALKERS EVOLVED HYPER-ACUTE TYMPANIC MEMBRANES. EVEN A SINGLE SPRINT STEP DRAWS THEM."};
        case 2:
            return {202, "LOG 03: BRYOPHYTE DAMPENING", "MATERIAL SCIENCE",
                    "CAVERN DAMP MOSS ABSORBS 80% OF SONAR REFLECTIONS AND HALVES FOOTSTEP NOISE WHEN CROUCHING NEARBY."};
        case 3:
            return {203, "LOG 04: SPECULAR STEEL MIRRORS", "ACOUSTIC OPTICS",
                    "POLISHED FOUNDRY PLATES REFLECT WAVES WITHOUT ENERGY LOSS AND ADD +2 EXTRA SPECULAR BOUNCES."};
        case 4:
            return {204, "LOG 05: BRITTLE SILICATE PARTITIONS", "STRUCTURAL HAZARD",
                    "CRYSTALLINE GLASS TRANSMITS 72% OF SOUND UNTIL SHATTERED BY RADIAL PULSE OR STOMP (+40 NOISE)."};
        case 5:
            return {205, "LOG 06: HYDRO-ACOUSTIC CONDUCTION", "FLUID DYNAMICS",
                    "FLOODED AQUEDUCT PUDDLES DOUBLE WAVE PROPAGATION RANGE BUT EMIT A +15 NOISE SPLASH WHEN CROSSED."};
        case 6:
            return {206, "LOG 07: HARMONIC PIPE COLUMNS", "RESONANCE LURES",
                    "STRIKING A HOLLOW CONDUIT COLUMN SETS OFF A 4-SECOND HARMONIC HUM THAT ILLUMINATES AND LURES HUNTERS."};
        case 7:
            return {207, "LOG 08: ULTRASONIC WHISTLE & STOMP", "TACTICAL ARSENAL",
                    "NARROW 15-DEG WHISTLE BEAMS ADD ONLY +5 NOISE; SEISMIC STOMPS (+35 NOISE) REPEL HUNTERS AND SHATTER GLASS."};
        case 8:
            return {208, "LOG 09: SPECIMEN ALPHA — SILENT HUNTER", "THREAT DOSSIER",
                    "EMITS ZERO PASSIVE FOOTSTEP TREMORS. LISTEN FOR ITS 0.5-SECOND INHALE HISS BEFORE IT LUNGES."};
        case 9:
            return {209, "LOG 10: SPECIMEN BETA — DEAF BRUTE", "THREAT DOSSIER",
                    "OSSIFIED CRANIUM IGNORES AIRBORNE WHISTLES AND PINGS; REACTS SOLELY TO GROUND VIBRATIONS."};
        case 10:
            return {210, "LOG 11: SPECIMEN GAMMA — RESONANCE MIMIC", "THREAT DOSSIER",
                    "IMITATES HARMONIC CRYSTALS WITH DECEPTIVE LURE PULSES. SCAN WITH HIGH FREQUENCY [3] OR WHISTLE [Q] TO UNMASK."};
        case 11:
        default:
            return {211, "LOG 12: THE CHOIR SOVEREIGN", "ABYSSAL CORE",
                    "THE THREE-PHASE APEX CONDUCTOR OF THE DEEP LABYRINTH. SHATTER ITS HARMONIC SHELL TO UNSEAL THE ASCENT."};
    }
}

void Labyrinth::markLoreCollected(int loreId) noexcept {
    if (!isLoreCollected(loreId)) {
        m_collectedLoreIds.push_back(loreId);
    }
    for (auto& room : m_rooms) {
        for (auto& lore : room.collectibles) {
            if (lore.id == loreId) {
                lore.collected = true;
            }
        }
    }
}

bool Labyrinth::isLoreCollected(int loreId) const noexcept {
    return std::find(m_collectedLoreIds.begin(), m_collectedLoreIds.end(), loreId) != m_collectedLoreIds.end();
}

} // namespace echo
