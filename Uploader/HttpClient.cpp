#include "HttpClient.h"

#include <vector>

long Get(GetRequest* ctx)
{
	try
	{
		curlpp::Cleanup cleaner;
		curlpp::Easy request;

		request.setOpt(new curlpp::options::Url(ctx->Url));

		ctx->Headers.push_back("Expect: "); // disable expect header
		request.setOpt(new curlpp::options::HttpHeader(ctx->Headers));

		request.perform();

		return curlpp::infos::ResponseCode::get(request);
	}
	catch (curlpp::LogicError & e)
	{
		std::cout << e.what() << std::endl;
	}
	catch (curlpp::RuntimeError & e) {
		std::cout << e.what() << std::endl;
	}
	catch (...)
	{

	}
	return 0;
}

long PostFile(PostFileRequest* ctx)
{
	try
	{
		curlpp::Cleanup cleaner;
		curlpp::Easy request;

		request.setOpt(new curlpp::options::Url(ctx->Url));

		ctx->Headers.push_back("Expect: "); // disable expect header
		request.setOpt(new curlpp::options::HttpHeader(ctx->Headers));

		{
			curlpp::Forms formParts;
			formParts.push_back(new curlpp::FormParts::File(ctx->ParamName, ctx->FilePath));
			request.setOpt(new curlpp::options::HttpPost(formParts));
		}

		request.perform();

		return curlpp::infos::ResponseCode::get(request);
	}
	catch (curlpp::LogicError & e)
	{
		ctx->message = e.what();
		std::cout << e.what() << std::endl;
	}
	catch (curlpp::RuntimeError & e) {
		ctx->message = e.what();
		std::cout << e.what() << std::endl;
	}
	catch (...)
	{
		ctx->message = "Unknown exception occurred";
	}
	return 0;
}

void GetAsyncThread(void* data)
{
	auto ctx = (GetRequest*)data;
	ctx->Status = Get(ctx);
	ctx->RequestComplete(ctx);
}

void GetAsync(GetRequest* request)
{
	std::thread http(GetAsyncThread, (void*)request);
	http.detach();
}

void PostFileThread(void* data)
{
	auto ctx = (PostFileRequest*)data;
	ctx->Status = PostFile(ctx);
	ctx->RequestComplete(ctx);
}

void PostFileAsync(PostFileRequest* request)
{
	std::thread http(PostFileThread, (void*)request);
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
	for (size_t i = 0; i < vector.size(); i++)
		reqData[i] = vector[i];
	reqData[vector.size()] = '\0';
	return reqData;
}