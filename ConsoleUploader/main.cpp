#include "..\Uploader\Ballchasing.h"
#include "..\Uploader\Calculated.h"

void Log(void* object, string message)
{
	cout << message << endl;
}

void SetVariable(void* object, string name, string value)
{
	cout << "Set: " << name << " to: " << value << endl;
}

int main()
{
	string replayFile = "C:/Program Files (x86)/Steam/steamapps/common/rocketleague/Binaries/Win32/bakkesmod/data/autoupload.replay";

	Calculated* calculated = new Calculated("consoleuploader", "----boundary", &Log, NULL);
	Ballchasing* ballchasing = new Ballchasing("consoleuploader", "----boundary", &Log, &SetVariable, NULL);

	*(ballchasing->authKey) = "";
	*(ballchasing->visibility) = "public";

	ballchasing->UploadReplay(replayFile);
	calculated->UploadReplay(replayFile);

	system("PAUSE");
}