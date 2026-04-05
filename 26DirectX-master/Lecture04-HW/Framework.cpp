#include "Types.h"

DXContext g_dx = { 0 };

// [B] 삼각형 렌더러 구현
void TriangleRenderer::Start(DXContext* ctx) {
    Vertex vertices[] = {
        { { 0.0f,  0.15f, 0.5f }, color },
        { { 0.15f, -0.15f, 0.5f }, color },
        { { -0.15f, -0.15f, 0.5f }, color }
    };
    D3D11_BUFFER_DESC bd = { sizeof(vertices), D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
    D3D11_SUBRESOURCE_DATA sd = { vertices, 0, 0 };
    ctx->device->CreateBuffer(&bd, &sd, &vBuffer);
}

void TriangleRenderer::OnRender(DXContext* ctx) {
    // GameObject의 현재 위치를 ConstantBuffer에 업데이트
    ConstantBuffer cb = { XMFLOAT4(pOwner->position.x, pOwner->position.y, 0, 0) };
    ctx->context->UpdateSubresource(ctx->cBuffer, 0, NULL, &cb, 0, 0);

    UINT stride = sizeof(Vertex), offset = 0;
    ctx->context->IASetVertexBuffers(0, 1, &vBuffer, &stride, &offset);
    ctx->context->Draw(3, 0);
}

TriangleRenderer::~TriangleRenderer() { if (vBuffer) vBuffer->Release(); }

// [A-1] 프레임 독립적 이동 공식 적용: P = P + (V * dt)
void Player1Control::OnUpdate(DXContext* ctx, float dt) {
    float velocity = 1.0f; // 초당 이동 속도
    if (GetAsyncKeyState(VK_UP) & 0x8000)    pOwner->position.y += velocity * dt;
    if (GetAsyncKeyState(VK_DOWN) & 0x8000)  pOwner->position.y -= velocity * dt;
    if (GetAsyncKeyState(VK_LEFT) & 0x8000)  pOwner->position.x -= velocity * dt;
    if (GetAsyncKeyState(VK_RIGHT) & 0x8000) pOwner->position.x += velocity * dt;
}

void Player2Control::OnUpdate(DXContext* ctx, float dt) {
    float velocity = 1.0f;
    if (GetAsyncKeyState('W') & 0x8000) pOwner->position.y += velocity * dt;
    if (GetAsyncKeyState('S') & 0x8000) pOwner->position.y -= velocity * dt;
    if (GetAsyncKeyState('A') & 0x8000) pOwner->position.x -= velocity * dt;
    if (GetAsyncKeyState('D') & 0x8000) pOwner->position.x += velocity * dt;
}

// [B] 시스템 제어 (ESC, F 키)
void VideoSystem::OnUpdate(DXContext* ctx, float dt) {
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) PostQuitMessage(0);
    if (GetAsyncKeyState('F') & 0x0001) {
        ctx->IsFullscreen = !ctx->IsFullscreen;
        ctx->swapChain->SetFullscreenState(ctx->IsFullscreen, NULL);
    }
}

bool InitDirectX(DXContext* ctx) {
    DXGI_SWAP_CHAIN_DESC sd = { 0 };
    sd.BufferCount = 1;
    sd.BufferDesc.Width = ctx->Width;
    sd.BufferDesc.Height = ctx->Height;
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