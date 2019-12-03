#include "stdafx.h"
#include "CppUnitTest.h"

#include "Replay.h"
#include "Match.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#define DEFAULT_REPLAY_NAME_TEMPLATE "{YEAR}-{MONTH}-{DAY}.{HOUR}.{MIN} {PLAYER} {MODE} {WINLOSS}"

namespace UnitTests
{
	TEST_CLASS(ApplyNameTemplateUnitTest)
	{
	public:

		TEST_METHOD(RemoveIllegalCharsFromName)
		{
			Match match;
			match.PrimaryPlayer.Name = "|\\*t?e\"/s<t>";

			int index = 1;
			string replayTemplate = "{PLAYER}";
			string newName = ApplyNameTemplate(replayTemplate, match, &index);

			Assert::AreEqual("test", newName.c_str());
		}
	};
}