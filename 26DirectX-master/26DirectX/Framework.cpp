#include "Types.h"
#include <vector>
#include <cmath>

DXContext g_dx = { 0 };

bool InitWindow(HINSTANCE hInst, int width, int height) {
    // 1. WinMain 기반이지만 printf를 위해 콘솔창을 수동으로 엽니다.
    AllocConsole();
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);

    WNDCLASSEX wcex = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, hInst,
                        LoadCursor(NULL, IDC_ARROW), NULL, NULL, NULL, L"DXStarMove", NULL };
    RegisterClassEx(&wcex);

    // 2. 픽셀 손실 방지: 600x600이 실제 그림 그려지는 영역(Client)이 되도록 창 크기 계산
    RECT rc = { 0, 0, width, height };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    g_dx.hWnd = CreateWindow(L"DXStarMove", L"DirectX11 Hexagram (600x600)",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInst, NULL);

    if (!g_dx.hWnd) return false;
    ShowWindow(g_dx.hWnd, SW_SHOW);
    return true;
}

#include <vector>
#include <cmath>

bool InitDirectX(DXContext* ctx) {
    if (!ctx->hWnd) return false;

    // 1. 스왑 체인 설정
    DXGI_SWAP_CHAIN_DESC sd = { 0 };
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = ctx->hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    // 2. 디바이스 생성 및 성공 여부 확인
    HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0,
        D3D11_SDK_VERSION, &sd, &ctx->swapChain, &ctx->device, NULL, &ctx->context);

    if (FAILED(hr)) {
        MessageBox(ctx->hWnd, L"DirectX Device 생성 실패!", L"Error", MB_OK);
        return false;
    }

    // 3. 렌더 타겟 뷰 생성 (디바이스 생성 직후 수행)
    ID3D11Texture2D* backBuffer = nullptr;
    ctx->swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    ctx->device->CreateRenderTargetView(backBuffer, NULL, &ctx->rtv);
    backBuffer->Release();

    // 4. 하트 정점 데이터 생성
    std::vector<Vertex> baseVertices; // 테두리 점 보관용
    const int segments = 100;
    const float PI = 3.1415926535f;

    // 중심점 (모든 삼각형의 시작점 - 진한 빨강)
    baseVertices.push_back({ 0.0f, 0.0f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f });

    // 테두리 점 계산
    for (int i = 0; i <= segments; ++i) {
        float t = (i / (float)segments) * 2.0f * PI;
        float x = 16.0f * powf(sinf(t), 3.0f);
        float y = 13.0f * cosf(t) - 5.0f * cosf(2.0f * t) - 2.0f * cosf(3.0f * t) - cosf(4.0f * t);

        // 스케일 조정 (0.04f) 및 테두리 색상 (밝은 빨강)
        baseVertices.push_back({ x * 0.04f, y * 0.04f, 0.5f, 1.0f, 0.3f, 0.3f, 1.0f });
    }

    // TRIANGLELIST를 위한 데이터 재배치 (중심 - 점1 - 점2)
    std::vector<Vertex> triangleListVertices;
    for (int i = 1; i <= segments; ++i) {
        triangleListVertices.push_back(baseVertices[0]);     // 중심
        triangleListVertices.push_back(baseVertices[i]);     // 현재 테두리
        triangleListVertices.push_back(baseVertices[i + 1]); // 다음 테두리
    }

    // 5. 정점 버퍼 생성
    D3D11_BUFFER_DESC bd = { 0 };
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = (UINT)(sizeof(Vertex) * triangleListVertices.size());
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA id = { triangleListVertices.data(), 0, 0 };
    hr = ctx->device->CreateBuffer(&bd, &id, &ctx->vBuffer);
    if (FAILED(hr)) return false;

    // 6. 셰이더 컴파일 및 상수 버퍼 생성
    const char* shaderCode = R"(
        cbuffer CB : register(b0) { float4 offset; }
        struct VI { float3 p : POSITION; float4 c : COLOR; };
        struct PI { float4 p : SV_POSITION; float4 c : COLOR; };
        PI VS(VI i) { PI o; o.p = float4(i.p + offset.xyz, 1.0); o.c = i.c; return o; }
        float4 PS(PI i) : SV_Target { return i.c; }
    )";

    ID3DBlob* vsBlob, * psBlob;
    D3DCompile(shaderCode, strlen(shaderCode), NULL, NULL, NULL, "VS", "vs_4_0", 0, 0, &vsBlob, NULL);
    D3DCompile(shaderCode, strlen(shaderCode), NULL, NULL, NULL, "PS", "ps_4_0", 0, 0, &psBlob, NULL);

    ctx->device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), NULL, &ctx->vShader);
    ctx->device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), NULL, &ctx->pShader);

    D3D11_INPUT_ELEMENT_DESC ied[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
    ctx->device->CreateInputLayout(ied, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &ctx->layout);

    D3D11_BUFFER_DESC cbd = { sizeof(ConstantBuffer), D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0 };
    ctx->device->CreateBuffer(&cbd, NULL, &ctx->cBuffer);

    return true;
}

