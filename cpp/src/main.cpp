#include "AudioManager.hpp"
#include "Game.hpp"
#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::string exeDir;
    if (argc > 0 && argv[0] != nullptr) {
        std::error_code ec;
        const std::filesystem::path p = std::filesystem::absolute(argv[0], ec);
        if (!ec && p.has_parent_path()) {
            exeDir = p.parent_path().string();
        }
    }

    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--version" || arg == "-v") {
            std::cout << "EchoLabyrinth 5.4.0 \"Resonance\" (Native C++17 / SFML 2.6.1 / OpenAL 3D / GLSL PostFX)\n";
            return 0;
        }
        if (arg == "--self-test") {
            return echo::Game::runSelfTest(exeDir);
        }
        if (arg == "--balance-report") {
            int runs = 1000;
            if (i + 1 < argc && argv[i + 1] != nullptr) {
                const int parsed = std::atoi(argv[i + 1]);
                if (parsed >= 10 && parsed <= 10000) {
                    runs = parsed;
                }
            }
            return echo::Game::runBalanceReport(std::cout, runs, echo::DifficultyMode::Surveyor);
        }
        if (arg == "--benchmark") {
            int frames = 1200;
            if (i + 1 < argc && argv[i + 1] != nullptr) {
                const int parsed = std::atoi(argv[i + 1]);
                if (parsed >= 60 && parsed <= 20000) {
                    frames = parsed;
                }
            }
            const auto summary = echo::Game::runBenchmark(std::cout, frames, exeDir);
            return (summary.avgFrameMs < 16.67f) ? 0 : 1;
        }
        if (arg == "--export-ost") {
            std::filesystem::path outDir = exeDir.empty()
                ? std::filesystem::path("assets/audio/music")
                : (std::filesystem::path(exeDir) / "assets" / "audio" / "music");
            if (i + 1 < argc && argv[i + 1] != nullptr) {
                outDir = std::filesystem::path(argv[i + 1]);
            }
            echo::AudioManager audio;
            audio.initialize(exeDir);
            if (audio.exportSoundtrackWavFiles(outDir.string())) {
                std::cout << "[OST] Exported 5 EchoLabyrinth soundtrack WAV tracks to: " << outDir.string() << "\n";
                return 0;
            }
            std::cerr << "[OST] Failed to export soundtrack WAV files to: " << outDir.string() << "\n";
            return 1;
        }
    }

    echo::Game game(exeDir);
    return game.run();
}
