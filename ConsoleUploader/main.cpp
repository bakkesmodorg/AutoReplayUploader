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
	string replayFile = "C:/Users/tyni/Desktop/7248E9C211E9A2BF122FB1BF6FD9AA21.replay";

	//Ballchasing* ballchasing = new Ballchasing("consoleuploader", &Log, &BallchasingUploadComplete, &BallchasingAuthTestComplete, NULL);
	//*(ballchasing->authKey) = "";
	//*(ballchasing->visibility) = "public";
	//ballchasing->UploadReplay(replayFile);

	Calculated* calculated = new Calculated("consoleuploader", &Log, &CalculatedUploadComplete, NULL);
	*(calculated->visibility) = "PUBLIC";
	calculated->UploadReplay(replayFile, "76561198011976380");

	system("PAUSE");
}