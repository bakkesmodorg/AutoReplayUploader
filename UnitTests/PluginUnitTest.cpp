#include "stdafx.h"
#include "CppUnitTest.h"

#include "Plugin.h"
#include <iostream>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;

#define DEFAULT_REPLAY_NAME_TEMPLATE "{YEAR}-{MONTH}-{DAY}.{HOUR}.{MIN} {PLAYER} {MODE} {WINLOSS}"

namespace UnitTests
{
	IReplay* replay;

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
			plugin.needToUploadReplay = true;
			replay = new IReplay();

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
		}
	};
}