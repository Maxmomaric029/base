#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include <windows.h>
#include <dwmapi.h>
#include <d3d9.h>
#include <tchar.h>
#include <vector>
#include <iostream>
#include <string>

#include "memory.h"
#include "gui.h"
#include "globals.h"

#pragma comment(lib, "dwmapi.lib")

static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Overlay State
HWND hTargetWindow = NULL;
RECT targetRect;
bool bMenuVisible = true;

// Feature Struct
struct Feature {
    std::string name;
    std::vector<unsigned char> searchPattern;
    std::vector<unsigned char> replacePattern;
    uintptr_t address = 0;
    bool applied = false;
};

// --- FEATURE DATA (Same as before) ---
// GUEST RESET
std::vector<unsigned char> guestS = {0x10, 0x4C, 0x2D, 0xE9, 0x08, 0xB0, 0x8D, 0xE2, 0x0C, 0x01, 0x9F, 0xE5, 0x00, 0x00, 0x8F, 0xE0};
std::vector<unsigned char> guestR = {0x01, 0x00, 0xA0, 0xE3, 0x1E, 0xFF, 0x2F, 0xE1, 0x0C, 0x01, 0x9F, 0xE5, 0x00, 0x00, 0x8F, 0xE0};
// AIM LOCK
std::vector<unsigned char> aimS = {0xC0, 0x3F, 0x0A, 0xD7, 0xA3, 0x3B, 0x0A, 0xD7, 0xA3, 0x3B, 0x8F, 0xC2, 0x75, 0x3D, 0xAE, 0x47, 0xE1, 0x3D, 0x9A, 0x99, 0x19, 0x3E, 0xCD, 0xCC, 0x4C, 0x3E, 0xA4, 0x70, 0xFD, 0x3E};
std::vector<unsigned char> aimR = {0x90, 0x65, 0x0A, 0xD7, 0xA3, 0x3B, 0x0A, 0xD7, 0xA3, 0x3B, 0x8F, 0xC2, 0x75, 0x3D, 0xAE, 0x47, 0xE1, 0x3D, 0x9A, 0x99, 0x19, 0x3E, 0xCD, 0xCC, 0x4C, 0x3E, 0xA4, 0x70, 0xFD, 0x3E};
// BLUE CROSS
std::vector<unsigned char> blueS = {0x01, 0x00, 0xA0, 0xE3, 0x1C, 0x00, 0x85, 0xE5, 0x00, 0x70};
std::vector<unsigned char> blueR = {0x0F, 0x00, 0xA0, 0xE1, 0x1C, 0x00, 0x85, 0xE5, 0x00, 0x70};

Feature fGuest = { "Guest Reset", guestS, guestR };
Feature fAim = { "Aim Lock", aimS, aimR };
Feature fBlue = { "Blue Cross", blueS, blueR };
Feature fAwm = { "AWM Switch", {}, {} };
Feature fM24 = { "M24 Switch", {}, {} };
Feature fNoRecoil = { "No Recoil", {}, {} };
Feature fSpeed = { "Speed", {}, {} };
Feature fHeadDmg = { "Head Damage", {}, {} };
Feature fWallhack = { "Wallhack", {}, {} };

// Helper
std::vector<unsigned char> ParseHexStr(const std::string& hex) {
    std::vector<unsigned char> bytes;
    std::stringstream ss(hex);
    std::string byteStr;
    while (ss >> byteStr) {
        if (byteStr == "?" || byteStr == "??") bytes.push_back(0x00);
        else bytes.push_back((unsigned char)std::stoi(byteStr, nullptr, 16));
    }
    return bytes;
}

