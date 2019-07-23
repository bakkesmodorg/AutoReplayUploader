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

void HttpFileUpload(string server, string path, string filepath, string paramName, string additionalHeaders, int requestId, void* requester, void(*RequestComplete)(HttpRequestObject*))
{
	
}

void HttpFileUpload(string& url, string& filePath, string& paramName)
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

		//curl_easy_setopt(curl, CURLOPT_PROXY, "http://localhost:8888");

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