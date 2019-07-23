#include "Ballchasing.h"
#include "Calculated.h"
#include "Utils.h"
#include "Replay.h"

void Log(void* object, string message)
{
	cout << message << endl;
}

void SetVariable(void* object, string name, string value)
{
	cout << "Set: " << name << " to: " << value << endl;
}

void CalculatedUploadComplete(void* object, bool result)
{
	cout << "Calculated upload completed with result: " << result;
}

void BallchasingUploadComplete(void* object, bool result)
{
	cout << "Ballchasing upload completed with result: " << result;
}

void BallchasingAuthTestComplete(void* object, bool result)
{
	cout << "Ballchasing authtest completed with result: " << result;
}

int main()
{
	string replayFile = "C:/Users/tyni/Desktop/979B723811E971FCE06E328BDF9F6172.replay";

	Calculated* calculated = new Calculated("consoleuploader", &Log, &CalculatedUploadComplete, NULL);
	Ballchasing* ballchasing = new Ballchasing("consoleuploader", &Log, &BallchasingUploadComplete, &BallchasingAuthTestComplete, NULL);

	*(ballchasing->authKey) = "";
	*(ballchasing->visibility) = "public";

	//ballchasing->UploadReplay(replayFile);
	calculated->UploadReplay(replayFile);

	system("PAUSE");
}