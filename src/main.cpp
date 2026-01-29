#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include <d3d9.h>
#include <tchar.h>
#include <vector>
#include <iostream>

#include "memory.h"
#include "gui.h"
#include "globals.h"

// Data
static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Feature Logic
struct Feature {
    std::string name;
    std::vector<unsigned char> searchPattern;
    std::vector<unsigned char> replacePattern;
    uintptr_t address = 0;
    bool applied = false;
};

// Byte Arrays (From User Request)
// GUEST RESET
// Search: 10 4C 2D E9 08 B0 8D E2 0C 01 9F E5 00 00 8F E0
std::vector<unsigned char> guestSearch = {0x10, 0x4C, 0x2D, 0xE9, 0x08, 0xB0, 0x8D, 0xE2, 0x0C, 0x01, 0x9F, 0xE5, 0x00, 0x00, 0x8F, 0xE0};
std::vector<unsigned char> guestReplace = {0x01, 0x00, 0xA0, 0xE3, 0x1E, 0xFF, 0x2F, 0xE1, 0x0C, 0x01, 0x9F, 0xE5, 0x00, 0x00, 0x8F, 0xE0};

// AIM LOCK
// Search: C0 3F 0A D7 A3 3B 0A D7 A3 3B 8F C2 75 3D AE 47 E1 3D 9A 99 19 3E CD CC 4C 3E A4 70 FD 3E
// Replace: 90 65 0A D7 A3 3B 0A D7 A3 3B 8F C2 75 3D AE 47 E1 3D 9A 99 19 3E CD CC 4C 3E A4 70 FD 3E
std::vector<unsigned char> aimSearch = {
    0xC0, 0x3F, 0x0A, 0xD7, 0xA3, 0x3B, 0x0A, 0xD7, 0xA3, 0x3B, 0x8F, 0xC2, 0x75, 0x3D, 0xAE, 0x47, 0xE1, 0x3D, 0x9A, 0x99, 0x19, 0x3E, 0xCD, 0xCC, 0x4C, 0x3E, 0xA4, 0x70, 0xFD, 0x3E
};
std::vector<unsigned char> aimReplace = {
    0x90, 0x65, 0x0A, 0xD7, 0xA3, 0x3B, 0x0A, 0xD7, 0xA3, 0x3B, 0x8F, 0xC2, 0x75, 0x3D, 0xAE, 0x47, 0xE1, 0x3D, 0x9A, 0x99, 0x19, 0x3E, 0xCD, 0xCC, 0x4C, 0x3E, 0xA4, 0x70, 0xFD, 0x3E
};

Feature fGuest = { "Guest Reset", guestSearch, guestReplace };
Feature fAim = { "Aim Lock", aimSearch, aimReplace };

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
        // Find Address if not found yet
        if (f.address == 0) {
            // Convert search bytes to string for pattern scanner (if using generic scanner)
            // But our scanner takes string. Let's make a byte string helper or just use the scanner differently.
            // Our memory class expects "AA BB CC" string.
            std::string pattern = BytesToString(f.searchPattern);
            // Note: Scanning the whole process or module.
            // For now, we assume searching the main module or a specific one.
            // If the user didn't specify the module, we might need to scan all.
            // Let's try finding in Globals::TARGET_MODULE if defined, or process base.
            f.address = mem->FindPattern(Globals::TARGET_MODULE, pattern);
        }

        if (f.address != 0) {
            if (mem->Patch(f.address, f.replacePattern)) {
                f.applied = true;
            }
        }
    } else if (!enabled && f.applied) {
        // Restore
        if (f.address != 0) {
            if (mem->Patch(f.address, f.searchPattern)) {
                f.applied = false;
            }
        }
    }
}

void FeatureLoop() {
    if (!Globals::isAttached) {
        for (const std::string& exeName : Globals::SUPPORTED_EMULATORS) {
            if (mem == nullptr) {
                mem = new Memory(exeName);
            }

            if (mem->Attach(exeName)) {
                Globals::currentProcess = exeName;
                Globals::isAttached = true;
                break;
            }
        }
    } else {
        // Check if process is still alive
        DWORD exitCode;
        if (GetExitCodeProcess(mem->hProcess, &exitCode) && exitCode != STILL_ACTIVE) {
            Globals::isAttached = false;
            Globals::currentProcess = "None";
            return;
        }

        HandleFeature(fGuest, Globals::bGuestReset);
        HandleFeature(fAim, Globals::bAimLock);
    }
}

// Main code
int main(int, char**)
{
    // Enable Console for Debugging
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);
    std::cout << "Bloodie Hack Started..." << std::endl;
    std::cout << "Waiting for emulators..." << std::endl;

    // Create application window
    // ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"Bloodie Trainer Class", NULL };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Bloodie Trainer", WS_OVERLAPPEDWINDOW, 100, 100, 600, 450, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup Dear ImGui style
    SetupStyle();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    // Main loop
    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Logic
        FeatureLoop();

        // Start the Dear ImGui frame
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        RenderGUI();

        // Rendering
        ImGui::EndFrame();
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(0.45f * 255.0f), (int)(0.55f * 255.0f), (int)(0.60f * 255.0f), 255);
        g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0)
        {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }
        HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);

        // Handle loss of D3D9 device
        if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
            ResetDevice();
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions
bool CreateDeviceD3D(HWND hWnd)
{
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
        return false;

    // Create the D3DDevice
    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN; // Need to use an explicit format with alpha if needing per-pixel alpha composition.
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
    //g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate

    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
        return false;

    return true;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = NULL; }
}

void ResetDevice()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (hr == D3DERR_INVALIDCALL)
        IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}

// Win32 message handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            g_d3dpp.BackBufferWidth = LOWORD(lParam);
            g_d3dpp.BackBufferHeight = HIWORD(lParam);
            ResetDevice();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
