// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#pragma once

enum class EImGuiNetDrawContext
{
	Local,
	Remote,
};

class FImGuiContextManager;
class FImGuiContextProxy;
struct ImDrawData;
struct FImguiServerState;

class FImGuiNetControl
{
public:
	// Call this function from somewhere inside an ImGui::Begin() block. It draws a
	// small menu that allows the user to control connecting to an out-of-process
	// game server, whether it is still on localhost or remote.
	// Use the -NetImguiClientPort=<Port> commandline parameter to control which
	// port Netimgui uses to connect client and server. If none is specified, it
	// will use the default port (8889).
	bool IMGUI_API ServerMenu(UWorld* World);

	// Determines if there is an active Netimgui connection.
	bool IMGUI_API IsConnected();

public:
	~FImGuiNetControl();

	void Startup(FImGuiContextManager* InContextManager);
	void Shutdown();
	bool OnWorldStartup(int32 InContextIndex, UWorld* World);
	bool IsConnected(int32 InContextIndex);
	void Disconnect(int32 InContextIndex);
	void ServerCaptureInput(int32 ContextIndex);
	TUniquePtr<ImDrawData> GetServerDrawData(int32 ContextIndex);

	// shared state
	FImGuiContextManager* ContextManager = nullptr;
	uint32 Port;
	TOptional<int32> ContextIndex;

	// client state
	TOptional<FString> GameServerHostname;

	// server state
	FImguiServerState* ServerState = nullptr;
};
