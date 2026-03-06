@echo off
setlocal
set "ROOT=%~dp0"
cd /d "%ROOT%"

:: VS 2022/2019 - vcvars32 (x86) bul
set "VCVARS="
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat" set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat"
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars32.bat" set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars32.bat"
if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars32.bat" set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars32.bat"
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat" set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat"

if "%VCVARS%"=="" (
  echo [HATA] vcvars32.bat bulunamadi. Visual Studio x86 build tools yuklu olmali.
  exit /b 1
)

call "%VCVARS%" >nul 2>&1
if errorlevel 1 (
  echo [HATA] vcvars cagrildi, hata.
  exit /b 1
)

:: Release x86 DLL: version.dll (tum proxyler C'de, proxy.asm yok)
cl.exe /nologo /W3 /O2 /DNDEBUG /DWIN32 /D_WINDOWS /D_USRDLL /D_CRT_SECURE_NO_WARNINGS /LD ^
  /I. pch.cpp dllmain.cpp /Fe:version.dll /link /DEF:version.def user32.lib

if errorlevel 1 (
  echo [HATA] Derleme basarisiz.
  exit /b 1
)

echo.
echo [OK] version.dll olusturuldu: %ROOT%version.dll
exit /b 0
