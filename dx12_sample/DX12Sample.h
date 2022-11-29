#pragma once

#include "stdafx.h"

#include "DXSample.h"
#include "SceneManager.h"

#include <utils/SceneObject.h>
#include <utils/InputManager.h>

class DX12Sample final :
    public DXSample
{
public:
    DX12Sample(int windowWidth, int windowHeight, std::set<optTypes>& opts, const char* mesh_name);
    ~DX12Sample() override;

    virtual void OnInit() override;
    virtual void OnUpdate() override;
    virtual void OnRender() override;
    virtual void OnDestroy() override;
    virtual bool OnEvent(MSG msg) override;

private:
    void CreateDXGIFactory();
    void CreateDevice();

    void CreateTexturesHeap();
    void CreateSwapChain();
    void CreateCommandQueue();
    void CreateObjects();
    void CreateTextures();

    void DumpFeatures();

    std::unique_ptr<InputManager>               _inputManager = nullptr;
    std::unique_ptr<SceneManager>               _sceneManager = nullptr;
    std::unique_ptr<RenderTargetManager>        _RTManager = nullptr;

    CommandLineOptions                          _cmdLineOpts {};

    // Main sample parameters
#ifdef _DEBUG
    static constexpr size_t                     _objectsInRow = 5;
#else
    static constexpr size_t                     _objectsInRow = 10;
#endif
    static constexpr float                      _objDistance = 1.0f;
    static constexpr size_t                     _drawObjectsCount = _objectsInRow * _objectsInRow * _objectsInRow;
    static constexpr size_t                     _swapChainBuffersCount = 2;
    bool                                        _cursorLock = false;

    ComPtr<ID3D12Device>                        _device = nullptr;
    ComPtr<IDXGIFactory4>                       _DXFactory = nullptr;
    ComPtr<ID3D12CommandQueue>                  _cmdQueue = nullptr;
    ComPtr<IDXGISwapChain3>                     _swapChain = nullptr;

    // Geometry meshes
    std::vector<std::shared_ptr<SceneObject>>   _objects;
    std::shared_ptr<SceneObject>                _plane = nullptr;

    ComPtr<ID3D12DescriptorHeap>                _texturesHeap = nullptr;
    std::array<ComPtr<ID3D12Resource>, 6>       _texture = {};
     const char*                                 _mesh_name;
	 double                                     _multiplier = 1.0f;
};
