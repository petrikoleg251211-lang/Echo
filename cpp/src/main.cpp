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
        if (arg == "--self-test") {
            return echo::Game::runSelfTest(exeDir);
        }
        if (arg == "--balance-report") {
            return echo::Game::runBalanceReport(std::cout, 1000, echo::DifficultyMode::Surveyor);
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