void ProcessInput(DXContext* ctx) {
    MSG msg = { 0 };
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) ctx->isRunning = 0;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void Update(DXContext* ctx, float dt) {
    // 1. WASD 입력에 따른 이동
    if (ctx->keys['A']) ctx->posX -= ctx->moveSpeed * dt;
    if (ctx->keys['D']) ctx->posX += ctx->moveSpeed * dt;
    if (ctx->keys['W']) ctx->posY += ctx->moveSpeed * dt;
    if (ctx->keys['S']) ctx->posY -= ctx->moveSpeed * dt;

    // 2. 화면 경계 제한 (Clamp)
    // 하트의 가로/세로 폭을 고려하여 약 0.8 정도로 제한하면 창 밖으로 완전히 나가지 않습니다.
    float limitX = 0.85f;
    float limitY = 0.85f;

    if (ctx->posX < -limitX) ctx->posX = -limitX;
    if (ctx->posX > limitX) ctx->posX = limitX;
    if (ctx->posY < -limitY) ctx->posY = -limitY;
    if (ctx->posY > limitY) ctx->posY = limitY;

    // 3. 상수 버퍼 업데이트
    ConstantBuffer cb;
    cb.offset = XMFLOAT4(ctx->posX, ctx->posY, 0.0f, 0.0f);
    ctx->context->UpdateSubresource(ctx->cBuffer, 0, nullptr, &cb, 0, 0);
}

void DisplayConsoleInfo(DXContext* ctx, float dt) {
    ctx->fpsTimer += dt;
    ctx->frameCount++;

    // 1. 매 프레임 dt 출력 (\r을 사용하여 한 줄에서 갱신함으로써 시간 손실 및 콘솔 도배 방지)
    printf("DeltaTime: %.6f s\r", dt);

    // 2. 1초마다 FPS 출력
    if (ctx->fpsTimer >= 1.0f) {
        float fps = (float)ctx->frameCount / ctx->fpsTimer;
        printf("\n--- Engine Heartbeat | FPS: %.2f (Interval: 1s) ---\n", fps);
        ctx->fpsTimer = 0.0f;
        ctx->frameCount = 0;
    }
}

void Render(DXContext* ctx) {
    float clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    ctx->context->ClearRenderTargetView(ctx->rtv, clearColor);
    ctx->context->OMSetRenderTargets(1, &ctx->rtv, nullptr);

    D3D11_VIEWPORT vp = { 0, 0, 600, 600, 0.0f, 1.0f };
    ctx->context->RSSetViewports(1, &vp);

    ctx->context->IASetInputLayout(ctx->layout);
    UINT stride = sizeof(Vertex), offset = 0;
    ctx->context->IASetVertexBuffers(0, 1, &ctx->vBuffer, &stride, &offset);

    // 삼각형 리스트 방식으로 설정
    ctx->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ctx->context->VSSetShader(ctx->vShader, nullptr, 0);
    ctx->context->VSSetConstantBuffers(0, 1, &ctx->cBuffer);
    ctx->context->PSSetShader(ctx->pShader, nullptr, 0);

    // 정점 개수: segments(100) * 3 = 300개
    // 만약 segments를 변수로 관리한다면 ctx 구조체에 저장해서 사용하는 것이 좋습니다.
    ctx->context->Draw(300, 0);

    ctx->swapChain->Present(1, 0);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_KEYDOWN: g_dx.keys[wParam] = true; break;
    case WM_KEYUP:   g_dx.keys[wParam] = false; break;
    case WM_DESTROY: PostQuitMessage(0); break;
    default: return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

void Cleanup(DXContext* ctx) {
    if (ctx->cBuffer) ctx->cBuffer->Release();
    if (ctx->vBuffer) ctx->vBuffer->Release();
    if (ctx->layout) ctx->layout->Release();
    if (ctx->vShader) ctx->vShader->Release();
    if (ctx->pShader) ctx->pShader->Release();
    if (ctx->rtv) ctx->rtv->Release();
    if (ctx->swapChain) ctx->swapChain->Release();
    if (ctx->context) ctx->context->Release();
    if (ctx->device) ctx->device->Release();
    FreeConsole(); // 콘솔 해제
}