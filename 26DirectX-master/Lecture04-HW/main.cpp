#include "Types.h"

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_DESTROY) PostQuitMessage(0);
    return DefWindowProc(hWnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    RECT rc = { 0, 0, 800, 600 };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    WNDCLASS wc = { 0, WndProc, 0, 0, hInst, 0, LoadCursor(NULL, IDC_ARROW), 0, 0, L"DXEngine" };
    RegisterClass(&wc);
    g_dx.hWnd = CreateWindow(L"DXEngine", L"Assignment: Player 1 (Arrows) / Player 2 (WASD)", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInst, NULL);
    ShowWindow(g_dx.hWnd, nShow);

    if (!InitDirectX(&g_dx)) return -1;

    // [B] 두 개의 서로 다른 삼각형 GameObject 생성
    std::vector<GameObject*> gameWorld;

    GameObject* p1 = new GameObject("Player1", -0.3f, 0.0f); // 왼쪽 시작
    p1->AddComponent(new TriangleRenderer(XMFLOAT4(1, 0, 0, 1))); // 빨간색
    p1->AddComponent(new Player1Control());
    gameWorld.push_back(p1);

    GameObject* p2 = new GameObject("Player2", 0.3f, 0.0f); // 오른쪽 시작
    p2->AddComponent(new TriangleRenderer(XMFLOAT4(0, 1, 0, 1))); // 초록색
    p2->AddComponent(new Player2Control());
    gameWorld.push_back(p2);

    GameObject* sys = new GameObject("System");
    sys->AddComponent(new VideoSystem());
    gameWorld.push_back(sys);

    // [A-1] 고해상도 타이머를 이용한 DeltaTime 계산
    auto prevTime = std::chrono::high_resolution_clock::now();
    MSG msg = { 0 };
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) { // Non-blocking
            TranslateMessage(&msg); DispatchMessage(&msg);
        }
        else {
            auto currTime = std::chrono::high_resolution_clock::now();
            float dt = std::chrono::duration<float>(currTime - prevTime).count();
            prevTime = currTime;

            float color[] = { 0.1f, 0.1f, 0.1f, 1.0f };
            g_dx.context->ClearRenderTargetView(g_dx.rtv, color);
            g_dx.context->OMSetRenderTargets(1, &g_dx.rtv, NULL);

            D3D11_VIEWPORT vp = { 0, 0, 800, 600, 0, 1 };
            g_dx.context->RSSetViewports(1, &vp);
            g_dx.context->IASetInputLayout(g_dx.layout);
            g_dx.context->VSSetShader(g_dx.vShader, NULL, 0);
            g_dx.context->VSSetConstantBuffers(0, 1, &g_dx.cBuffer);
            g_dx.context->PSSetShader(g_dx.pShader, NULL, 0);
            g_dx.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            // 일괄 업데이트 및 렌더링
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

    for (auto obj : gameWorld) delete obj; // 메모리 해제
    return 0;
}