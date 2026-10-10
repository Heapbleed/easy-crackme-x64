// easy-crackme-x64 - a tiny GUI login crackme (Dear ImGui + DirectX 11)
//
// It's a login box. There's no database and no network - the "right" password
// for a username is computed on the spot and compared. Your job: figure out how
// that password is built, then write a keygen that spits one out for any name.
//
// Everything that matters lives in CheckPassword() and the two helpers above it.
// Nothing is hidden or obfuscated; this one is meant to be gentle. If you'd
// rather not sweat it, SOLUTION.md walks through the whole thing.

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "user32.lib")

// ---------------------------------------------------------------------------
// D3D11 state
// ---------------------------------------------------------------------------
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ---------------------------------------------------------------------------
// The check
// ---------------------------------------------------------------------------
//
// A valid password looks like "XXXX-XXXX-XXXX-XXXX": sixteen hex digits, which
// together make up one 64-bit number (we just print it in four groups of four
// so it's easier on the eyes).
//
// The expected number comes straight from the username. We start from a fixed
// seed and stir in one character at a time. Nothing clever - just enough mixing
// that you can't eyeball the answer, but every step is right here in the open.

// Rotate the 64-bit value left by s bits (the bits that fall off the top wrap
// back around to the bottom).
static uint64_t rotl64(uint64_t v, int s) { return (v << s) | (v >> (64 - s)); }

// Turn a username into its one and only valid password value.
static uint64_t DeriveKey(const char* username)
{
    uint64_t state = 0xC0FFEE1234567890ULL;         // arbitrary starting point
    for (const char* p = username; *p; ++p)
    {
        state ^= (uint8_t)(*p);                     // fold in this character
        state *= 0x100000001B3ULL;                  // FNV prime - scrambles the bits
        state = rotl64(state, 11);                  // shuffle them around some more
        state += 0x9E3779B97F4A7C15ULL;             // the "golden ratio" constant
    }
    return state;
}

// Read the password the user typed back into a number. Dashes and spaces are
// ignored, so "ABCD-...", "ABCD ..." and "ABCD..." all work the same. We insist
// on exactly 16 hex digits and nothing weird in between, otherwise it's a no.
static bool ParsePassword(const char* s, uint64_t* out)
{
    uint64_t value = 0;
    int digits = 0;
    for (const char* p = s; *p; ++p)
    {
        char c = *p;
        if (c == '-' || c == ' ') continue;         // cosmetic separators, skip them
        int nibble;
        if (c >= '0' && c <= '9')      nibble = c - '0';
        else if (c >= 'A' && c <= 'F') nibble = c - 'A' + 10;
        else if (c >= 'a' && c <= 'f') nibble = c - 'a' + 10;
        else return false;                          // not a hex digit - reject
        if (digits >= 16) return false;             // more than 16 digits - too long
        value = (value << 4) | (uint64_t)nibble;
        digits++;
    }
    if (digits != 16) return false;                 // need exactly 16, no more no less
    *out = value;
    return true;
}

// The whole gate: non-empty username, well-formed password, and the number the
// user typed has to match the one we derive from their name.
static bool CheckPassword(const char* username, const char* password)
{
    if (!username[0]) return false;                 // no anonymous logins
    uint64_t entered;
    if (!ParsePassword(password, &entered)) return false;
    return entered == DeriveKey(username);
}

// ---------------------------------------------------------------------------
// UI
// ---------------------------------------------------------------------------
static void DrawUI()
{
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::Begin("CrackMe", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

    ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, 1.0f), "Login");
    ImGui::Separator();
    ImGui::Spacing();

    // Kept around between frames so the box remembers what you typed.
    static char username[64] = "";
    static char password[64] = "";
    static int  result = 0;   // 0 = nothing tried yet, 1 = granted, 2 = denied

    ImGui::Text("Username");
    ImGui::SetNextItemWidth(320);
    if (ImGui::InputText("##username", username, sizeof(username)))
        result = 0;           // they're editing again, wipe the old verdict

    ImGui::Text("Password");
    ImGui::SetNextItemWidth(320);
    // Enter in the password box submits, just like a real login form.
    if (ImGui::InputText("##password", password, sizeof(password),
                         ImGuiInputTextFlags_Password | ImGuiInputTextFlags_EnterReturnsTrue))
        result = CheckPassword(username, password) ? 1 : 2;

    ImGui::Spacing();
    if (ImGui::Button("Login", ImVec2(150, 0)))
        result = CheckPassword(username, password) ? 1 : 2;

    ImGui::SameLine();
    if (ImGui::Button("Clear", ImVec2(150, 0)))
    {
        username[0] = password[0] = 0;
        result = 0;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    switch (result)
    {
    case 1:
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.4f, 1.0f), "Access granted.");
        break;
    case 2:
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Access denied.");
        break;
    default:
        break;
    }

    ImGui::End();
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
    ImGui_ImplWin32_EnableDpiAwareness();
    float dpi = ImGui_ImplWin32_GetDpiScaleForMonitor(MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, hInstance,
                       LoadIcon(nullptr, IDI_APPLICATION), LoadCursor(nullptr, IDC_ARROW),
                       nullptr, nullptr, L"CrackMe1", nullptr };
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowW(wc.lpszClassName, L"Login", WS_OVERLAPPEDWINDOW,
                              100, 100, (int)(520 * dpi), (int)(360 * dpi), nullptr, nullptr, hInstance, nullptr);

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        UnregisterClassW(wc.lpszClassName, hInstance);
        MessageBoxW(nullptr, L"Failed to create a Direct3D 11 device.", L"CrackMe", MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 4.0f;
    style.ScaleAllSizes(dpi);
    io.FontGlobalScale = dpi;

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    const float clear[4] = { 0.09f, 0.10f, 0.11f, 1.00f };
    bool done = false;
    while (!done)
    {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        DrawUI();
        ImGui::Render();

        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        HRESULT hr = g_pSwapChain->Present(1, 0);
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, hInstance);
    return 0;
}

// ---------------------------------------------------------------------------
// D3D helpers
// ---------------------------------------------------------------------------
bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL level;
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, 2,
        D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &level, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, levels, 2,
            D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &level, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain)        { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice)        { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* backBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    g_pd3dDevice->CreateRenderTargetView(backBuffer, nullptr, &g_mainRenderTargetView);
    backBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
