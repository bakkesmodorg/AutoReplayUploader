#pragma once

#include <tchar.h>
#include <Windows.h>
#include <wininet.h>
#include <string>

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
