#include "gui.h"
#include "globals.h"
#include "imgui.h"

void SetupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsDark();
    
    style.WindowRounding = 5.0f;
    style.FrameRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
}

void RenderGUI() {
    // Center the window for the first run
    ImGui::SetNextWindowSize(ImVec2(450, 300), ImGuiCond_FirstUseEver);
    
    ImGui::Begin("Bloodie Trainer - Free Fire", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

    ImGui::Text("Status: Waiting for process...");
    ImGui::Separator();
    
    ImGui::Spacing();
    
    // Features
    ImGui::Text("Memory Patches:");
    
    if (ImGui::Checkbox("GUEST RESET", &Globals::bGuestReset)) {
        // Logic will be handled in main loop or separate thread
    }
    
    if (ImGui::Checkbox("AIM LOCK", &Globals::bAimLock)) {
        // Logic handled in main loop
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextDisabled("Instructions:\n1. Open Emulator\n2. Run Game\n3. Toggle features");

    ImGui::End();
}
