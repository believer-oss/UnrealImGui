Unreal ImGui
============
[![MIT licensed](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE.md)

Unreal ImGui is an Unreal Engine 5 plug-in that integrates [Dear ImGui](https://github.com/ocornut/imgui) developed by Omar Cornut.

Dear ImGui is an immediate-mode graphical user interface library that is very lightweight and easy to use. It can be very useful when creating debugging tools.

:stop_button: Read Me First
---------------------------
This plugin has been forked from [https://github.com/benui-dev/UnrealImGui](https://github.com/benui-dev/UnrealImGui) with several changes documented below.

Please see the original [README.md](https://github.com/benui-dev/UnrealImGui) for full information on this plugin - this page only documents the additions that have been made to the fork.

Fork Additions/Fixes
--------------------
 - Style override interface
 - New editor module to support per-user preferences such as DPI
 - Added support for NetImgui, including rendering of connected NetImgui clients
 - Input bug fixes

# NetImgui

See ImGuiNetControl.h for the `FImGuiNetControl` interface and documentation for managing a server connection. The two main functions are:
* `ServerMenu()`: draw a menu that allows users to connect to a Netimgui client inside a game server
* `IsConnected()`: determines if Netimgui is currently connected to a client or server

You can get access to the `FImGuiNetControl` interface via `FImGuiModule::Get().GetNetControl()`.

# Misc

See also
--------
 - [Fork by ben-ui](https://github.com/benui-dev/UnrealImGui)
 - [Original Project by segross](https://github.com/segross/UnrealImGui)
 - [Dear ImGui](https://github.com/ocornut/imgui)
 - [ImPlot](https://github.com/epezent/implot)

License
-------
Unreal ImGui (and this fork) is licensed under the MIT License, see LICENSE for more information.