void InitFeatures() {
    if (!fAwm.searchPattern.empty()) return;
    // (Patterns truncated for brevity, assume populated from previous turns)
    // IMPORTANT: Make sure these are filled in the final code. I will re-paste the patterns.
    fAwm.searchPattern = ParseHexStr("3F 0A D7 A3 3D 00 00 00 00 00 00 5C 43 00 00 90 42 00 00 B4 42 96 00 00 00 00 00 00 00 00 00 00 3F 00 00 80 3E 00 00 00 00 04 00 00 00 00 00 80 3F 00 00 20 41 00 00 34 42 01 00 00 00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 80 3F 0A D7 23 3F 9A 99 99 3F 00 00 80 3F 00 00 00 00 00 00 80 3F 00 00 80 3F 00 00 80 3F 00 00 00 00 00 00 00 00 00 00 00 3F 00 00 00 00 00 00 00 00 00 00 00 00 00 00 80 3F 00 00 80 3F 00 00 80 3F 00 00 00 00 01 00 00 00 0A D7 23 3C CD CC CC 3D 9A 99 19 3F 1F 85 6B 3F");
    fAwm.replacePattern = ParseHexStr("3F 0A D7 A3 3D 00 00 00 00 00 00 5C 43 00 00 90 42 00 00 B4 42 96 00 00 00 00 00 00 00 00 00 00 3F 00 00 80 3E 00 00 00 3C 04");

    fM24.searchPattern = ParseHexStr("3F 9A 99 99 3E 00 00 00 00 00 00 5C 43 00 00 78 42 00 00 A4 42 32 00 00 00 00 00 00 00 00 00 00 3F 00 00 80 3E 00 00 00 00 06 00 00 00 CD CC 4C 3F 00 00 20 41 00 00 34 42 01 00 00 00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 80 3F 66 66 66 3F 9A 99 99 3F 00 00 80 3F 00 00 00 00 33 33 93 3F 00 00 80 3F 00 00 80 3F 00 00 00 00 00 00 00 00 00 00 80 3F 00 00 00 00 00 00 80 3E 00 00 00 00 00 00 80 3F 00 00 80 3F 00 00 80 3F 00 00 00 00 01 00 00 00 0A D7 A3 3C 0A D7 A3 3C");
    fM24.replacePattern = ParseHexStr("3F 9A 99 99 3E 00 00 00 00 00 00 5C 43 00 00 78 42 00 00 A4 42 32 00 00 00 00 00 00 00 00 00 00 3F 00 00 80 3E 00 00 00 01 06 00 00 00 CD CC 4C 3F 00 00 20 41 00 00 34 42 01 00 00 00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 80 3F 66 66 66 3F 9A 99 99 3F 00 00 80 3F 00 00 00 00 33 33 93 3F 00 00 80 3F 00 00 80 3F 00 00 00 00 00 00 00 00 00 00 80 3F 00 00 00 00 00 00 80 3E 00 00 00 00 00 00 80 3F 00 00 80 3F 00 00 80 3F 00 00 00 00 01 00 00 00 0A D7 A3 3C 0A D7 A3 3C");

    fNoRecoil.searchPattern = ParseHexStr("30 48 2D E9 08 B0 8D E2 02 8B 2D ED 00 40 A0 E1 38 01 9F E5 00 00 8F E0 00 00 D0 E5");
    fNoRecoil.replacePattern = ParseHexStr("00 00 A0 E3 1E FF 2F E1 02 8B 2D ED 00 40 A0 E1 38 01 9F E5 00 00 8F E0 00 00 D0 E5");

    fSpeed.searchPattern = ParseHexStr("02 2B 07 3D 02 2B 07 3D 02 2B 07 3D 00 00 00 00 9B 6C");
    fSpeed.replacePattern = ParseHexStr("E3 A5 9B 3C E3 A5 9B 3C 02 2B 07 3D 00 00 00 00 9B 6C");

    fHeadDmg.searchPattern = ParseHexStr("FF FF FF FF 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 FF FF FF FF FF FF FF FF 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00");
    fHeadDmg.replacePattern = ParseHexStr("A5 43 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 FF FF FF FF FF FF FF FF 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00");

    fWallhack.searchPattern = ParseHexStr("00 00 00 00 00 AE 47 81 3F AE 47 81 3F AE 47 81 3F AE 47 81 3F 00 1A B7 EE DC 3A 9F ED 30 00 4F E2 43 2A B0 EE EF 0A 60 F4 43 6A F0 EE 1C 00 8A E2 43 5A F0 EE 8F 0A 48 F4 43 2A F0 EE 43 7A B0 EE 8F 0A 40 F4 41 AA B0 EE FE");
    fWallhack.replacePattern = ParseHexStr("00 00 00 00 00 AE 47 81 3F AE 47 81 3F AE 47 81 BF AE 47 81 3F AE 47 81 BF AE 47 81 3F 00 1A B7 EE DC 3A 9F ED EF 0A 60 F4 43 6A F0 EE 1C 00 8A E2 43 5A F0 EE 8F 0A 48 F4 43 2A F0 EE 43 7A B0 EE 8F 0A 40 F4 41 AA B0 EE FE");
}

Memory* mem = nullptr;

std::string BytesToString(const std::vector<unsigned char>& bytes) {
    std::string s;
    char buf[4];
    for (auto b : bytes) {
        sprintf_s(buf, "%02X ", b);
        s += buf;
    }
    return s;
}

void HandleFeature(Feature& f, bool enabled) {
    if (!mem || !mem->hProcess) return;
    if (enabled && !f.applied) {
        if (f.address == 0) f.address = mem->FindPattern("", BytesToString(f.searchPattern));
        if (f.address != 0 && mem->Patch(f.address, f.replacePattern)) { f.applied = true; std::cout << "[SUCCESS] " << f.name << std::endl; }
    } else if (!enabled && f.applied) {
        if (f.address != 0 && mem->Patch(f.address, f.searchPattern)) f.applied = false;
    }
}

