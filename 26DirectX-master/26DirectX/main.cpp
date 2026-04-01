#include "Types.h"

// [주의] Framework.cpp를 include 하지 마세요. 
// Visual Studio 프로젝트에 추가만 되어 있으면 됩니다.

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_KEYDOWN: // 키가 눌렸을 때
        if (wp == VK_ESCAPE) { // 그게 ESC라면
            DestroyWindow(hWnd); // 창을 닫아라 (결국 WM_DESTROY로 이어짐)
        }
        return 0;
    }
    return DefWindowProc(hWnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    AllocConsole();
    FILE* pFile;
    freopen_s(&pFile, "CONOUT$", "w", stdout);

    RECT rc = { 0, 0, 800, 600 };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    WNDCLASS wc = { 0, WndProc, 0, 0, hInst, 0, LoadCursor(NULL, IDC_ARROW), 0, 0, L"DXHeart" };
    RegisterClass(&wc);
    g_dx.hWnd = CreateWindow(L"DXHeart", L"DX11 Heart Component Engine", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInst, NULL);

    if (!g_dx.hWnd) return -1;
    ShowWindow(g_dx.hWnd, nShow);

    // DirectX 초기화 확인 (여기서 실패하면 즉시 종료하여 nullptr 참조 방지)
    if (!InitDirectX(&g_dx)) {
        MessageBox(NULL, L"DirectX 초기화 실패!", L"에러", MB_OK);
        return -1;
    }

    // 게임 월드 구성
    std::vector<GameObject*> gameWorld;
    GameObject* videoManager = new GameObject("VideoManager");
    videoManager->AddComponent(new VideoSystem());
    gameWorld.push_back(videoManager);

    GameObject* player = new GameObject("PlayerHeart");
    player->AddComponent(new HeartRenderer());
    player->AddComponent(new PlayerControl());
    gameWorld.push_back(player);

    GameObject* system = new GameObject("SystemInfo");
    system->AddComponent(new InfoDisplay());
    gameWorld.push_back(system);

    auto prevTime = std::chrono::high_resolution_clock::now();
    MSG msg = { 0 };
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg); DispatchMessage(&msg);
        }
        else {
            auto currTime = std::chrono::high_resolution_clock::now();
            float dt = std::chrono::duration<float>(currTime - prevTime).count();
            prevTime = currTime;

            // 렌더링 시작
            float color[] = { 0.1f, 0.1f, 0.1f, 1.0f };
            g_dx.context->ClearRenderTargetView(g_dx.rtv, color);
            g_dx.context->OMSetRenderTargets(1, &g_dx.rtv, NULL);

            D3D11_VIEWPORT vp = { 0.0f, 0.0f, (float)g_dx.Width, (float)g_dx.Height, 0.0f, 1.0f };
            g_dx.context->RSSetViewports(1, &vp);
            g_dx.context->IASetInputLayout(g_dx.layout);
            g_dx.context->VSSetShader(g_dx.vShader, NULL, 0);
            g_dx.context->VSSetConstantBuffers(0, 1, &g_dx.cBuffer);
            g_dx.context->PSSetShader(g_dx.pShader, NULL, 0);

            // 컴포넌트 업데이트 및 렌더링
            for (auto obj : gameWorld) {
                for (auto comp : obj->components) {
                    if (!comp->isStarted) { comp->Start(&g_dx); comp->isStarted = true; }
                    comp->OnUpdate(&g_dx, dt);
                    comp->OnRender(&g_dx);
                }
            }
            g_dx.swapChain->Present(1, 0);
        }
    }

    for (auto obj : gameWorld) delete obj;
    return 0;
}