#include "Wininet.h"

Wininet::Wininet(void)
{
	m_hConnectedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hRequestOpenedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hRequestCompleteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	m_hInstance = NULL;
	m_hConnect = NULL;
	m_hRequest = NULL;
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
	SContext* pContext = (SContext*)dwContext;

	switch (pContext->dwContext)
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
		switch (dwInternetStatus)
		{
		case INTERNET_STATUS_HANDLE_CREATED:
		{
			INTERNET_ASYNC_RESULT *pRes = (INTERNET_ASYNC_RESULT *)lpStatusInfo;
			pContext->pObj->m_hRequest = (HINTERNET)pRes->dwResult;
			SetEvent(pContext->pObj->m_hRequestOpenedEvent);
		}
		break;

		case INTERNET_STATUS_REQUEST_SENT:
		{
			DWORD *lpBytesSent = (DWORD*)lpStatusInfo;
		}
		break;

		case INTERNET_STATUS_REQUEST_COMPLETE:
		{
			INTERNET_ASYNC_RESULT *pAsyncRes = (INTERNET_ASYNC_RESULT *)lpStatusInfo;
			SetEvent(pContext->pObj->m_hRequestCompleteEvent);
		}
		break;

		case INTERNET_STATUS_REDIRECT:
			//string strRealAddr = (LPSTR) lpStatusInfo;
			break;

		case INTERNET_STATUS_RECEIVING_RESPONSE:
			break;

		case INTERNET_STATUS_RESPONSE_RECEIVED:
		{
			DWORD *dwBytesReceived = (DWORD*)lpStatusInfo;
			//if (*dwBytesReceived == 0)
			//    bAllDone = TRUE;

		}
		}
	}
	}
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
		return FALSE;
	}


	if (InternetSetStatusCallback(m_hInstance,
		(INTERNET_STATUS_CALLBACK)&Callback)
		== INTERNET_INVALID_STATUS_CALLBACK)
	{
		return FALSE;
	}


	m_context.dwContext = CONTEXT_CONNECT;
	m_context.pObj = this;

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

		if (WaitForSingleObject(m_hConnectedEvent, dwTimeOut) == WAIT_TIMEOUT)
			return FALSE;
	}

	if (m_hConnect == NULL)
		return FALSE;


	return TRUE;
}

BOOL Wininet::Request(string method, string page, string headers, char* data, size_t data_size, DWORD flags, DWORD dwTimeOut)
{

	LPCTSTR szAcceptType = _T("*/*");


	m_context.dwContext = CONTEXT_REQUESTHANDLE;
	m_context.pObj = this;

	m_hRequest = HttpOpenRequest(m_hConnect,
		method.c_str(),
		page.c_str(),
		NULL,
		NULL,
		NULL,
		INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION
		| INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_FORMS_SUBMIT
		| INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS
		| INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP | flags,
		(DWORD)&m_context);


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

	string contentLength = "Content-Length" + to_string(data_size);
	HttpAddRequestHeaders(m_hRequest, contentLength.c_str(), contentLength.length(), HTTP_ADDREQ_FLAG_ADD);

	if (!HttpSendRequest(m_hRequest,
		NULL,
		0,
		data,
		data_size))
	{
		if (GetLastError() != ERROR_IO_PENDING)
			return FALSE;

	}


	if (WaitForSingleObject(m_hRequestCompleteEvent, dwTimeOut) == WAIT_TIMEOUT)
	{
		Close();
		return FALSE;
	}


	return TRUE;
}

DWORD Wininet::Read(PBYTE pBuffer, DWORD dwSize, DWORD dwTimeOut)
{
	INTERNET_BUFFERS InetBuff;

	FillMemory(&InetBuff, sizeof(InetBuff), 0);

	InetBuff.dwStructSize = sizeof(InetBuff);
	InetBuff.lpvBuffer = pBuffer;
	InetBuff.dwBufferLength = dwSize - 1;


	m_context.dwContext = CONTEXT_REQUESTHANDLE;
	m_context.pObj = this;

	if (!InternetReadFileEx(m_hRequest,
		&InetBuff,
		0,
		(DWORD)&m_context))
	{
		if (GetLastError() == ERROR_IO_PENDING)
		{
			if (WaitForSingleObject(m_hRequestCompleteEvent, dwTimeOut) == WAIT_TIMEOUT)
				return FALSE;
		}
		else
			return FALSE;
	}


	return InetBuff.dwBufferLength;
}

DWORD Wininet::GetStatusCode()
{
	DWORD statusCode = 0;
	DWORD length = sizeof(DWORD);
	HttpQueryInfo(
		m_hRequest,
		HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
		&statusCode,
		&length,
		NULL
	);
	return statusCode;
}


DWORD WINAPI HttpPostThread(void* data) {

	auto ctx = (HttpRequestObject*)data;

	Wininet client;
	if (client.Connect(ctx->Server.c_str(), ctx->Port, ctx->UserAgent.c_str()))  //timeout
	{
		if (client.Request(ctx->Method, ctx->Page, ctx->Headers, ctx->ReqData, ctx->ReqDataSize, ctx->Flags, ctx->Timeout))
		{
			//ctx->Status = client.GetStatusCode();

			//DWORD nLen;
			//while ((nLen = client.Read((PBYTE)ctx->RespData, ctx->RespDataSize)) > 0)
			//{
			//	ctx->RespData[nLen] = 0;
			//}

			//client.Close();

			ctx->RequestComplete(ctx);
		}
	}

	return 0;
}

bool HttpRequestAsync(HttpRequestObject* request)
{
	HANDLE thread = CreateThread(NULL, 0, HttpPostThread, (void*)request, 0, NULL);
	if (thread) {
		return true;
	}
	return false;
}
