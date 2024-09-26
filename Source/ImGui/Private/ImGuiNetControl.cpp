// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiNetControl.h"
#include "ImGuiModuleManager.h"
#include "ImGuiContextManager.h"
#include "ImGuiContextProxy.h"
#include "imgui.h"
#include "NetImgui_Config.h"
#include "NetImgui_Api.h"
#include "NetImgui_Network.h"
#include "NetImguiServer_App.h"
#include "NetImguiServer_RemoteClient.h"

#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Misc/ScopeRWLock.h"
#include "Internationalization/Regex.h"
#include "HAL/PlatformApplicationMisc.h"

static TAnsiStringBuilder<256> gUserSettingFolderPath;

///////////////////////////////////////////////////////////////////////////////////////////////////
// FImguiServerState

namespace NetImguiServer::Network
{
	bool Communications_InitializeClient(NetImgui::Internal::Network::SocketInfo* pClientSocket, NetImguiServer::RemoteClient::Client* pClient);
	bool Communications_Incoming(NetImgui::Internal::Network::SocketInfo* pClientSocket, NetImguiServer::RemoteClient::Client* pClient);
	bool Communications_Outgoing(NetImgui::Internal::Network::SocketInfo* pClientSocket, NetImguiServer::RemoteClient::Client* pClient);
	void Communications_UpdateClientStats(RemoteClient::Client& Client);
}

enum class EImguiConnectionState
{
	None,
	Pending,
	Disconnecting,
	Connected,
};

class FImguiClientConnectionRunnable : public FRunnable
{
public:
	virtual uint32 Run() override;
	virtual void Stop() override;

	FImguiServerState* State;
	TArray<char> Hostname;
	int32 Port = 0;
	bool bExitRequested = false;
};

struct FImguiServerState
{
	bool IsConnected();
	bool IsConnectionPending();
	void Connect(const char* Hostname, int32 Port);
	void Disconnect();
	void CaptureInput();
	TUniquePtr<ImDrawData> GetDrawData();

	// worker thread data
	NetImgui::Internal::Network::SocketInfo* ClientSocket = nullptr;
	FImguiClientConnectionRunnable ClientRunnable;
	FRunnableThread* ClientConnectionThread = nullptr;

	// Shared data
	FRWLock NetClientLock;
	TUniquePtr<NetImguiServer::RemoteClient::Client> NetClient;
	std::atomic<EImguiConnectionState> ConnectionState = EImguiConnectionState::None;

	// main thread data
	TArray<ImDrawList> TempDrawLists;
	FString Clipboard;
	TArray<UTF8CHAR> Utf8Clipboard;
};

uint32 FImguiClientConnectionRunnable::Run()
{
	State->ClientSocket = NetImgui::Internal::Network::Connect(Hostname.GetData(), Port);

	// Failed to connect
	if (State->ClientSocket)
	{
		{
			auto Lock = FWriteScopeLock(State->NetClientLock);

			State->NetClient = MakeUnique<NetImguiServer::RemoteClient::Client>();
			State->NetClient->mClientConfigID = 1;
			State->NetClient->mClientIndex = 0;
			NetImguiServer::App::HAL_GetSocketInfo(State->ClientSocket, State->NetClient->mConnectHost, sizeof(State->NetClient->mConnectHost), State->NetClient->mConnectPort);
			State->NetClient->mbIsConnected = true;
			State->NetClient->mbCompressionSkipOncePending = true; // Always get at least one uncompressed frame up-front
		}

		// Init/loop logic adapted from NetImguiServer::Network::Communications_ClientExchangeLoop()
		{
			bool bConnected = NetImguiServer::Network::Communications_InitializeClient(State->ClientSocket, State->NetClient.Get());

			auto ExpectedState = EImguiConnectionState::Pending;
			if (State->ConnectionState.compare_exchange_strong(ExpectedState, EImguiConnectionState::Connected) == false)
			{
				bConnected = false;
			}

			while (bConnected && !bExitRequested)
			{
				if (bConnected && State->NetClient->mbIsConnected)
				{
					if (State->ConnectionState == EImguiConnectionState::Disconnecting)
					{
						State->NetClient->mbDisconnectPending = true;
					}

					bConnected =	NetImguiServer::Network::Communications_Outgoing(State->ClientSocket, State->NetClient.Get()) &&
									NetImguiServer::Network::Communications_Incoming(State->ClientSocket, State->NetClient.Get());

					NetImguiServer::Network::Communications_UpdateClientStats(*State->NetClient);

					FPlatformProcess::Sleep(1.0f/30.0f);
				}
				else
				{
					bConnected = false;
				}
			}
		}
	}

	// Disconnect was requested at some point
	NetImgui::Internal::Network::Disconnect(State->ClientSocket);
	State->ClientSocket = nullptr;
	State->ConnectionState = EImguiConnectionState::None;

	auto Lock = FWriteScopeLock(State->NetClientLock);
	State->NetClient = nullptr;

	return 0;
}

