#include "Wininet.h"

#include <iostream>
#include <string>

using namespace std;

Wininet::Wininet(Logger* logger)
{
	m_hConnectedEvent		= CreateEvent(NULL, FALSE, FALSE, NULL);
    m_hRequestOpenedEvent	= CreateEvent(NULL, FALSE, FALSE, NULL);
    m_hRequestCompleteEvent	= CreateEvent(NULL, FALSE, FALSE, NULL);

	m_hInstance	= NULL;
	m_hConnect	= NULL;
	m_hRequest	= NULL;

	this->logger = logger;
}

Wininet::~Wininet(void)
{
	if (m_hConnectedEvent)
		CloseHandle(m_hConnectedEvent);
	if (m_hRequestOpenedEvent)
		CloseHandle(m_hRequestOpenedEvent);
	if (m_hRequestCompleteEvent)
		CloseHandle(m_hRequestCompleteEvent);

	Close();
}

void WINAPI Wininet::Callback(HINTERNET hInternet,
				  DWORD dwContext,
				  DWORD dwInternetStatus,
				  LPVOID lpStatusInfo,
				  DWORD dwStatusInfoLen)
{
	SContext* pContext = (SContext*) dwContext;
	pContext->pObj->logger->Log("Wininet::Callback dwContext:" + to_string(pContext->dwContext) + " dwInternetStatus:" + to_string(dwInternetStatus));

	switch(pContext->dwContext)
	{
	case CONTEXT_CONNECT:
		if (dwInternetStatus == INTERNET_STATUS_HANDLE_CREATED)
		{
			INTERNET_ASYNC_RESULT *pRes = (INTERNET_ASYNC_RESULT *)lpStatusInfo;
			pContext->pObj->m_hConnect = (HINTERNET)pRes->dwResult;
			SetEvent(pContext->pObj->m_hConnectedEvent);
		}
		break;

	case CONTEXT_REQUESTHANDLE: // Request handle
		{
			switch(dwInternetStatus)
			{
			case INTERNET_STATUS_HANDLE_CREATED:
				{
					pContext->pObj->logger->Log("INTERNET_STATUS_HANDLE_CREATED");
					INTERNET_ASYNC_RESULT *pRes = (INTERNET_ASYNC_RESULT *)lpStatusInfo;
					pContext->pObj->m_hRequest = (HINTERNET)pRes->dwResult;
					SetEvent(pContext->pObj->m_hRequestOpenedEvent);
				}
				break;

			case INTERNET_STATUS_REQUEST_SENT:
				{
					pContext->pObj->logger->Log("INTERNET_STATUS_REQUEST_SENT");
					DWORD *lpBytesSent = (DWORD*)lpStatusInfo;
				}
				break;

			case INTERNET_STATUS_REQUEST_COMPLETE:
				{
					pContext->pObj->logger->Log("INTERNET_STATUS_REQUEST_COMPLETE");
					INTERNET_ASYNC_RESULT *pAsyncRes = (INTERNET_ASYNC_RESULT *)lpStatusInfo;
					SetEvent(pContext->pObj->m_hRequestCompleteEvent);

					DWORD statusCode = 0;
					DWORD length = sizeof(DWORD);
					HttpQueryInfo(
						pContext->pObj->m_hRequest,
						HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
						&statusCode,
						&length,
						NULL
					);

					for (;;)
					{
						// reading data
						DWORD dwByteRead;
						BOOL isRead = InternetReadFile(pContext->pObj->m_hRequest, pContext->pObj->result, pContext->pObj->result_length - 1, &dwByteRead);

						// break cycle if error or end
						if (isRead == FALSE || dwByteRead == 0)
							break;

						// saving result
						pContext->pObj->result[dwByteRead] = 0;
					}

					pContext->pObj->RequestCompleteEvent(pContext->pObj->requester, statusCode, pContext->pObj->result, pContext->pObj->result_length);
				}
				break;

			case INTERNET_STATUS_REDIRECT:
				{
					pContext->pObj->logger->Log("INTERNET_STATUS_REDIRECT");
					//string strRealAddr = (LPSTR) lpStatusInfo;
				}
				break;

			case INTERNET_STATUS_RECEIVING_RESPONSE:
				{
					pContext->pObj->logger->Log("INTERNET_STATUS_RECEIVING_RESPONSE");
				}
				break;

			case INTERNET_STATUS_RESPONSE_RECEIVED:
				{
					pContext->pObj->logger->Log("INTERNET_STATUS_RESPONSE_RECEIVED");
					DWORD *dwBytesReceived = (DWORD*)lpStatusInfo;
					//if (*dwBytesReceived == 0)
					//    bAllDone = TRUE;
				}
				break;
			default:
				{
					pContext->pObj->logger->Log("Unhandled dwInternetStatus");
				}
				break;
			}
		}
		default:
			{
				pContext->pObj->logger->Log("Unhandled dwContext");
			}
			break;
	}
	
	pContext->pObj->logger->Log("Wininet::Callback end");
}

