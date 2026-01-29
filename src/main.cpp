#include <windows.h>
#include <dwmapi.h>
#include <d3d9.h>
#include <tchar.h>
#include <vector>
#include <iostream>
#include <string>
#include <sstream>

#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include "memory.h"
#include "gui.h"
#include "globals.h"

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "d3d9.lib")

static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

HWND hTargetWindow = NULL;
bool bMenuVisible = true;

struct Feature {
    std::string name;
    std::vector<unsigned char> searchPattern;
    std::vector<unsigned char> replacePattern;
    uintptr_t address = 0;
    bool applied = false;
};

std::vector<unsigned char> ParseHexStr(const std::string& hex) {
    std::vector<unsigned char> bytes;
    std::stringstream ss(hex);
    std::string byteStr;
    while (ss >> byteStr) {
        if (byteStr == "?" || byteStr == "??") bytes.push_back(0x00);
        else {
            try { bytes.push_back((unsigned char)std::stoi(byteStr, nullptr, 16)); }
            catch (...) { bytes.push_back(0x00); }
        }
    }
    return bytes;
}

// --- FEATURES ---
Feature fGuest, fAim, fBlue, fAwm, fM24, fNoRecoil, fSpeed, fHeadDmg, fWallhack, fM82B;

void InitFeatures() {
    if (!fAwm.searchPattern.empty()) return;

    fGuest = { "Guest Reset", 
        ParseHexStr("10 4C 2D E9 08 B0 8D E2 0C 01 9F E5 00 00 8F E0"), 
        ParseHexStr("01 00 A0 E3 1E FF 2F E1 0C 01 9F E5 00 00 8F E0") 
    };
    
    fAim = { "Aim Lock", 
        ParseHexStr("C0 3F 0A D7 A3 3B 0A D7 A3 3B 8F C2 75 3D AE 47 E1 3D 9A 99 19 3E CD CC 4C 3E A4 70 FD 3E"),
        ParseHexStr("90 65 0A D7 A3 3B 0A D7 A3 3B 8F C2 75 3D AE 47 E1 3D 9A 99 19 3E CD CC 4C 3E A4 70 FD 3E")
    };

    fAwm = { "AWM Switch",
        ParseHexStr("3F 0A D7 A3 3D 00 00 00 00 00 00 5C 43 00 00 90 42 00 00 B4 42 96 00 00 00 00 00 00 00 00 00 00 3F 00 00 80 3E 00 00 00 00 04 00 00 00 00 00 80 3F 00 00 20 41 00 00 34 42 01 00 00 00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 80 3F 0A D7 23 3F 9A 99 99 3F 00 00 80 3F 00 00 00 00 00 00 80 3F 00 00 80 3F 00 00 80 3F 00 00 00 00 00 00 00 00 00 00 00 3F 00 00 00 00 00 00 00 00 00 00 00 00 00 00 80 3F 00 00 80 3F 00 00 80 3F 00 00 00 00 01 00 00 00 0A D7 23 3C CD CC CC 3D 9A 99 19 3F 1F 85 6B 3F"),
        ParseHexStr("3F 0A D7 A3 3D 00 00 00 00 00 00 5C 43 00 00 90 42 00 00 B4 42 96 00 00 00 00 00 00 00 00 00 00 3F 00 00 80 3E 00 00 00 3C 04")
    };

    fM82B = { "M82B Ultra",
        ParseHexStr("3F 00 00 80 3E 00 00 00 00 04 00 00 00 00 00 80 3F 00 00 20 41 00 00 34 42 01 00 00 00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 80 3F 9A 99 19 3F CD CC 8C 3F 00 00 80 3F 00 00 00 00 66 66 66 3F 00 00 80 3F 00 00 80 3F 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 80 3F 00 00 80 3F 00 00 80 3F 00 00 00 00 01 00 00 00 0A D7 23 3C CD CC CC 3D 9A 99 19 3F 1F 85 6B 3F 00 00 00 00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 3F 00 00 00 3F 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"),
        ParseHexStr("3F 00 00 80 3E EC 51 B8 3D 04 00 00 00 00 00 80 3F 00 00 20 41 00 00 34 42 01 00 00 00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 80 3F 9A 99 19 3F CD CC 8C 3F 00 00 80 3F 00 00 00 00 66 66 66 3F 00 00 80 3F 00 00 80 3F 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 80 3F 00 00 80 3F 00 00 80 3F 00 00 00 00 01 00 00 00 0A D7 23 3C CD CC CC 3D 9A 99 19 3F 1F 85 6B 3F 00 00 00 00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 3F 00 00 00 3F 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00")
    };

    fM24 = { "M24 Switch",
        ParseHexStr("3F 9A 99 99 3E 00 00 00 00 00 00 5C 43 00 00 78 42 00 00 A4 42 32 00 00 00 00 00 00 00 00 00 00 3F 00 00 80 3E 00 00 00 00 06 00 00 00 CD CC 4C 3F 00 00 20 41 00 00 34 42 01 00 00 00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 80 3F 66 66 66 3F 9A 99 99 3F 00 00 80 3F 00 00 00 00 33 33 93 3F 00 00 80 3F 00 00 80 3F 00 00 00 00 00 00 00 00 00 00 80 3F 00 00 00 00 00 00 80 3E 00 00 00 00 00 00 80 3F 00 00 80 3F 00 00 80 3F 00 00 00 00 01 00 00 00 0A D7 A3 3C 0A D7 A3 3C"),
        ParseHexStr("3F 9A 99 99 3E 00 00 00 00 00 00 5C 43 00 00 78 42 00 00 A4 42 32 00 00 00 00 00 00 00 00 00 00 3F 00 00 80 3E 00 00 00 01 06 00 00 00 CD CC 4C 3F 00 00 20 41 00 00 34 42 01 00 00 00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 80 3F 66 66 66 3F 9A 99 99 3F 00 00 80 3F 00 00 00 00 33 33 93 3F 00 00 80 3F 00 00 80 3F 00 00 00 00 00 00 00 00 00 00 80 3F 00 00 00 00 00 00 80 3E 00 00 00 00 00 00 80 3F 00 00 80 3F 00 00 80 3F 00 00 00 00 01 00 00 00 0A D7 A3 3C 0A D7 A3 3C")
    };

    fNoRecoil = { "No Recoil",
        ParseHexStr("30 48 2D E9 08 B0 8D E2 02 8B 2D ED 00 40 A0 E1 38 01 9F E5 00 00 8F E0 00 00 D0 E5"),
        ParseHexStr("00 00 A0 E3 1E FF 2F E1 02 8B 2D ED 00 40 A0 E1 38 01 9F E5 00 00 8F E0 00 00 D0 E5")
    };

    fSpeed = { "Speed Run",
        ParseHexStr("02 2B 07 3D 02 2B 07 3D 02 2B 07 3D 00 00 00 00 9B 6C"),
        ParseHexStr("E3 A5 9B 3C E3 A5 9B 3C 02 2B 07 3D 00 00 00 00 9B 6C")
    };

    fHeadDmg = { "Head Damage",
        ParseHexStr("FF FF FF FF 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 FF FF FF FF FF FF FF FF 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"),
        ParseHexStr("A5 43 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 FF FF FF FF FF FF FF FF 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00")
    };

    fWallhack = { "Wallhack",
        ParseHexStr("00 00 00 00 00 AE 47 81 3F AE 47 81 3F AE 47 81 3F AE 47 81 3F 00 1A B7 EE DC 3A 9F ED 30 00 4F E2 43 2A B0 EE EF 0A 60 F4 43 6A F0 EE 1C 00 8A E2 43 5A F0 EE 8F 0A 48 F4 43 2A F0 EE 43 7A B0 EE 8F 0A 40 F4 41 AA B0 EE FE"),
        ParseHexStr("00 00 00 00 00 AE 47 81 3F AE 47 81 3F AE 47 81 BF AE 47 81 3F AE 47 81 BF AE 47 81 3F 00 1A B7 EE DC 3A 9F ED EF 0A 60 F4 43 6A F0 EE 1C 00 8A E2 43 5A F0 EE 8F 0A 48 F4 43 2A F0 EE 43 7A B0 EE 8F 0A 40 F4 41 AA B0 EE FE")
    };

    fBlue = { "Blue Cross",
        ParseHexStr("01 00 A0 E3 1C 00 85 E5 00 70"),
        ParseHexStr("0F 00 A0 E1 1C 00 85 E5 00 70")
    };
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
        for (const std::string& exeName : Globals::SUPPORTED_EMULATORS) {
            if (mem == nullptr) mem = new Memory(exeName);
            if (mem->Attach(exeName)) { Globals::currentProcess = exeName; Globals::isAttached = true; break; }
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
        HandleFeature(fM82B, Globals::bM82B);
    }
}

