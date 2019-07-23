#include "Ballchasing.h"
#include "Calculated.h"
#include "Utils.h"
#include "Replay.h"

#include <fstream>
#include <cstdlib>
#include <cerrno>

#ifdef _WIN32
#pragma comment(lib, "Wldap32.Lib")
#pragma comment(lib, "Crypt32.Lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "Wldap32.lib")
#pragma comment(lib, "Normaliz.lib")
#pragma comment(lib, "libcurl_a.lib")
#pragma comment(lib, "curlpp.lib")

#include <windows.h>
#include <Shlobj_core.h>
#include <sstream>
#include <memory>
#include <curl/curl.h>

//#include "curlpp/curlpp.cpp"
#include "curlpp/cURLpp.hpp"
#include "curlpp/Easy.hpp"
#include "curlpp/Options.hpp"
#include "curlpp/Exception.hpp"

#include <thread>
#include <future>
#endif

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

void PostFileCurl(string& url, string& filePath, string& paramName)
{
	CURL *curl;
	CURLcode res;

	curl_mime *form = NULL;
	curl_mimepart *field = NULL;
	struct curl_slist *headerlist = NULL;
	static const char buf[] = "Expect:";

	curl_global_init(CURL_GLOBAL_ALL);

	curl = curl_easy_init();
	if (curl) {
		/* Create the form */
		form = curl_mime_init(curl);

		/* Fill in the file upload field */
		field = curl_mime_addpart(form);
		curl_mime_name(field, paramName.c_str());
		curl_mime_filedata(field, filePath.c_str());

		/* initialize custom header list */
		headerlist = curl_slist_append(headerlist, buf);
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
		curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

		curl_easy_setopt(curl, CURLOPT_PROXY, "http://localhost:8888");

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);

		/* Check for errors */
		if (res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));

		/* always cleanup */
		curl_easy_cleanup(curl);

		/* then cleanup the form */
		curl_mime_free(form);

		/* free slist */
		curl_slist_free_all(headerlist);
	}
}

void PostFileCurlcpp(string& url, string& filePath, string& paramName)
{
	try {
		curlpp::Cleanup cleaner;
		curlpp::Easy request;

		request.setOpt(new curlpp::options::Verbose(true));
		request.setOpt(new curlpp::options::Url(url));
		
		{
			// Forms takes ownership of pointers!
			curlpp::Forms formParts;
			formParts.push_back(new curlpp::FormParts::Content("name1", "value1"));
			formParts.push_back(new curlpp::FormParts::Content("name2", "value2"));

			request.setOpt(new curlpp::options::HttpPost(formParts));
		}

		// The forms have been cloned and are valid for the request, even
		// if the original forms are out of scope.
		std::ofstream myfile(filePath);
		myfile << request << std::endl << request << std::endl;
	}
	catch (curlpp::LogicError & e) {
		std::cout << e.what() << std::endl;
	}
	catch (curlpp::RuntimeError & e) {
		std::cout << e.what() << std::endl;
	}
}

int main(int argc, char *argv[])
{
	string url("http://calculated.gg/api/upload");
	string path("C:/Users/tyni/Desktop/979B723811E971FCE06E328BDF9F6172.replay");
	string paramName("replays");

	PostFileCurlcpp(url, path, paramName);

	return EXIT_SUCCESS;
}