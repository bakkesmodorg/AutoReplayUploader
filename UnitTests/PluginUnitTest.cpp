#include "stdafx.h"
#include "CppUnitTest.h"

#include "Plugin.h"
#include <iostream>
#include <fstream>
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;

#define DEFAULT_REPLAY_NAME_TEMPLATE "{YEAR}-{MONTH}-{DAY}.{HOUR}.{MIN} {PLAYER} {MODE} {WINLOSS}"

namespace UnitTests
{
	void Log(void* object, string message)
	{
		cout << message << endl;
	}

	class MockUploader : IReplayUploader
	{
	public:
		int numUploadInvoked = 0;

		void UploadReplay(string replayPath, string playerId) 
		{
			numUploadInvoked++;
		}
	};

	class MockReplay : IReplay
	{
	public:
		bool FailExportOnFirstCall = false;
		int ExportReplayCalled = 0;
		string replayPath = "";

		int SetReplayNameCalled = 0;
		string replayName = "";

		void ExportReplay(string replayPath)
		{
			this->replayPath = replayPath;
			this->ExportReplayCalled++;

			if (!(ExportReplayCalled == 1 && FailExportOnFirstCall))
			{
				ofstream myfile;
				myfile.open(replayPath, ios::out);
				if (myfile.is_open())
				{
					myfile << "Writing this to a file.\n";
					myfile.close();
				}
			}
		};

		void SetReplayName(string replayName)
		{
			this->replayName = replayName;
			this->SetReplayNameCalled++;
		};
	};

	IReplay* replay;

	TEST_CLASS(PluginUnitTest)
	{
	public:

		TEST_METHOD(DefaultState_DoesNotUpload)
		{
			auto ballchasing = new MockUploader();
			auto calculated = new MockUploader();

			Plugin plugin(Log, this, (IReplayUploader*)ballchasing, (IReplayUploader*)calculated);

			plugin.OnGameComplete(
				"test",
				this,
				[](void* serverWrapper, void(*Log)(void* object, string message), void* object) -> IReplay*
				{
					return new IReplay();
				},
				[](void* serverWrapper, IReplay* replay) -> Match
				{
					Match m;
					return m;
				});

			Assert::AreEqual(0, ballchasing->numUploadInvoked);
			Assert::AreEqual(0, calculated->numUploadInvoked);
		}

		TEST_METHOD(AllUpload_GameNotComplete_DoesNotUpload)
		{
			auto ballchasing = new MockUploader();
			auto calculated = new MockUploader();

			Plugin plugin(Log, this, (IReplayUploader*)ballchasing, (IReplayUploader*)calculated);
			*plugin.uploadToBallchasing = true;
			*plugin.uploadToCalculated = true;

			plugin.OnGameComplete(
				"test",
				this,
				[](void* serverWrapper, void(*Log)(void* object, string message), void* object) -> IReplay*
				{
					return new IReplay();
				},
				[](void* serverWrapper, IReplay* replay) -> Match
				{
					Match m;
					return m;
				});

			Assert::AreEqual(0, ballchasing->numUploadInvoked);
			Assert::AreEqual(0, calculated->numUploadInvoked);
		}

		TEST_METHOD(AllUpload_GameComplete_Uploads)
		{
			auto ballchasing = new MockUploader();
			auto calculated = new MockUploader();

			Plugin plugin(Log, this, (IReplayUploader*)ballchasing, (IReplayUploader*)calculated);
			*plugin.uploadToBallchasing = true;
			*plugin.uploadToCalculated = true;
			*plugin.exportPath = "./";

			plugin.needToUploadReplay = true;
			auto mockReplay = new MockReplay();
			replay = (IReplay*)mockReplay;

			plugin.OnGameComplete(
				"test",
				this,
				[](void* serverWrapper, void(*Log)(void* object, string message), void* object) -> IReplay*
				{
					return replay;
				},
				[](void* serverWrapper, IReplay* replay) -> Match
				{
					Match m;
					return m;
				});

			Assert::AreEqual(1, ballchasing->numUploadInvoked);
			Assert::AreEqual(1, calculated->numUploadInvoked);

			Assert::AreEqual(1, mockReplay->SetReplayNameCalled);
			Assert::AreEqual(1, mockReplay->ExportReplayCalled);
		}

		TEST_METHOD(AllUpload_GameComplete_FirstExportFails)
		{
			auto ballchasing = new MockUploader();
			auto calculated = new MockUploader();

			Plugin plugin(Log, this, (IReplayUploader*)ballchasing, (IReplayUploader*)calculated);
			*plugin.uploadToBallchasing = true;
			*plugin.uploadToCalculated = true;
			*plugin.exportPath = "./";

			plugin.needToUploadReplay = true;
			auto mockReplay = new MockReplay();
			mockReplay->FailExportOnFirstCall = true;
			replay = (IReplay*)mockReplay;

			plugin.OnGameComplete(
				"test",
				this,
				[](void* serverWrapper, void(*Log)(void* object, string message), void* object) -> IReplay*
				{
					return replay;
				},
				[](void* serverWrapper, IReplay* replay) -> Match
				{
					Match m;
					return m;
				});

			Assert::AreEqual(1, ballchasing->numUploadInvoked);
			Assert::AreEqual(1, calculated->numUploadInvoked);

			Assert::AreEqual(1, mockReplay->SetReplayNameCalled);
			Assert::AreEqual(2, mockReplay->ExportReplayCalled);
		}

		TEST_METHOD(AllUpload_GameComplete_UseTemplateSequence)
		{
			auto ballchasing = new MockUploader();
			auto calculated = new MockUploader();

			Plugin plugin(Log, this, (IReplayUploader*)ballchasing, (IReplayUploader*)calculated);
			*plugin.uploadToBallchasing = true;
			*plugin.uploadToCalculated = true;
			*plugin.exportPath = "./";
			*plugin.replayNameTemplate = "{NUM}";

			plugin.needToUploadReplay = true;
			auto mockReplay = new MockReplay();
			replay = (IReplay*)mockReplay;

			plugin.OnGameComplete(
				"test",
				this,
				[](void* serverWrapper, void(*Log)(void* object, string message), void* object) -> IReplay*
				{
					return replay;
				},
				[](void* serverWrapper, IReplay* replay) -> Match
				{
					Match m;
					return m;
				});

			Assert::AreEqual(1, ballchasing->numUploadInvoked);
			Assert::AreEqual(1, calculated->numUploadInvoked);

			Assert::AreEqual(1, mockReplay->SetReplayNameCalled);
			Assert::AreEqual(1, mockReplay->ExportReplayCalled);

			Assert::AreEqual(1, *plugin.templateSequence);
		}
	};
}