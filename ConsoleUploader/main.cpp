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
	Calculated* calculated = new Calculated("consoleuploader", "----boundary", &Log, NULL);
	Ballchasing* ballchasing = new Ballchasing("consoleuploader", "----boundary", &Log, &SetVariable, NULL);

	*(ballchasing->authKey) = "6CWzkHqkuUi3ESOHBnupMGLohIvDd1akLFMfxlyP";
	*(ballchasing->visibility) = "public";

	//ballchasing->UploadReplay("C:/Users/Tyler/OneDrive/RL-Replays/2019-05-21-17-45.replay");

	calculated->UploadReplay("C:/Users/Tyler/OneDrive/RL-Replays/2019-05-21-17-45.replay");

	system("PAUSE");
}