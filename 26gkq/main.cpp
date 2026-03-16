#include <windows.h>      // Windows API
#include <d3d11.h>        // DirectX 11
#include <d3dcompiler.h>  // 셰이더 컴파일
#include <directxmath.h>  // 수학 라이브러리 (벡터, 행렬 등)

using namespace DirectX;

// DirectX 라이브러리 자동 링크
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// ======================================================
// GPU에 전달할 상수 버퍼 구조체
// CPU → GPU 로 전달되는 데이터
// ======================================================
struct ConstantBuffer {
    // 삼각형을 이동시키기 위한 좌표값
    // DirectX 상수버퍼는 16바이트 정렬이 필요하므로 float4 사용
    XMFLOAT4 offset;
};

// ======================================================
// DirectX 관련 객체들과 게임 상태를 저장하는 구조체
// ======================================================
struct DXContext {

    // 윈도우 핸들
    HWND hWnd;

    // 프로그램 실행 여부
    int isRunning;

    // DirectX 핵심 객체
    ID3D11Device* device;             // GPU 장치
    ID3D11DeviceContext* context;     // GPU 명령 실행
    IDXGISwapChain* swapChain;        // 화면 버퍼 교체
    ID3D11RenderTargetView* rtv;      // 화면에 출력할 대상

    // 셰이더
    ID3D11VertexShader* vShader;      // 정점 셰이더
    ID3D11PixelShader* pShader;       // 픽셀 셰이더

    // 입력 레이아웃
    ID3D11InputLayout* layout;

    // 버퍼
    ID3D11Buffer* vBuffer;            // 정점 버퍼
    ID3D11Buffer* cBuffer;            // 상수 버퍼

    // 삼각형 위치 정보
    float posX, posY;

    // 이동 속도
    float moveSpeed;
};

// ======================================================
// 정점(Vertex) 구조체
// GPU에 전달되는 정점 데이터
// ======================================================
struct Vertex {

    // 위치 좌표
    float x, y, z;

    // 색상
    float r, g, b, a;
};

// 전역 DirectX 컨텍스트
DXContext g_dx = { 0 };

// --- 1. 입력 단계 (Process Input) ---
void ProcessInput(DXContext* ctx) {
    MSG msg = { 0 };
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) ctx->isRunning = 0;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

// --- 2. 업데이트 단계 (Update) ---
// ======================================================
// 게임 상태 업데이트
// ======================================================
void Update(DXContext* ctx)
{
    // 화면 경계 제한 (NDC 좌표 -1 ~ 1)

    if (ctx->posX < -1.0f) ctx->posX = -1.0f;
    if (ctx->posX > 1.0f)  ctx->posX = 1.0f;

    if (ctx->posY < -1.0f) ctx->posY = -1.0f;
    if (ctx->posY > 1.0f)  ctx->posY = 1.0f;

    // GPU에 전달할 상수버퍼 생성
    ConstantBuffer cb;

    // 현재 위치를 offset에 저장
    cb.offset = XMFLOAT4(ctx->posX, ctx->posY, 0.0f, 0.0f);

    // 상수버퍼 업데이트 (CPU → GPU)
    ctx->context->UpdateSubresource(ctx->cBuffer, 0, nullptr, &cb, 0, 0);
}

// --- 3. 출력 단계 (Render) ---
// ======================================================
// 화면 그리기(Render)
// ======================================================
void Render(DXContext* ctx)
{
    // 화면 배경 색
    float clearColor[] = { 0.2f, 0.3f, 0.4f, 1.0f };

    // 화면 초기화
    ctx->context->ClearRenderTargetView(ctx->rtv, clearColor);

    // 출력 대상 설정
    ctx->context->OMSetRenderTargets(1, &ctx->rtv, nullptr);

    // 화면 영역 설정
    D3D11_VIEWPORT vp = { 0,0,800,600,0.0f,1.0f };
    ctx->context->RSSetViewports(1, &vp);

    // 입력 레이아웃 설정
    ctx->context->IASetInputLayout(ctx->layout);

    // 정점 버퍼 설정
    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    ctx->context->IASetVertexBuffers(0, 1, &ctx->vBuffer, &stride, &offset);

    // 삼각형 그리기 설정
    ctx->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 셰이더 설정
    ctx->context->VSSetShader(ctx->vShader, nullptr, 0);

    // 상수 버퍼 연결
    ctx->context->VSSetConstantBuffers(0, 1, &ctx->cBuffer);

    ctx->context->PSSetShader(ctx->pShader, nullptr, 0);

    // 정점 6개로 삼각형 2개 그리기
    ctx->context->Draw(6, 0);

    // 화면 출력
    ctx->swapChain->Present(1, 0);
}

