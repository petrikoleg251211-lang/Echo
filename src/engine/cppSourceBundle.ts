import cmakeRaw from '../../cpp/CMakeLists.txt?raw';
import licenseRaw from '../../cpp/LICENSE?raw';
import readmeRaw from '../../cpp/README.md?raw';
import changelogRaw from '../../cpp/CHANGELOG.md?raw';
import clangFormatRaw from '../../cpp/.clang-format?raw';
import batRaw from '../../cpp/build_windows.bat?raw';
import winRcRaw from '../../cpp/resources/echolabyrinth.rc?raw';
import ciWorkflowRaw from '../../.github/workflows/ci.yml?raw';
import releaseWorkflowRaw from '../../.github/workflows/release.yml?raw';
import manifestRaw from '../../cpp/assets/ASSET_MANIFEST.md?raw';
import appIconSvgRaw from '../../cpp/assets/icon/echolabyrinth_icon.svg?raw';
import surveyorObjRaw from '../../cpp/assets/models/surveyor_rig.obj?raw';
import stalkerObjRaw from '../../cpp/assets/models/listener_stalker.obj?raw';
import wardenObjRaw from '../../cpp/assets/models/listener_warden.obj?raw';
import crystalObjRaw from '../../cpp/assets/models/resonator_crystal.obj?raw';
import leverObjRaw from '../../cpp/assets/models/harmonic_lever.obj?raw';
import terminalObjRaw from '../../cpp/assets/models/surveyor_terminal.obj?raw';
import turbineObjRaw from '../../cpp/assets/models/vent_turbine.obj?raw';
import postFxFragRaw from '../../cpp/assets/shaders/acoustic_postfx.frag?raw';

import configHppRaw from '../../cpp/include/Config.hpp?raw';
import acousticHppRaw from '../../cpp/include/AcousticSystem.hpp?raw';
import assetHppRaw from '../../cpp/include/AssetManager.hpp?raw';
import audioHppRaw from '../../cpp/include/AudioManager.hpp?raw';
import postProcessorHppRaw from '../../cpp/include/PostProcessor.hpp?raw';
import uiHppRaw from '../../cpp/include/UiRenderer.hpp?raw';
import saveHppRaw from '../../cpp/include/SaveSystem.hpp?raw';
import labyrinthHppRaw from '../../cpp/include/Labyrinth.hpp?raw';
import playerHppRaw from '../../cpp/include/Player.hpp?raw';
import enemyHppRaw from '../../cpp/include/EnemyAI.hpp?raw';
import gameHppRaw from '../../cpp/include/Game.hpp?raw';

import mainCppRaw from '../../cpp/src/main.cpp?raw';
import acousticCppRaw from '../../cpp/src/AcousticSystem.cpp?raw';
import assetCppRaw from '../../cpp/src/AssetManager.cpp?raw';
import audioCppRaw from '../../cpp/src/AudioManager.cpp?raw';
import soundtrackComposerCppRaw from '../../cpp/src/SoundtrackComposer.cpp?raw';
import postProcessorCppRaw from '../../cpp/src/PostProcessor.cpp?raw';
import uiCppRaw from '../../cpp/src/UiRenderer.cpp?raw';
import saveCppRaw from '../../cpp/src/SaveSystem.cpp?raw';
import labyrinthGenCppRaw from '../../cpp/src/LabyrinthGen.cpp?raw';
import labyrinthRoomsCppRaw from '../../cpp/src/LabyrinthRooms.cpp?raw';
import relicsCppRaw from '../../cpp/src/Relics.cpp?raw';
import codexCppRaw from '../../cpp/src/Codex.cpp?raw';
import playerCppRaw from '../../cpp/src/Player.cpp?raw';
import enemyCppRaw from '../../cpp/src/EnemyAI.cpp?raw';
import enemyVariantsCppRaw from '../../cpp/src/EnemyVariants.cpp?raw';
import bossAiCppRaw from '../../cpp/src/BossAI.cpp?raw';
import gameCppRaw from '../../cpp/src/Game.cpp?raw';
import gameRunCppRaw from '../../cpp/src/GameRun.cpp?raw';
import gameNoiseCppRaw from '../../cpp/src/GameNoise.cpp?raw';
import gameInputCppRaw from '../../cpp/src/GameInput.cpp?raw';
import gameWorldRenderCppRaw from '../../cpp/src/GameWorldRender.cpp?raw';
import gameHudRenderCppRaw from '../../cpp/src/GameHudRender.cpp?raw';
import gameBalanceCppRaw from '../../cpp/src/GameBalance.cpp?raw';
import gameSelfTestCppRaw from '../../cpp/src/GameSelfTest.cpp?raw';