int main(int, char**)
{
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"BloodieOverlay", NULL };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW, wc.lpszClassName, L"Bloodie Hack", WS_POPUP, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), NULL, NULL, wc.hInstance, NULL);

    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    if (!CreateDeviceD3D(hwnd)) { CleanupDeviceD3D(); return 1; }
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    SetupStyle();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    bool done = false;
    while (!done) {
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        if (Globals::isAttached) {
            if (!IsWindow(hTargetWindow)) {
                hTargetWindow = FindWindowA(NULL, "BlueStacks App Player");
                if (!hTargetWindow) hTargetWindow = FindWindowA(NULL, "MSI App Player");
                if (!hTargetWindow) hTargetWindow = FindWindowA(NULL, "LDPlayer");
            }
            if (hTargetWindow) {
                RECT rect; GetWindowRect(hTargetWindow, &rect);
                MoveWindow(hwnd, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, TRUE);
            }
        }

        FeatureLoop();
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        if (bMenuVisible) RenderGUI();
        
        // --- Click-Through Logic ---
        static bool wasVisible = true;
        if (bMenuVisible != wasVisible) {
            wasVisible = bMenuVisible;
            LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
            if (bMenuVisible)
                exStyle &= ~WS_EX_TRANSPARENT; // Menu open: Enable clicks
            else
                exStyle |= WS_EX_TRANSPARENT;  // Menu closed: Click through to game
            SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);
        }
        // ---------------------------

        if (GetAsyncKeyState(VK_INSERT) & 1) bMenuVisible = !bMenuVisible;
        ImGui::EndFrame();

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
    g_d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
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