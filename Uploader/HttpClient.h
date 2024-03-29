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



struct PostFileRequest
{
	unsigned int RequestId = 0;
	void* Requester = NULL;

	std::string Url;
	std::list<std::string> Headers;

	std::string FilePath;
	std::string ParamName;

	void(*RequestComplete)(PostFileRequest*);
	long Status;
	std::string Message;
	std::string ResponseBody;
};

struct PostJsonRequest
{
	unsigned int RequestId = 0;
	void* Requester = NULL;

	std::string Url;
	std::list<std::string> Headers;

	std::string body;

	void(*RequestComplete)(PostJsonRequest*);
	long Status;
	std::string Message;
	std::string ResponseBody;
};

struct GetRequest
{
	unsigned int RequestId = 0;
	void* Requester = NULL;

	std::string Url;
	std::list<std::string> Headers;

	void(*RequestComplete)(GetRequest*);
	long Status;
};

long Get(GetRequest* request);
void GetAsync(GetRequest* request);

void PostFileAsync(PostFileRequest* request);
void PostJsonAsync(PostJsonRequest* request);
long PostFile(PostFileRequest* request);
std::string AppendGetParams(std::string baseUrl, std::map<std::string, std::string> getParams);