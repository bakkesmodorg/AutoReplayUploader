#pragma once

#ifdef _WIN32
#pragma comment(lib, "Wldap32.Lib")
#pragma comment(lib, "Crypt32.Lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libssl_static.lib")
#pragma comment(lib, "Wldap32.lib")
#pragma comment(lib, "Normaliz.lib")
#pragma comment(lib, "libcurl.lib")
#pragma comment(lib, "curlpp.lib")
#include <windows.h>
#include <Shlobj_core.h>
#include <sstream>
#include <memory>

//#include "curlpp/curlpp.cpp"
#include "curlpp/cURLpp.hpp"
#include "curlpp/Easy.hpp"
#include "curlpp/Options.hpp"
#include "curlpp/Exception.hpp"
#include "curlpp/Infos.hpp"

#include <thread>
#endif
#include <filesystem>


using namespace std;

struct PostFileRequest
{
	unsigned int RequestId = 0;
	void* Requester = NULL;

	string Url;
	list<string> Headers;

	std::filesystem::path FilePath;
	string ParamName;

	void(*RequestComplete)(PostFileRequest*);
	long Status;
	string Message;
	string ResponseBody;
};

struct GetRequest
{
	unsigned int RequestId = 0;
	void* Requester = NULL;

	string Url;
	list<string> Headers;

	void(*RequestComplete)(GetRequest*);
	long Status;
};

long Get(GetRequest* request);
void GetAsync(GetRequest* request);

void PostFileAsync(PostFileRequest* request);
long PostFile(PostFileRequest* request);
string AppendGetParams(string baseUrl, map<string, string> getParams);