// ======================================================
// 메시지 처리 (키 입력 / 창 종료 등)
// ======================================================
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {

        // 키가 눌렸을 때
    case WM_KEYDOWN:

        // 어떤 키가 눌렸는지 확인
        switch (wParam)
        {
        case VK_LEFT:
            g_dx.posX -= g_dx.moveSpeed; // 왼쪽 이동
            break;

        case VK_RIGHT:
            g_dx.posX += g_dx.moveSpeed; // 오른쪽 이동
            break;

        case VK_UP:
            g_dx.posY += g_dx.moveSpeed; // 위쪽 이동
            break;

        case VK_DOWN:
            g_dx.posY -= g_dx.moveSpeed; // 아래쪽 이동
            break;
        }

        break;

        // 창이 닫힐 때
    case WM_DESTROY:

        // 프로그램 종료 메시지 발생
        PostQuitMessage(0);

        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

bool InitWindow(DXContext* ctx, HINSTANCE hInst);
bool InitDirectX(DXContext* ctx);
void Cleanup(DXContext* ctx);

// ======================================================
// 프로그램 시작점
// ======================================================
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow)
{

    // 윈도우 생성
    if (!InitWindow(&g_dx, hInst))
        return -1;

    // DirectX 초기화
    if (!InitDirectX(&g_dx))
        return -1;

    // 초기 위치
    g_dx.posX = 0.0f;
    g_dx.posY = 0.0f;

    // 이동 속도
    g_dx.moveSpeed = 0.005f;

    g_dx.isRunning = 1;

    // ==================================================
    // 게임 루프
    // ==================================================
    while (g_dx.isRunning)
    {

        // 입력 처리
        ProcessInput(&g_dx);

        // 게임 상태 업데이트
        Update(&g_dx);

        // 화면 출력
        Render(&g_dx);
    }

    // 종료 시 리소스 해제
    Cleanup(&g_dx);

    return 0;
}

bool InitWindow(DXContext* ctx, HINSTANCE hInst) {
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, hInst,
                        LoadCursor(NULL, IDC_ARROW), NULL, NULL, NULL, L"DXStarMove", NULL };
    RegisterClassEx(&wcex);
    ctx->hWnd = CreateWindow(L"DXStarMove", L"Move Star with Arrow Keys", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInst, NULL);
    if (!ctx->hWnd) return false;
    ShowWindow(ctx->hWnd, SW_SHOW);
    return true;
}

bool InitDirectX(DXContext* ctx) {
    // 1. Device, Context, SwapChain 생성 (이전과 동일)
    DXGI_SWAP_CHAIN_DESC sd = { 0 };
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = ctx->hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0,
        D3D11_SDK_VERSION, &sd, &ctx->swapChain, &ctx->device, NULL, &ctx->context);

    ID3D11Texture2D* backBuffer;
    ctx->swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    ctx->device->CreateRenderTargetView(backBuffer, NULL, &ctx->rtv);
    backBuffer->Release();

    // 2. 셰이더 작성 (중요: 상수 버퍼를 수용하도록 HLSL 수정)
    const char* src = R"(
        cbuffer ConstantBuffer : register(b0) {
            float4 offset;
        }
        struct VI { float3 p : POSITION; float4 c : COLOR; };
        struct PI { float4 p : SV_POSITION; float4 c : COLOR; };
        PI VS(VI i) {
            PI o;
            // 입력받은 정점 좌표에 offset(이동값)을 더해줍니다.
            o.p = float4(i.p + offset.xyz, 1.0);
            o.c = i.c;
            return o;
        }
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

    // 3. 정점 버퍼 (이전과 동일)
    Vertex vertices[] = {
        // [1] 뒤에 있는 검은색 삼각형 (역삼각형 ▽)
        // 밑변이 위로 가고, 꼭짓점이 아래를 향함
        {  0.0f,   -0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f }, // 아래 꼭짓점
        { -0.433f,  0.25f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f }, // 왼쪽 위
        {  0.433f,  0.25f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f }, // 오른쪽 위

        // [2] 앞에 있는 하얀색 삼각형 (정삼각형 △)
        // 꼭짓점이 위를 향함
        {  0.0f,    0.5f,  0.4f, 1.0f, 1.0f, 1.0f, 1.0f }, // 위 꼭짓점
        {  0.433f, -0.25f, 0.4f, 1.0f, 1.0f, 1.0f, 1.0f }, // 오른쪽 아래
        { -0.433f, -0.25f, 0.4f, 1.0f, 1.0f, 1.0f, 1.0f }  // 왼쪽 아래
    };
    D3D11_BUFFER_DESC bd = { sizeof(vertices), D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
    D3D11_SUBRESOURCE_DATA id = { vertices, 0, 0 };
    ctx->device->CreateBuffer(&bd, &id, &ctx->vBuffer);

    // 4. 상수 버퍼 생성 (새로 추가된 부분)
    D3D11_BUFFER_DESC cbd = { sizeof(ConstantBuffer), D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0 };
    ctx->device->CreateBuffer(&cbd, nullptr, &ctx->cBuffer);

    return true;
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
}