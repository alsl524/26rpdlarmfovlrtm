#pragma once
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <chrono>
#include <stdio.h> // printf용

using namespace DirectX;

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

struct ConstantBuffer {
    XMFLOAT4 offset;
};

struct Vertex {
    float x, y, z;
    float r, g, b, a;
};

struct DXContext {
    HWND hWnd;
    int isRunning;

    ID3D11Device* device;
    ID3D11DeviceContext* context;
    IDXGISwapChain* swapChain;
    ID3D11RenderTargetView* rtv;

    ID3D11VertexShader* vShader;
    ID3D11PixelShader* pShader;
    ID3D11InputLayout* layout;
    ID3D11Buffer* vBuffer;
    ID3D11Buffer* cBuffer;

    bool keys[256];
    float posX, posY;
    float moveSpeed;
    std::chrono::steady_clock::time_point prevTime;

    // FPS 및 시간 측정용
    float fpsTimer;
    int frameCount;
};

extern DXContext g_dx;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
bool InitWindow(HINSTANCE hInst, int width, int height);
bool InitDirectX(DXContext* ctx);
void ProcessInput(DXContext* ctx);
void Update(DXContext* ctx, float dt);
void Render(DXContext* ctx);
void Cleanup(DXContext* ctx);
void DisplayConsoleInfo(DXContext* ctx, float dt);