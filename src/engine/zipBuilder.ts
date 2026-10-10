import { CPP_SOURCE_FILES } from './cppSourceBundle';

// Standard CRC-32 implementation for valid ZIP archives
const CRC_TABLE = (() => {
  const table = new Uint32Array(256);
  for (let n = 0; n < 256; n++) {
    let c = n;
    for (let k = 0; k < 8; k++) {
      c = c & 1 ? 0xedb88320 ^ (c >>> 1) : c >>> 1;
    }
    table[n] = c >>> 0;
  }
  return table;
})();

function crc32(bytes: Uint8Array): number {
  let crc = 0xffffffff;
  for (let i = 0; i < bytes.length; i++) {
    crc = CRC_TABLE[(crc ^ bytes[i]) & 0xff] ^ (crc >>> 8);
  }
  return (crc ^ 0xffffffff) >>> 0;
}

export interface ZipEntryInput {
  path: string;
  content: string;
}

export function createZipBlob(entries: ZipEntryInput[]): Blob {
  const encoder = new TextEncoder();
  const fileRecords: {
    nameBytes: Uint8Array;
    dataBytes: Uint8Array;
    crc: number;
    offset: number;
  }[] = [];

  const chunks: Uint8Array[] = [];
  let offset = 0;

  for (const entry of entries) {
    const nameBytes = encoder.encode(entry.path);
    const dataBytes = encoder.encode(entry.content);
    const crc = crc32(dataBytes);

    // Local file header (30 bytes + filename)
    const localHeader = new Uint8Array(30 + nameBytes.length);
    const view = new DataView(localHeader.buffer);
    view.setUint32(0, 0x04034b50, true); // Local file header signature
    view.setUint16(4, 20, true); // Version needed to extract (2.0)
    view.setUint16(6, 0x0800, true); // UTF-8 filename flag
    view.setUint16(8, 0, true); // Compression method (0 = Store)
    view.setUint16(10, 0, true); // File last mod time
    view.setUint16(12, 0x5421, true); // File last mod date
    view.setUint32(14, crc, true); // CRC-32
    view.setUint32(18, dataBytes.length, true); // Compressed size
    view.setUint32(22, dataBytes.length, true); // Uncompressed size
    view.setUint16(26, nameBytes.length, true); // File name length
    view.setUint16(28, 0, true); // Extra field length
    localHeader.set(nameBytes, 30);

    fileRecords.push({ nameBytes, dataBytes, crc, offset });
    chunks.push(localHeader, dataBytes);
    offset += localHeader.length + dataBytes.length;
  }

  const centralDirOffset = offset;
  let centralDirSize = 0;

  for (const rec of fileRecords) {
    const cdHeader = new Uint8Array(46 + rec.nameBytes.length);
    const view = new DataView(cdHeader.buffer);
    view.setUint32(0, 0x02014b50, true); // Central directory signature
    view.setUint16(4, 20, true); // Version made by
    view.setUint16(6, 20, true); // Version needed
    view.setUint16(8, 0x0800, true); // UTF-8 flag
    view.setUint16(10, 0, true); // Compression method (Store)
    view.setUint16(12, 0, true); // Mod time
    view.setUint16(14, 0x5421, true); // Mod date
    view.setUint32(16, rec.crc, true); // CRC-32
    view.setUint32(20, rec.dataBytes.length, true); // Compressed size
    view.setUint32(24, rec.dataBytes.length, true); // Uncompressed size
    view.setUint16(28, rec.nameBytes.length, true); // Filename length
    view.setUint16(30, 0, true); // Extra field length
    view.setUint16(32, 0, true); // Comment length
    view.setUint16(34, 0, true); // Disk number
    view.setUint16(36, 0, true); // Internal attrs
    view.setUint32(38, 0, true); // External attrs
    view.setUint32(42, rec.offset, true); // Local header offset
    cdHeader.set(rec.nameBytes, 46);

    chunks.push(cdHeader);
    centralDirSize += cdHeader.length;
  }

  // End of central directory record (22 bytes)
  const eocd = new Uint8Array(22);
  const eocdView = new DataView(eocd.buffer);
  eocdView.setUint32(0, 0x06054b50, true);
  eocdView.setUint16(4, 0, true);
  eocdView.setUint16(6, 0, true);
  eocdView.setUint16(8, fileRecords.length, true);
  eocdView.setUint16(10, fileRecords.length, true);
  eocdView.setUint32(12, centralDirSize, true);
  eocdView.setUint32(16, centralDirOffset, true);
  eocdView.setUint16(20, 0, true);
  chunks.push(eocd);

  return new Blob(chunks as unknown as BlobPart[], { type: 'application/zip' });
}

export function downloadCppProjectZip(): void {
  const readmeContent = `# EchoLabyrinth_SFML — 2D Acoustic Horror Game (C++17 / SFML 2.6)

## Як зібрати та запустити на Windows (Visual Studio 2022 + CMake)

1. Переконайтеся, що у вас встановлено **CMake 3.20+** та **Visual Studio 2022** (із компонентом "Desktop development with C++") та **Git**.
2. Двічі клікніть на файл \`build_windows.bat\` АБО виконайте у терміналі:
   \`\`\`powershell
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --config Release
   .\\build\\Release\\EchoLabyrinth_SFML.exe
   \`\`\`
   *Примітка:* Якщо SFML 2.6 не встановлено локально, \`CMakeLists.txt\` автоматично завантажить SFML 2.6.1 через \`FetchContent\` і скопіює всі потрібні \`.dll\` поруч із \`.exe\`.

## Керування у грі
- **WASD**: Рух
- **LShift**: Тихий крок (Sneak — вороги майже не чують)
- **LCtrl**: Біг (Sprint — гучні кроки)
- **Ліва кнопка миші**: Спрямований сонарний імпульс у бік курсора
- **Пробіл (Space)**: Круговий акустичний імпульс на 360°
- **Права кнопка миші**: Кинути камінь-приманку (Decoy Stone)
- **F**: Активувати гармонічний важіль дверей / підібрати запис
- **F5 / F9**: Зберегти / Завантажити чекпойнт
- **Esc**: Пауза
`;

  const batScript = `@echo off
echo ==========================================================
echo Building EchoLabyrinth_SFML (C++17 + SFML 2.6)...
echo ==========================================================
cmake -B build -DCMAKE_BUILD_TYPE=Release
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed. Make sure CMake and a C++ compiler are installed.
    pause
    exit /b %errorlevel%
)
cmake --build build --config Release
if %errorlevel% neq 0 (
    echo [ERROR] Compilation failed.
    pause
    exit /b %errorlevel%
)
echo.
echo [SUCCESS] Build finished! Launching EchoLabyrinth_SFML...
if exist "build\\Release\\EchoLabyrinth_SFML.exe" (
    start "" "build\\Release\\EchoLabyrinth_SFML.exe"
) else if exist "build\\EchoLabyrinth_SFML.exe" (
    start "" "build\\EchoLabyrinth_SFML.exe"
)
pause
`;

  const entries: ZipEntryInput[] = [
    { path: 'EchoLabyrinth_SFML/README.md', content: readmeContent },
    { path: 'EchoLabyrinth_SFML/build_windows.bat', content: batScript },
    ...CPP_SOURCE_FILES.map((file) => ({
      path: file.path.replace(/^cpp\//, 'EchoLabyrinth_SFML/'),
      content: file.content,
    })),
  ];

  const blob = createZipBlob(entries);
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = 'EchoLabyrinth_SFML_Cpp17_Project.zip';
  a.click();
  URL.revokeObjectURL(url);
}
