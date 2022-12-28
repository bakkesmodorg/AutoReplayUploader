#include "stdafx.h"
#include "CppUnitTest.h"

#include "Replay.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#define DEFAULT_EXPORT_PATH "./defaultPath"
namespace UnitTests
{
	TEST_CLASS(SanitizePlayerNameUnitTest)
	{
	public:

		TEST_METHOD(RemovesIllegalChars)
		{
			std::string newName = SanitizePlayerName("|\\*t?e\"/s<t>", "Player");
			Assert::AreEqual("test", newName.c_str());
		}

		TEST_METHOD(RemovesIllegalChars2)
		{
			std::string newName = SanitizePlayerName("testing.,/;'[]♿", "Player");
			Assert::AreEqual("testing.,;'[]", newName.c_str());
		}
	};
}