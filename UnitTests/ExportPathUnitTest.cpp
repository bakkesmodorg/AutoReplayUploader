#include "stdafx.h"
#include "CppUnitTest.h"

#include "Replay.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#define DEFAULT_EXPORT_PATH "./defaultPath"
namespace UnitTests
{		
	TEST_CLASS(ExportPathUnitTests)
	{
	public:

		TEST_METHOD(ValidReturnsFalse)
		{
			std::shared_ptr<std::string> exportPath = std::make_shared<std::string>("C:/test");
			bool changed = SanitizeExportPath(exportPath, DEFAULT_EXPORT_PATH);

			// ensure nothing changed and we returned false
			Assert::AreEqual("C:/test", exportPath->c_str());
			Assert::AreEqual(false, changed);

			// try a couple more valid values
			*exportPath = "./test";
			changed = SanitizeExportPath(exportPath, DEFAULT_EXPORT_PATH);
			Assert::AreEqual("./test", exportPath->c_str());
			Assert::AreEqual(false, changed);

			*exportPath = "te/st";
			changed = SanitizeExportPath(exportPath, DEFAULT_EXPORT_PATH);
			Assert::AreEqual("te/st", exportPath->c_str());
			Assert::AreEqual(false, changed);
		}
		
		TEST_METHOD(ConvertsAllBSlashToFSlash)
		{
			std::shared_ptr<std::string> exportPath = std::make_shared<std::string>("C:\\test\\");
			bool changed = SanitizeExportPath(exportPath, DEFAULT_EXPORT_PATH);

			// ensure it no longer contains back slash
			// ensure we replaced with forward
			// ensure we removed trailing slash
			Assert::AreEqual("C:/test", exportPath->c_str());
			Assert::AreEqual(true, changed);
		}

		TEST_METHOD(EmptyPathReturnsDefault)
		{
			std::shared_ptr<std::string> exportPath = std::make_shared<std::string>("");
			bool changed = SanitizeExportPath(exportPath, DEFAULT_EXPORT_PATH);

			// ensure an empty path returns the default
			Assert::AreEqual(DEFAULT_EXPORT_PATH, exportPath->c_str());
			Assert::AreEqual(true, changed);
		}

		TEST_METHOD(IllegalCharsRemoved)
		{
			std::shared_ptr<std::string> exportPath = std::make_shared<std::string>("|C:\\*t?e\"s<t>");
			bool changed = SanitizeExportPath(exportPath, DEFAULT_EXPORT_PATH);

			// ensure all illegal chars are removed
			Assert::AreEqual("C:/test", exportPath->c_str());
			Assert::AreEqual(true, changed);
		}

		TEST_METHOD(OnlyIllegalCharsReturnsDefault)
		{
			std::shared_ptr<std::string> exportPath = std::make_shared<std::string>("*?\"<>|");
			bool changed = SanitizeExportPath(exportPath, DEFAULT_EXPORT_PATH);

			// ensure if orignal is all illegal strings the we return the default
			Assert::AreEqual(DEFAULT_EXPORT_PATH, exportPath->c_str());
			Assert::AreEqual(true, changed);
		}

		TEST_METHOD(OnlyTrailingSlashReturnsDefault)
		{
			std::shared_ptr<std::string> exportPath = std::make_shared<std::string>("/");
			bool changed = SanitizeExportPath(exportPath, DEFAULT_EXPORT_PATH);

			// ensure only a trailing slash returns the default
			Assert::AreEqual(DEFAULT_EXPORT_PATH, exportPath->c_str());
			Assert::AreEqual(true, changed);
		}

	};
}