void FImguiClientConnectionRunnable::Stop()
{
	bExitRequested = true;
}

bool FImguiServerState::IsConnected()
{
	return ConnectionState == EImguiConnectionState::Connected;
}

bool FImguiServerState::IsConnectionPending()
{
	return ConnectionState == EImguiConnectionState::Pending;
}

void FImguiServerState::Connect(const char* Hostname, int32 Port)
{
	if (ConnectionState == EImguiConnectionState::None)
	{
		check(ClientSocket == nullptr);
		check(NetClient == nullptr);

		auto ExpectedState = EImguiConnectionState::None;
		if (ConnectionState.compare_exchange_strong(ExpectedState, EImguiConnectionState::Pending))
		{
			if (ClientConnectionThread)
			{
				delete ClientConnectionThread;
			}

			ClientRunnable.State = this;
			ClientRunnable.Hostname.SetNum(FCStringAnsi::Strlen(Hostname) + 1, EAllowShrinking::Yes);
			FCStringAnsi::Strcpy(ClientRunnable.Hostname.GetData(), ClientRunnable.Hostname.Num(), Hostname);
			ClientRunnable.Port = Port;
			ClientRunnable.bExitRequested = false;
			ClientConnectionThread = FRunnableThread::Create(&ClientRunnable, TEXT("NetimguiServerThread"));
		}
	}
}

void FImguiServerState::Disconnect()
{
	if (ConnectionState == EImguiConnectionState::Pending || ConnectionState == EImguiConnectionState::Connected)
	{
		ConnectionState = EImguiConnectionState::Disconnecting;

		check(ClientConnectionThread);
	}
}

void FImguiServerState::CaptureInput()
{
	if (IsConnected())
	{
		auto Lock = FWriteScopeLock(NetClientLock);

		// Give the local imgui priority to set the mouse cursor. If no windows are hovered then we'll fall
		// back to the current net cursor state.
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AnyWindow) == false)
		{
			ImGui::SetMouseCursor(NetClient->mMouseCursor);
		}

		NetClient->CaptureImguiInput();

		// Update clipboard
		FString NewClipboard;
		FPlatformApplicationMisc::ClipboardPaste(NewClipboard);

		if (Clipboard != NewClipboard)
		{
			Clipboard = MoveTemp(NewClipboard);
			Utf8Clipboard = StringToArray<UTF8CHAR>(TCHAR_TO_UTF8(*Clipboard));

			NetImgui::Internal::CmdClipboard* pClipboard = NetImgui::Internal::CmdClipboard::Create(reinterpret_cast<char*>(Utf8Clipboard.GetData()));
			NetClient->mPendingClipboardOut.Assign(pClipboard);
		}

		NetImgui::Internal::CmdClipboard* pClientIncomingClipboard = NetClient->mPendingClipboardIn.Release();
		if (pClientIncomingClipboard) {
			Clipboard = FString(UTF8_TO_TCHAR(pClientIncomingClipboard->mContentUTF8.Get()));
			FPlatformApplicationMisc::ClipboardCopy(*Clipboard);

			NetImgui::Internal::netImguiDeleteSafe(pClientIncomingClipboard);
		}
	}
}

