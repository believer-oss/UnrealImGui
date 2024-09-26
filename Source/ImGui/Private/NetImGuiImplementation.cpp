// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "imgui.h"

// NetImgui implementation
#undef snprintf // imgui defines snprintf to _snprintf, but netimgui uses std::snprintf, and std::_snprintf doesn't exist
#pragma warning(push)
#pragma warning(disable:4996)
#include "NetImgui_Api.cpp"
#include "NetImgui_Client.cpp"
#include "NetImgui_CmdPackets_DrawFrame.cpp"
#include "NetImgui_NetworkPosix.cpp"
#include "NetImgui_NetworkUE4.cpp"
#include "NetImgui_NetworkWin32.cpp"

#include "NetImguiServer_Config.cpp"
#include "NetImguiServer_RemoteClient.cpp"
#include "NetImguiServer_Network.cpp"
#include "NetImguiServer_App.cpp"
// #include "NetImguiServer_UI.cpp" // we provide our own UI layer
#include "Custom/NetImguiServer_App_Custom.cpp"
#pragma warning(pop)
