#include "Game.hpp"
#include <filesystem>
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
        if (std::string(argv[i]) == "--self-test") {
            return echo::Game::runSelfTest(exeDir);
        }
    }

    echo::Game game(exeDir);
    return game.run();
}
