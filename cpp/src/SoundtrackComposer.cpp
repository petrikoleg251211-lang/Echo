#include "AudioManager.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

namespace echo {

namespace {
constexpr unsigned int kSampleRate = 44100;
constexpr float kTwoPi = 6.28318530717958647692f;
constexpr float kPi = 3.14159265358979323846f;

float hashNoiseOst(std::uint32_t seed) noexcept {
    seed = (seed ^ 61u) ^ (seed >> 16u);
    seed *= 9u;
    seed = seed ^ (seed >> 4u);
    seed *= 0x27d4eb2du;
    seed = seed ^ (seed >> 15u);
    return (static_cast<float>(seed & 0xFFFFu) / 32768.0f) - 1.0f;
}

float softClipOst(float x) noexcept {
    return std::tanh(x);
}

template <typename TrackFn>
sf::SoundBuffer makeSoundtrackLoop(
    float durationSec,
    float delayTimeSec,
    float delayFeedback,
    float delayMix,
    TrackFn&& trackFn
) {
    const std::size_t count = static_cast<std::size_t>(kSampleRate * durationSec);
    const std::size_t delaySamples = std::max<std::size_t>(
        1u,
        static_cast<std::size_t>(kSampleRate * delayTimeSec)
    );
    std::vector<float> dry(count, 0.0f);
    for (std::size_t i = 0; i < count; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        dry[i] = trackFn(t, static_cast<std::uint32_t>(i));
    }

    // Two-pass circular delay line so the reverb tail wraps seamlessly around the loop point
    std::vector<float> delayLine(delaySamples, 0.0f);
    std::vector<float> wet(count, 0.0f);
    std::size_t cursor = 0;
    float lpState = 0.0f;

    for (int pass = 0; pass < 2; ++pass) {
        for (std::size_t i = 0; i < count; ++i) {
            const float delayed = delayLine[cursor];
            lpState = lpState * 0.72f + (dry[i] + delayed * delayFeedback) * 0.28f;
            delayLine[cursor] = lpState;
            cursor = (cursor + 1) % delaySamples;
            if (pass == 1) {
                wet[i] = dry[i] * (1.0f - delayMix) + delayed * delayMix;
            }
        }
    }

    // Apply subtle 35 ms crossfade window at loop boundaries for zero-click looping
    const std::size_t fadeSamples = std::min<std::size_t>(count / 16u, static_cast<std::size_t>(kSampleRate * 0.035f));
    for (std::size_t i = 0; i < fadeSamples; ++i) {
        const float alpha = static_cast<float>(i) / static_cast<float>(fadeSamples);
        const float w = 0.5f - 0.5f * std::cos(kPi * alpha);
        const std::size_t tailIdx = count - fadeSamples + i;
        const float blended = wet[i] * w + wet[tailIdx] * (1.0f - w);
        wet[i] = blended;
        wet[tailIdx] = blended * (1.0f - w) + wet[tailIdx] * w;
    }

    std::vector<sf::Int16> pcm(count, 0);
    for (std::size_t i = 0; i < count; ++i) {
        const float limited = std::clamp(softClipOst(wet[i]), -1.0f, 1.0f);
        pcm[i] = static_cast<sf::Int16>(limited * 27500.0f);
    }

    sf::SoundBuffer buf;
    buf.loadFromSamples(pcm.data(), pcm.size(), 1, kSampleRate);
    return buf;
}

bool writeWavFileFromBuffer(const sf::SoundBuffer& buffer, const std::filesystem::path& filePath) {
    const sf::Int16* samples = buffer.getSamples();
    const std::uint64_t sampleCount = buffer.getSampleCount();
    const unsigned int channels = buffer.getChannelCount();
    const unsigned int sampleRate = buffer.getSampleRate();
    if (samples == nullptr || sampleCount == 0 || channels == 0 || sampleRate == 0) {
        return false;
    }

    std::ofstream out(filePath, std::ios::binary);
    if (!out.is_open()) return false;

    const std::uint32_t dataBytes = static_cast<std::uint32_t>(sampleCount * sizeof(sf::Int16));
    const std::uint32_t riffSize = 36u + dataBytes;
    const std::uint32_t fmtChunkSize = 16u;
    const std::uint16_t audioFormat = 1u; // PCM
    const std::uint16_t numChannels = static_cast<std::uint16_t>(channels);
    const std::uint32_t byteRate = sampleRate * channels * sizeof(sf::Int16);
    const std::uint16_t blockAlign = static_cast<std::uint16_t>(channels * sizeof(sf::Int16));
    const std::uint16_t bitsPerSample = 16u;

    auto writeU32 = [&out](std::uint32_t v) {
        const char bytes[4] = {
            static_cast<char>(v & 0xFFu),
            static_cast<char>((v >> 8u) & 0xFFu),
            static_cast<char>((v >> 16u) & 0xFFu),
            static_cast<char>((v >> 24u) & 0xFFu)
        };
        out.write(bytes, 4);
    };
    auto writeU16 = [&out](std::uint16_t v) {
        const char bytes[2] = {
            static_cast<char>(v & 0xFFu),
            static_cast<char>((v >> 8u) & 0xFFu)
        };
        out.write(bytes, 2);
    };

    out.write("RIFF", 4);
    writeU32(riffSize);
    out.write("WAVE", 4);
    out.write("fmt ", 4);
    writeU32(fmtChunkSize);
    writeU16(audioFormat);
    writeU16(numChannels);
    writeU32(sampleRate);
    writeU32(byteRate);
    writeU16(blockAlign);
    writeU16(bitsPerSample);
    out.write("data", 4);
    writeU32(dataBytes);
    out.write(reinterpret_cast<const char*>(samples), static_cast<std::streamsize>(dataBytes));
    return out.good();
}
} // namespace

const char* AudioManager::trackFilename(SoundtrackLayerId layer) noexcept {
    switch (layer) {
        case SoundtrackLayerId::SubterraneanSurvey: return "01_subterranean_survey.wav";
        case SoundtrackLayerId::FracturedEchoes:    return "02_fractured_echoes.wav";
        case SoundtrackLayerId::BlindPursuit:       return "03_blind_pursuit.wav";
        case SoundtrackLayerId::ResonanceSanctum:   return "04_resonance_sanctum.wav";
        case SoundtrackLayerId::ChoirSovereign:     return "05_the_choir_sovereign.wav";
        case SoundtrackLayerId::Count:              return "01_subterranean_survey.wav";
    }
    return "01_subterranean_survey.wav";
}

const char* AudioManager::trackTitle(SoundtrackLayerId layer) noexcept {
    switch (layer) {
        case SoundtrackLayerId::SubterraneanSurvey: return "01 - Subterranean Survey (Exploration)";
        case SoundtrackLayerId::FracturedEchoes:    return "02 - Fractured Echoes (Suspicion & Tension)";
        case SoundtrackLayerId::BlindPursuit:       return "03 - Blind Pursuit (120 BPM Hunt Darksynth)";
        case SoundtrackLayerId::ResonanceSanctum:   return "04 - Resonance Sanctum (Safe Haven Motif)";
        case SoundtrackLayerId::ChoirSovereign:     return "05 - The Choir Sovereign (3-Phase Boss Symphony)";
        case SoundtrackLayerId::Count:              return "01 - Subterranean Survey";
    }
    return "01 - Subterranean Survey";
}

void AudioManager::buildProceduralSoundtracks(const std::string& assetRootDir) {
    auto tryLoadExternalTrack = [&](sf::SoundBuffer& target, const char* filename) -> bool {
        if (assetRootDir.empty()) return false;
        std::error_code ec;
        const std::filesystem::path p = std::filesystem::path(assetRootDir) / "assets" / "audio" / "music" / filename;
        if (std::filesystem::exists(p, ec) && !ec) {
            return target.loadFromFile(p.string());
        }
        return false;
    };

    // TRACK 01: "Subterranean Survey" — Exploration Theme (16.0s, 4 bars in D Dorian)
    if (!tryLoadExternalTrack(m_bufExploreDrone, "01_subterranean_survey.wav")) {
        m_bufExploreDrone = makeSoundtrackLoop(
            16.0f,
            0.375f,
            0.46f,
            0.34f,
            [](float t, std::uint32_t idx) {
                constexpr std::array<float, 4> kBassRoots = {73.42f, 58.27f, 49.00f, 55.00f};
                constexpr std::array<float, 4> kPadFifths = {110.00f, 87.31f, 73.42f, 82.41f};
                constexpr std::array<float, 4> kPadColors = {174.61f, 146.83f, 116.54f, 164.81f};
                constexpr std::array<float, 8> kArpNotes  = {
                    293.66f, 349.23f, 440.00f, 523.25f,
                    440.00f, 329.63f, 392.00f, 293.66f
                };

                const std::size_t bar = static_cast<std::size_t>(t / 4.0f) & 3u;
                const float barPhase = std::fmod(t, 4.0f);
                const float barEnv = std::sin(std::min(1.0f, barPhase / 0.45f) * 1.570796f)
                                   * (0.85f + 0.15f * std::sin(kTwoPi * 0.25f * barPhase));

                const float root = kBassRoots[bar];
                const float fifth = kPadFifths[bar];
                const float color = kPadColors[bar];

                const float sub = (std::sin(kTwoPi * root * t)
                                 + 0.35f * std::sin(kTwoPi * (root * 0.5f) * t)
                                 + 0.22f * std::sin(kTwoPi * (root * 2.0f) * t)) * 0.36f * barEnv;

                const float pad = (std::sin(kTwoPi * (fifth - 0.32f) * t)
                                 + std::sin(kTwoPi * (fifth + 0.32f) * t)
                                 + 0.70f * std::sin(kTwoPi * color * t)
                                 + 0.40f * std::sin(kTwoPi * (color * 1.5f) * t + std::sin(kTwoPi * 0.25f * t)))
                                * 0.16f * barEnv;

                const std::size_t step = static_cast<std::size_t>(t / 1.0f) % kArpNotes.size();
                const float noteTime = std::fmod(t, 1.0f);
                const float bellFreq = kArpNotes[(step + bar) % kArpNotes.size()];
                const float bellMod = std::sin(kTwoPi * (bellFreq * 2.0f) * noteTime) * std::exp(-noteTime * 5.5f);
                const float bell = std::sin(kTwoPi * bellFreq * noteTime + 1.4f * bellMod)
                                 * std::exp(-noteTime * 3.2f) * 0.14f;

                const float air = hashNoiseOst(idx) * 0.015f * (0.5f + 0.5f * std::sin(kTwoPi * 0.125f * t));
                return sub + pad + bell + air;
            }
        );
    }

    // TRACK 02: "Fractured Echoes" — Suspicion & Tension Layer (16.0s seamless loop)
    if (!tryLoadExternalTrack(m_bufTensionLayer, "02_fractured_echoes.wav")) {
        m_bufTensionLayer = makeSoundtrackLoop(
            16.0f,
            0.28f,
            0.52f,
            0.38f,
            [](float t, std::uint32_t idx) {
                constexpr std::array<float, 4> kDissonantA = {146.83f, 155.56f, 138.59f, 164.81f};
                constexpr std::array<float, 4> kDissonantB = {207.65f, 220.00f, 196.00f, 233.08f};

                const std::size_t bar = static_cast<std::size_t>(t / 4.0f) & 3u;
                const float fA = kDissonantA[bar];
                const float fB = kDissonantB[bar];

                const float tremolo = 0.62f + 0.38f * std::sin(kTwoPi * 6.0f * t);
                const float mod = std::sin(kTwoPi * (fA * 3.0f) * t) * 0.45f;
                const float strings = (std::sin(kTwoPi * fA * t + mod)
                                     + 0.85f * std::sin(kTwoPi * fB * t)
                                     + 0.40f * std::sin(kTwoPi * (fB * 1.99f) * t))
                                    * 0.20f * tremolo;

                const float beatT = std::fmod(t, 0.5f);
                const float pulseEnv = std::exp(-beatT * 7.5f);
                const float dreadPulse = std::sin(kTwoPi * (65.41f - beatT * 18.0f) * beatT) * pulseEnv * 0.34f;

                const float clickPhase = std::fmod(t + 0.125f, 0.75f);
                const float click = (clickPhase < 0.045f)
                    ? std::exp(-clickPhase * 85.0f) * (std::sin(kTwoPi * 1320.0f * clickPhase) + hashNoiseOst(idx) * 0.4f) * 0.22f
                    : 0.0f;

                return strings + dreadPulse + click;
            }
        );
    }

    // TRACK 03: "Blind Pursuit" — Active Hunt Combat Darksynth (8.0s, 120 BPM)
    if (!tryLoadExternalTrack(m_bufPursuitPulse, "03_blind_pursuit.wav")) {
        m_bufPursuitPulse = makeSoundtrackLoop(
            8.0f,
            0.1875f,
            0.35f,
            0.22f,
            [](float t, std::uint32_t idx) {
                constexpr std::array<float, 16> kBassSeq = {
                    73.42f, 73.42f, 146.83f, 73.42f,
                    87.31f, 73.42f, 103.83f, 98.00f,
                    73.42f, 73.42f, 146.83f, 73.42f,
                    110.00f, 103.83f, 87.31f, 65.41f
                };

                const float beatT = std::fmod(t, 0.5f);
                const float barT = std::fmod(t, 2.0f);
                const float stepT = std::fmod(t, 0.125f);
                const std::size_t stepIdx = static_cast<std::size_t>(t / 0.125f) & 15u;

                const float kickPitch = 52.0f + 115.0f * std::exp(-beatT * 36.0f);
                const float kick = std::sin(kTwoPi * kickPitch * beatT) * std::exp(-beatT * 11.5f) * 0.52f;

                float snare = 0.0f;
                const float snarePhase = std::fmod(barT + 0.5f, 1.0f);
                if (snarePhase < 0.24f) {
                    const float sEnv = std::exp(-snarePhase * 22.0f);
                    const float body = std::sin(kTwoPi * 210.0f * snarePhase) + 0.6f * std::sin(kTwoPi * 465.0f * snarePhase);
                    const float noise = hashNoiseOst(idx + 4096u) * 0.85f;
                    snare = (body * 0.45f + noise * 0.55f) * sEnv * 0.38f;
                }

                const float hatAccent = ((stepIdx & 3u) == 2u) ? 1.0f : 0.45f;
                const float hat = hashNoiseOst(idx + 16384u) * std::exp(-stepT * 55.0f) * 0.16f * hatAccent;

                const float bassFreq = kBassSeq[stepIdx];
                const float fmEnv = std::exp(-stepT * 18.0f);
                const float mod = std::sin(kTwoPi * (bassFreq * 2.0f) * t) * (1.8f * fmEnv + 0.4f);
                const float bass = std::sin(kTwoPi * bassFreq * t + mod) * std::exp(-stepT * 9.0f) * 0.34f;

                float horn = 0.0f;
                if (barT < 0.45f) {
                    const float hEnv = std::sin(std::min(1.0f, barT / 0.05f) * 1.570796f) * std::exp(-barT * 5.2f);
                    const float hFreq = (t < 4.0f) ? 146.83f : 155.56f;
                    horn = (std::sin(kTwoPi * hFreq * t)
                          + 0.7f * std::sin(kTwoPi * (hFreq * 2.0f) * t)
                          + 0.45f * std::sin(kTwoPi * (hFreq * 3.0f) * t)) * hEnv * 0.22f;
                }

                return kick + snare + hat + bass + horn;
            }
        );
    }

    // TRACK 04: "Resonance Sanctum" — Checkpoint & Safe Haven Motif (16.0s loop)
    if (!tryLoadExternalTrack(m_bufSanctumTheme, "04_resonance_sanctum.wav")) {
        m_bufSanctumTheme = makeSoundtrackLoop(
            16.0f,
            0.42f,
            0.55f,
            0.44f,
            [](float t, std::uint32_t) {
                constexpr std::array<std::array<float, 4>, 4> kChords = {{
                    {146.83f, 220.00f, 277.18f, 329.63f},
                    {123.47f, 185.00f, 220.00f, 277.18f},
                    {98.00f,  196.00f, 246.94f, 293.66f},
                    {110.00f, 220.00f, 277.18f, 329.63f}
                }};
                constexpr std::array<float, 8> kHarpPattern = {
                    293.66f, 369.99f, 440.00f, 554.37f,
                    659.25f, 554.37f, 440.00f, 369.99f
                };

                const std::size_t bar = static_cast<std::size_t>(t / 4.0f) & 3u;
                const float barPhase = std::fmod(t, 4.0f);
                const float swell = 0.72f + 0.28f * std::sin(kTwoPi * 0.25f * barPhase);

                const auto& chord = kChords[bar];
                const float pad = (0.45f * std::sin(kTwoPi * chord[0] * t)
                                 + 0.35f * std::sin(kTwoPi * chord[1] * t)
                                 + 0.28f * std::sin(kTwoPi * chord[2] * t)
                                 + 0.22f * std::sin(kTwoPi * chord[3] * t)) * 0.24f * swell;

                const std::size_t harpIdx = static_cast<std::size_t>(t / 0.5f) & 7u;
                const float pluckT = std::fmod(t, 0.5f);
                const float hFreq = kHarpPattern[harpIdx];
                const float harp = (std::sin(kTwoPi * hFreq * pluckT)
                                  + 0.40f * std::sin(kTwoPi * (hFreq * 2.0f) * pluckT) * std::exp(-pluckT * 10.0f)
                                  + 0.15f * std::sin(kTwoPi * (hFreq * 3.0f) * pluckT) * std::exp(-pluckT * 16.0f))
                                 * std::exp(-pluckT * 5.2f) * 0.20f;

                return pad + harp;
            }
        );
    }

    // TRACK 05: "The Choir Sovereign" — Boss Arena 3-Phase Symphony (16.0s loop)
    if (!tryLoadExternalTrack(m_bufBossChoir, "05_the_choir_sovereign.wav")) {
        m_bufBossChoir = makeSoundtrackLoop(
            16.0f,
            0.333f,
            0.48f,
            0.36f,
            [](float t, std::uint32_t idx) {
                constexpr std::array<std::array<float, 4>, 4> kOrganChords = {{
                    {73.42f, 146.83f, 174.61f, 220.00f},
                    {58.27f, 116.54f, 174.61f, 233.08f},
                    {49.00f,  98.00f, 146.83f, 196.00f},
                    {55.00f, 110.00f, 138.59f, 233.08f}
                }};
                constexpr std::array<float, 8> kChoirMelody = {
                    293.66f, 349.23f, 329.63f, 293.66f,
                    233.08f, 261.63f, 220.00f, 277.18f
                };

                const std::size_t bar = static_cast<std::size_t>(t / 4.0f) & 3u;
                const auto& org = kOrganChords[bar];

                auto organVoice = [t](float f) {
                    return std::sin(kTwoPi * f * t)
                         + 0.55f * std::sin(kTwoPi * (f * 2.0f) * t)
                         + 0.30f * std::sin(kTwoPi * (f * 3.0f) * t)
                         + 0.20f * std::sin(kTwoPi * (f * 4.0f) * t);
                };
                const float organ = (0.45f * organVoice(org[0])
                                   + 0.30f * organVoice(org[1])
                                   + 0.25f * organVoice(org[2])
                                   + 0.22f * organVoice(org[3])) * 0.14f;

                const std::size_t melIdx = static_cast<std::size_t>(t / 2.0f) & 7u;
                const float melNoteT = std::fmod(t, 2.0f);
                const float vibrato = std::sin(kTwoPi * 5.4f * t) * 1.8f;
                const float f0 = kChoirMelody[melIdx] + vibrato;
                const float h1 = std::sin(kTwoPi * f0 * t);
                const float h2 = std::sin(kTwoPi * (f0 * 2.0f) * t) * 0.85f;
                const float h3 = std::sin(kTwoPi * (f0 * 3.0f) * t) * 0.70f;
                const float h4 = std::sin(kTwoPi * (f0 * 4.0f) * t) * 0.45f;
                const float choirEnv = std::sin(std::min(1.0f, melNoteT / 0.22f) * 1.570796f)
                                     * (1.0f - 0.25f * (melNoteT / 2.0f));
                const float choir = (h1 * 0.35f + h2 * 0.40f + h3 * 0.35f + h4 * 0.25f) * choirEnv * 0.24f;

                const float beatT = std::fmod(t, 0.5f);
                const float subBeatT = std::fmod(t, 0.25f);
                const std::size_t subStep = static_cast<std::size_t>(t / 0.25f) & 7u;
                const float timpaniFreq = org[0] + 65.0f * std::exp(-beatT * 24.0f);
                const float timpani = std::sin(kTwoPi * timpaniFreq * beatT) * std::exp(-beatT * 8.5f) * 0.36f;

                float anvil = 0.0f;
                if (subStep == 2u || subStep == 6u || subStep == 7u) {
                    const float aEnv = std::exp(-subBeatT * 26.0f);
                    anvil = (std::sin(kTwoPi * 620.0f * subBeatT)
                           + 0.65f * std::sin(kTwoPi * 1140.0f * subBeatT)
                           + hashNoiseOst(idx) * 0.5f) * aEnv * 0.22f;
                }

                return organ + choir + timpani + anvil;
            }
        );
    }
}

bool AudioManager::exportSoundtrackWavFiles(const std::string& outputDir) {
    if (!m_initialized) {
        initialize(outputDir);
    }
    std::error_code ec;
    const std::filesystem::path dirPath = std::filesystem::path(outputDir);
    std::filesystem::create_directories(dirPath, ec);
    if (ec) return false;

    bool ok = true;
    ok = writeWavFileFromBuffer(m_bufExploreDrone, dirPath / "01_subterranean_survey.wav") && ok;
    ok = writeWavFileFromBuffer(m_bufTensionLayer, dirPath / "02_fractured_echoes.wav") && ok;
    ok = writeWavFileFromBuffer(m_bufPursuitPulse, dirPath / "03_blind_pursuit.wav") && ok;
    ok = writeWavFileFromBuffer(m_bufSanctumTheme, dirPath / "04_resonance_sanctum.wav") && ok;
    ok = writeWavFileFromBuffer(m_bufBossChoir,    dirPath / "05_the_choir_sovereign.wav") && ok;
    return ok;
}

const sf::SoundBuffer& AudioManager::soundtrackBuffer(SoundtrackLayerId layer) const noexcept {
    switch (layer) {
        case SoundtrackLayerId::SubterraneanSurvey: return m_bufExploreDrone;
        case SoundtrackLayerId::FracturedEchoes:    return m_bufTensionLayer;
        case SoundtrackLayerId::BlindPursuit:       return m_bufPursuitPulse;
        case SoundtrackLayerId::ResonanceSanctum:   return m_bufSanctumTheme;
        case SoundtrackLayerId::ChoirSovereign:     return m_bufBossChoir;
        case SoundtrackLayerId::Count:              return m_bufExploreDrone;
    }
    return m_bufExploreDrone;
}

} // namespace echo
