#pragma once

#include <tchar.h>
#include <Windows.h>
#include <wininet.h>
#include <string>
#include <map>

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


using namespace std;

#pragma comment( lib, "wininet" )



class HttpClient;

struct HttpRequestObject {
	unsigned int RequestId = 0;
	void* Requester = NULL;

	string Server;

	string Method = "GET";
	int Port = 80;
	string Page = "/";

	string UserAgent = "AsyncClient";
	string Headers = "";
	DWORD Flags = 0;
	DWORD Timeout = 30000;

	DWORD Status = 0;

	char* ReqData = NULL;
	size_t ReqDataSize = 0;

	char* RespData = NULL;
	size_t RespDataSize = 0;

	void(*RequestComplete)(HttpRequestObject*);
};

void HttpRequestAsync(HttpRequestObject* object);
void HttpFileUploadAsync(string server, string path, string userAgent, string filepath, string paramName, string additionalHeaders, string uploadBoundary, int requestId, void* requester, void(*RequestComplete)(HttpRequestObject*));
string AppendGetParams(string baseUrl, map<string, string> getParams);

struct SContext
{
	HttpClient* pObj;
	DWORD dwContext;
};


class HttpClient
{

protected:

	SContext m_context;

	HANDLE m_hConnectedEvent;
	HANDLE m_hRequestOpenedEvent;
	HANDLE m_hRequestCompleteEvent;

	HINTERNET m_hInstance;
	HINTERNET m_hConnect;
	HINTERNET m_hRequest;


public:

	HttpClient(void);
	~HttpClient(void);


public:

	enum
	{
		CONTEXT_CONNECT,
		CONTEXT_REQUESTHANDLE
	};

	BOOL Connect(LPCTSTR lpszAddr,
		USHORT uPort = INTERNET_DEFAULT_HTTP_PORT,
		LPCTSTR lpszAgent = _T("AsyncClient"),
		DWORD dwTimeOut = 30000);

	BOOL Request(string method, string page, string headers = "", char* data = NULL, size_t data_size = 0, DWORD flags = 0, DWORD dwTimeOut = 30000);
	DWORD Read(PBYTE pBuffer, DWORD dwSize, DWORD dwTimeOut = 30000);
	DWORD GetStatusCode();

	void Close();


	static void WINAPI Callback(HINTERNET hInternet,
		DWORD dwContext,
		DWORD dwInternetStatus,
		LPVOID lpStatusInfo,
		DWORD dwStatusInfoLen);

};
