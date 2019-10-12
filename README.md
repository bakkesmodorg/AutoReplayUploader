# AutoReplayUploader
BakkesMod plugin that automatically uploads Rocket League replays to other services (calculated.gg, ballchasing.com) when a match ends.

## Description of Source Projects

**AutoReplayUploader:** Contains only the plugin code and anything that has to interact with the Bakkes API.

* NOTE: The project had lots of warnings when compiling in VS2019 build tools. 
  The are silenced for now by adding to ConsoleUploader.vcxproj:
  
  <ClCompile>
  <DisableSpecificWarnings>4251;4244</DisableSpecificWarnings>
  ...
  and:
  <Link>
  <GenerateDebugInformation>False</GenerateDebugInformation>
  ...

  More info about warnings:
    Compiler Warning (level 1) C4251
      -'identifier' : class 'type' needs to have dll-interface to be used by clients of class 'type2'
    Compiler Warning (levels 3 and 4) C4244
      -'conversion' conversion from 'type1' to 'type2', possible loss of data
    Linker Tools Warning LNK4099
      -PDB 'filename' was not found with 'object/library' or at 'path'; linking object as if no debug info


**NOTE2: In order to build, the AutoReplayUploader project has to be updated to add the BakkesMod include directory to the compiler properties as well as adding the folder that contains the bakkesmod.dll to the additional library directory in the linker.*

Bakkesmod folders can be set in:
AutoReplayUploader/AutoReplayUploader/bakkesmod.props

Can be compiled with Visual Studio Build Tools 2019
* Use the Developer PowerShell for VS 2019
* To build the project: 
----------------------
* You must go to AutoReplayUploader folder with the: AutoReplayUploader.sln
* Build the project with command:
  - msbuild AutoReplayUploader.sln /p:PlatformToolset=v141 -p:Configuration=Release
* if you have trouble with paths you can try running:
  - &'C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars32.bat'
----------------------

For the Visual Studio Build Tools 2019 I had these packages installed (in Win10):
Installation details.
MSBuild Tools 
- Provides the tools required to build MSBuild-based applications. 

C++ build tools:
Included:
- C++ Build Tools core features
- C++ 2019 Redistributable Update
- Optional:
- MSVC v142 - VS 2019 C++ x64/x86 build tools (v14.23)
- Windows 10 SDK (10.0.18362.0)
- C++ CMake tools for Windows
- Windows 10 SDK (10.0.17763.0)
- Windows 10 SDK (10.0.17134.0)
- MSVC v141 - VS 2017 C++ x64/x86 build tools (v14.16)
- MSVC v140 - VS 2015 C++ build tools (v14.00)

Individual components
- C# and Visual Basic Roslyn compilers
- MSBuild
- C++ 2019 Redistributable Update
- C++ Clang Compiler for Windows (8.0.1)
- Windows 10 SDK (10.0.17763.0)
- Windows 10 SDK (10.0.17134.0)
- Text Template Transformation
- C++ core features
- MSVC v141 - VS 2017 C++ x64/x86 build tools (v14.16)
- Windows Universal CRT SDK
- MSVC v140 - VS 2015 C++ build tools (v14.00)
- NuGet targets and build tasks
- .NET Framework 4.6 targeting pack
- ClickOnce Build Tools
- Visual Studio SDK Build Tools Core
- NuGet package manager
- MSVC v141 - VS 2017 C++ x64/x86 Spectre-mitigated libs (v14.16)


**Uploader:** Contains all code to upload replays to an endpoint and does not have any dependencies on the Bakkes API.

**ConsoleUploader:** A simple console app to run the uploader outside of Rocket League.

**UnitTests:** UnitTests for the Uploader.vcxproj project.

## Settings available in BakesMod Settings Console

Upload to Calculated - Enable automatic replay uploading to calculated.gg
Upload to Balchasing - Enable automatic replay uploading to ballchasing.com

Replay visibility - Sets the replay visiblity on ballchasing.com when it uploads a replay. Possible values include:
* public
* private
* unlisted

Ballchasing auth key - an authentication key required by ballchasing.com to autoupload replays. Get one at https://ballchasing.com/upload

Replay Name Template - A templatized string to name your replays.  Possible token's that will be replaced are:
* {PLAYER} - Name of current player
* {MODE} - Game mode of replay (Private, Ranked Standard, etc...)
* {NUM} - Current sequence number to allow for uniqueness
* {YEAR} - Year since 1900 % 100, eg. 2019 returns 19
* {MONTH} - Month 1-12
* {DAY} - Day of the month 1-31
* {HOUR} - Hour of the day 0-23
* {MIN} - Min of the hour 0-59
* {WL} - W or L depending on if the player won or lost
* {WINLOSS} - Win or Loss depending on if the player won or lost
* Default = {YEAR}-{MONTH}-{DAY}.{HOUR}.{MIN} {PLAYER} {MODE} {WINLOSS}

Replay Sequence Number - Value use in the {NUM} token in the replay name template above.  Used to give uniqueness to games in a session or series. Usually you want this to start at 1 and it will auto increment from there every time it saves a replay.

Save Replays - Enable exporting all replay files to ExportPath setting

ExportPath - The path the plugin will save replays to
