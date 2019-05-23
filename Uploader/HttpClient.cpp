#include "HttpClient.h"

#include <thread>
#include <sstream>
#include <fstream>
#include <vector>

HttpClient::HttpClient(void)
{
	m_hConnectedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hRequestOpenedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hRequestCompleteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	m_hInstance = NULL;
	m_hConnect = NULL;
	m_hRequest = NULL;
}

HttpClient::~HttpClient(void)
{
	if (m_hConnectedEvent)
		CloseHandle(m_hConnectedEvent);
	if (m_hRequestOpenedEvent)
		CloseHandle(m_hRequestOpenedEvent);
	if (m_hRequestCompleteEvent)
		CloseHandle(m_hRequestCompleteEvent);

	Close();
}

void WINAPI HttpClient::Callback(HINTERNET hInternet,
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

void HttpClient::Close()
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


BOOL HttpClient::Connect(LPCTSTR lpszAddr, USHORT uPort, LPCTSTR lpszAgent, DWORD dwTimeOut)
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

BOOL HttpClient::Request(string method, string page, string headers, char* data, size_t data_size, DWORD flags, DWORD dwTimeOut)
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

DWORD HttpClient::Read(PBYTE pBuffer, DWORD dwSize, DWORD dwTimeOut)
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

DWORD HttpClient::GetStatusCode()
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

	HttpClient client;
	if (!client.Connect(ctx->Server.c_str(), ctx->Port, ctx->UserAgent.c_str(), ctx->Timeout))
		return 1;

	if (!client.Request(ctx->Method, ctx->Page, ctx->Headers, ctx->ReqData, ctx->ReqDataSize, ctx->Flags, ctx->Timeout))
		return 1;

	ctx->Status = client.GetStatusCode();

	if (ctx->RespData != NULL)
	{
		DWORD nLen;
		while ((nLen = client.Read((PBYTE)ctx->RespData, ctx->RespDataSize)) > 0)
		{
			ctx->RespData[nLen] = 0;
		}
	}

	client.Close();

	ctx->RequestComplete(ctx);

	return 0;
}

void HttpRequestAsync(HttpRequestObject* request)
{
	std::thread http(HttpPostThread, (void*)request);
	http.detach();
}

vector<uint8_t> GetFileBytes(string filename)
{
	// open the file
	std::streampos fileSize;
	std::ifstream file(filename, ios::binary);

	// get its size
	file.seekg(0, ios::end);
	fileSize = file.tellg();

	if (fileSize <= 0) // in case the file does not exist for some reason
	{
		fileSize = 0;
	}

	// initialize byte vector to size of replay
	vector<uint8_t> fileData(fileSize);

	// read replay file from the beginning
	file.seekg(0, ios::beg);
	file.read((char*)&fileData[0], fileSize);
	file.close();

	return fileData;
}

string GetFileName(const string& s)
{

	char sep = '/';

#ifdef _WIN32
	sep = '\\';
#endif

	size_t i = s.rfind(sep, s.length());
	if (i != string::npos)
	{
		return(s.substr(i + 1, s.length() - i));
	}

	return("");
}

string AppendGetParams(string baseUrl, map<string, string> getParams)
{
	std::stringstream urlStream;
	urlStream << baseUrl;
	if (!getParams.empty())
	{
		urlStream << "?";

		for (auto it = getParams.begin(); it != getParams.end(); it++)
		{
			if (it != getParams.begin())
				urlStream << "&";
			urlStream << (*it).first << "=" << (*it).second;
		}
	}
	return urlStream.str();
}

char* CopyToCharPtr(vector<uint8_t>& vector)
{
	char *reqData = new char[vector.size() + 1];
	for (int i = 0; i < vector.size(); i++)
		reqData[i] = vector[i];
	reqData[vector.size()] = '\0';
	return reqData;
}

void HttpFileUploadAsync(string server, string path, string userAgent, string filepath, string paramName, string additionalHeaders, string uploadBoundary, int requestId, void* requester, void(*RequestComplete)(HttpRequestObject*))
{
	string filename = GetFileName(filepath);

	// Get Replay file bytes to upload
	auto bytes = GetFileBytes(filepath);

	// Construct headers
	stringstream headers;
	headers << "Content-Type: multipart/form-data;boundary=" << uploadBoundary;
	if (!additionalHeaders.empty())
		headers << "\r\n" << additionalHeaders;
	auto header_str = headers.str();

	// Construct body
	stringstream body;
	body << "--" << uploadBoundary << "\r\n";
	body << "Content-Disposition: form-data; name=\"" << paramName << "\"; filename=\"" << filename << "\"" << "\r\n";
	body << "Content-Type: application/form-data" << "\r\n";
	body << "\r\n";
	body << string(bytes.begin(), bytes.end());
	body << "\r\n";
	body << "--" << uploadBoundary << "--" << "\r\n";

	// Convert body to vector of bytes instead of using str() which may have trouble with null termination chars
	vector<uint8_t> buffer;
	const string& str = body.str();
	buffer.insert(buffer.end(), str.begin(), str.end());

	// Copy vector to char* for upload
	char *reqData = CopyToCharPtr(buffer);

	// Setup Http Request context
	HttpRequestObject* ctx = new HttpRequestObject();
	ctx->RequestId = requestId;
	ctx->Requester = requester;
	ctx->Headers = header_str;
	ctx->Server = server;
	ctx->Page = path;
	ctx->Method = "POST";
	ctx->UserAgent = userAgent;
	ctx->Port = INTERNET_DEFAULT_HTTPS_PORT;
	ctx->ReqData = reqData;
	ctx->ReqDataSize = buffer.size();
	ctx->RespData = new char[4096];
	ctx->RespDataSize = 4096;
	ctx->RequestComplete = RequestComplete;
	ctx->Flags = INTERNET_FLAG_SECURE;

	HttpRequestAsync(ctx);
}