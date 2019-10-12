# AutoReplayUploader
BakkesMod plugin that automatically uploads Rocket League replays to other services (calculated.gg, ballchasing.com) when a match ends.

## Description of Source Projects

**AutoReplayUploader:** Contains only the plugin code and anything that has to interact with the Bakkes API.

*NOTE: In order to build, the AutoReplayUploader project has to be updated to add the BakkesMod include directory to the compiler properties as well as adding the folder that contains the bakkesmod.dll to the additional library directory in the linker.*

**Uploader:** Contains all code to upload replays to an endpoint and does not have any dependencies on the Bakkes API.

**ConsoleUploader:** A simple console app to run the uploader outside of Rocket League.

**UnitTests:** UnitTests for the Uploader.vcxproj project.

## Settings available in BakkesMod Settings Console

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
