#pragma once
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <vector>
#include <string>
#include <chrono>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

struct Vertex {
    XMFLOAT3 pos;
    XMFLOAT4 color;
};

struct ConstantBuffer {
    XMFLOAT4 offset; // GameObject의 Position이 여기에 담겨 Shader로 전달됨
};

struct DXContext {
    HWND hWnd;
    ID3D11Device* device;
    ID3D11DeviceContext* context;
    IDXGISwapChain* swapChain;
    ID3D11RenderTargetView* rtv;
    ID3D11VertexShader* vShader;
    ID3D11PixelShader* pShader;
    ID3D11InputLayout* layout;
    ID3D11Buffer* cBuffer;
    int Width = 800;
    int Height = 600;
    bool IsFullscreen = false;
};

// [A-2] 추상 클래스 Component 설계
class Component {
public:
    class GameObject* pOwner;
    bool isStarted = false;
    virtual void Start(DXContext* ctx) {}
    virtual void OnUpdate(DXContext* ctx, float dt) {}
    virtual void OnRender(DXContext* ctx) {}
    virtual ~Component() {}
};

// [A-2] GameObject 설계 (위치 정보 포함)
class GameObject {
public:
    std::string name;
    XMFLOAT2 position = { 0.0f, 0.0f }; // [요구사항] 위치 정보 보유
    std::vector<Component*> components;

    GameObject(std::string n, float x = 0, float y = 0) : name(n), position(x, y) {}
    ~GameObject() { for (auto c : components) delete c; }

    void AddComponent(Component* pComp) {
        pComp->pOwner = this;
        components.push_back(pComp);
    }
};

// --- 컴포넌트 클래스 선언 ---
class TriangleRenderer : public Component {
    ID3D11Buffer* vBuffer = nullptr;
    XMFLOAT4 color;
public:
    TriangleRenderer(XMFLOAT4 col) : color(col) {}
    void Start(DXContext* ctx) override;
    void OnRender(DXContext* ctx) override;
    ~TriangleRenderer();
};

class Player1Control : public Component { // 방향키 조작
public:
    void OnUpdate(DXContext* ctx, float dt) override;
};

class Player2Control : public Component { // WASD 조력
public:
    void OnUpdate(DXContext* ctx, float dt) override;
};

class VideoSystem : public Component { // ESC, F 키 조작
public:
    void OnUpdate(DXContext* ctx, float dt) override;
};

extern DXContext g_dx;
bool InitDirectX(DXContext* ctx);