TUniquePtr<ImDrawData> FImguiServerState::GetDrawData()
{
	if (IsConnected())
	{
		auto Lock = FWriteScopeLock(NetClientLock);
		NetClient->ProcessPendingTextures();

		// deep copy the data since the internal arrays get transferred to the context proxy and we may
		// need to reuse the same data if we don't get another net update before the next frame
		TUniquePtr<ImDrawData> DrawData = MakeUnique<ImDrawData>();
		ImDrawData* ServerData = NetClient->GetImguiDrawData(nullptr);
		if (ServerData)
		{
			TempDrawLists.Reset();
			for (ImDrawList* DrawList : ServerData->CmdLists)
			{
				TempDrawLists.Emplace(*DrawList);
			}

			*DrawData = *ServerData;
			DrawData->CmdLists.clear();
			for (ImDrawList& DrawList : TempDrawLists)
			{
				DrawData->CmdLists.push_back(&DrawList);
			}

			return DrawData;
		}
	}

	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// FImGuiNetControl

// Defined in the .cpp so this function can
FImGuiNetControl::~FImGuiNetControl()
{
	delete ServerState;
}

void FImGuiNetControl::Startup(FImGuiContextManager* InContextManager)
{
	ServerState = new FImguiServerState();

	if (!FParse::Value(FCommandLine::Get(), TEXT("NetImguiClientPort="), Port))
	{
		Port = NetImgui::kDefaultClientPort;
	}

	ContextManager = InContextManager;

	NetImgui::Startup();
}

void FImGuiNetControl::Shutdown()
{
	NetImgui::Shutdown();
}

bool FImGuiNetControl::OnWorldStartup(int32 InContextIndex, UWorld* World)
{
	ContextIndex = InContextIndex;

	if (World->GetNetMode() == NM_Client)
	{
		// Let the client decide when they want to connect - this happens in ServerMenu()
		if (World->WorldType == EWorldType::Game)
		{
			// Logic adapeted from UGameInstance::StartGameInstance(), where the URL is parsed
			// from the commandline
			const TCHAR* Cmd = FCommandLine::Get();
			FString Hostname;
			if (FParse::Token(Cmd, Hostname, 0) && **Hostname != '-')
			{
				int32 PortStartIndex = 0;
				if (Hostname.FindLastChar(':', PortStartIndex))
				{
					Hostname.LeftInline(PortStartIndex);
				}
				GameServerHostname = MoveTemp(Hostname);
			}
		}
		else
		{
			GameServerHostname = TEXT("localhost");
		}
	}
	else
	{
		// Only forward the game server world to the netimgui server
		if((NetImgui::IsConnectionPending() == false) && (NetImgui::IsConnected() == false))
		{
			FString SessionName = FString::Format(TEXT("{0}-{1}"), { FApp::GetProjectName(), FPlatformProcess::ComputerName() });
			return NetImgui::ConnectFromApp(TCHAR_TO_ANSI(*SessionName), Port, nullptr, nullptr);
		}
	}

	return false;
}

bool FImGuiNetControl::IsConnected()
{
	return NetImgui::IsConnected();
}

bool FImGuiNetControl::IsConnected(int32 InContextIndex)
{
	if (ContextIndex && *ContextIndex == InContextIndex)
	{
		return IsConnected();
	}

	return false;
}

void FImGuiNetControl::Disconnect(int32 InContextIndex)
{
	if (ContextIndex && *ContextIndex == InContextIndex)
	{
		ContextIndex.Reset();
		if (NetImgui::IsConnected() || NetImgui::IsConnectionPending())
		{
			NetImgui::Disconnect();
		}
	}

	GameServerHostname.Reset();
}

bool FImGuiNetControl::ServerMenu(UWorld* World)
{
	// If this is a client PIE instance, there may be another server PIE instance in-process.
	// In that case, we don't want to display the imgui server menu.
	bool bIsNetImguiServer = World->GetNetMode() == NM_Client;
	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		UWorld* EngineWorld = WorldContext.World();
		if (World != EngineWorld && EngineWorld->IsGameWorld() && (EngineWorld->GetNetMode() < NM_Client))
		{
			bIsNetImguiServer = false;
			break;
		}
	}

	if (bIsNetImguiServer)
	{
		if (GameServerHostname.IsSet())
		{
			ImGui::Text("Game Server: %s:%d", TCHAR_TO_ANSI(**GameServerHostname), Port);
			EImguiConnectionState ServerConnection = ServerState->ConnectionState;
			switch (ServerConnection)
			{
				case EImguiConnectionState::None:
				{
					ContextIndex.Reset();

					if (ImGui::Button("Connect"))
					{
						ServerState->Connect(TCHAR_TO_ANSI(**GameServerHostname), Port);

						int32 FoundContextIndex;
						ContextManager->GetWorldContextProxy(*World, FoundContextIndex);
						ContextIndex = FoundContextIndex;
					}
					break;
				}

				case EImguiConnectionState::Pending:
				{
					ImGui::Text("Connecting...");
					break;
				}

				case EImguiConnectionState::Disconnecting:
				{
					ImGui::Text("Disconnecting...");
					break;
				}

				case EImguiConnectionState::Connected:
				{
					if (ImGui::Button("Disconnect"))
					{
						ServerState->Disconnect();
					}
					break;
				}
			}
		}
		else
		{
			ImGui::Text("Failed to find Game Server hostname");
		}
	}

	return bIsNetImguiServer;
}

void FImGuiNetControl::ServerCaptureInput(int32 InContextIndex)
{
	if (ServerState->IsConnected() && ContextIndex && *ContextIndex == InContextIndex)
	{
		ServerState->CaptureInput();
	}
}

