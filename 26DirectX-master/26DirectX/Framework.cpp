#include "Types.h"
#include <cmath>
#include <cstdio>

// 전역 변수 실체 생성
DXContext g_dx = { 0 };

// --- HeartRenderer 구현 ---
void HeartRenderer::Start(DXContext* ctx) {
    std::vector<Vertex> baseVertices;
    const float PI = 3.141592f;
    baseVertices.push_back({ 0.0f, 0.0f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f });
    for (int i = 0; i <= 100; ++i) {
        float t = (i / 100.0f) * 2.0f * PI;
        float x = 16.0f * powf(sinf(t), 3.0f);
        float y = 13.0f * cosf(t) - 5.0f * cosf(2.0f * t) - 2.0f * cosf(3.0f * t) - cosf(4.0f * t);
        baseVertices.push_back({ x * 0.04f, y * 0.04f, 0.5f, 1.0f, 0.3f, 0.3f, 1.0f });
    }
    std::vector<Vertex> triangleList;
    for (int i = 1; i <= 100; ++i) {
        triangleList.push_back(baseVertices[0]);
        triangleList.push_back(baseVertices[i]);
        triangleList.push_back(baseVertices[i + 1]);
    }
    vertexCount = (int)triangleList.size();
    D3D11_BUFFER_DESC bd = { (UINT)(sizeof(Vertex) * vertexCount), D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
    D3D11_SUBRESOURCE_DATA id = { triangleList.data(), 0, 0 };
    ctx->device->CreateBuffer(&bd, &id, &vBuffer);
}

void HeartRenderer::OnRender(DXContext* ctx) {
    UINT stride = sizeof(Vertex), offset = 0;
    ctx->context->IASetVertexBuffers(0, 1, &vBuffer, &stride, &offset);
    ctx->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->context->Draw(vertexCount, 0);
}

HeartRenderer::~HeartRenderer() { if (vBuffer) vBuffer->Release(); }

// --- PlayerControl 구현 ---
void PlayerControl::OnUpdate(DXContext* ctx, float dt) {
    float speed = 1.5f;
    if (GetAsyncKeyState('W') & 0x8000) ctx->posY += speed * dt;
    if (GetAsyncKeyState('S') & 0x8000) ctx->posY -= speed * dt;
    if (GetAsyncKeyState('A') & 0x8000) ctx->posX -= speed * dt;
    if (GetAsyncKeyState('D') & 0x8000) ctx->posX += speed * dt;
    ctx->posX = max(-0.85f, min(ctx->posX, 0.85f));
    ctx->posY = max(-0.85f, min(ctx->posY, 0.85f));
    ConstantBuffer cb = { XMFLOAT4(ctx->posX, ctx->posY, 0, 0) };
    ctx->context->UpdateSubresource(ctx->cBuffer, 0, NULL, &cb, 0, 0);
}

// --- [VideoSystem 구현] ---
void VideoSystem::RebuildVideoResources(DXContext* ctx) {
    if (!ctx->swapChain) return;

    // 1. 기존 RTV 해제 (필수!)
    if (ctx->rtv) { ctx->rtv->Release(); ctx->rtv = nullptr; }

    // 2. 버퍼 크기 재설정
    ctx->swapChain->ResizeBuffers(0, ctx->Width, ctx->Height, DXGI_FORMAT_UNKNOWN, 0);

    // 3. 새 RTV 생성
    ID3D11Texture2D* pBackBuffer = nullptr;
    ctx->swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
    ctx->device->CreateRenderTargetView(pBackBuffer, nullptr, &ctx->rtv);
    pBackBuffer->Release();

    // 4. 윈도우 실제 크기 조정 (전체화면 아닐 때만)
    if (!ctx->IsFullscreen) {
        RECT rc = { 0, 0, ctx->Width, ctx->Height };
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
        SetWindowPos(ctx->hWnd, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER);
    }
    ctx->NeedsResize = false;
}

void VideoSystem::OnUpdate(DXContext* ctx, float dt) {
    // ESC 키: 프로그램 종료
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        PostQuitMessage(0);
    }
    // F키: 전체화면 토글
    if (GetAsyncKeyState('F') & 0x0001) {
        ctx->IsFullscreen = !ctx->IsFullscreen;
        ctx->swapChain->SetFullscreenState(ctx->IsFullscreen, nullptr);
    }
    // 1키: 800x600
    if (GetAsyncKeyState('1') & 0x0001) {
        ctx->Width = 800; ctx->Height = 600; ctx->NeedsResize = true;
    }
    // 2키: 1280x1080
    if (GetAsyncKeyState('2') & 0x0001) {
        ctx->Width = 1280; ctx->Height = 1080; ctx->NeedsResize = true;
    }

    // 변경 사항이 있으면 리소스 재구축
    if (ctx->NeedsResize) {
        RebuildVideoResources(ctx);
    }
}

// --- InfoDisplay 구현 ---
void InfoDisplay::OnUpdate(DXContext* ctx, float dt) {
    fpsTimer += dt; frames++;
    if (fpsTimer >= 1.0f) {
        printf("FPS: %d | DT: %.6f\n", frames, dt);
        fpsTimer = 0; frames = 0;
    }
}

// --- DirectX 초기화 구현 (에러 체크 강화) ---
bool InitDirectX(DXContext* ctx) {
    DXGI_SWAP_CHAIN_DESC sd = { 0 };
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = ctx->hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0,
        D3D11_SDK_VERSION, &sd, &ctx->swapChain, &ctx->device, NULL, &ctx->context);
    if (FAILED(hr)) return false;

    ID3D11Texture2D* bb;
    ctx->swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb);
    ctx->device->CreateRenderTargetView(bb, NULL, &ctx->rtv);
    bb->Release();

    const char* src = R"(
        cbuffer CB : register(b0) { float4 offset; }
        struct VI { float3 p : POSITION; float4 c : COLOR; };
        struct PI { float4 p : SV_POSITION; float4 c : COLOR; };
        PI VS(VI i) { PI o; o.p = float4(i.p + offset.xyz, 1.0); o.c = i.c; return o; }
        float4 PS(PI i) : SV_Target { return i.c; }
    )";
    ID3DBlob* vs, * ps;
    D3DCompile(src, strlen(src), NULL, NULL, NULL, "VS", "vs_4_0", 0, 0, &vs, NULL);
    D3DCompile(src, strlen(src), NULL, NULL, NULL, "PS", "ps_4_0", 0, 0, &ps, NULL);
    ctx->device->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), NULL, &ctx->vShader);
    ctx->device->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), NULL, &ctx->pShader);

    D3D11_INPUT_ELEMENT_DESC ied[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
    ctx->device->CreateInputLayout(ied, 2, vs->GetBufferPointer(), vs->GetBufferSize(), &ctx->layout);
    D3D11_BUFFER_DESC cbd = { sizeof(ConstantBuffer), D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0 };
    ctx->device->CreateBuffer(&cbd, NULL, &ctx->cBuffer);

    return true;
}