#pragma once

#include <tchar.h>
#include <iostream>
#include <windows.h>
#include <wininet.h>
#include "Logger.h"

#pragma comment( lib, "wininet" )

using namespace std;

class Wininet;

struct SContext
{
	Wininet* pObj;
	DWORD dwContext;
};


class Wininet
{

protected:

	SContext m_context;

	HANDLE m_hConnectedEvent;
	HANDLE m_hRequestOpenedEvent;
	HANDLE m_hRequestCompleteEvent;

	HINTERNET m_hInstance;
	HINTERNET m_hConnect;
	HINTERNET m_hRequest;

	Logger* logger;
	void* requester = NULL;
	char* result = NULL;
	size_t result_length = 0;
	void(*RequestCompleteEvent) (void*, DWORD, char*, size_t) = NULL;


public:

	Wininet(Logger*);
	~Wininet(void);


public:

	enum
	{
		CONTEXT_CONNECT,
		CONTEXT_REQUESTHANDLE
	};

	BOOL Connect(LPCTSTR lpszAddr,
			USHORT uPort = INTERNET_DEFAULT_HTTP_PORT,
			LPCTSTR lpszAgent = _T("Wininet"),
			DWORD dwTimeOut = 30000);

	BOOL Post(string page, string headers, string data, char* result, size_t result_length, void(*RequestCompleteEvent) (void*, DWORD, char*, size_t), void* requester, DWORD dwTimeOut = 30000, DWORD Flags = 0);
	DWORD Read(PBYTE pBuffer, DWORD dwSize, DWORD dwTimeOut = 30000);
	void Close();


	static void WINAPI Callback(HINTERNET hInternet,
				  DWORD dwContext,
				  DWORD dwInternetStatus,
				  LPVOID lpStatusInfo,
				  DWORD dwStatusInfoLen);

};
