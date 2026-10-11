import cmakeRaw from '../../cpp/CMakeLists.txt?raw';
import readmeRaw from '../../cpp/README.md?raw';
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
import uiCppRaw from '../../cpp/src/UiRenderer.cpp?raw';
import saveCppRaw from '../../cpp/src/SaveSystem.cpp?raw';
import labyrinthCppRaw from '../../cpp/src/Labyrinth.cpp?raw';
import playerCppRaw from '../../cpp/src/Player.cpp?raw';
import enemyCppRaw from '../../cpp/src/EnemyAI.cpp?raw';
import gameCppRaw from '../../cpp/src/Game.cpp?raw';
import gameInputCppRaw from '../../cpp/src/GameInput.cpp?raw';
import gameWorldRenderCppRaw from '../../cpp/src/GameWorldRender.cpp?raw';
import gameHudRenderCppRaw from '../../cpp/src/GameHudRender.cpp?raw';
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
    summary: 'Скрипт збірки CMake 3.16+ (C++17, SFML 2.6.1 FetchContent, 5 модулів Game*.cpp та копіювання 3D .obj моделей)',
    content: cmakeRaw,
  },
  {
    path: 'cpp/build_windows.bat',
    module: 'Build',
    summary: 'Автоматичний скрипт збірки в 1 клік для Windows 10/11 (Visual Studio 2022 / MinGW + запуск --self-test)',
    content: batRaw,
  },
  {
    path: '.github/workflows/ci.yml',
    module: 'CI',
    summary: 'GitHub Actions CI (Windows MSVC + Ubuntu GCC): компіляція без попереджень та запуск 13 тестів --self-test',
    content: ciWorkflowRaw,
  },
  {
    path: 'cpp/README.md',
    module: 'Docs',
    summary: 'Повна документація EchoLabyrinth 5.0 "Resonance": Noise Meter, матеріали, реліквії та інструкція запуску',
    content: readmeRaw,
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
    summary: 'Константи v5.0 (Phase A + B): NoiseTier, AcousticFrequency (Low/Mid/High), 7 матеріалів (включно з Glass) та 9 реліквій',
    content: configHppRaw,
  },
  {
    path: 'cpp/include/AcousticSystem.hpp',
    module: 'Header',
    summary: 'Заголовок акустичного рушія: фізика матеріалів (Moss -80%, Metal +2 відбиття, Glass руйнування, Water 2x, 4с гул колон) і частоти',
    content: acousticHppRaw,
  },
  {
    path: 'cpp/include/AssetManager.hpp',
    module: 'Header',
    summary: 'Заголовок менеджера ресурсів: завантажувач 3D Wavefront OBJ моделей, 2.5D-проєктор і 7 текстур матеріалів',
    content: assetHppRaw,
  },
  {
    path: 'cpp/include/AudioManager.hpp',
    module: 'Header',
    summary: 'Заголовок 3D-аудіорушія OpenAL: просторове позиціонування звуків, оклюзія стінами та 4-шаровий саундтрек',
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
    summary: 'Заголовок атомарної системи збережень V4 з перевіркою контрольної суми FNV-1a',
    content: saveHppRaw,
  },
  {
    path: 'cpp/include/Labyrinth.hpp',
    module: 'Header',
    summary: 'Заголовок процедурного генератора забігу, 9 типів ворогів (Блок 4), міжкімнатного шуму та Ghost Clear',
    content: labyrinthHppRaw,
  },
  {
    path: 'cpp/include/Player.hpp',
    module: 'Header',
    summary: 'Заголовок гравця (Блок 3): Свист [Q], Тупіт [C], Відкладена Ехо-пастка [R] (3с), частоти сонара [1/2/3] та реліквії',
    content: playerHppRaw,
  },
  {
    path: 'cpp/include/EnemyAI.hpp',
    module: 'Header',
    summary: 'Заголовок ШІ ворогів (Блок 4): 9 архетипів (SilentStalker, DeafBrute, Mimic, EchoSwarm) та відштовхування від Stomp',
    content: enemyHppRaw,
  },
  {
    path: 'cpp/include/Game.hpp',
    module: 'Header',
    summary: 'Головний клас гри (модульна архітектура v5.0: Game, GameInput, GameWorldRender, GameHudRender, GameSelfTest)',
    content: gameHppRaw,
  },
  {
    path: 'cpp/src/main.cpp',
    module: 'Source',
    summary: 'Точка входу C++ застосунку з підтримкою прапорця --self-test',
    content: mainCppRaw,
  },
  {
    path: 'cpp/src/AcousticSystem.cpp',
    module: 'Source',
    summary: 'Реалізація Блоку 2 і 3: DampMoss (-80% та сховище), Metal (+2 відбиття), розбиття Glass (+40 шуму), Water (2x), 4с гул колон і частоти',
    content: acousticCppRaw,
  },
  {
    path: 'cpp/src/AssetManager.cpp',
    module: 'Source',
    summary: 'Реалізація парсера Wavefront OBJ, 2.5D-проєкції з освітленням Ламберта та генерації 7 текстур матеріалів (включно з Brittle Glass)',
    content: assetCppRaw,
  },
  {
    path: 'cpp/src/AudioManager.cpp',
    module: 'Source',
    summary: 'Процедурний синтез PCM-ефектів (включно з Whistle, Stomp, EchoTrap, GlassShatter), OpenAL 3D-позиціонування та саундтрек',
    content: audioCppRaw,
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
    summary: 'Реалізація атомарного запису збережень V4 через .tmp файл та перевірки FNV-1a checksum',
    content: saveCppRaw,
  },
  {
    path: 'cpp/src/Labyrinth.cpp',
    module: 'Source',
    summary: 'Побудова 32 процедурних кімнат з матеріалами Блоку 2 (Glass, DampMoss, Metal, Water), 9 ворогами Блоку 4 та BFS-верифікатор',
    content: labyrinthCppRaw,
  },
  {
    path: 'cpp/src/Player.cpp',
    module: 'Source',
    summary: 'Реалізація Блоку 3: вузький промінь Свисту (15°), сейсмічний Тупіт, 3-секундна Ехо-пастка та перемикання частот Low/Mid/High',
    content: playerCppRaw,
  },
  {
    path: 'cpp/src/EnemyAI.cpp',
    module: 'Source',
    summary: 'Реалізація Блоку 4: SilentStalker (вдих за 88px), DeafBrute (тільки вібрація підлоги), Mimic (фальшивий кристал) та EchoSwarm (+12 шуму)',
    content: enemyCppRaw,
  },
  {
    path: 'cpp/src/Game.cpp',
    module: 'Source',
    summary: 'Ядро ігрового циклу: інтеграція Блоків 1–4, сплески води (+15 шуму), розбиття скла (+40), сховища в моху (2x спад шуму) та Stomp',
    content: gameCppRaw,
  },
  {
    path: 'cpp/src/GameInput.cpp',
    module: 'Source',
    summary: 'Модуль вводу: гарячі клавіші Свисту [Q], Тупоту [C/V], Ехо-пастки [R/X], частот сонара [1/2/3] та магазину',
    content: gameInputCppRaw,
  },
  {
    path: 'cpp/src/GameWorldRender.cpp',
    module: 'Source',
    summary: 'Модуль рендерингу світу: 2.5D/3D об’єкти, 4-секундна аура гудіння резонаторних колон, пастки та розбите скло',
    content: gameWorldRenderCppRaw,
  },
  {
    path: 'cpp/src/GameHudRender.cpp',
    module: 'Source',
    summary: 'Модуль HUD та оверлеїв: хвилеподібний Noise Meter (0..100), індикатори пасток/частоти сонара, карта та Field Manual',
    content: gameHudRenderCppRaw,
  },
  {
    path: 'cpp/src/GameSelfTest.cpp',
    module: 'Source',
    summary: 'Модуль автоматичної верифікації: 16 детермінованих тестів --self-test (покриття Phase A та Phase B: Блоки 1, 2, 3 і 4)',
    content: gameSelfTestCppRaw,
  },
];
