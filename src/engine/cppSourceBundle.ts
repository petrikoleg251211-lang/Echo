import cmakeRaw from '../../cpp/CMakeLists.txt?raw';
import readmeRaw from '../../cpp/README.md?raw';
import changelogRaw from '../../cpp/CHANGELOG.md?raw';
import clangFormatRaw from '../../cpp/.clang-format?raw';
import batRaw from '../../cpp/build_windows.bat?raw';
import ciWorkflowRaw from '../../.github/workflows/ci.yml?raw';
import manifestRaw from '../../cpp/assets/ASSET_MANIFEST.md?raw';
import surveyorObjRaw from '../../cpp/assets/models/surveyor_rig.obj?raw';
import stalkerObjRaw from '../../cpp/assets/models/listener_stalker.obj?raw';
import wardenObjRaw from '../../cpp/assets/models/listener_warden.obj?raw';
import crystalObjRaw from '../../cpp/assets/models/resonator_crystal.obj?raw';
import leverObjRaw from '../../cpp/assets/models/harmonic_lever.obj?raw';
import terminalObjRaw from '../../cpp/assets/models/surveyor_terminal.obj?raw';
import turbineObjRaw from '../../cpp/assets/models/vent_turbine.obj?raw';

import configHppRaw from '../../cpp/include/Config.hpp?raw';
import acousticHppRaw from '../../cpp/include/AcousticSystem.hpp?raw';
import assetHppRaw from '../../cpp/include/AssetManager.hpp?raw';
import audioHppRaw from '../../cpp/include/AudioManager.hpp?raw';
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
    summary: 'Скрипт збірки CMake 3.16+ (C++17, SFML 2.6.1 FetchContent, 23 модулі Stage 0-2 та опція ASan/UBSan)',
    content: cmakeRaw,
  },
  {
    path: 'cpp/build_windows.bat',
    module: 'Build',
    summary: 'Автоматичний скрипт збірки в 1 клік для Windows 10/11 (Visual Studio 2022 / MinGW + 19-suite --self-test)',
    content: batRaw,
  },
  {
    path: '.github/workflows/ci.yml',
    module: 'CI',
    summary: 'GitHub Actions CI (Windows MSVC + Ubuntu GCC + ASan/UBSan + кеш FetchContent + артефакти + 19-suite --self-test)',
    content: ciWorkflowRaw,
  },
  {
    path: 'cpp/README.md',
    module: 'Docs',
    summary: 'Повна документація EchoLabyrinth 5.2 "Resonance": Етапи 0-2, 4 біоми, 13 ворогів, 5 треків OST та 19-suite self-test',
    content: readmeRaw,
  },
  {
    path: 'cpp/CHANGELOG.md',
    module: 'Docs',
    summary: 'Історія версій проєкту EchoLabyrinth від v1.0 до v5.2.0 (Етап 0 + Етап 1 + Етап 2)',
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
    summary: 'Реєстр усіх 3D-моделей (.obj), процедурних текстур матеріалів та 3D-аудіо з ліцензією CC0 1.0 Public Domain',
    content: manifestRaw,
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
    summary: 'Централізовані константи v5.2: BalanceTable, 4 складності, 4 біоми, 7 матеріалів (EchoCrystal), 12 реліквій та 12 Codex',
    content: configHppRaw,
  },
  {
    path: 'cpp/include/AcousticSystem.hpp',
    module: 'Header',
    summary: 'Заголовок акустичного рушія: багатократні відбиття променів, розщеплення на EchoCrystal (1->3 промені) та Echo Trap',
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
    summary: 'Заголовок 3D-аудіорушія OpenAL та генератора 5 повноцінних композицій саундтреку (SoundtrackComposer)',
    content: audioHppRaw,
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
    summary: 'Заголовок атомарної системи збережень ELSV6 з перевіркою контрольної суми FNV-1a та міграцією V3/V4/V5 -> V6',
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
    summary: 'Головний клас гри (Hit-Stop, 4 режими складності, 3-кімнатний туторіал, 4 біоми та 19 тест-сьютів)',
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
    summary: 'Реалізація рейкастингу, хаотичного розщеплення променів на EchoCrystal (1->3 промені), Glass shatter та Echo Trap',
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
    summary: 'OpenAL 3D просторове позиціонування звуків, синтез SFX та динамічне кросфейд-зведення саундтреку',
    content: audioCppRaw,
  },
  {
    path: 'cpp/src/SoundtrackComposer.cpp',
    module: 'Source',
    summary: 'Процедурний DSP-композитор 5 треків саундтреку (44.1 кГц стерео) та автоматичний експорт 16-bit WAV',
    content: soundtrackComposerCppRaw,
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
    summary: 'Реалізація атомарного запису збережень ELSV6 через .tmp файл, FNV-1a checksum та міграція V3/V4/V5 -> V6',
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
    summary: 'Ядро ігрового циклу: Hit-Stop (30-60 мс), паровий молот Iron Foundry, хвилі води Drowned Galleries та взаємодія з ворогами',
    content: gameCppRaw,
  },
  {
    path: 'cpp/src/GameRun.cpp',
    module: 'Source',
    summary: 'Модуль життєвого циклу забігу: 4 режими складності, 3-кімнатний туторіал, спавн хвиль та чекпоінти V6',
    content: gameRunCppRaw,
  },
  {
    path: 'cpp/src/GameNoise.cpp',
    module: 'Source',
    summary: 'Модуль шумової системи з правилами 4 біомів (приглушення кроків у воді, маскування молотом, поглинання міцелієм)',
    content: gameNoiseCppRaw,
  },
  {
    path: 'cpp/src/GameInput.cpp',
    module: 'Source',
    summary: 'Модуль обробки клавіатури/миші, скидання Leech через Radial Pulse [Space] та перемикання складності',
    content: gameInputCppRaw,
  },
  {
    path: 'cpp/src/GameWorldRender.cpp',
    module: 'Source',
    summary: 'Модуль 2.5D рендерингу світу, біолюмінесцентних спор Living Mycelium та ударної хвилі молота Iron Foundry',
    content: gameWorldRenderCppRaw,
  },
  {
    path: 'cpp/src/GameHudRender.cpp',
    module: 'Source',
    summary: 'Модуль HUD: хвильовий індикатор шуму з причиною, стрілки загрози, бейдж біому, банер туторіалу та шкала The Conductor',
    content: gameHudRenderCppRaw,
  },
  {
    path: 'cpp/src/GameBalance.cpp',
    module: 'Source',
    summary: 'Модуль безголової симуляції 1000 пробігів бота (--balance-report), звіту Top-5 реліквій та тестів Stage 1 (Suite 18) і Stage 2 (Suite 19)',
    content: gameBalanceCppRaw,
  },
  {
    path: 'cpp/src/GameSelfTest.cpp',
    module: 'Source',
    summary: 'Головний раннер автоматичної верифікації: усі 19 детермінованих наборів тестів --self-test (Stage 0 + Stage 1 + Stage 2)',
    content: gameSelfTestCppRaw,
  },
];

