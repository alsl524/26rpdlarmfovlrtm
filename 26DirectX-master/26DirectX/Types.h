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

// [구조체 정의]
struct Vertex {
    float x, y, z;
    float r, g, b, a;
};

struct ConstantBuffer {
    XMFLOAT4 offset;
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
    float posX = 0, posY = 0;
    int Width = 800;
    int Height = 600;
    bool IsFullscreen = false;
    bool NeedsResize = false;
};

// [엔진 코어 클래스]
class Component {
public:
    class GameObject* pOwner;
    bool isStarted = false;
    virtual void Start(DXContext* ctx) {}
    virtual void OnInput(DXContext* ctx) {}
    virtual void OnUpdate(DXContext* ctx, float dt) {}
    virtual void OnRender(DXContext* ctx) {}
    virtual ~Component() {}
};

class GameObject {
public:
    std::string name;
    std::vector<Component*> components;
    GameObject(std::string n) : name(n) {}
    ~GameObject() { for (auto c : components) delete c; }
    void AddComponent(Component* pComp) {
        pComp->pOwner = this;
        components.push_back(pComp);
    }
};

// [구현부 클래스 선언]
class HeartRenderer : public Component {
    ID3D11Buffer* vBuffer = nullptr;
    int vertexCount = 0;
public:
    void Start(DXContext* ctx) override;
    void OnRender(DXContext* ctx) override;
    ~HeartRenderer();
};

class PlayerControl : public Component {
public:
    void OnUpdate(DXContext* ctx, float dt) override;
};

class InfoDisplay : public Component {
    float fpsTimer = 0;
    int frames = 0;
public:
    void OnUpdate(DXContext* ctx, float dt) override;
};

//비디오 제어용 컴포넌트 선언
class VideoSystem : public Component {
public:
    void OnUpdate(DXContext* ctx, float dt) override;
    void RebuildVideoResources(DXContext* ctx); // 리소스를 새로 만드는 핵심 함수
};

// 전역 변수 및 초기화 함수 선언 (실체는 Framework.cpp에 있음)
extern DXContext g_dx;
bool InitDirectX(DXContext* ctx);