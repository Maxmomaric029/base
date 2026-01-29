#include "gui.h"
#include "globals.h"
#include "imgui.h"
#include <windows.h>
#include <TlHelp32.h>
#include <vector>
#include <string>
#include <algorithm>

// Debug helper to list processes
std::vector<std::string> GetProcessList() {
    std::vector<std::string> list;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return list;

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hSnap, &pe)) {
        int count = 0;
        do {
#ifdef UNICODE
            std::wstring wStr(pe.szExeFile);
            std::string sName(wStr.begin(), wStr.end());
#else
            std::string sName(pe.szExeFile);
#endif
            list.push_back(sName);
            count++;
        } while (Process32Next(hSnap, &pe) && count < 20); 
    }
    CloseHandle(hSnap);
    return list;
}

void SetupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsDark();
    
    style.WindowRounding = 12.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 6.0f;

    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 1.0f;

    ImVec4* colors = style.Colors;
    
    colors[ImGuiCol_WindowBg]             = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_ChildBg]              = ImVec4(0.10f, 0.10f, 0.10f, 0.00f);
    colors[ImGuiCol_PopupBg]              = ImVec4(0.10f, 0.10f, 0.10f, 0.94f);
    colors[ImGuiCol_Border]               = ImVec4(0.44f, 0.00f, 0.00f, 0.50f);
    colors[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    
    colors[ImGuiCol_FrameBg]              = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.24f, 0.00f, 0.00f, 0.54f);
    colors[ImGuiCol_FrameBgActive]        = ImVec4(0.44f, 0.00f, 0.00f, 0.67f);
    
    colors[ImGuiCol_TitleBg]              = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
    colors[ImGuiCol_TitleBgActive]        = ImVec4(0.20f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    
    colors[ImGuiCol_CheckMark]            = ImVec4(0.80f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_SliderGrab]           = ImVec4(0.80f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]     = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    
    colors[ImGuiCol_Button]               = ImVec4(0.44f, 0.00f, 0.00f, 0.40f);
    colors[ImGuiCol_ButtonHovered]        = ImVec4(0.80f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_ButtonActive]         = ImVec4(0.44f, 0.00f, 0.00f, 1.00f);
    
    colors[ImGuiCol_Header]               = ImVec4(0.44f, 0.00f, 0.00f, 0.31f);
    colors[ImGuiCol_HeaderHovered]        = ImVec4(0.44f, 0.00f, 0.00f, 0.80f);
    colors[ImGuiCol_HeaderActive]         = ImVec4(0.44f, 0.00f, 0.00f, 1.00f);

    colors[ImGuiCol_Separator]            = ImVec4(0.44f, 0.00f, 0.00f, 0.50f);
    colors[ImGuiCol_Text]                 = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
}

void RenderGUI() {
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    
    ImGui::Begin("BLOODIE HACK | ROG EDITION", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

    if (Globals::isAttached) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Status: Attached to %s", Globals::currentProcess.c_str());
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Status: Waiting for Emulator...");
        ImGui::TextDisabled("(Run as Administrator if not detected)");
    }

    ImGui::Separator();
    ImGui::Spacing();
    
    ImGui::Text("MEMORY MODIFICATIONS");
    ImGui::Spacing();
    
    ImGui::Checkbox("GUEST ACCOUNT RESET", &Globals::bGuestReset);
    ImGui::Checkbox("AIMBOT LOCK (AOB)", &Globals::bAimLock);
    ImGui::Checkbox("AIMBOT HEAD (OB52)", &Globals::bHead);
    ImGui::Checkbox("M82B ULTRA FAST", &Globals::bM82B);
    ImGui::Checkbox("AWM SWITCH FAST", &Globals::bAwm);
    ImGui::Checkbox("M24 SWITCH FAST", &Globals::bM24);
    ImGui::Checkbox("NO RECOIL", &Globals::bNoRecoil);
    ImGui::Checkbox("SPEED RUN", &Globals::bSpeed);
    ImGui::Checkbox("HEAD DAMAGE", &Globals::bHeadDmg);
    ImGui::Checkbox("WALLHACK", &Globals::bWallhack);
    ImGui::Checkbox("BLUE CROSSHAIR", &Globals::bBlueCross);

    ImGui::Spacing();
    ImGui::Separator();
    
    if (ImGui::CollapsingHeader("Debug: Process List (First 20)")) {
        static std::vector<std::string> procs = GetProcessList();
        if (ImGui::Button("Refresh List")) {
            procs = GetProcessList();
        }
        for (const auto& p : procs) {
            ImGui::Text("%s", p.c_str());
        }
    }
    
    ImGui::TextDisabled("Supported: HD-Player.exe, dnplayer.exe, AndroidEmulatorEx.exe");

    ImGui::End();
}
