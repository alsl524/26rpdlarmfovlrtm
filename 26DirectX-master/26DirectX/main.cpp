#include "Types.h"

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    // 1. 초기화
    if (!InitWindow(hInst, 600, 600)) return -1;
    if (!InitDirectX(&g_dx)) return -1;

    g_dx.posX = 0.0f;
    g_dx.posY = 0.0f;
    g_dx.moveSpeed = 1.0f;
    g_dx.isRunning = 1;
    g_dx.fpsTimer = 0.0f;
    g_dx.frameCount = 0;
    g_dx.prevTime = std::chrono::steady_clock::now();

    // 2. WinMain 기반 GameLoop
    while (g_dx.isRunning) {
        // DeltaTime 측정 시작
        auto currentTime = std::chrono::steady_clock::now();
        std::chrono::duration<float> elapsed = currentTime - g_dx.prevTime;
        float dt = elapsed.count();
        g_dx.prevTime = currentTime;

        ProcessInput(&g_dx);
        Update(&g_dx, dt);

        // 정보 출력 (dt는 매 프레임, FPS는 1초마다)
        DisplayConsoleInfo(&g_dx, dt);

        Render(&g_dx);

        // ESC 종료 처리
        if (g_dx.keys[VK_ESCAPE]) g_dx.isRunning = 0;
    }

    // 3. 종료
    Cleanup(&g_dx);
    return 0;
}