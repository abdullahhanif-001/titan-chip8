@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" > nul

echo [*] Compiling CHIP-8 Virtual Machine & Disassembler...
cl.exe /nologo /O2 /W4 /D_CRT_SECURE_NO_WARNINGS ^
    C:\chip8_emulator\src\chip8.c ^
    C:\chip8_emulator\src\disassembler.c ^
    C:\chip8_emulator\src\main.c ^
    /Fe:C:\chip8_emulator\bin\chip8.exe ^
    /Fo:C:\chip8_emulator\bin\ ^
    /link /OPT:REF /OPT:ICF

if %ERRORLEVEL% neq 0 (
    echo [-] Failed to compile chip8.exe!
    exit /b 1
)

echo [*] Compiling Core Architectural Test Suite...
cl.exe /nologo /O2 /W4 /D_CRT_SECURE_NO_WARNINGS ^
    C:\chip8_emulator\src\chip8.c ^
    C:\chip8_emulator\src\test_suite.c ^
    /Fe:C:\chip8_emulator\bin\test_chip8.exe ^
    /Fo:C:\chip8_emulator\bin\ ^
    /link /OPT:REF /OPT:ICF

if %ERRORLEVEL% neq 0 (
    echo [-] Failed to compile test_chip8.exe!
    exit /b 1
)

echo [*] Compiling Historical Quirks Verification Matrix...
cl.exe /nologo /O2 /W4 /D_CRT_SECURE_NO_WARNINGS ^
    C:\chip8_emulator\src\chip8.c ^
    C:\chip8_emulator\src\test_quirks.c ^
    /Fe:C:\chip8_emulator\bin\test_quirks.exe ^
    /Fo:C:\chip8_emulator\bin\ ^
    /link /OPT:REF /OPT:ICF

if %ERRORLEVEL% neq 0 (
    echo [-] Failed to compile test_quirks.exe!
    exit /b 1
)

echo [+] Build Complete!
del C:\chip8_emulator\bin\*.obj >nul 2>&1