export interface CppSourceFile {
  path: string;
  module: string;
  summary: string;
  content: string;
}

export const CPP_SOURCE_FILES: CppSourceFile[] = [
  {
    path: 'cpp/CMakeLists.txt',
    module: 'Build',
    summary: 'Скрипт збірки CMake 3.16+ (C++17, SFML 2.6.1 FetchContent, 24 модулі, CPack ZIP/NSIS/TGZ та ASan/UBSan)',
    content: cmakeRaw,
  },
  {
    path: 'cpp/LICENSE',
    module: 'Docs',
    summary: 'Двоконтурна ліцензія релізу: MIT License для C++17 коду та CC0 1.0 Public Domain для 3D/Аудіо/Шейдерів',
    content: licenseRaw,
  },
  {
    path: 'cpp/resources/echolabyrinth.rc',
    module: 'Build',
    summary: 'Windows Resource Script (.rc) із метаданими VS_VERSION_INFO (v5.4.0.0) для виконуваного файлу .exe',
    content: winRcRaw,
  },
  {
    path: 'cpp/build_windows.bat',
    module: 'Build',
    summary: 'Автоматичний скрипт збірки в 1 клік для Windows 10/11 (Visual Studio 2022 / MinGW + 21-suite --self-test)',
    content: batRaw,
  },
  {
    path: '.github/workflows/ci.yml',
    module: 'CI',
    summary: 'GitHub Actions CI (Windows MSVC + Ubuntu GCC + ASan/UBSan + кеш FetchContent + артефакти + 21-suite --self-test)',
    content: ciWorkflowRaw,
  },
  {
    path: '.github/workflows/release.yml',
    module: 'CI',
    summary: 'GitHub Actions Release Pipeline: автозбірка Windows ZIP/NSIS та Linux TGZ через CPack + --benchmark 1200 + --export-ost',
    content: releaseWorkflowRaw,
  },
  {
    path: 'cpp/README.md',
    module: 'Docs',
    summary: 'Повна документація EchoLabyrinth 5.4 "Resonance": Етапи 0-4, GLSL шейдери, 4 біоми, 13 ворогів, 6-stem аудіо та CPack реліз',
    content: readmeRaw,
  },
  {
    path: 'cpp/CHANGELOG.md',
    module: 'Docs',
    summary: 'Історія версій проєкту EchoLabyrinth від v1.0 до v5.4.0 (Етап 0 + Етап 1 + Етап 2 + Етап 3 + Етап 4)',
    content: changelogRaw,
  },
  {
    path: 'cpp/.clang-format',
    module: 'Build',
    summary: 'Конфігурація форматування C++17 (Google/LLVM стиль, 4 пробіли, 110 колонок)',
    content: clangFormatRaw,
  },
  {
    path: 'cpp/assets/ASSET_MANIFEST.md',
    module: 'Asset3D',
    summary: 'Реєстр усіх 3D-моделей (.obj), GLSL шейдерів, процедурних текстур матеріалів та 3D-аудіо з ліцензією CC0 1.0 Public Domain',
    content: manifestRaw,
  },
  {
    path: 'cpp/assets/icon/echolabyrinth_icon.svg',
    module: 'Asset3D',
    summary: 'Офіційна векторна іконка застосунку EchoLabyrinth 5.4 (концентричний сонарний приціл та кристал-резонатор)',
    content: appIconSvgRaw,
  },
  {
    path: 'cpp/assets/shaders/acoustic_postfx.frag',
    module: 'Shader',
    summary: 'GLSL #version 120 фрагментний шейдер пост-обробки: 9-tap Gaussian Bloom, хроматична аберація, UV-хвиля удару та CRT сканлайни',
    content: postFxFragRaw,
  },
  {
    path: 'cpp/assets/models/surveyor_rig.obj',
    module: 'Asset3D',
    summary: 'Wavefront 3D OBJ модель дослідника з параболічним сонарним випромінювачем (17 трикутників)',
    content: surveyorObjRaw,
  },
  {
    path: 'cpp/assets/models/listener_stalker.obj',
    module: 'Asset3D',
    summary: 'Wavefront 3D OBJ модель хижака The Listener — Stalker Carapace (14 трикутників)',
    content: stalkerObjRaw,
  },
  {
    path: 'cpp/assets/models/listener_warden.obj',
    module: 'Asset3D',
    summary: 'Wavefront 3D OBJ модель броньованого вартового Heavy Warden Juggernaut (15 трикутників)',
    content: wardenObjRaw,
  },
  {
    path: 'cpp/assets/models/resonator_crystal.obj',
    module: 'Asset3D',
    summary: 'Wavefront 3D OBJ модель октаедричного акустичного кристала-резонатора (8 трикутників)',
    content: crystalObjRaw,
  },
  {
    path: 'cpp/assets/models/harmonic_lever.obj',
    module: 'Asset3D',
    summary: 'Wavefront 3D OBJ модель інтерактивного гармонійного важеля (6 трикутників)',
    content: leverObjRaw,
  },
  {
    path: 'cpp/assets/models/surveyor_terminal.obj',
    module: 'Asset3D',
    summary: 'Wavefront 3D OBJ модель голографічного термінала логів дослідника (8 трикутників)',
    content: terminalObjRaw,
  },
  {
    path: 'cpp/assets/models/vent_turbine.obj',
    module: 'Asset3D',
    summary: 'Wavefront 3D OBJ модель підземної вентиляційної турбіни (8 трикутників)',
    content: turbineObjRaw,
  },
  {
    path: 'cpp/include/Config.hpp',
    module: 'Header',
    summary: 'Централізовані константи v5.3: BalanceTable, 4 складності, 4 біоми, 7 матеріалів, 4 палітри дальтонізму та Closed Captions',
    content: configHppRaw,
  },
  {
    path: 'cpp/include/AcousticSystem.hpp',
    module: 'Header',
    summary: 'Заголовок акустичного рушія: багатократні відбиття, розщеплення EchoCrystal (1->3), палітри ColorblindMode та Echo Trap',
    content: acousticHppRaw,
  },
  {
    path: 'cpp/include/AssetManager.hpp',
    module: 'Header',
    summary: 'Заголовок менеджера ресурсів: завантажувач 3D Wavefront OBJ моделей та процедурні 64x64 текстури (включно з EchoCrystal)',
    content: assetHppRaw,
  },
  {
    path: 'cpp/include/AudioManager.hpp',
    module: 'Header',
    summary: 'Заголовок 3D-аудіорушія OpenAL, 6-стемового адаптивного кросфейду та процедурного Low-Pass / Schroeder Reverb DSP',
    content: audioHppRaw,
  },
  {
    path: 'cpp/include/PostProcessor.hpp',
    module: 'Header',
    summary: 'Заголовок GLSL пост-процесора (sf::RenderTexture FBO + sf::Shader: Bloom, Chromatic Aberration, Shockwave UV, CRT Scanline)',
    content: postProcessorHppRaw,
  },
  {
    path: 'cpp/include/UiRenderer.hpp',
    module: 'Header',
    summary: 'Заголовок векторно-шрифтового рендерера інтерфейсу, HUD та панелей меню',
    content: uiHppRaw,
  },
  {
    path: 'cpp/include/SaveSystem.hpp',
    module: 'Header',
    summary: 'Заголовок атомарної системи збережень ELSV6 (включно з налаштуваннями доступності Stage 3 та FNV-1a checksum)',
    content: saveHppRaw,
  },
  {
    path: 'cpp/include/Labyrinth.hpp',
    module: 'Header',
    summary: 'Заголовок процедурного лабіринту (32 кімнати, 4 біоми по 8 кімнат, міні-бос The Conductor у кімнаті 16)',
    content: labyrinthHppRaw,
  },
  {
    path: 'cpp/include/Player.hpp',
    module: 'Header',
    summary: 'Заголовок гравця: частоти сонара [1/2/3], Charged Ping, Radial Pulse, Echo Trap, Stomp та ефекти реліквій',
    content: playerHppRaw,
  },
  {
    path: 'cpp/include/EnemyAI.hpp',
    module: 'Header',
    summary: 'Заголовок ШІ ворогів: 13 архетипів (Leech, Banshee, TuningForkConstruct, міні-бос The Conductor та фінальний бос)',
    content: enemyHppRaw,
  },
  {
    path: 'cpp/include/Game.hpp',
    module: 'Header',
    summary: 'Головний клас гри (Hit-Stop, GLSL PostProcessor, 4 палітри дальтонізму, Closed Captions, 4 біоми та 20 тест-сьютів)',
    content: gameHppRaw,
  },
  {
    path: 'cpp/src/main.cpp',
    module: 'Source',
    summary: 'Точка входу C++ застосунку з прапорцями --self-test, --balance-report та --export-ost',
    content: mainCppRaw,
  },
  {
    path: 'cpp/src/AcousticSystem.cpp',
    module: 'Source',
    summary: 'Реалізація рейкастингу, розщеплення променів на EchoCrystal (1->3), трансформації кольорів ColorblindMode та Echo Trap',
    content: acousticCppRaw,
  },
  {
    path: 'cpp/src/AssetManager.cpp',
    module: 'Source',
    summary: 'Реалізація парсера Wavefront OBJ, 2.5D-проєкції з освітленням Ламберта та генерації 64x64 текстур (Stone..EchoCrystal)',
    content: assetCppRaw,
  },
  {
    path: 'cpp/src/AudioManager.cpp',
    module: 'Source',
    summary: 'OpenAL 3D просторове аудіо, 6-стемовий адаптивний кросфейд, біомний Low-Pass фільтр та реверберація Шредера',
    content: audioCppRaw,
  },
  {
    path: 'cpp/src/SoundtrackComposer.cpp',
    module: 'Source',
    summary: 'Процедурний DSP-композитор 5 треків саундтреку (44.1 кГц стерео) та автоматичний експорт 16-bit WAV',
    content: soundtrackComposerCppRaw,
  },
  {
    path: 'cpp/src/PostProcessor.cpp',
    module: 'Source',
    summary: 'Реалізація GLSL пост-обробки (Bloom, Chromatic Aberration, Shockwave UV, CRT Scanline) з CPU fallback для headless CI',
    content: postProcessorCppRaw,
  },
  {
    path: 'cpp/src/UiRenderer.cpp',
    module: 'Source',
    summary: 'Реалізація crisp-векторного шрифту та компонентів HUD без зовнішніх бінарних залежностей',
    content: uiCppRaw,
  },
  {
    path: 'cpp/src/SaveSystem.cpp',
    module: 'Source',
    summary: 'Реалізація атомарного запису збережень ELSV6 (включно з colorblindMode, subtitles, shakeIntensity, reduceFlashes, postFx)',
    content: saveCppRaw,
  },
  {
    path: 'cpp/src/LabyrinthGen.cpp',
    module: 'Source',
    summary: 'Генерація 32 кімнат у 4 біомах (Drowned Galleries, Crystal Vaults, Iron Foundry, Living Mycelium) та кімнати 16 (The Conductor)',
    content: labyrinthGenCppRaw,
  },
  {
    path: 'cpp/src/LabyrinthRooms.cpp',
    module: 'Source',
    summary: 'Модуль керування активною кімнатою, поширення шуму крізь двері (0.85/0.35) та BFS-верифікація прохідності',
    content: labyrinthRoomsCppRaw,
  },
  {
    path: 'cpp/src/Relics.cpp',
    module: 'Source',
    summary: 'Модуль 12 реліквій, ротації пулу п’єдесталів та 3 прихованих синергій',
    content: relicsCppRaw,
  },
  {
    path: 'cpp/src/Codex.cpp',
    module: 'Source',
    summary: 'Модуль 12 аудіо-записів Codex (лор експедиції Vane) та прогресу колекції',
    content: codexCppRaw,
  },
  {
    path: 'cpp/src/Player.cpp',
    module: 'Source',
    summary: 'Модуль руху дослідника, Charged Ping, Radial Pulse, Stomp, Echo Trap та Decoy Stone',
    content: playerCppRaw,
  },
  {
    path: 'cpp/src/EnemyAI.cpp',
    module: 'Source',
    summary: 'Модуль ШІ ворогів: 0.45с телеграфування атак, присмоктування Leech, крик Banshee та 2.5D рендеринг',
    content: enemyCppRaw,
  },
  {
    path: 'cpp/src/EnemyVariants.cpp',
    module: 'Source',
    summary: 'Модуль 13 архетипів ворогів (включно з Leech, Banshee, TuningForkConstruct, TheConductor) та вразливостей',
    content: enemyVariantsCppRaw,
  },
  {
    path: 'cpp/src/BossAI.cpp',
    module: 'Source',
    summary: 'Модуль босів та турелей: 3-фазний The Choir Sovereign, 2-фазний The Conductor (4-променевий хрест) та подвійний промінь TuningFork',
    content: bossAiCppRaw,
  },
  {
    path: 'cpp/src/Game.cpp',
    module: 'Source',
    summary: 'Ядро ігрового циклу: Hit-Stop, черга субтитрів звуків (Closed Captions), паровий молот Iron Foundry та взаємодія з ворогами',
    content: gameCppRaw,
  },
  {
    path: 'cpp/src/GameRun.cpp',
    module: 'Source',
    summary: 'Модуль життєвого циклу забігу: синхронізація біомного DSP аудіо, 4 режими складності, туторіал та чекпоінти V6',
    content: gameRunCppRaw,
  },
  {
    path: 'cpp/src/GameNoise.cpp',
    module: 'Source',
    summary: 'Модуль шумової системи з генерацією Closed Captions та запуском GLSL-хвилі спотворення (Shockwave UV)',
    content: gameNoiseCppRaw,
  },
  {
    path: 'cpp/src/GameInput.cpp',
    module: 'Source',
    summary: 'Модуль обробки клавіатури/миші та гарячих клавіш доступності ([F6] Дальтонізм, [F7] Субтитри, [F8] GLSL Post-FX)',
    content: gameInputCppRaw,
  },
  {
    path: 'cpp/src/GameWorldRender.cpp',
    module: 'Source',
    summary: 'Модуль 2.5D рендерингу світу крізь GLSL PostProcessor (Bloom, Chromatic Aberration, Shockwave UV, CRT Scanline)',
    content: gameWorldRenderCppRaw,
  },
  {
    path: 'cpp/src/GameHudRender.cpp',
    module: 'Source',
    summary: 'Модуль HUD: панель візуальних субтитрів звуків ([DIR] + відстань), хвильовий індикатор шуму, стрілки загрози та меню доступності',
    content: gameHudRenderCppRaw,
  },
  {
    path: 'cpp/src/GameBalance.cpp',
    module: 'Source',
    summary: 'Модуль симуляції 1000 пробігів бота (--balance-report) та верифікаційних наборів Stage 1 (Suite 18), Stage 2 (Suite 19) і Stage 3 (Suite 20)',
    content: gameBalanceCppRaw,
  },
  {
    path: 'cpp/src/GameSelfTest.cpp',
    module: 'Source',
    summary: 'Головний раннер автоматичної верифікації: усі 20 детермінованих наборів тестів --self-test (Stage 0 + Stage 1 + Stage 2 + Stage 3)',
    content: gameSelfTestCppRaw,
  },
];