void FeatureLoop() {
    InitFeatures();

    if (!Globals::isAttached) {
        // Find Process ID for "HD-Player.exe", etc.
        for (const std::string& exeName : Globals::SUPPORTED_EMULATORS) {
            if (mem == nullptr) mem = new Memory(exeName);
            if (mem->Attach(exeName)) { 
                Globals::currentProcess = exeName; 
                Globals::isAttached = true;
                
                // Find the window handle for the overlay
                // This is a simple find. For better results, enumerate windows by PID.
                // But finding by name is usually okay for emulators.
                std::string winName;
                if(exeName == "HD-Player.exe") winName = "BlueStacks App Player"; // Or "MSI App Player"
                else if(exeName == "dnplayer.exe") winName = "LDPlayer";
                
                // Fallback attempt: FindWindow(NULL, ...) might be needed if class name unknown
                // We will try finding by executable name logic later if needed.
                break; 
            }
        }
    } else {
        DWORD exitCode;
        if (GetExitCodeProcess(mem->hProcess, &exitCode) && exitCode != STILL_ACTIVE) {
            Globals::isAttached = false; return;
        }
        HandleFeature(fGuest, Globals::bGuestReset);
        HandleFeature(fAim, Globals::bAimLock);
        HandleFeature(fBlue, Globals::bBlueCross);
        HandleFeature(fAwm, Globals::bAwm);
        HandleFeature(fM24, Globals::bM24);
        HandleFeature(fNoRecoil, Globals::bNoRecoil);
        HandleFeature(fSpeed, Globals::bSpeed);
        HandleFeature(fHeadDmg, Globals::bHeadDmg);
        HandleFeature(fWallhack, Globals::bWallhack);
    }
}

int main(int, char**)
{
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    std::cout << "Overlay Trainer Started..." << std::endl;

    // Create Transparent Overlay Window
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"BloodieOverlay", NULL };
    ::RegisterClassExW(&wc);
    
    // WS_EX_LAYERED for transparency, WS_EX_TOPMOST for staying on top
    HWND hwnd = ::CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW, 
        wc.lpszClassName, L"Bloodie Hack", 
        WS_POPUP, 
        0, 0, 1920, 1080, // Fullscreen initially, will resize to emulator
        NULL, NULL, wc.hInstance, NULL
    );

    // Set Transparency
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    if (!CreateDeviceD3D(hwnd)) { CleanupDeviceD3D(); return 1; }
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    SetupStyle(); // From gui.cpp
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);
    
    // Style adjustments for Overlay
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg].w = 0.95f; // Slight opacity for menu
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

    bool done = false;
    while (!done) {
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        // Overlay Logic: Follow Emulator
        if (Globals::isAttached) {
            // Find window if we haven't or lost it
            if (!IsWindow(hTargetWindow)) {
                // Try finding by Class or Name. 
                // For BlueStacks: Class "Qt5QWindowIcon" usually (or generic)
                // Let's try finding the window belonging to the Process ID we attached to
                // This requires EnumWindows. For now, we assume user brings emulator to front or we find by name.
                // Simple hack: FindWindow logic
                if (Globals::currentProcess == "HD-Player.exe") {
                   hTargetWindow = FindWindowA(NULL, "BlueStacks App Player");
                   if (!hTargetWindow) hTargetWindow = FindWindowA(NULL, "MSI App Player");
                }
            }

            if (hTargetWindow) {
                RECT rect;
                GetWindowRect(hTargetWindow, &rect);
                int w = rect.right - rect.left;
                int h = rect.bottom - rect.top;
                // Move overlay to match
                MoveWindow(hwnd, rect.left, rect.top, w, h, TRUE);
            }
        }

        FeatureLoop();

        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        
        // Transparent fullscreen background
        ImGui::SetNextWindowPos(ImVec2(0,0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("Overlay", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove);
        // Draw ESP here if needed
        ImGui::End();

        // The Menu itself
        if (bMenuVisible) {
             RenderGUI(); // From gui.cpp
        }
        
        // Handle Insert key to toggle menu
        if (GetAsyncKeyState(VK_INSERT) & 1) {
            bMenuVisible = !bMenuVisible;
            // Passthrough logic could be added here (SetWindowLong GWL_EXSTYLE...)
        }

        ImGui::EndFrame();

        // Clear with 0 alpha for transparency
        g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0) {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }
        if (g_pd3dDevice->Present(NULL, NULL, NULL, NULL) == D3DERR_DEVICELOST) ResetDevice();
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    return 0;
}

bool CreateDeviceD3D(HWND hWnd) {
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL) return false;
    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8; // Alpha channel support needed!
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0) return false;
    return true;
}
void CleanupDeviceD3D() { if (g_pd3dDevice) g_pd3dDevice->Release(); if (g_pD3D) g_pD3D->Release(); }
void ResetDevice() { ImGui_ImplDX9_InvalidateDeviceObjects(); g_pd3dDevice->Reset(&g_d3dpp); ImGui_ImplDX9_CreateDeviceObjects(); }
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg) {
        case WM_SIZE: if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED) { g_d3dpp.BackBufferWidth = LOWORD(lParam); g_d3dpp.BackBufferHeight = HIWORD(lParam); ResetDevice(); } return 0;
        case WM_SYSCOMMAND: if ((wParam & 0xfff0) == SC_KEYMENU) return 0; break;
        case WM_DESTROY: ::PostQuitMessage(0); return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
