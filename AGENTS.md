# AGENTS.md

This repository must be handled as UTF-8 text.

## Coordinate System

- This repository uses a left-handed coordinate system.
- Unless a file or subsystem explicitly says otherwise, treat `+Y` as up and keep handedness conversions intentional and explicit.

## Encoding Rules

- Read and write all text files as UTF-8.
- When a tool allows choosing an encoding, explicitly choose UTF-8 instead of relying on defaults.
- Do not save source files, Markdown, JSON, CMake files, shader files, or docs in Shift_JIS, CP932, UTF-16, or ANSI.
- Preserve existing non-ASCII text correctly. Do not replace readable Japanese with mojibake.

## PowerShell Rules

- When reading text, prefer commands that explicitly specify UTF-8.
- When writing text, explicitly use UTF-8 encoding.
- If a command risks using the system default code page, avoid it or force UTF-8.
- Before building with MinGW from PowerShell, prepend `C:\qt\Tools\mingw1310_64\bin` to `PATH`.
- In this repo, prefer:

```powershell
$env:PATH='C:\qt\Tools\mingw1310_64\bin;' + $env:PATH
cmake --build build
```

- Without that `PATH` setup, `g++` may fail because `cc1plus.exe` cannot find its dependent DLLs.

Examples:

```powershell
Get-Content README.md -Encoding utf8
Set-Content AGENTS.md -Encoding utf8 -NoNewline
[IO.File]::WriteAllText($path, $text, [Text.UTF8Encoding]::new($false))
```

## Editing Rules

- Before editing a file that appears garbled, stop and assume an encoding issue first.
- If mojibake is observed, do not continue rewriting blindly. Diagnose the encoding path and then fix it.
- New files must be created in UTF-8.
- Prefer UTF-8 without BOM unless the file already requires a different format for a confirmed reason.

## Verification

- After editing a Japanese text file, quickly re-open it as UTF-8 and confirm the text still renders correctly.
- If a file was already corrupted before the edit, call that out clearly instead of silently normalizing it.
