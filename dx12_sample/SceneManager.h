#pragma once

#include <utils/RenderTargetManager.h>
#include <utils/ComputePipelineState.h>
#include <utils/GraphicsPipelineState.h>
#include <utils/MeshManager.h>
#include <utils/RootSignature.h>
#include <utils/SceneObject.h>
#include <utils/CommandList.h>
#include <utils/Types.h>
#include <utils/SphericalCamera.h>

#include "stdafx.h"

class SceneManager
{
public:
    using SceneObjectPtr = std::shared_ptr<SceneObject>;

    SceneManager(ComPtr<ID3D12Device> pDevice,
                 UINT screenWidth,
                 UINT screenHeight,
                 CommandLineOptions cmdLineOpts,
                 ComPtr<ID3D12DescriptorHeap> pTexturesHeap,
                 ComPtr<ID3D12CommandQueue> pCmdQueue,
                 ComPtr<IDXGISwapChain3> pSwapChain,
                 RenderTargetManager * rtManager,
                 size_t objectOnSceneInRow);
    ~SceneManager();

    SceneManager(const SceneManager&) = delete;
    SceneManager(SceneManager&&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;
    SceneManager& operator=(SceneManager&&) = delete;

    void SetTextures(ComPtr<ID3D12Resource> pTexture[6]); // ugly and terrible

    SceneObjectPtr CreateFilledCube();
    SceneObjectPtr CreateOpenedCube();
    SceneObjectPtr CreatePlane();

    void DrawAll();

    // only for texture creation!
    void ExecuteCommandLists(const CommandList & commandList);

    Graphics::SphericalCamera * GetViewCamera();
    Graphics::SphericalCamera * GetShadowCamera();

private:
    void ThreadDrawRoutine(size_t threadId);
    void FinishWorkerThreads();

    void CreateCommandLists();
    void CreateConstantBuffer(size_t bufferSize, ComPtr<ID3D12Resource> * pOutBuffer);
    void CreateFrameConstantBuffers();
    void CreateMRTPassRootSignature();
    void CreateLightPassRootSignature();
    void CreateDepthPassRootSignature();
    void CreateLDRPassRootSignature();
    void CreateShadersAndPSOs();
    void CreateDepthPassPSO();
    void CreateLDRPassPSO();
    void CreateLightPassPSO();
    void CreateMRTPassPSO();
    void CreateRootSignatures();
    void CreateRenderTargets();

    void FillViewProjMatrix();
    void FillSceneProperties();

    void PopulateDepthPassCommandList();
    void PopulateWorkerCommandLists();
    void PopulateClearPassCommandList();
    void PopulateLightPassCommandList();

    void WaitCurrentFrame();

    std::unique_ptr<MeshManager>                _meshManager = nullptr;

    // objects vault
    std::vector<SceneObjectPtr>                 _objects {};
    std::unique_ptr<SceneObject>                _objScreenQuad = nullptr;

    // context objects
    ComPtr<ID3D12Device>                        _device = nullptr;
    ComPtr<ID3D12CommandQueue>                  _cmdQueue = nullptr;
    ComPtr<IDXGISwapChain3>                     _swapChain = nullptr;

    // command-lists
    std::unique_ptr<CommandList>                _clearPassCmdList = nullptr;
    std::unique_ptr<CommandList>                _depthPassCmdList = nullptr;
    std::unique_ptr<CommandList>                _lightPassCmdList = nullptr;
    std::vector<std::unique_ptr<CommandList>>   _workerCmdLists {};

    // pipeline states for every object
    std::unique_ptr<GraphicsPipelineState>      _depthPassState = nullptr;
    std::unique_ptr<GraphicsPipelineState>      _mrtPipelineState = nullptr;
    std::unique_ptr<GraphicsPipelineState>      _lightPassState = nullptr;
    std::unique_ptr<GraphicsPipelineState>      _LDRPassState = nullptr;
    std::unique_ptr<ComputePipelineState>       _IntensityPassState = nullptr;

    // sync primitives
    HANDLE                                      _frameEndEvent = nullptr;
    uint64_t                                    _fenceValue = 0;
    uint32_t                                    _frameIndex = 0;
    ComPtr<ID3D12Fence>                         _frameFence = nullptr;

    // multithreading objects
    std::condition_variable                     _workerBeginCV {};
    std::condition_variable                     _workerEndCV {};
    bool                                        _workerThreadExit = false;
    size_t                                      _pendingWorkers = 0;
    std::mutex                                  _workerMutex {};
    uint32_t                                    _drawObjectIndex = ~0x0;
    std::vector<std::thread>                    _threadPool {};

    // root signatures
    RootSignature                               _depthPassRootSignature;
    RootSignature                               _MRTRootSignature;
    RootSignature                               _lightRootSignature;
    RootSignature                               _LDRRootSignature;
    RootSignature                               _computePassRootSignature;

    // cbvs
    ComPtr<ID3D12Resource>                      _cbvMrtFrameParams = nullptr;
    ComPtr<ID3D12Resource>                      _cbvSceneParams = nullptr;
    ComPtr<ID3D12Resource>                      _cbvDepthFrameParams = nullptr;
    void *                                      _cbvMRT = nullptr;
    void *                                      _cbvScene = nullptr;
    void *                                      _cbvDepth = nullptr;

    // other objects from outside
    CommandLineOptions                          _cmdLineOpts {};
    UINT                                        _screenWidth = 0;
    UINT                                        _screenHeight = 0;
    std::vector<std::shared_ptr<RenderTarget>>  _swapChainRTs;
    ComPtr<ID3D12Resource>                      _texture[6] = {};
    ComPtr<ID3D12Resource>                      _intermediateIntensityBuffer = nullptr;
    ComPtr<ID3D12Resource>                      _finalIntensityBuffer = nullptr;
    Graphics::SphericalCamera                   _viewCamera;
    Graphics::SphericalCamera                   _shadowCamera;
    RenderTargetManager *                       _rtManager = nullptr;
    ComPtr<ID3D12DescriptorHeap>                _texturesHeap = nullptr;
    ComPtr<ID3D12DescriptorHeap>                _customsHeap = nullptr;

    std::shared_ptr<RenderTarget>               _mrtRts[3]; // diffuse, normals, depth
    std::shared_ptr<RenderTarget>               _HDRRt;
    std::shared_ptr<DepthStencil>               _mrtDepth;
    std::shared_ptr<DepthStencil>               _shadowDepth;

    void CreateIntensityPassPSO();
    void CreateIntensityPassRootSignature();
};
