#include "stdafx.h"

#include "SceneManager.h"

#include <utils/RenderTargetManager.h>
#include <utils/Shaders.h>

constexpr float clearColor[] = {0.0f, 0.4f, 0.7f, 1.0f};
constexpr int depthMapSize = 2048;

D3D12_INPUT_ELEMENT_DESC defaultGeometryInputElements[] =
{
    /* semantic name, semantics count, format,     ? , offset, vertex or instance , ?*/
    {"POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"BINORMAL",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
};

D3D12_INPUT_ELEMENT_DESC screenQuadInputElements[] =
{
    /* semantic name, semantics count, format,     ? , offset, vertex or instance , ?*/
    {"POSITION",  0, DXGI_FORMAT_R32G32_FLOAT,    0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0,  8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
};

#define M_PI 3.14159265f

SceneManager::SceneManager(ComPtr<ID3D12Device> pDevice,
                           UINT screenWidth,
                           UINT screenHeight,
                           CommandLineOptions cmdLineOpts,
                           ComPtr<ID3D12DescriptorHeap> pTexturesHeap,
                           ComPtr<ID3D12CommandQueue> pCmdQueue,
                           ComPtr<IDXGISwapChain3> pSwapChain,
                           RenderTargetManager * rtManager,
                           size_t objectOnSceneInRow)
    : _device(pDevice)
    , _screenWidth(screenWidth)
    , _screenHeight(screenHeight)
    , _cmdLineOpts(cmdLineOpts)
    , _texturesHeap(pTexturesHeap)
    , _cmdQueue(pCmdQueue)
    , _swapChain(pSwapChain)
    , _frameIndex(pSwapChain->GetCurrentBackBufferIndex())
    , _swapChainRTs(_swapChainRTs)
    , _rtManager(rtManager)
    , _viewCamera(Graphics::ProjectionType::Perspective,
                  0.1f,
                  objectOnSceneInRow * 5.0f,
                  5.0f * M_PI / 18.0f,
                  static_cast<float>(screenWidth),
                  static_cast<float>(screenHeight))
    , _shadowCamera(Graphics::ProjectionType::Perspective,
                    1.0f,
                    objectOnSceneInRow * 5.0f,
                    9.0f * M_PI / 18.0f,
                    depthMapSize / depthMapSize * objectOnSceneInRow * 3.0f,
                    depthMapSize / depthMapSize * objectOnSceneInRow * 3.0f)
{
    assert(pDevice);
    assert(pTexturesHeap);
    assert(pCmdQueue);
    assert(pSwapChain);
    assert(rtManager);

    _meshManager.reset(new MeshManager(cmdLineOpts.tessellation, pDevice));

    _viewCamera.SetCenter({0.0f, 0.0f, 0.0f});
    _viewCamera.SetRadius((float)objectOnSceneInRow);

    _shadowCamera.SetCenter({0.0f, 0.0f, 0.0f});
    _shadowCamera.SetRadius(objectOnSceneInRow * 2.0f);

    if (cmdLineOpts.threads)
    {
        unsigned concurrent_threads_count = std::thread::hardware_concurrency();
        if (concurrent_threads_count == 0) // unable to detect
            concurrent_threads_count = 8;

        // create thread pool
        _threadPool.resize(concurrent_threads_count);
        for (size_t tid = 0; tid < concurrent_threads_count; ++tid)
            _threadPool[tid] = std::thread([this, tid] {ThreadDrawRoutine(tid); });
    }

    // Have to create Fence and Event.
    ThrowIfFailed(pDevice->CreateFence(0,
                                       D3D12_FENCE_FLAG_NONE,
                                       IID_PPV_ARGS(&_frameFence)));
    _frameEndEvent = CreateEvent(NULL, FALSE, FALSE, nullptr);

    CreateRenderTargets();
    CreateRootSignatures();
    CreateShadersAndPSOs();
    CreateCommandLists();
    CreateFrameConstantBuffers();
    PopulateClearPassCommandList();

    std::vector<screenQuadVertex> screenQuadVertices =
    {
        {{-1.0f,  1.0f}, {0.0f, 0.0f}},
        {{1.0f,  1.0f}, {1.0f, 0.0f}},
        {{1.0f, -1.0f}, {1.0f, 1.0f}},
        {{-1.0f,  1.0f}, {0.0f, 0.0f}},
        {{1.0f, -1.0f}, {1.0f, 1.0f}},
        {{-1.0f, -1.0f}, {0.0f, 1.0f}},
    };

    _objScreenQuad = std::make_unique<SceneObject>(_meshManager->CreateScreenQuad(), pDevice, nullptr);

    // Create Heap for the Texture
    D3D12_DESCRIPTOR_HEAP_DESC texHeapDesc = {};
    texHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    texHeapDesc.NumDescriptors = 100;
    texHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(pDevice->CreateDescriptorHeap(&texHeapDesc, IID_PPV_ARGS(&_customsHeap)));

    {
        D3D12_CPU_DESCRIPTOR_HANDLE customsHeapHandle = _customsHeap->GetCPUDescriptorHandleForHeapStart();
        UINT CbvSrvUavHeapIncSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_HEAP_PROPERTIES defaultHeapProp = {D3D12_HEAP_TYPE_DEFAULT};

        // *** RTV's ***
        // Now we need to prepare SRV for RT's
        for (int i = 0; i < 3; ++i)
        {
            pDevice->CreateShaderResourceView(_mrtRts[i]->_texture.Get(), nullptr, customsHeapHandle);
            customsHeapHandle.ptr += CbvSrvUavHeapIncSize;
        }

        if (cmdLineOpts.shadow_pass)
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC dsDesc = {};
            dsDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
            dsDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            dsDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            dsDesc.Texture2D.MipLevels = 1;
            dsDesc.Texture2D.MostDetailedMip = 0;

            pDevice->CreateShaderResourceView(_shadowDepth->_texture.Get(), &dsDesc, customsHeapHandle);
            customsHeapHandle.ptr += CbvSrvUavHeapIncSize;
        }

        // for HDR -> LDR pass
        // Intensity computing pass 1
        // translate screen quad colored into intensity buffer
        pDevice->CreateShaderResourceView(_HDRRt->_texture.Get(), nullptr, customsHeapHandle);
        customsHeapHandle.ptr += CbvSrvUavHeapIncSize;

        D3D12_RESOURCE_DESC intermediateBufferDesc = {};
        intermediateBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        intermediateBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        intermediateBufferDesc.Width = screenHeight * sizeof(float) * 2; // screen quad height * 2 (average + max luma)
        intermediateBufferDesc.Height = 1;
        intermediateBufferDesc.MipLevels = 1;
        intermediateBufferDesc.SampleDesc.Count = 1;
        intermediateBufferDesc.DepthOrArraySize = 1;
        intermediateBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        pDevice->CreateCommittedResource(&defaultHeapProp,
                                         D3D12_HEAP_FLAG_NONE,
                                         &intermediateBufferDesc,
                                         D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                                         {},
                                         IID_PPV_ARGS(&_intermediateIntensityBuffer));
        _intermediateIntensityBuffer->SetName(L"IntemediateIntensity");

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.NumElements = screenHeight;
        uavDesc.Buffer.StructureByteStride = sizeof(float) * 2;
        uavDesc.Buffer.CounterOffsetInBytes = 0;
        uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
        pDevice->CreateUnorderedAccessView(_intermediateIntensityBuffer.Get(), nullptr, &uavDesc, customsHeapHandle);
        customsHeapHandle.ptr += CbvSrvUavHeapIncSize;

        intermediateBufferDesc.Width = sizeof(float) * 2;
        pDevice->CreateCommittedResource(&defaultHeapProp,
                                         D3D12_HEAP_FLAG_NONE,
                                         &intermediateBufferDesc,
                                         D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                                         nullptr,
                                         IID_PPV_ARGS(&_finalIntensityBuffer));
        _finalIntensityBuffer->SetName(L"FinalIntensity");
        uavDesc.Buffer.NumElements = 1;
        pDevice->CreateUnorderedAccessView(_finalIntensityBuffer.Get(), nullptr, &uavDesc, customsHeapHandle);
        customsHeapHandle.ptr += CbvSrvUavHeapIncSize;
    }
}

SceneManager::~SceneManager()
{
    FinishWorkerThreads();
    WaitCurrentFrame();
}

void SceneManager::SetTextures(ComPtr<ID3D12Resource> pTexture[6])
{
    for (size_t i = 0; i < countof(this->_texture); ++i)
        this->_texture[i] = pTexture[i];
}

void SceneManager::SetBackgroundCubemap(const std::wstring& name)
{
    ComPtr<ID3D12Resource> textureUploadBuffer;

    CommandList uploadCommandList {CommandListType::Direct, _device};
    ID3D12GraphicsCommandList *pCmdList = uploadCommandList.GetInternal().Get();

    UINT incrementSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = _customsHeap->GetCPUDescriptorHandleForHeapStart();
    cpuHandle.ptr += incrementSize * (_cmdLineOpts.shadow_pass ? 7 : 6);

    ID3D12Resource * pTex = nullptr;
    ID3D12Resource * pTex_upload = nullptr;

    ThrowIfFailed(CreateDDSTextureFromFile(_device.Get(), name.c_str(), 1024, true, &pTex, pCmdList, &pTex_upload, cpuHandle));
    _backgroundTexture.Attach(pTex);
    _backgroundTexture->SetName(L"Background cubemap with mips");
    textureUploadBuffer.Attach(pTex_upload);

    uploadCommandList.Close();
    ExecuteCommandLists(uploadCommandList);
}

SceneManager::SceneObjectPtr SceneManager::CreateFilledCube()
{
    ComPtr<ID3D12PipelineState> pipelineState = _cmdLineOpts.bundles ? _mrtPipelineState->GetPSO() : nullptr;
    _objects.push_back(std::make_shared<SceneObject>(_meshManager->CreateCube(), _device, pipelineState));
    _objects.back()->Scale({0.5f, 0.5f, 0.5f});
    return _objects.back();
}

SceneManager::SceneObjectPtr SceneManager::CreateOpenedCube()
{
    ComPtr<ID3D12PipelineState> pipelineState = _cmdLineOpts.bundles ? _mrtPipelineState->GetPSO() : nullptr;
    _objects.push_back(std::make_shared<SceneObject>(_meshManager->CreateEmptyCube(), _device, pipelineState));
    _objects.back()->Scale({0.5f, 0.5f, 0.5f});
    return _objects.back();
}

SceneManager::SceneObjectPtr SceneManager::CreatePlane()
{
    _objects.push_back(std::make_shared<SceneObject>(_meshManager->CreatePlane(), _device, nullptr));
    return _objects.back();
}

void SceneManager::DrawAll()
{
    FillViewProjMatrix();
    FillSceneProperties();

    if (_cmdLineOpts.shadow_pass)
        PopulateDepthPassCommandList();

    PopulateWorkerCommandLists();
    PopulateLightPassCommandList();

    std::vector<ID3D12CommandList*> cmdListArray;
    cmdListArray.reserve(_workerCmdLists.size() + 3);

    cmdListArray.push_back(_clearPassCmdList->GetInternal().Get());
    if (_cmdLineOpts.shadow_pass)
        cmdListArray.push_back(_depthPassCmdList->GetInternal().Get());

    for (auto &pWorkList : _workerCmdLists)
        cmdListArray.push_back(pWorkList->GetInternal().Get());
    cmdListArray.push_back(_lightPassCmdList->GetInternal().Get());

    _cmdQueue->ExecuteCommandLists((UINT)cmdListArray.size(), cmdListArray.data());

    // swap rt buffers.
    _swapChain->Present(1, 0);
    WaitCurrentFrame();
}

void SceneManager::ExecuteCommandLists(const CommandList & commandList)
{
    ID3D12CommandList* cmdListsArray[] = {commandList.GetInternal().Get()};
    _cmdQueue->ExecuteCommandLists((UINT)countof(cmdListsArray), cmdListsArray);

    _fenceValue++;
    WaitCurrentFrame();
}

Graphics::SphericalCamera * SceneManager::GetViewCamera()
{
    return &_viewCamera;
}

Graphics::SphericalCamera * SceneManager::GetShadowCamera()
{
    return &_shadowCamera;
}

void SceneManager::PopulateClearPassCommandList()
{
    // PRE-PASS - clear final render targets to draw
    _clearPassCmdList->Reset();
    ID3D12GraphicsCommandList *pCmdList = _clearPassCmdList->GetInternal().Get();

    PIXBeginEvent(pCmdList, 0, "Render targets clear");

    D3D12_RESOURCE_BARRIER barriers[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(_mrtRts[0]->_texture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
        CD3DX12_RESOURCE_BARRIER::Transition(_mrtRts[1]->_texture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
        CD3DX12_RESOURCE_BARRIER::Transition(_mrtRts[2]->_texture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
    };

    pCmdList->ResourceBarrier((UINT)countof(barriers), barriers);

    RenderTarget* rts[8] = {_mrtRts[0].get(), _mrtRts[1].get(), _mrtRts[2].get()};
    // is not necessary, but just for test
    _rtManager->BindRenderTargets(rts, _mrtDepth.get(), *_clearPassCmdList);

    _rtManager->ClearRenderTarget(*_mrtRts[0], *_clearPassCmdList);
    _rtManager->ClearRenderTarget(*_mrtRts[1], *_clearPassCmdList);
    _rtManager->ClearRenderTarget(*_mrtRts[2], *_clearPassCmdList);
    _rtManager->ClearDepthStencil(*_mrtDepth, *_clearPassCmdList);

    PIXEndEvent(pCmdList);

    _clearPassCmdList->Close();
}

void SceneManager::PopulateDepthPassCommandList()
{
    _depthPassCmdList->Reset();
    ComPtr<ID3D12GraphicsCommandList> pCmdList = _depthPassCmdList->GetInternal();

    PIXBeginEvent(pCmdList.Get(), 0, "Shadow rendering");
    pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D12_RECT scissor = {0, 0, (LONG)depthMapSize, (LONG)depthMapSize};
    pCmdList->RSSetScissorRects(1, &scissor);

    D3D12_VIEWPORT viewport = {0, 0, (float)depthMapSize, (float)depthMapSize, 0.0f, 1.0f};
    pCmdList->RSSetViewports(1, &viewport);

    pCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_shadowDepth->_texture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE));

    _rtManager->ClearDepthStencil(*_shadowDepth, *_depthPassCmdList);
    RenderTarget* rts[8] = {};
    _rtManager->BindRenderTargets(rts, _shadowDepth.get(), *_depthPassCmdList);

    pCmdList->SetGraphicsRootSignature(_depthPassRootSignature.GetInternal().Get());
    pCmdList->SetGraphicsRootConstantBufferView(1, _cbvDepthFrameParams->GetGPUVirtualAddress());
    for (size_t i = 0; i < _objects.size(); ++i)
    {
        pCmdList->SetGraphicsRootConstantBufferView(0, _objects[i]->GetConstantBuffer()->GetGPUVirtualAddress());
        _objects[i]->Draw(pCmdList, true);
    }

    PIXEndEvent(pCmdList.Get());
    _depthPassCmdList->Close();
}

void SceneManager::PopulateWorkerCommandLists()
{
    if (_cmdLineOpts.threads)
    {
        // wake up worker threads
        _drawObjectIndex = 0;
        _pendingWorkers = _threadPool.size();
        _workerBeginCV.notify_all();

        // wait until all work is completed
        {
            std::unique_lock<std::mutex> lock(_workerMutex);
            while (_drawObjectIndex < _objects.size() || _pendingWorkers)
                _workerEndCV.wait(lock);
        }
    }
    else
    {
        _drawObjectIndex = 0;
        while (_drawObjectIndex < _objects.size())
            ThreadDrawRoutine(0);
    }
}

void SceneManager::PopulateLightPassCommandList()
{
    UINT rtvHeapIncSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    UINT texHeapIncSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // PASS 2 - render ScreenQuad
    // Reset cmd list before rendering
    _lightPassCmdList->Reset();

    // Indicate that the back buffer will be used as a render target.
    ID3D12GraphicsCommandList *pCmdList = _lightPassCmdList->GetInternal().Get();

    PIXBeginEvent(pCmdList, 0, "Light rendering");
    pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D12_RECT scissor = {0, 0, (LONG)_screenWidth, (LONG)_screenHeight};
    pCmdList->RSSetScissorRects(1, &scissor);

    D3D12_VIEWPORT viewport = {0, 0, (FLOAT)_screenWidth, (FLOAT)_screenHeight, 0.0f, 1.0f};
    pCmdList->RSSetViewports(1, &viewport);

    pCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_HDRRt->_texture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));

    RenderTarget* rts[8] = {_HDRRt.get()};
    _rtManager->BindRenderTargets(rts, nullptr, *_lightPassCmdList);
    _rtManager->ClearRenderTarget(*rts[0], *_lightPassCmdList);

    D3D12_RESOURCE_BARRIER barriers[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(_mrtRts[0]->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(_mrtRts[1]->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(_mrtRts[2]->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
    };

    pCmdList->ResourceBarrier((UINT)countof(barriers), barriers);

    if (_cmdLineOpts.shadow_pass)
        pCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_shadowDepth->_texture.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

    pCmdList->SetGraphicsRootSignature(_lightRootSignature.GetInternal().Get());

    // Set sampler heap.
    ID3D12DescriptorHeap* ppHeaps[] = {_customsHeap.Get()};
    pCmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    D3D12_GPU_DESCRIPTOR_HANDLE texHandle = _customsHeap->GetGPUDescriptorHandleForHeapStart();
    pCmdList->SetGraphicsRootDescriptorTable(0, texHandle);

    texHandle.ptr += texHeapIncSize * (_cmdLineOpts.shadow_pass ? 7 : 6);
    pCmdList->SetGraphicsRootDescriptorTable(1, texHandle);

    pCmdList->SetGraphicsRootConstantBufferView(2, _cbvSceneParams->GetGPUVirtualAddress());

    _objScreenQuad->Draw(pCmdList);
    PIXEndEvent(pCmdList);

    // First, we need to compute average intensity through the rendered and lighted render target
    // Next, we should use computed intensity to tone final RT
    // This can be easily done with compute shaders

    PIXBeginEvent(pCmdList, 0, "Luminance computing");
    pCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_HDRRt->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
    pCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_finalIntensityBuffer.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

    ppHeaps[0] = _customsHeap.Get();
    pCmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    pCmdList->SetPipelineState(_IntensityPassState->GetPSO().Get());
    pCmdList->SetComputeRootSignature(_computePassRootSignature.GetInternal().Get());

    D3D12_GPU_DESCRIPTOR_HANDLE inputBufferHandle = _customsHeap->GetGPUDescriptorHandleForHeapStart();
    inputBufferHandle.ptr += texHeapIncSize * (_cmdLineOpts.shadow_pass ? 4 : 3); // get colored RT as the first input
    pCmdList->SetComputeRootDescriptorTable(0, inputBufferHandle);

    D3D12_GPU_DESCRIPTOR_HANDLE uavsBufferHandle = _customsHeap->GetGPUDescriptorHandleForHeapStart();
    uavsBufferHandle.ptr += texHeapIncSize * (_cmdLineOpts.shadow_pass ? 5 : 4); // get two UAV buffers as the second input
    pCmdList->SetComputeRootDescriptorTable(1, uavsBufferHandle);

    pCmdList->Dispatch((_screenHeight / 32) + 1, 1, 1);

    pCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_finalIntensityBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
    PIXEndEvent(pCmdList);

    //////////////////////////////////////////////////////////////////////////

    PIXBeginEvent(pCmdList, 0, "HDR -> LDR pass");
    pCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_swapChainRTs[_frameIndex]->_texture.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
    pCmdList->SetPipelineState(_LDRPassState->GetPSO().Get());
    pCmdList->SetGraphicsRootSignature(_LDRRootSignature.GetInternal().Get());

    rts[0] = _swapChainRTs[_frameIndex].get();
    _rtManager->BindRenderTargets(rts, nullptr, *_lightPassCmdList);
    _rtManager->ClearRenderTarget(*rts[0], *_lightPassCmdList);

    texHandle = _customsHeap->GetGPUDescriptorHandleForHeapStart();
    texHandle.ptr += texHeapIncSize * (_cmdLineOpts.shadow_pass ? 4 : 3);    // Get SRV's from MRT rendering.
    pCmdList->SetGraphicsRootDescriptorTable(0, texHandle);
    pCmdList->SetGraphicsRootConstantBufferView(1, _finalIntensityBuffer->GetGPUVirtualAddress());

    _objScreenQuad->Draw(pCmdList);

    // Indicate that the back buffer will be used as a render target.
    pCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_swapChainRTs[_frameIndex]->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
    PIXEndEvent(pCmdList);

    _lightPassCmdList->Close();
}

void SceneManager::WaitCurrentFrame()
{
    uint64_t newFenceValue = _fenceValue;

    ThrowIfFailed(_cmdQueue->Signal(_frameFence.Get(), newFenceValue));
    _fenceValue++;

    if (_frameFence->GetCompletedValue() != newFenceValue)
    {
        ThrowIfFailed(_frameFence->SetEventOnCompletion(newFenceValue, _frameEndEvent));
        WaitForSingleObject(_frameEndEvent, INFINITE);
    }

    _frameIndex = _swapChain->GetCurrentBackBufferIndex();
}

void SceneManager::ThreadDrawRoutine(size_t threadId)
{
    UINT rtvHeapIncSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    UINT texHeapIncSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    while (true)
    {
        if (_workerThreadExit)
            return; // terminate thread

        uint32_t currentObjectIndex = 0;
        if (!_threadPool.empty())
        {
            std::unique_lock<std::mutex> lock(_workerMutex);

            while (_drawObjectIndex >= _objects.size())
            {
                // nothing to do, remove self from active threads and suspend.
                --_pendingWorkers;
                _workerEndCV.notify_one();

                _workerBeginCV.wait(lock);
            }

            currentObjectIndex = _drawObjectIndex++;
            // mutex is unlocked here
        }

        // setting initial state of command lists here
        ComPtr<ID3D12GraphicsCommandList> pThreadCmdList = _workerCmdLists[threadId]->GetInternal();
        _workerCmdLists[threadId]->Reset();

        PIXBeginEvent(pThreadCmdList.Get(), 0, "G-Buffer objects rendering");

        if (_cmdLineOpts.tessellation)
            pThreadCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
        else
            pThreadCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // scissor
        D3D12_RECT scissor = {0, 0, (LONG)_screenWidth, (LONG)_screenHeight};
        pThreadCmdList->RSSetScissorRects(1, &scissor);

        // viewports
        D3D12_VIEWPORT viewport = {0, 0, (FLOAT)_screenWidth, (FLOAT)_screenHeight, 0.0f, 1.0f};
        pThreadCmdList->RSSetViewports(1, &viewport);

        RenderTarget* rts[8] = {_mrtRts[0].get(), _mrtRts[1].get(), _mrtRts[2].get()};
        _rtManager->BindRenderTargets(rts, _mrtDepth.get(), *_workerCmdLists[threadId]);

        if (_cmdLineOpts.textures)
        {
            // descriptor heaps
            ID3D12DescriptorHeap* ppHeaps[] = {_texturesHeap.Get()};
            pThreadCmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
        }

        // root signatures/constants
        pThreadCmdList->SetGraphicsRootSignature(_MRTRootSignature.GetInternal().Get());
        pThreadCmdList->SetGraphicsRootConstantBufferView(1, _cbvMrtFrameParams->GetGPUVirtualAddress());

        // draw all objects we can
        while (currentObjectIndex < _objects.size())
        {
            pThreadCmdList->SetGraphicsRootConstantBufferView(0, _objects[currentObjectIndex]->GetConstantBuffer()->GetGPUVirtualAddress());

            if (_cmdLineOpts.textures)
            {
                UINT parameterOffset = _cmdLineOpts.root_constants ? 3 : 2;

                // diffuse texture binding
                D3D12_GPU_DESCRIPTOR_HANDLE texHandle = _texturesHeap->GetGPUDescriptorHandleForHeapStart();
                texHandle.ptr += texHeapIncSize * ((currentObjectIndex + 1) % 3);
                pThreadCmdList->SetGraphicsRootDescriptorTable(parameterOffset, texHandle);
            }

            if (_cmdLineOpts.root_constants)
            {
                float shift = 0.125f * currentObjectIndex;
                pThreadCmdList->SetGraphicsRoot32BitConstant(2, *reinterpret_cast<uint32_t*>(&shift), 0);
            }

            _objects[currentObjectIndex]->Draw(pThreadCmdList);
            currentObjectIndex = _drawObjectIndex++;
        }

        PIXEndEvent(pThreadCmdList.Get());

        pThreadCmdList->Close();

        // stop loop, we are the function only.
        if (_threadPool.empty())
            break;
    }
}

void SceneManager::CreateRenderTargets()
{
    _swapChainRTs = _rtManager->CreateRenderTargetsForSwapChain(_swapChain);

    _mrtRts[0] = _rtManager->CreateRenderTarget(DXGI_FORMAT_R8G8B8A8_UNORM, _screenWidth, _screenHeight, L"DiffuseRT");
    _mrtRts[1] = _rtManager->CreateRenderTarget(DXGI_FORMAT_R11G11B10_FLOAT, _screenWidth, _screenHeight, L"NormalsRT");

    D3D12_CLEAR_VALUE clearValue;
    clearValue.Color[0] = 1.0f;
    clearValue.Color[1] = 1.0f;
    clearValue.Color[2] = 1.0f;
    clearValue.Color[3] = 1.0f;
    clearValue.Format = DXGI_FORMAT_R32_FLOAT;
    _mrtRts[2] = _rtManager->CreateRenderTarget(DXGI_FORMAT_R32_FLOAT, _screenWidth, _screenHeight, L"DepthRT", false, &clearValue);
    _mrtDepth = _rtManager->CreateDepthStencil(_screenWidth, _screenHeight, DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_D24_UNORM_S8_UINT, L"MRTDepthRT");

    _HDRRt = _rtManager->CreateRenderTarget(DXGI_FORMAT_R16G16B16A16_FLOAT, _screenWidth, _screenHeight, L"HDRRT", true);

    if (_cmdLineOpts.shadow_pass)
        _shadowDepth = _rtManager->CreateDepthStencil(depthMapSize, depthMapSize, DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_D24_UNORM_S8_UINT, L"ShadowPassDepthRT");

    // move resources into correct states to avoid DXDebug errors on first barrier calls
    D3D12_RESOURCE_BARRIER barriers[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(_mrtRts[0]->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(_mrtRts[1]->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(_mrtRts[2]->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(_HDRRt->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(_mrtDepth->_texture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE)
    };

    CommandList temporaryCmdList {CommandListType::Direct, _device};
    temporaryCmdList.GetInternal()->ResourceBarrier((UINT)countof(barriers), barriers);
    temporaryCmdList.Close();

    ExecuteCommandLists(temporaryCmdList);
}

void SceneManager::CreateCommandLists()
{
    if (_cmdLineOpts.threads)
    {
        _workerCmdLists.resize(_threadPool.size());
        for (uint32_t i = 0; i < _threadPool.size(); ++i)
        {
            _workerCmdLists[i] = std::make_unique<CommandList>(CommandListType::Direct, _device, _mrtPipelineState->GetPSO());
            _workerCmdLists[i]->Close();
        }
    }
    else
    {
        _workerCmdLists.push_back(std::make_unique<CommandList>(CommandListType::Direct, _device, _mrtPipelineState->GetPSO()));
        _workerCmdLists.back()->Close();
    }

    _clearPassCmdList = std::make_unique<CommandList>(CommandListType::Direct, _device, _mrtPipelineState->GetPSO());
    _clearPassCmdList->Close();

    _lightPassCmdList = std::make_unique<CommandList>(CommandListType::Direct, _device, _lightPassState->GetPSO());
    _lightPassCmdList->Close();

    if (_cmdLineOpts.shadow_pass)
    {
        _depthPassCmdList = std::make_unique<CommandList>(CommandListType::Direct, _device, _depthPassState->GetPSO());
        _depthPassCmdList->Close();
    }
}

void SceneManager::FinishWorkerThreads()
{
    if (!_cmdLineOpts.threads)
        return;

    // wake up worker threads with term signal
    {
        std::unique_lock<std::mutex> lock(_workerMutex);
        _workerThreadExit = true;
        _workerBeginCV.notify_all();
    }

    for (uint32_t tid = 0; tid < _threadPool.size(); ++tid)
        _threadPool[tid].join();
}

void SceneManager::CreateConstantBuffer(size_t bufferSize, ComPtr<ID3D12Resource> * pOutBuffer)
{
    D3D12_RESOURCE_DESC constantBufferDesc = {};
    constantBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    constantBufferDesc.Width = bufferSize;
    constantBufferDesc.Height = 1;
    constantBufferDesc.MipLevels = 1;
    constantBufferDesc.SampleDesc.Count = 1;
    constantBufferDesc.DepthOrArraySize = 1;
    constantBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    D3D12_HEAP_PROPERTIES heapProp = {D3D12_HEAP_TYPE_UPLOAD};
    ThrowIfFailed(_device->CreateCommittedResource(&heapProp,
                                                   D3D12_HEAP_FLAG_NONE,
                                                   &constantBufferDesc,
                                                   D3D12_RESOURCE_STATE_GENERIC_READ,
                                                   nullptr,
                                                   IID_PPV_ARGS(&(*pOutBuffer))));
}

void SceneManager::CreateFrameConstantBuffers()
{
    CreateConstantBuffer(sizeof(perFrameParamsConstantBuffer), &_cbvMrtFrameParams);
    D3D12_RANGE readRange = {0, 0};
    ThrowIfFailed(_cbvMrtFrameParams->Map(0, &readRange, &_cbvMRT));
    CreateConstantBuffer(sizeof(perFrameParamsConstantBuffer), &_cbvDepthFrameParams);
    ThrowIfFailed(_cbvDepthFrameParams->Map(0, &readRange, &_cbvDepth));
    CreateConstantBuffer(sizeof(sceneParamsConstantBuffer), &_cbvSceneParams);
    ThrowIfFailed(_cbvSceneParams->Map(0, &readRange, &_cbvScene));
}

void SceneManager::CreateMRTPassRootSignature()
{
    // 4 entries
    //
    // 1. constant buffer with model parameters
    // 2. constant buffer with frame parameters
    // 3. root constants with texture coords offset
    // 4. texture table with diffuse texture

    UINT entriesCount = 4;
    if (!_cmdLineOpts.root_constants)
        entriesCount--;
    if (!_cmdLineOpts.textures)
        entriesCount--;

    _MRTRootSignature.Init(entriesCount, 1);

    _MRTRootSignature[0].InitAsCBV(0);
    _MRTRootSignature[1].InitAsCBV(1);

    if (_cmdLineOpts.root_constants)
        _MRTRootSignature[2].InitAsConstants(1, 2);

    if (_cmdLineOpts.textures)
    {
        _MRTRootSignature[_cmdLineOpts.root_constants ? 3 : 2].InitAsDescriptorsTable(1);
        _MRTRootSignature[_cmdLineOpts.root_constants ? 3 : 2].InitTableRange(0, 0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
    }

    _MRTRootSignature.InitStaticSampler(0, {});
    _MRTRootSignature.Finalize(_device, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
}

void SceneManager::CreateLightPassRootSignature()
{
    _lightRootSignature.Init(3, 2);

    _lightRootSignature[0].InitAsDescriptorsTable(1);
    _lightRootSignature[0].InitTableRange(0, 0, 4, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);

    _lightRootSignature[1].InitAsDescriptorsTable(1);
    _lightRootSignature[1].InitTableRange(0, 4, 1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);

    _lightRootSignature[2].InitAsCBV(0);

    StaticSampler textureSampler = {};
    D3D12_STATIC_SAMPLER_DESC& textureSamplerDesc = textureSampler;
    textureSamplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
    textureSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    textureSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;

    StaticSampler shadowSampler = {};
    D3D12_STATIC_SAMPLER_DESC& shadowSamplerDesc = shadowSampler;
    shadowSamplerDesc.Filter = D3D12_FILTER_COMPARISON_ANISOTROPIC;
    shadowSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    shadowSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    shadowSamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS;
    shadowSamplerDesc.ShaderRegister = 1;

    _lightRootSignature.InitStaticSampler(0, textureSampler);
    _lightRootSignature.InitStaticSampler(1, shadowSampler);

    _lightRootSignature.Finalize(_device, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
}

void SceneManager::CreateLDRPassRootSignature()
{
    _LDRRootSignature.Init(2, 1);

    _LDRRootSignature[0].InitAsDescriptorsTable(1);
    _LDRRootSignature[0].InitTableRange(0, 0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);

    _LDRRootSignature[1].InitAsCBV(0);

    StaticSampler textureSampler {};
    D3D12_STATIC_SAMPLER_DESC& textureSamplerDesc = textureSampler;
    textureSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    textureSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    textureSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

    _LDRRootSignature.InitStaticSampler(0, textureSampler);
    _LDRRootSignature.Finalize(_device, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
}

void SceneManager::CreateDepthPassRootSignature()
{
    _depthPassRootSignature.Init(2, 0);

    _depthPassRootSignature[0].InitAsCBV(0);
    _depthPassRootSignature[1].InitAsCBV(1);

    _depthPassRootSignature.Finalize(_device, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
}

void SceneManager::CreateIntensityPassRootSignature()
{
    _computePassRootSignature.Init(2, 0);

    _computePassRootSignature[0].InitAsDescriptorsTable(1);
    _computePassRootSignature[0].InitTableRange(0, 0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);

    _computePassRootSignature[1].InitAsDescriptorsTable(1);
    _computePassRootSignature[1].InitTableRange(0, 0, 2, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);

    _computePassRootSignature.Finalize(_device);
}

void SceneManager::CreateShadersAndPSOs()
{
    CreateMRTPassPSO();
    CreateLightPassPSO();
    CreateLDRPassPSO();
    CreateIntensityPassPSO();
    if (_cmdLineOpts.shadow_pass)
        CreateDepthPassPSO();
}

void SceneManager::CreateDepthPassPSO()
{
    // Prepare our HLSL shaders
    ComPtr<ID3DBlob> VSblob = nullptr;
    ComPtr<ID3DBlob> PSblob = nullptr;
    ComPtr<ID3DBlob> HSblob = nullptr;
    ComPtr<ID3DBlob> DSblob = nullptr;

    std::vector<D3D_SHADER_MACRO> macro;
    if (_cmdLineOpts.tessellation)
        macro.push_back({"UseTessellation", "1"});
    macro.push_back({NULL, NULL});

    std::wstring depthPassShader = L"assets/shaders/DepthPass.hlsl";
    ShadersUtils::CompileShaderFromFile(depthPassShader, ShaderType::Vertex, &VSblob, macro.data());

    if (_cmdLineOpts.tessellation)
    {
        ShadersUtils::CompileShaderFromFile(depthPassShader, ShaderType::Hull, &HSblob, macro.data());
        ShadersUtils::CompileShaderFromFile(depthPassShader, ShaderType::Domain, &DSblob, macro.data());
    }

    _depthPassState = std::make_unique<GraphicsPipelineState>(_depthPassRootSignature,
                                                              D3D12_INPUT_LAYOUT_DESC {defaultGeometryInputElements, _countof(defaultGeometryInputElements)});
    _depthPassState->SetShaderCode(VSblob, ShaderType::Vertex);
    if (_cmdLineOpts.tessellation)
    {
        _depthPassState->SetShaderCode(HSblob, ShaderType::Hull);
        _depthPassState->SetShaderCode(DSblob, ShaderType::Domain);
    }
    _depthPassState->SetDepthStencilFormat(DXGI_FORMAT_D24_UNORM_S8_UINT);

    if (_cmdLineOpts.tessellation)
        _depthPassState->SetPritimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH);

    _depthPassState->SetDepthStencilState({
        TRUE,                       // DepthEnable
        D3D12_DEPTH_WRITE_MASK_ALL, // DepthWriteMask
        D3D12_COMPARISON_FUNC_LESS, // DepthFunc
        TRUE,                       // StencilEnable
        0,                          // StencilReadMask
        0xff,                       // StencilWriteMask
        {                           // FrontFace
            D3D12_STENCIL_OP_ZERO,           // StencilFailOp
            D3D12_STENCIL_OP_ZERO,           // StencilDepthFailOp
            D3D12_STENCIL_OP_ZERO,           // StencilPassOp
            D3D12_COMPARISON_FUNC_ALWAYS        // StencilFunc
        },
        {                           // BackFace
            D3D12_STENCIL_OP_ZERO,           // StencilFailOp
            D3D12_STENCIL_OP_ZERO,           // StencilDepthFailOp
            D3D12_STENCIL_OP_ZERO,           // StencilPassOp
            D3D12_COMPARISON_FUNC_ALWAYS        // StencilFunc
        },
    });

    D3D12_RASTERIZER_DESC rasterizerDesc = {
        D3D12_FILL_MODE_SOLID,                      // FillMode
        D3D12_CULL_MODE_BACK,                       // CullMode
        FALSE,                                      // FrontCounterClockwise
        1000,                                       // DepthBias
        1.0f,                                       // DepthBiasClamp
        1.0f,                                       // SlopeScaledDepthBias
        TRUE,                                       // DepthClipEnable
        FALSE,                                      // MultisampleEnable
        FALSE,                                      // AntialiasedLineEnable
        0,                                          // ForcedSampleCount
        D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF   // ConservativeRaster
    };

    _depthPassState->SetRasterizerState(rasterizerDesc);
    _depthPassState->Finalize(_device);
}

void SceneManager::CreateLightPassPSO()
{
    // Prepare our HLSL shaders
    ComPtr<ID3DBlob> VSblob = nullptr;
    ComPtr<ID3DBlob> PSblob = nullptr;
    D3D_SHADER_MACRO *macro = nullptr;

    D3D_SHADER_MACRO ShadowMacro[] = {"ShadowMapping", "1", NULL, NULL};
    macro = nullptr;
    if (_cmdLineOpts.shadow_pass)
        macro = ShadowMacro;

    std::wstring squadFileName = L"assets/shaders/LightPass.hlsl";
    ShadersUtils::CompileShaderFromFile(squadFileName, ShaderType::Vertex, &VSblob, macro);
    ShadersUtils::CompileShaderFromFile(squadFileName, ShaderType::Pixel, &PSblob, macro);

    _lightPassState = std::make_unique<GraphicsPipelineState>(_lightRootSignature,
                                                              D3D12_INPUT_LAYOUT_DESC {screenQuadInputElements, _countof(screenQuadInputElements)});
    _lightPassState->SetShaderCode(VSblob, ShaderType::Vertex);
    _lightPassState->SetShaderCode(PSblob, ShaderType::Pixel);
    _lightPassState->SetRenderTargetFormats({DXGI_FORMAT_R16G16B16A16_FLOAT});
    _lightPassState->SetDepthStencilState({FALSE});
    _lightPassState->Finalize(_device);
}

void SceneManager::CreateMRTPassPSO()
{
    // Prepare our HLSL shaders
    ComPtr<ID3DBlob> VSblob = nullptr;
    ComPtr<ID3DBlob> PSblob = nullptr;
    ComPtr<ID3DBlob> HSblob = nullptr;
    ComPtr<ID3DBlob> DSblob = nullptr;

    std::vector<D3D_SHADER_MACRO> macro;
    if (_cmdLineOpts.root_constants)
        macro.push_back({"RootConstants", "1"});
    if (_cmdLineOpts.textures)
        macro.push_back({"UseTextures", "1"});
    if (_cmdLineOpts.tessellation)
        macro.push_back({"UseTessellation", "1"});
    macro.push_back({NULL, NULL});

    std::wstring mrtShaders = L"assets/shaders/MRTPass.hlsl";
    ShadersUtils::CompileShaderFromFile(mrtShaders, ShaderType::Vertex, &VSblob, macro.data());
    ShadersUtils::CompileShaderFromFile(mrtShaders, ShaderType::Pixel, &PSblob, macro.data());
    if (_cmdLineOpts.tessellation)
    {
        ShadersUtils::CompileShaderFromFile(mrtShaders, ShaderType::Hull, &HSblob, macro.data());
        ShadersUtils::CompileShaderFromFile(mrtShaders, ShaderType::Domain, &DSblob, macro.data());
    }

    _mrtPipelineState = std::make_unique<GraphicsPipelineState>(_MRTRootSignature,
                                                                D3D12_INPUT_LAYOUT_DESC {defaultGeometryInputElements, _countof(defaultGeometryInputElements)});
    _mrtPipelineState->SetShaderCode(VSblob, ShaderType::Vertex);
    _mrtPipelineState->SetShaderCode(PSblob, ShaderType::Pixel);
    if (_cmdLineOpts.tessellation)
    {
        _mrtPipelineState->SetShaderCode(HSblob, ShaderType::Hull);
        _mrtPipelineState->SetShaderCode(DSblob, ShaderType::Domain);
    }
    _mrtPipelineState->SetRenderTargetFormats({DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R11G11B10_FLOAT, DXGI_FORMAT_R32_FLOAT});
    _mrtPipelineState->SetDepthStencilFormat(DXGI_FORMAT_D24_UNORM_S8_UINT);

    if (_cmdLineOpts.tessellation)
        _mrtPipelineState->SetPritimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH);

    _mrtPipelineState->Finalize(_device);
}

void SceneManager::CreateLDRPassPSO()
{
    // Prepare our HLSL shaders
    ComPtr<ID3DBlob> VSblob = nullptr;
    ComPtr<ID3DBlob> PSblob = nullptr;
    D3D_SHADER_MACRO macro[] = {NULL, NULL};

    std::wstring squadFileName = L"assets/shaders/LDRPass.hlsl";
    ShadersUtils::CompileShaderFromFile(squadFileName, ShaderType::Vertex, &VSblob, macro);
    ShadersUtils::CompileShaderFromFile(squadFileName, ShaderType::Pixel, &PSblob, macro);

    _LDRPassState = std::make_unique<GraphicsPipelineState>(_LDRRootSignature,
                                                            D3D12_INPUT_LAYOUT_DESC {screenQuadInputElements, _countof(screenQuadInputElements)});
    _LDRPassState->SetShaderCode(VSblob, ShaderType::Vertex);
    _LDRPassState->SetShaderCode(PSblob, ShaderType::Pixel);
    _LDRPassState->SetRenderTargetFormats({DXGI_FORMAT_R8G8B8A8_UNORM_SRGB});
    _LDRPassState->SetDepthStencilState({FALSE});
    _LDRPassState->Finalize(_device);
}

void SceneManager::CreateIntensityPassPSO()
{
    // Prepare our HLSL shaders
    ComPtr<ID3DBlob> CSblob = nullptr;
    D3D_SHADER_MACRO macro[] = {NULL, NULL};

    std::wstring squadFileName = L"assets/shaders/IntensityPass.hlsl";
    ShadersUtils::CompileShaderFromFile(squadFileName, ShaderType::Compute, &CSblob, macro);

    _IntensityPassState = std::make_unique<ComputePipelineState>(_computePassRootSignature);
    _IntensityPassState->SetShaderCode(CSblob);
    _IntensityPassState->Finalize(_device);
}

void SceneManager::FillViewProjMatrix()
{
    assert(_cbvMRT);
    assert(_cbvDepth);

    XMMATRIX viewProjectionMatrix = _viewCamera.GetViewProjMatrix();
    std::memcpy(reinterpret_cast<perFrameParamsConstantBuffer*>(_cbvMRT)->viewProjectionMatrix, viewProjectionMatrix.r, sizeof(XMMATRIX));
    std::memcpy(reinterpret_cast<perFrameParamsConstantBuffer*>(_cbvMRT)->cameraPosition, &_viewCamera.GetEyePosition(), sizeof(XMFLOAT4));

    XMMATRIX depthProjectionMatrix = _shadowCamera.GetViewProjMatrix();
    std::memcpy(reinterpret_cast<perFrameParamsConstantBuffer*>(_cbvDepth)->viewProjectionMatrix, depthProjectionMatrix.r, sizeof(XMMATRIX));
    std::memcpy(reinterpret_cast<perFrameParamsConstantBuffer*>(_cbvDepth)->cameraPosition, &_shadowCamera.GetEyePosition(), sizeof(XMFLOAT4));
}

void SceneManager::FillSceneProperties()
{
    assert(_cbvScene);

    XMFLOAT4 lightPos = _shadowCamera.GetEyePosition();
    XMFLOAT3 ambientColor = {0.2f, 0.2f, 0.2f};
    XMFLOAT3 fogColor = {clearColor[0], clearColor[1], clearColor[2]};
    XMMATRIX shadowMatrix = _shadowCamera.GetViewProjMatrix();
    XMMATRIX inverseViewProj = XMMatrixInverse(nullptr, _viewCamera.GetViewProjMatrix());
    XMFLOAT4 eyePosition = _viewCamera.GetEyePosition();

    sceneParamsConstantBuffer* cbuffer = reinterpret_cast<sceneParamsConstantBuffer*>(_cbvScene);
    std::memcpy(cbuffer->lightPosition, &lightPos, sizeof(XMFLOAT4));
    std::memcpy(cbuffer->ambientColor, &ambientColor, sizeof(XMFLOAT3));
    std::memcpy(cbuffer->fogColor, &fogColor, sizeof(XMFLOAT3));
    // assume that we have box-shaped scene
    cbuffer->sceneSize = std::powf((float)_objects.size(), 0.333f);
    cbuffer->depthMapSize = static_cast<float>(depthMapSize);

    std::memcpy(cbuffer->shadowMatrix, &shadowMatrix.r, sizeof(XMMATRIX));
    std::memcpy(cbuffer->inverseViewProj, &inverseViewProj, sizeof(XMMATRIX));
    std::memcpy(cbuffer->camPosition, &eyePosition, sizeof(XMFLOAT4));
}

void SceneManager::CreateRootSignatures()
{
    // this is becoming monstrous...
    CreateMRTPassRootSignature();
    CreateLightPassRootSignature();
    CreateLDRPassRootSignature();
    CreateIntensityPassRootSignature();

    if (_cmdLineOpts.shadow_pass)
        CreateDepthPassRootSignature();
}
