#pragma once
#pragma comment( lib, "bakkesmod.lib" )
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/wrappers/GameEvent/ServerWrapper.h"
#include "ISteamHTTP.h"
#include <string>

#define CALCULATED_ENDPOINT_DEFAULT "https://calculated.gg/api/upload"
#define BALLCHASING_ENDPOINT_DEFAULT "https://ballchasing.com/api/upload"
#define UPLOAD_BOUNDARY "----BakkesModFileUpload90m8924r390j34f0"

//S_API void SteamAPI_RegisterCallResult( class CCallbackBase *pCallback, SteamAPICall_t hAPICall );
//S_API void SteamAPI_UnregisterCallResult(class CCallbackBase *pCallback, SteamAPICall_t hAPICall);


typedef void(__cdecl* SteamAPI_RunCallbacks_typedef)();
typedef void*(__cdecl* SteamAPI_ISteamClient_GetISteamHTTP_typedef)(void* steamClient);
typedef void(__cdecl* SteamAPI_RegisterCallResult_typedef)(class CCallbackBase *pCallback, SteamAPICall_t hAPICall);
typedef void(__cdecl* SteamAPI_UnregisterCallResult_typedef)(class CCallbackBase *pCallback, SteamAPICall_t hAPICall);

SteamAPI_RunCallbacks_typedef SteamAPI_RunCallbacks_Function;
SteamAPI_RegisterCallResult_typedef SteamAPI_RegisterCallResult_Function;
SteamAPI_UnregisterCallResult_typedef SteamAPI_UnregisterCallResult_Function;

//-----------------------------------------------------------------------------
// Purpose: base for callbacks, 
//			used only by CCallback, shouldn't be used directly
//-----------------------------------------------------------------------------
class CCallbackBase
{
public:
	CCallbackBase() { m_nCallbackFlags = 0; m_iCallback = 0; }
	// don't add a virtual destructor because we export this binary interface across dll's
	virtual void Run(void *pvParam) = 0;
	virtual void Run(void *pvParam, bool bIOFailure, SteamAPICall_t hSteamAPICall) = 0;
	int GetICallback() { return m_iCallback; }
	virtual int GetCallbackSizeBytes() = 0;

protected:
	enum { k_ECallbackFlagsRegistered = 0x01, k_ECallbackFlagsGameServer = 0x02 };
	uint8 m_nCallbackFlags;
	int m_iCallback;
	friend class CCallbackMgr;
};


//-----------------------------------------------------------------------------
// Purpose: maps a steam async call result to a class member function
//			template params: T = local class, P = parameter struct
//-----------------------------------------------------------------------------
template< class T, class P >
class CCallResult : private CCallbackBase
{
public:
	typedef void (T::*func_t)(P*, bool);

	CCallResult()
	{
		m_hAPICall = k_uAPICallInvalid;
		m_pObj = NULL;
		m_Func = NULL;
		m_iCallback = P::k_iCallback;
	}

	void Set(SteamAPICall_t hAPICall, T *p, func_t func)
	{
		if (m_hAPICall)
			SteamAPI_UnregisterCallResult_Function(this, m_hAPICall);

		m_hAPICall = hAPICall;
		m_pObj = p;
		m_Func = func;

		if (hAPICall)
			SteamAPI_RegisterCallResult_Function(this, hAPICall);
	}

	bool IsActive() const
	{
		return (m_hAPICall != k_uAPICallInvalid);
	}

	void Cancel()
	{
		if (m_hAPICall != k_uAPICallInvalid)
		{
			SteamAPI_UnregisterCallResult_Function(this, m_hAPICall);
			m_hAPICall = k_uAPICallInvalid;
		}

	}

	~CCallResult()
	{
		Cancel();
	}

	void SetGameserverFlag() { m_nCallbackFlags |= k_ECallbackFlagsGameServer; }
private:
	virtual void Run(void *pvParam)
	{
		m_hAPICall = k_uAPICallInvalid; // caller unregisters for us
		(m_pObj->*m_Func)((P *)pvParam, false);
	}
	void Run(void *pvParam, bool bIOFailure, SteamAPICall_t hSteamAPICall)
	{
		if (hSteamAPICall == m_hAPICall)
		{
			m_hAPICall = k_uAPICallInvalid; // caller unregisters for us
			(m_pObj->*m_Func)((P *)pvParam, bIOFailure);
		}
	}
	int GetCallbackSizeBytes()
	{
		return sizeof(P);
	}

	SteamAPICall_t m_hAPICall;
	T *m_pObj;
	func_t m_Func;
};


struct HTTPRequestData
{
public:
	BakkesMod::Plugin::BakkesModPlugin* requester = NULL;
	HTTPRequestHandle requestHandle = NULL;
	SteamAPICall_t apiCall = NULL;
	bool canBeDeleted = false;
	bool successful = false;
	EHTTPStatusCode statusCode = k_EHTTPStatusCodeInvalid;
	virtual void OnRequestComplete(HTTPRequestCompleted_t* pCallback, bool failure)
	{
		successful = pCallback->m_bRequestSuccessful;
		statusCode = pCallback->m_eStatusCode;
		canBeDeleted = true;
	}

	CCallResult< HTTPRequestData, HTTPRequestCompleted_t > requestCompleteCallback;
};

struct FileUploadData : public HTTPRequestData
{
	CCallResult< FileUploadData, HTTPRequestCompleted_t > requestCompleteCallback;
};

struct ReplayFileUploadData : public FileUploadData
{
public:
	std::string endpoint;
	
	//
	virtual void OnRequestComplete(HTTPRequestCompleted_t* pCallback, bool failure);

};

struct AuthKeyCheckUploadData : public HTTPRequestData
{
public:
	std::shared_ptr<CVarManagerWrapper> cvarManager;
	AuthKeyCheckUploadData(std::shared_ptr<CVarManagerWrapper> cvm)
	{
		cvarManager = cvm;
	}

	void OnRequestComplete(HTTPRequestCompleted_t* pCallback, bool failure);
	

	CCallResult< AuthKeyCheckUploadData, HTTPRequestCompleted_t > requestCompleteCallback;
};

class AutoReplayUploaderPlugin : public BakkesMod::Plugin::BakkesModPlugin
{
private:
	std::string userAgent;
	ISteamHTTP* steamHTTPInstance = NULL;
	std::shared_ptr<bool> uploadToCalculated = std::make_shared<bool>(false);
	std::shared_ptr<bool> uploadToBallchasing = std::make_shared<bool>(false);
	std::shared_ptr<int> templateSequence = std::make_shared<int>(0);

	std::vector<HTTPRequestData*> fileUploadsInProgress;
	std::vector<uint8> postData;
	std::string steamUserName;
	bool fileUploadThreadActive = false;
	
public:
	std::shared_ptr<bool> showNotifications = std::make_shared<bool>(true);
	AutoReplayUploaderPlugin();
	virtual void onLoad();
	virtual void onUnload();
	void OnGameComplete(ServerWrapper caller, void* params, std::string eventName);
	void UploadReplayToEndpoint(std::string filename, std::string endpointUrl, std::string postName, std::string authKey, std::string endpointBaseUrl);
	std::vector<uint8> LoadReplay(std::string filename);
	void CheckFileUploadProgress(GameWrapper* gw);
	void TestBallchasingAuth(std::vector<std::string> params);
	void SetReplayName(ServerWrapper& server, ReplaySoccarWrapper& soccarReplay, std::string templateString);
};