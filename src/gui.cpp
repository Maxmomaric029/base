#include "gui.h"
#include "globals.h"
#include "imgui.h"

void SetupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsDark();
    
    // Rounded Corners
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
    
    // ROG Style: Black and Dark Red
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
    ImGui::SetNextWindowSize(ImVec2(450, 320), ImGuiCond_FirstUseEver);
    
    // Modern look with dark red accents
    ImGui::Begin("BLOODIE HACK | ROG EDITION", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

    if (Globals::isAttached) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Status: Attached to %s", Globals::currentProcess.c_str());
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Status: Waiting for Emulator...");
    }

    ImGui::Separator();
    ImGui::Spacing();
    
    ImGui::Text("MEMORY MODIFICATIONS");
    ImGui::Spacing();
    
    if (ImGui::Checkbox("GUEST ACCOUNT RESET", &Globals::bGuestReset)) {
    }
    
    if (ImGui::Checkbox("AIMBOT LOCK (AOB)", &Globals::bAimLock)) {
    }

    ImGui::Spacing();
    ImGui::Separator();
    
    ImGui::TextDisabled("Detected Emulators:");
    ImGui::BulletText("BlueStacks / MSI (HD-Player)");
    ImGui::BulletText("LDPlayer (dnplayer)");
    ImGui::BulletText("GameLoop (AndroidEmulatorEx)");

    ImGui::End();
}
