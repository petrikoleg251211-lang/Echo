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
    summary: 'Константи v5.0: NoiseTier, NoiseAction, cfg::noise, 9 реліквій (включно з SilentStep) та матеріали',
    content: configHppRaw,
  },
  {
    path: 'cpp/include/AcousticSystem.hpp',
    module: 'Header',
    summary: 'Заголовок акустичного рушія: пули променів, хвильові фронти, резонатори та багатостінна оклюзія',
    content: acousticHppRaw,
  },
  {
    path: 'cpp/include/AssetManager.hpp',
    module: 'Header',
    summary: 'Заголовок менеджера ресурсів: завантажувач 3D Wavefront OBJ моделей, 2.5D-проєктор і текстури матеріалів',
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
    summary: 'Заголовок атомарної системи збережень з перевіркою контрольної суми FNV-1a',
    content: saveHppRaw,
  },
  {
    path: 'cpp/include/Labyrinth.hpp',
    module: 'Header',
    summary: 'Заголовок процедурного генератора забігу, міжкімнатного поширення шуму (RoomNoiseLeak) та Ghost Clear',
    content: labyrinthHppRaw,
  },
  {
    path: 'cpp/include/Player.hpp',
    module: 'Header',
    summary: 'Заголовок контролера гравця: фізика, витривалість, приманки, реліквії та множники шуму',
    content: playerHppRaw,
  },
  {
    path: 'cpp/include/EnemyAI.hpp',
    module: 'Header',
    summary: 'Заголовок ШІ ворогів: 6-станові автомати, 5 архетипів та реакція на пороги шуму (applyNoiseTierStimulus)',
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
    summary: 'Реалізація відбиття променів, затухання, резонаторів та рендерингу текстурованих 2.5D стін',
    content: acousticCppRaw,
  },
  {
    path: 'cpp/src/AssetManager.cpp',
    module: 'Source',
    summary: 'Реалізація парсера Wavefront OBJ, 2.5D-проєкції з освітленням Ламберта та генерації текстур матеріалів',
    content: assetCppRaw,
  },
  {
    path: 'cpp/src/AudioManager.cpp',
    module: 'Source',
    summary: 'Процедурний синтез 17 PCM-ефектів, OpenAL 3D-позиціонування та динамічний 4-шаровий dark-ambient саундтрек',
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
    summary: 'Реалізація атомарного запису збережень через .tmp файл та перевірки FNV-1a checksum',
    content: saveCppRaw,
  },
  {
    path: 'cpp/src/Labyrinth.cpp',
    module: 'Source',
    summary: 'Побудова 32 процедурних кімнат, поширення шуму через відкриті двері (50% загасання) та BFS-верифікатор',
    content: labyrinthCppRaw,
  },
  {
    path: 'cpp/src/Player.cpp',
    module: 'Source',
    summary: 'Реалізація руху, колізій, приманок, реліквії SilentStep та 2.5D рендерингу моделі дослідника',
    content: playerCppRaw,
  },
  {
    path: 'cpp/src/EnemyAI.cpp',
    module: 'Source',
    summary: 'Реалізація слуху ШІ, реакції на пороги шуму (Quiet/Audible/Loud/Scream) та 2.5D рендерингу моделей',
    content: enemyCppRaw,
  },
  {
    path: 'cpp/src/Game.cpp',
    module: 'Source',
    summary: 'Ядро ігрового циклу (~440 рядків): переходи між кімнатами, Noise Meter, прорив ворогів на шум та Ghost Clear',
    content: gameCppRaw,
  },
  {
    path: 'cpp/src/GameInput.cpp',
    module: 'Source',
    summary: 'Модуль вводу та взаємодії (~310 рядків): клавіатура, миша, магазин, реліквії та навігація по меню',
    content: gameInputCppRaw,
  },
  {
    path: 'cpp/src/GameWorldRender.cpp',
    module: 'Source',
    summary: 'Модуль рендерингу світу (~320 рядків): 2.5D/3D об’єкти, портали, кристали, пастки та атмосферний пил',
    content: gameWorldRenderCppRaw,
  },
  {
    path: 'cpp/src/GameHudRender.cpp',
    module: 'Source',
    summary: 'Модуль HUD та оверлеїв (~470 рядків): хвилеподібний Noise Meter (0..100), мінікарта, повна карта та меню',
    content: gameHudRenderCppRaw,
  },
  {
    path: 'cpp/src/GameSelfTest.cpp',
    module: 'Source',
    summary: 'Модуль автоматичної верифікації (~548 рядків): 13 детермінованих тестів --self-test (включно з інтеграційними тестами Блоку 1)',
    content: gameSelfTestCppRaw,
  },
];