TUniquePtr<ImDrawData> FImGuiNetControl::GetServerDrawData(int32 InContextIndex)
{
	if (ServerState->IsConnected() && ContextIndex && *ContextIndex == InContextIndex)
	{
		return ServerState->GetDrawData();
	}
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// NetImgui HAL interface

bool NetImguiServer::App::HAL_Startup(const char* CmdLine)
{
	return true;
}

void NetImguiServer::App::HAL_Shutdown()
{
}

bool NetImguiServer::App::HAL_GetSocketInfo(NetImgui::Internal::Network::SocketInfo* pClientSocket, char* pOutHostname, size_t HostNameLen, int& outPort)
{
	FSocket* Socket = *reinterpret_cast<FSocket**>(pClientSocket);

	TSharedRef<FInternetAddr> Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	Socket->GetAddress(*Addr);

	bool bAppendPort = false;
	FString HostnameStr = Addr->ToString(bAppendPort);
	FCStringAnsi::Strcpy(pOutHostname, HostNameLen, TCHAR_TO_ANSI(*HostnameStr));

	outPort = Addr->GetPort();

	return true;
}

const char* NetImguiServer::App::HAL_GetUserSettingFolder()
{
	if (gUserSettingFolderPath.Len() == 0)
	{
		gUserSettingFolderPath.Append(FPaths::ProjectIntermediateDir());
		gUserSettingFolderPath.Append(TEXT("NetImgui/"));
	}

	return gUserSettingFolderPath.ToString();
}

bool NetImguiServer::App::HAL_CreateTexture(uint16_t Width, uint16_t Height, NetImgui::eTexFormat Format, const uint8_t* pPixelData, ServerTexture& OutTexture)
{
	TStringBuilder<64> TextureNameString;
	TextureNameString.Appendf(TEXT("NetImguiServer_%llu"), OutTexture.mImguiId);
	auto TextureName = FName(TextureNameString.ToString());

	const uint32 BytesPerPixel = 4; // convert everything to use RGBA8

	uint32* OwnedPixelData = new uint32[Width * Height];

	check(Format != NetImgui::kTexFmtCustom);
	if (Format == NetImgui::kTexFmtA8)
	{
		for (int i = 0; i < Width * Height; ++i)
		{
			OwnedPixelData[i] = 0x00FFFFFF | (static_cast<uint32_t>(pPixelData[i]) << 24);
		}
	}
	else if (Format == NetImgui::kTexFmtRGBA8)
	{
		// Ideally we could avoid making this copy, but NetImgui immediately deletes pPixelData.
		// We do this to avoid making more changes to the library.
		memcpy(OwnedPixelData, pPixelData, Width * Height * BytesPerPixel);
	}

	FTextureManager& TextureManager = FImGuiModuleManager::Get()->GetTextureManager();
	TextureIndex TextureId = TextureManager.CreateTexture(TextureName, Width, Height, BytesPerPixel, reinterpret_cast<uint8*>(OwnedPixelData), [](uint8* Pixels)
		{
			delete[] Pixels;
		});

	if (TextureId != INDEX_NONE)
	{
		OutTexture.mpHAL_Texture = reinterpret_cast<void*>(static_cast<uint64>(TextureId));
		return true;
	}

	return false;
}

void NetImguiServer::App::HAL_DestroyTexture( ServerTexture& OutTexture )
{
	TextureIndex TextureId = static_cast<uint32>(reinterpret_cast<uint64>(OutTexture.mpHAL_Texture));
	if (TextureId != INDEX_NONE)
	{
		FTextureManager& TextureManager = FImGuiModuleManager::Get()->GetTextureManager();
		TextureManager.ReleaseTextureResources(TextureId);
	}
}

// These HAL functions are needed for linking purposes but we don't call any code paths that exercise them

bool NetImguiServer::App::HAL_GetClipboardUpdated()
{
	return false;
}

void NetImguiServer::App::HAL_RenderDrawData(RemoteClient::Client& client, ImDrawData* pDrawData)
{
}

bool NetImguiServer::App::HAL_CreateRenderTarget(uint16_t Width, uint16_t Height, void*& pOutRT, void*& pOutTexture )
{
	return false;
}

void NetImguiServer::App::HAL_DestroyRenderTarget(void*& pOutRT, void*& pOutTexture )
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// NetImgui UI interface

namespace NetImguiServer::UI
{
	static NetImguiServer::App::ServerTexture gBackgroundTexture;

	float GetDisplayFPS(void) { return 1.0f / 60.0f; }
	bool Startup(void) { return true; }
	void Shutdown(void) {}
	void DrawCenteredBackground(const NetImguiServer::App::ServerTexture&, const ImVec4& ) {}
	float GetFontDPIScale() { return 1.0f; }
	const NetImguiServer::App::ServerTexture& GetBackgroundTexture() { return gBackgroundTexture; }
}