void Wininet::Close()
{
	if (m_hInstance)
	{
		InternetCloseHandle(m_hInstance);
		m_hInstance = NULL;
	}

	if (m_hConnect)
	{
		InternetCloseHandle(m_hConnect);
		m_hConnect = NULL;
	}

	if (m_hRequest)
	{
		InternetCloseHandle(m_hRequest);
		m_hRequest = NULL;
	}
}


BOOL Wininet::Connect(LPCTSTR lpszAddr, USHORT uPort, LPCTSTR lpszAgent, DWORD dwTimeOut)
{
	logger->Log("Wininet::Connect.start " + string(lpszAddr));
	Close();

	ResetEvent(m_hConnectedEvent);
	ResetEvent(m_hRequestOpenedEvent);
	ResetEvent(m_hRequestCompleteEvent);


	if (!(m_hInstance = InternetOpen(lpszAgent, 
					INTERNET_OPEN_TYPE_PRECONFIG,
					NULL,
					NULL,
					INTERNET_FLAG_ASYNC)))
	{
		logger->Log("InternetOpen failed");
        return FALSE;
	}


	if (InternetSetStatusCallback(m_hInstance,
				(INTERNET_STATUS_CALLBACK)&Callback)
				== INTERNET_INVALID_STATUS_CALLBACK)
	{
		logger->Log("InternetSetStatusCallback failed");
		return FALSE;
	}


	m_context.dwContext	= CONTEXT_CONNECT;
	m_context.pObj		= this;

	m_hConnect = InternetConnect(m_hInstance, 
					lpszAddr,
					uPort,
					NULL,
					NULL,
					INTERNET_SERVICE_HTTP,
					INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_NO_CACHE_WRITE,
					(DWORD)&m_context);


	if (m_hConnect == NULL)
	{
		if (GetLastError() != ERROR_IO_PENDING)
			return FALSE;

		if(WaitForSingleObject(m_hConnectedEvent, dwTimeOut) == WAIT_TIMEOUT)
			return FALSE;
	}

	if(m_hConnect == NULL)
		return FALSE;


	return TRUE;
}

BOOL Wininet::Post(string page, string headers, string data, char* result, size_t result_length, void(*requestCompleteEvent) (void*, DWORD, char*, size_t), void* requester, DWORD dwTimeOut, DWORD Flags)
{
	logger->Log("Wininet::Post.start " + page);

	this->requester = requester;
	this->result_length = result_length;
	this->result = result;
	this->RequestCompleteEvent = requestCompleteEvent;

	m_context.dwContext	= CONTEXT_REQUESTHANDLE;
	m_context.pObj		= this;

	m_hRequest = HttpOpenRequest(m_hConnect, 
					"POST",
					page.c_str(),
					NULL,
					NULL,
					0,
					INTERNET_FLAG_KEEP_CONNECTION | Flags,
					(DWORD)INTERNET_NO_CALLBACK);

	if (m_hRequest == NULL)
	{
		if (GetLastError() != ERROR_IO_PENDING)
			return FALSE;

		if (WaitForSingleObject(m_hRequestOpenedEvent, dwTimeOut) == WAIT_TIMEOUT)
			return FALSE;
	}

	if (m_hRequest == NULL)
		return FALSE;

	HttpAddRequestHeaders(m_hRequest, headers.c_str(), headers.length(), HTTP_ADDREQ_FLAG_ADD);

	string contentLength = "Content-Length" + to_string(data.size());
	HttpAddRequestHeaders(m_hRequest, contentLength.c_str(), contentLength.length(), HTTP_ADDREQ_FLAG_ADD);

	char *postData = new char[data.size() + 1];
	data.copy(postData, data.size() + 1);
	postData[data.size()] = '\0';

	if (!HttpSendRequest(m_hRequest, 
					 NULL, 
					 0, 
					 postData,
					 data.size()))
	{
		if (GetLastError() != ERROR_IO_PENDING)
			return FALSE;

	}

	if (this->RequestCompleteEvent == NULL && WaitForSingleObject(m_hRequestCompleteEvent, dwTimeOut) == WAIT_TIMEOUT)
	{
		Close();
		return FALSE;
	}


	return TRUE;
}

DWORD Wininet::Read(PBYTE pBuffer, DWORD dwSize, DWORD dwTimeOut)
{
	logger->Log("Wininet::Read.start");

	INTERNET_BUFFERS InetBuff;

	FillMemory(&InetBuff, sizeof(InetBuff), 0);

	InetBuff.dwStructSize	= sizeof(InetBuff);
	InetBuff.lpvBuffer		= pBuffer;
	InetBuff.dwBufferLength	= dwSize - 1;


	m_context.dwContext	= CONTEXT_REQUESTHANDLE;
	m_context.pObj			= this;

	if (!InternetReadFileEx(m_hRequest,
			  &InetBuff,
			  0,
			  (DWORD)&m_context))
	{
		if (GetLastError() == ERROR_IO_PENDING)
		{
			if(WaitForSingleObject(m_hRequestCompleteEvent, dwTimeOut) == WAIT_TIMEOUT)
				return FALSE;
		}
		else
			return FALSE;
	}


	return InetBuff.dwBufferLength;
}