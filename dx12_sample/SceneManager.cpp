#include "stdafx.h"

#include "SceneManager.h"

#include <utils/RenderTargetManager.h>
#include <utils/Shaders.h>
#include <locale>
#include <codecvt>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include <random>

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

constexpr auto pi = 3.14159265f;

unsigned GetRandomNumber(unsigned min, unsigned max)
{
    std::random_device r;
    std::default_random_engine e1(r());
    std::uniform_int_distribution<unsigned> uniform_dist(min, max);
    return uniform_dist(e1);
}

// create screen quad
std::vector<screenQuadVertex> screenQuadVertices =
{
    { { -1.0f,  1.0f },{ 0.0f, 0.0f } },
    { { 1.0f,  1.0f },{ 1.0f, 0.0f } },
    { { 1.0f, -1.0f },{ 1.0f, 1.0f } },
    { { -1.0f,  1.0f },{ 0.0f, 0.0f } },
    { { 1.0f, -1.0f },{ 1.0f, 1.0f } },
    { { -1.0f, -1.0f },{ 0.0f, 1.0f } },
};

void BlurEffect::Init(UINT _screenWidth, UINT _screenHeight, float direction[], DXGI_FORMAT format,
                    RenderTargetManager* _rtManager, ComPtr<ID3D12Device> _pDevice, std::shared_ptr<MeshManager>&  _meshManager, std::shared_ptr<RenderTarget> Input,
                    std::shared_ptr<RenderTarget> Output, std::shared_ptr<RenderTarget> Depth, const std::wstring& BlurName)
{
    m_screenWidth = _screenWidth;	m_screenHeight = _screenHeight;
    m_direction[0] = direction[0]; m_direction[1] = direction[1];
    m_Inp = Input;
    m_rtManager = _rtManager;
    m_pDevice = _pDevice;
    m_Name = BlurName;
    m_Format = format;
    m_Depth = Depth;
    m_meshManager = _meshManager;

    if (_rtManager == nullptr || m_pDevice == nullptr )
        return;

    // Create Heap for the Texture
    D3D12_DESCRIPTOR_HEAP_DESC texHeapDesc = {};
    texHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    texHeapDesc.NumDescriptors = 100;
    texHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(m_pDevice->CreateDescriptorHeap(&texHeapDesc, IID_PPV_ARGS(&m_customsHeap)));
    const std::wstring tmp_name = m_Name + L"_heap";
    m_customsHeap.Get()->SetName(tmp_name.c_str());

    if (Output != nullptr)
    {
        m_Out = Output;
        m_OutIsExternalRT = true;
    }
    else
    {
        m_Out = _rtManager->CreateRenderTarget(format, m_screenWidth, m_screenHeight, m_Name + L"_texture", true);
        std::wstring name = m_Name + L"_buffer";
        m_Out->_texture->SetName(name.c_str());
        m_OutIsExternalRT = false;
    }

    // map input and output
    D3D12_CPU_DESCRIPTOR_HANDLE customsHeapHandle = m_customsHeap->GetCPUDescriptorHandleForHeapStart();
    UINT CbvSrvUavHeapIncSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_pDevice->CreateShaderResourceView(m_Inp->_texture.Get(), nullptr, customsHeapHandle);
    customsHeapHandle.ptr += CbvSrvUavHeapIncSize;
    m_pDevice->CreateShaderResourceView(m_Depth->_texture.Get(), nullptr, customsHeapHandle);
    customsHeapHandle.ptr += CbvSrvUavHeapIncSize;
    m_pDevice->CreateShaderResourceView(m_Out->_texture.Get(), nullptr, customsHeapHandle);
    customsHeapHandle.ptr += CbvSrvUavHeapIncSize;

    // Create root signature
    StaticSampler Sampler{};
    D3D12_STATIC_SAMPLER_DESC& SamplerDesc = Sampler;
    SamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    SamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    SamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

    m_RootSignature.Init(2, 1);
    m_RootSignature[0].InitAsDescriptorsTable(1);
    m_RootSignature[0].InitTableRange(0, 0, 2, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
    m_RootSignature[1].InitAsCBV(0);

    m_RootSignature.InitStaticSampler(0, Sampler);
    m_RootSignature.Finalize(m_pDevice, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // Create pipeline state
    ComPtr<ID3DBlob> VSblob = nullptr;
    ComPtr<ID3DBlob> PSblob = nullptr;
    D3D_SHADER_MACRO macro[] = { NULL, NULL };

    std::wstring squadFileName = L"assets/shaders/Blur.hlsl";
    ShadersUtils::CompileShaderFromFile(squadFileName, ShaderType::Vertex, &VSblob, macro);
    ShadersUtils::CompileShaderFromFile(squadFileName, ShaderType::Pixel, &PSblob, macro);

    m_PassState = std::make_unique<GraphicsPipelineState>(m_RootSignature,
                        D3D12_INPUT_LAYOUT_DESC{ screenQuadInputElements, _countof(screenQuadInputElements) });
    m_PassState->SetShaderCode(VSblob, ShaderType::Vertex);
    m_PassState->SetShaderCode(PSblob, ShaderType::Pixel);
    m_PassState->SetRenderTargetFormats({ m_Format });
    m_PassState->SetDepthStencilState({ FALSE });
    m_PassState->Finalize(m_pDevice);

    m_objScreenQuad = std::make_unique<SceneObject>(m_meshManager->CreateScreenQuad(), m_pDevice, nullptr);

    // create constant buffer
    D3D12_RESOURCE_DESC constantBufferDesc = {};
    constantBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    constantBufferDesc.Width = sizeof(BlurParams);
    constantBufferDesc.Height = 1;
    constantBufferDesc.MipLevels = 1;
    constantBufferDesc.SampleDesc.Count = 1;
    constantBufferDesc.DepthOrArraySize = 1;
    constantBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    D3D12_HEAP_PROPERTIES heapProp = { D3D12_HEAP_TYPE_UPLOAD };
    ThrowIfFailed(m_pDevice->CreateCommittedResource(&heapProp,
        D3D12_HEAP_FLAG_NONE,	&constantBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,	nullptr, IID_PPV_ARGS(&m_cbvParams)));

    D3D12_RANGE readRange = { 0, 0 };
    ThrowIfFailed(m_cbvParams->Map(0, &readRange, &m_cbvParamsMapped));

    BlurParams* pParams = reinterpret_cast<BlurParams*>(m_cbvParamsMapped);
    pParams->step[0] = (float)m_direction[0]/(float)m_screenWidth;
    pParams->step[1] = (float)m_direction[1]/(float)m_screenHeight;
}

void BlurEffect::DoBlur(std::unique_ptr<CommandList>&  CmdList)
{
    static bool first_pass = false;

    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converterX;
    std::string tmp = converterX.to_bytes(m_Name) + " execution";
    ID3D12GraphicsCommandList *pCmdList = CmdList->GetInternal().Get();
    PIXBeginEvent(pCmdList, 0, tmp.c_str());


    ID3D12DescriptorHeap* ppHeaps[] = { m_customsHeap.Get() };
    pCmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D12_RECT scissor = { 0, 0, (LONG)m_screenWidth, (LONG)m_screenHeight };
    D3D12_VIEWPORT viewport = { 0, 0, (FLOAT)m_screenWidth, (FLOAT)m_screenHeight, 0.0f, 1.0f };
    pCmdList->RSSetViewports(1, &viewport);
    pCmdList->RSSetScissorRects(1, &scissor);
    pCmdList->SetPipelineState(m_PassState->GetPSO().Get());
    pCmdList->SetGraphicsRootSignature(m_RootSignature.GetInternal().Get());

    pCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_Inp->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

    D3D12_GPU_DESCRIPTOR_HANDLE texHandle = m_customsHeap->GetGPUDescriptorHandleForHeapStart();
    pCmdList->SetGraphicsRootDescriptorTable(0, texHandle);
    pCmdList->SetGraphicsRootConstantBufferView(1, m_cbvParams->GetGPUVirtualAddress());

    RenderTarget* rts[8] = { m_Out.get() };
    m_rtManager->BindRenderTargets(rts, nullptr, *CmdList);
    m_rtManager->ClearRenderTarget(*rts[0], *CmdList);

    m_objScreenQuad->Draw(pCmdList);

    pCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_Inp->_texture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET ));

    PIXEndEvent(pCmdList);
    first_pass = false;
}


void HBAO::Init(UINT _screenWidth, UINT _screenHeight, const math::mat4f& ProjectionMatrix, DXGI_FORMAT format,
    RenderTargetManager* _rtManager, ComPtr<ID3D12Device> _pDevice, std::shared_ptr<MeshManager>&  _meshManager, CommandList& CmdList,
    std::shared_ptr<RenderTarget> Output, std::shared_ptr<RenderTarget> Depth, const std::wstring& BlurName)
{
    m_screenWidth = _screenWidth;	m_screenHeight = _screenHeight;
    m_rtManager = _rtManager;
    m_pDevice = _pDevice;
    m_Name = BlurName;
    m_Format = format;
    m_Depth = Depth;
    m_meshManager = _meshManager;
    m_AOscreenWidth = m_screenWidth / 2;
    m_AOscreenHeight = m_screenHeight / 2;

    if (_rtManager == nullptr || m_pDevice == nullptr)
        return;

    // Create Heap for the Texture
    D3D12_DESCRIPTOR_HEAP_DESC texHeapDesc = {};
    texHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    texHeapDesc.NumDescriptors = 100;
    texHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(m_pDevice->CreateDescriptorHeap(&texHeapDesc, IID_PPV_ARGS(&m_customsHeap)));
    const std::wstring tmp_name = m_Name + L"_heap";
    m_customsHeap.Get()->SetName(tmp_name.c_str());

    // output of HBAO shader
    m_AOOut = _rtManager->CreateRenderTarget(format, m_AOscreenWidth, m_AOscreenHeight, m_Name + L"_texture", true);
    std::wstring name = m_Name + L"_buffer";
    m_AOOut->_texture->SetName(name.c_str());
    m_OutIsExternalRT = false;

    if (Output != nullptr)
    {
        m_AOBluredOut = Output;
        m_OutIsExternalRT = true;
    }
    else
    {
        m_AOBluredOut = _rtManager->CreateRenderTarget(format, m_screenWidth, m_screenHeight, m_Name + L"_blured_texture", true);
        std::wstring name = m_Name + L"_blured_buffer";
        m_AOBluredOut->_texture->SetName(name.c_str());
        m_OutIsExternalRT = false;
    }

    CreateNoise(CmdList);

    // map input noise, and output
    D3D12_CPU_DESCRIPTOR_HANDLE customsHeapHandle = m_customsHeap->GetCPUDescriptorHandleForHeapStart();
    UINT CbvSrvUavHeapIncSize = m_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_pDevice->CreateShaderResourceView(m_Depth->_texture.Get(), nullptr, customsHeapHandle);
    customsHeapHandle.ptr += CbvSrvUavHeapIncSize;
    m_pDevice->CreateShaderResourceView(_Noise.Get(), nullptr, customsHeapHandle);
    customsHeapHandle.ptr += CbvSrvUavHeapIncSize;

    // Create root signature
    StaticSampler Sampler{};
    D3D12_STATIC_SAMPLER_DESC& SamplerDesc = Sampler;
    SamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    SamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    SamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    SamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;

    StaticSampler NoiseSampler{};
    D3D12_STATIC_SAMPLER_DESC& NoiseSamplerDesc = NoiseSampler;
    NoiseSamplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    NoiseSamplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    NoiseSamplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    NoiseSamplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    NoiseSamplerDesc.ShaderRegister = 1;

    m_RootSignature.Init(2, 2);
    m_RootSignature[0].InitAsDescriptorsTable(1);
    m_RootSignature[0].InitTableRange(0, 0, 3, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
    m_RootSignature[1].InitAsCBV(0);

    m_RootSignature.InitStaticSampler(0, Sampler);
    m_RootSignature.InitStaticSampler(1, NoiseSampler);
    m_RootSignature.Finalize(m_pDevice, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // Create pipeline state
    ComPtr<ID3DBlob> VSblob = nullptr;
    ComPtr<ID3DBlob> PSblob = nullptr;
    D3D_SHADER_MACRO macro[] = { NULL, NULL };

    std::wstring squadFileName = L"assets/shaders/AOPass.hlsl";
    ShadersUtils::CompileShaderFromFile(squadFileName, ShaderType::Vertex, &VSblob, macro);
    ShadersUtils::CompileShaderFromFile(squadFileName, ShaderType::Pixel, &PSblob, macro);

    m_PassState = std::make_unique<GraphicsPipelineState>(m_RootSignature,
        D3D12_INPUT_LAYOUT_DESC{ screenQuadInputElements, _countof(screenQuadInputElements) });
    m_PassState->SetShaderCode(VSblob, ShaderType::Vertex);
    m_PassState->SetShaderCode(PSblob, ShaderType::Pixel);
    m_PassState->SetRenderTargetFormats({ m_Format });
    m_PassState->SetDepthStencilState({ FALSE });
    m_PassState->Finalize(m_pDevice);

    m_objScreenQuad = std::make_unique<SceneObject>(m_meshManager->CreateScreenQuad(), m_pDevice, nullptr);

    // create constant buffer
    D3D12_RESOURCE_DESC constantBufferDesc = {};
    constantBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    constantBufferDesc.Width = sizeof(AOParams);
    constantBufferDesc.Height = 1;
    constantBufferDesc.MipLevels = 1;
    constantBufferDesc.SampleDesc.Count = 1;
    constantBufferDesc.DepthOrArraySize = 1;
    constantBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    D3D12_HEAP_PROPERTIES heapProp = { D3D12_HEAP_TYPE_UPLOAD };
    ThrowIfFailed(m_pDevice->CreateCommittedResource(&heapProp,
        D3D12_HEAP_FLAG_NONE, &constantBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_cbvParams)));

    D3D12_RANGE readRange = { 0, 0 };
    ThrowIfFailed(m_cbvParams->Map(0, &readRange, &m_cbvParamsMapped));

    AOParams* pParams = reinterpret_cast<AOParams*>(m_cbvParamsMapped);

    math::mat4f projectionInvMatrix  = matrixInverse(ProjectionMatrix);;

    std::memcpy(reinterpret_cast<AOParams*>(m_cbvParamsMapped)->viewProjectionInvMatrix, (float*)projectionInvMatrix, sizeof(XMMATRIX));
    pParams->_screenInvWidth = 1.0f / (float)m_screenWidth;
    pParams->_screenInvHeight = 1.0f / (float)m_screenHeight;

    // create blur
    float dir_x[2] = { 1.0, 0.0 };
    BlurEffect_X.Init(m_screenWidth, m_screenHeight, dir_x, m_Format, m_rtManager, m_pDevice, m_meshManager, m_AOOut, nullptr, m_Depth, L"HBAO_Blur_X");

    float dir_y[2] = { 0.0, 1.0 };
    std::shared_ptr<RenderTarget> tmpX = BlurEffect_X.GetOut();
    BlurEffect_Y.Init(m_screenWidth, m_screenHeight, dir_y, m_Format, m_rtManager, m_pDevice, m_meshManager, tmpX, m_AOBluredOut, m_Depth, L"HBAO_Blur_Y");

}

void HBAO::CreateNoise(CommandList& CmdList)
{
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels = 1;
    textureDesc.Format = DXGI_FORMAT_R32_FLOAT;
    textureDesc.Width = m_screenWidth / 10;
    textureDesc.Height = m_screenHeight / 10;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    ThrowIfFailed(m_pDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&_Noise)));

    const UINT64 uploadBufferSize = GetRequiredIntermediateSize(_Noise.Get(), 0, 1);

    ThrowIfFailed(m_pDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&_NoiseUploadHeap)));

    m_texture_data = new float[textureDesc.Width * textureDesc.Height];
    for (uint32_t i = 0; i < textureDesc.Width * textureDesc.Height; i++)
        m_texture_data[i] = (float)rand() / (float)RAND_MAX;

    D3D12_SUBRESOURCE_DATA textureData = {};
    textureData.pData = m_texture_data;
    textureData.RowPitch = textureDesc.Width * sizeof(float);
    textureData.SlicePitch = textureData.RowPitch * textureDesc.Height;

    UpdateSubresources(CmdList.GetInternal().Get(), _Noise.Get(), _NoiseUploadHeap.Get(), 0, 0, 1, &textureData);
    _Noise->SetName(L"Noise");

    CmdList.GetInternal()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_Noise.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
}

void HBAO::DoHBAO(std::unique_ptr<CommandList>&  CmdList)
{
    ID3D12GraphicsCommandList *pCmdList = CmdList->GetInternal().Get();

    //////////////////////////////////////////////////////////////////////////
    PIXBeginEvent(pCmdList, 0, "HBAO");

    ID3D12DescriptorHeap* ppHeaps[] = { m_customsHeap.Get() };
    pCmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D12_VIEWPORT AOviewport = { 0, 0, (FLOAT)m_AOscreenWidth, (FLOAT)m_AOscreenHeight, 0.0f, 1.0f };
    D3D12_RECT AOscissor = { 0, 0, (LONG)m_AOscreenWidth, (LONG)m_AOscreenHeight };

    pCmdList->RSSetViewports(1, &AOviewport);
    pCmdList->RSSetScissorRects(1, &AOscissor);

    pCmdList->SetPipelineState(m_PassState->GetPSO().Get());
    pCmdList->SetGraphicsRootSignature(m_RootSignature.GetInternal().Get());

    D3D12_GPU_DESCRIPTOR_HANDLE texHandle = m_customsHeap->GetGPUDescriptorHandleForHeapStart();
    pCmdList->SetGraphicsRootDescriptorTable(0, texHandle);
    pCmdList->SetGraphicsRootConstantBufferView(1, m_cbvParams->GetGPUVirtualAddress());

    RenderTarget* AOrts[8] = { m_AOOut.get() };
    m_rtManager->BindRenderTargets(AOrts, nullptr, *CmdList);
    m_rtManager->ClearRenderTarget(*AOrts[0], *CmdList);

    m_objScreenQuad->Draw(pCmdList);

    BlurEffect_X.DoBlur(CmdList);
    BlurEffect_Y.DoBlur(CmdList);
    PIXEndEvent(pCmdList);
}

const float __zNear = 0.01f;
const float __zFar  = 100.0f;

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
    , _viewCamera(static_cast<float>(screenWidth),
                  static_cast<float>(screenHeight),
                  __zNear,
                  objectOnSceneInRow * __zFar * 100.0f,
                  5.0f * M_PIF / 18.0f,
                  Graphics::ProjectionType::Perspective)
    , _shadowCamera(depthMapSize / depthMapSize * objectOnSceneInRow * 3.0f,
                    depthMapSize / depthMapSize * objectOnSceneInRow * 3.0f,
                    __zNear,
                    objectOnSceneInRow * __zFar * 1000.0f,
                    9.0f * M_PIF / 18.0f,
                    Graphics::ProjectionType::Perspective)
{
    assert(pDevice);
    assert(pTexturesHeap);
    assert(pCmdQueue);
    assert(pSwapChain);
    assert(rtManager);

    SetThreadDescription(GetCurrentThread(), L"Main thread");

    _meshManager.reset(new MeshManager(cmdLineOpts.tessellation, pDevice));

    _viewCamera.SetPosition({ -5, 1, 0 });
    _viewCamera.LookAt({-4, 1, 0});

    _shadowCamera.SetSphericalCoords({}, 0, 0, objectOnSceneInRow * 2.0f);

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
    std::wstring tmp_name = L"_customsHeap";
    _customsHeap.Get()->SetName(tmp_name.c_str());

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

        const math::mat4f projectionMatrix = _viewCamera.GetProjectionMatrix();
        CommandList temporaryCmdList{ CommandListType::Direct, _device };
        _HBAO_effect.Init(_screenWidth, _screenHeight, projectionMatrix, DXGI_FORMAT_R32_FLOAT, _rtManager, pDevice, _meshManager, temporaryCmdList,
                                                                                                                                                    nullptr, _mrtRts[2], L"HBAO");
        temporaryCmdList.Close();
        ExecuteCommandLists(temporaryCmdList);

        customsHeapHandle.ptr += CbvSrvUavHeapIncSize;
        pDevice->CreateShaderResourceView(_HBAO_effect.GetOut()->_texture.Get(), nullptr, customsHeapHandle);
        customsHeapHandle.ptr += CbvSrvUavHeapIncSize;
    }
}

void SceneManager::CreateSceneNodeFromAssimpNode(const aiScene *scene, const aiNode *node, float scale)
{
    if (!scene || !node)
        return;

    for (size_t i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        auto meshObject = _meshManager->CreateFromAssimpMesh(mesh);
        _objects.push_back(std::make_shared<SceneObject>(meshObject, _device, nullptr));
        _objects.back()->Scale({ scale, scale, scale });
    }

    for (size_t i = 0; i < node->mNumChildren; ++i)
        CreateSceneNodeFromAssimpNode(scene, node->mChildren[i], scale);
}

SceneManager::~SceneManager()
{
    FinishWorkerThreads();
    WaitCurrentFrame();
}

void SceneManager::SetBackgroundCubemap(const std::wstring& name)
{
    ComPtr<ID3D12Resource> textureUploadBuffer;

    CommandList uploadCommandList {CommandListType::Direct, _device};
    ID3D12GraphicsCommandList *pCmdList = uploadCommandList.GetInternal().Get();
    PIXSetMarker(pCmdList, 0, "Somewhere in the depths");

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

    {
        PIXScopedEvent(_cmdQueue.Get(), PIX_COLOR(255, 0, 0), "Cubemap filling");
        ExecuteCommandLists(uploadCommandList);
    }
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
    if (_isFrameWaiting && GetRandomNumber(1, 100) > 75)
    {
        _isFrameWaiting = false;
        PIXEndEvent(_cmdQueue.Get());
    }
    else if (!_isFrameWaiting && GetRandomNumber(1, 100) > 75)
    {
        _isFrameWaiting = true;
        PIXBeginEvent(_cmdQueue.Get(), PIX_COLOR(0, 0, 255), "Multiframe random region");
    }

    std::vector<ID3D12CommandList*> cmdListArray;
    cmdListArray.reserve(16); // just to avoid any allocations

    FillViewProjMatrix();
    FillSceneProperties();

    // Clear and shadow pass (if enabled)
    {
        PIXScopedEvent(_cmdQueue.Get(), PIX_COLOR(0, 0, 0), "Clear & shadows");
        if (_cmdLineOpts.shadow_pass)
            PopulateDepthPassCommandList();
        cmdListArray.push_back(_clearPassCmdList->GetInternal().Get());
        if (_cmdLineOpts.shadow_pass)
            cmdListArray.push_back(_depthPassCmdList->GetInternal().Get());
        _cmdQueue->ExecuteCommandLists(_cmdLineOpts.shadow_pass ? 2 : 1, cmdListArray.data());
    }

    // G-buffer pass
    {
        PIXScopedEvent(_cmdQueue.Get(), PIX_COLOR(0, 255, 0), "G-buffer");
        cmdListArray.clear();
        PopulateWorkerCommandLists();
        for (auto& pWorkList : _workerCmdLists)
            cmdListArray.push_back(pWorkList->GetInternal().Get());
        PIXSetMarker(_cmdQueue.Get(), 0, "Queue marker");
        _cmdQueue->ExecuteCommandLists((UINT)_workerCmdLists.size(), cmdListArray.data());
    }

    // Lighting and post-processing
    {
        PIXScopedEvent(_cmdQueue.Get(), PIX_COLOR(255, 255, 255), "Lighting and post-process");
        cmdListArray.clear();
        PopulateLightPassCommandList();
        cmdListArray.push_back(_lightPassCmdList->GetInternal().Get());
        _cmdQueue->ExecuteCommandLists(1, cmdListArray.data());
    }

    // Swap buffers
    _swapChain->Present(0, 0);
    WaitCurrentFrame();
}

void SceneManager::ExecuteCommandLists(const CommandList & commandList)
{
    {
        PIXScopedEvent(_cmdQueue.Get(), 0, "Submitting");
        std::array<ID3D12CommandList*, 1> cmdListsArray = { commandList.GetInternal().Get() };
        _cmdQueue->ExecuteCommandLists((UINT)cmdListsArray.size(), cmdListsArray.data());
    }

    _fenceValue++;
    WaitCurrentFrame();
}

Graphics::Camera * SceneManager::GetViewCamera()
{
    return &_viewCamera;
}

Graphics::Camera * SceneManager::GetShadowCamera()
{
    return &_shadowCamera;
}

void SceneManager::LoadScene(const std::string& filename, float scale)
{
    Assimp::Importer importer;
    const aiScene * scene = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_GenNormals | aiProcess_OptimizeGraph);
    CreateSceneNodeFromAssimpNode(scene, scene->mRootNode, scale);
}

void SceneManager::ReLoadScene(const std::string& filename, float scale)
{
    _objects.clear();
    LoadScene(filename, scale);
}

void SceneManager::PopulateClearPassCommandList()
{
    // PRE-PASS - clear final render targets to draw
    _clearPassCmdList->Reset();
    ID3D12GraphicsCommandList *pCmdList = _clearPassCmdList->GetInternal().Get();

    PIXBeginEvent(pCmdList, 0, "Render targets clear");

    std::array barriers{
        CD3DX12_RESOURCE_BARRIER::Transition(_mrtRts[0]->_texture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
        CD3DX12_RESOURCE_BARRIER::Transition(_mrtRts[1]->_texture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
        CD3DX12_RESOURCE_BARRIER::Transition(_mrtRts[2]->_texture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET),
    };

    pCmdList->ResourceBarrier((UINT)barriers.size(), barriers.data());

    RenderTarget* rts[8] = {_mrtRts[0].get(), _mrtRts[1].get(), _mrtRts[2].get()};
    // is not necessary, but just for test
    _rtManager->BindRenderTargets(rts, _mrtDepth.get(), *_clearPassCmdList);

    _rtManager->ClearRenderTarget(*_mrtRts[0], *_clearPassCmdList);
    _rtManager->ClearRenderTarget(*_mrtRts[1], *_clearPassCmdList);
    _rtManager->ClearRenderTarget(*_mrtRts[2], *_clearPassCmdList);
    PIXSetMarker(pCmdList, 0, "Some marker!");
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

    {
        auto transition = CD3DX12_RESOURCE_BARRIER::Transition(_shadowDepth->_texture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        pCmdList->ResourceBarrier(1, &transition);
    }

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

    _HBAO_effect.DoHBAO(_lightPassCmdList);

    D3D12_GPU_DESCRIPTOR_HANDLE texHandle;
    ID3D12DescriptorHeap* ppHeaps[] = { _customsHeap.Get() };
    pCmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    //////////////////////////////////////////////////////////////////////////
    D3D12_VIEWPORT viewport = { 0, 0, (FLOAT)_screenWidth, (FLOAT)_screenHeight, 0.0f, 1.0f };
    D3D12_RECT scissor = { 0, 0, (LONG)_screenWidth, (LONG)_screenHeight };

    pCmdList->RSSetViewports(1, &viewport);
    pCmdList->RSSetScissorRects(1, &scissor);

    pCmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    PIXBeginEvent(pCmdList, 0, "Light rendering");
    pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    {
        auto transition = CD3DX12_RESOURCE_BARRIER::Transition(_HDRRt->_texture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        pCmdList->ResourceBarrier(1, &transition);
    }

    RenderTarget* rts[8] = {_HDRRt.get()};
    _rtManager->BindRenderTargets(rts, nullptr, *_lightPassCmdList);
    _rtManager->ClearRenderTarget(*rts[0], *_lightPassCmdList);

    std::array<D3D12_RESOURCE_BARRIER, 4> barriers = {
        CD3DX12_RESOURCE_BARRIER::Transition(_mrtRts[0]->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(_mrtRts[1]->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(_mrtRts[2]->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(_HBAO_effect.GetOut()->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
    };

    pCmdList->ResourceBarrier((UINT)barriers.size(), barriers.data());

    if (_cmdLineOpts.shadow_pass) {
        auto transition = CD3DX12_RESOURCE_BARRIER::Transition(_shadowDepth->_texture.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        pCmdList->ResourceBarrier(1, &transition);
    }

    //pCmdList->ResourceBarrier(std::size(barriers), barriers.data());
    pCmdList->SetPipelineState(_lightPassState->GetPSO().Get());
    pCmdList->SetGraphicsRootSignature(_lightRootSignature.GetInternal().Get());

    // Set sampler heap.
    texHandle = _customsHeap->GetGPUDescriptorHandleForHeapStart();
    pCmdList->SetGraphicsRootDescriptorTable(0, texHandle);

    texHandle.ptr += texHeapIncSize * (_cmdLineOpts.shadow_pass ? 7 : 6);
    pCmdList->SetGraphicsRootDescriptorTable(1, texHandle);

    texHandle = _customsHeap->GetGPUDescriptorHandleForHeapStart();
    texHandle.ptr += texHeapIncSize * (_cmdLineOpts.shadow_pass ? 8 : 7);
    pCmdList->SetGraphicsRootDescriptorTable(2, texHandle);

    pCmdList->SetGraphicsRootConstantBufferView(3, _cbvSceneParams->GetGPUVirtualAddress());

    _objScreenQuad->Draw(pCmdList);
    PIXEndEvent(pCmdList);

    //////////////////////////////////////////////////////////////////////////
    // First, we need to compute average intensity through the rendered and lighted render target
    // Next, we should use computed intensity to tone final RT
    // This can be easily done with compute shaders


    PIXBeginEvent(pCmdList, 0, "Luminance computing");
    {
        auto transition = CD3DX12_RESOURCE_BARRIER::Transition(_HDRRt->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        pCmdList->ResourceBarrier(1, &transition);
    }
    {
        auto transition = CD3DX12_RESOURCE_BARRIER::Transition(_finalIntensityBuffer.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        pCmdList->ResourceBarrier(1, &transition);
    }

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

    {
        auto transition = CD3DX12_RESOURCE_BARRIER::Transition(_finalIntensityBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        pCmdList->ResourceBarrier(1, &transition);
    }
    PIXEndEvent(pCmdList);

    //////////////////////////////////////////////////////////////////////////


    PIXBeginEvent(pCmdList, 0, "HDR -> LDR pass");
    {
        auto transition = CD3DX12_RESOURCE_BARRIER::Transition(_swapChainRTs[_frameIndex]->_texture.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        pCmdList->ResourceBarrier(1, &transition);
    }
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
    {
        auto transition = CD3DX12_RESOURCE_BARRIER::Transition(_swapChainRTs[_frameIndex]->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        pCmdList->ResourceBarrier(1, &transition);
        transition = CD3DX12_RESOURCE_BARRIER::Transition(_HBAO_effect.GetOut()->_texture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        pCmdList->ResourceBarrier(1, &transition);
    }

    PIXEndEvent(pCmdList);

    _lightPassCmdList->Close();
}

void SceneManager::WaitCurrentFrame()
{
    PIXScopedEvent(_cmdQueue.Get(), 0, "Waiting for a fence");
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
    if (!_threadPool.empty())
        SetThreadDescription(GetCurrentThread(), L"Render thread");

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

                if (_workerThreadExit)
                    return; // terminate thread
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
            currentObjectIndex = ++_drawObjectIndex;
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
    std::array<D3D12_RESOURCE_BARRIER, 5> barriers = {
        CD3DX12_RESOURCE_BARRIER::Transition(_mrtRts[0]->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(_mrtRts[1]->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(_mrtRts[2]->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(_HDRRt->_texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(_mrtDepth->_texture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE)
    };

    CommandList temporaryCmdList {CommandListType::Direct, _device};
    temporaryCmdList.GetInternal()->ResourceBarrier((UINT)barriers.size(), barriers.data());
    temporaryCmdList.Close();

    ExecuteCommandLists(temporaryCmdList);
}

constexpr uint32_t Align(uint32_t size, uint32_t align) {return (size+align-1) & (~(align-1));}

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

    for (auto & tid : _threadPool)
        tid.join();
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
    _lightRootSignature.Init(4, 2);

    _lightRootSignature[0].InitAsDescriptorsTable(1);
    _lightRootSignature[0].InitTableRange(0, 0, 4, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);

    _lightRootSignature[1].InitAsDescriptorsTable(1);
    _lightRootSignature[1].InitTableRange(0, 4, 1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);

    _lightRootSignature[2].InitAsDescriptorsTable(1);
    _lightRootSignature[2].InitTableRange(0, 5, 1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);

    _lightRootSignature[3].InitAsCBV(0);

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

    math::mat4f viewProjectionMatrix = _viewCamera.GetViewProjMatrix();
    std::memcpy(reinterpret_cast<perFrameParamsConstantBuffer*>(_cbvMRT)->viewProjectionMatrix, viewProjectionMatrix.arr, sizeof(math::mat4f));
    std::memcpy(reinterpret_cast<perFrameParamsConstantBuffer*>(_cbvMRT)->cameraPosition, &_viewCamera.GetPosition(), sizeof(math::vec3f));

    viewProjectionMatrix = _shadowCamera.GetViewProjMatrix();
    std::memcpy(reinterpret_cast<perFrameParamsConstantBuffer*>(_cbvDepth)->viewProjectionMatrix, viewProjectionMatrix.arr, sizeof(math::mat4f));
    std::memcpy(reinterpret_cast<perFrameParamsConstantBuffer*>(_cbvDepth)->cameraPosition, &_shadowCamera.GetPosition(), sizeof(math::vec3f));
}

void SceneManager::FillSceneProperties()
{
    assert(_cbvScene);
    if (!_cbvScene)
        return;

    math::vec3f lightPos = _shadowCamera.GetPosition();
    XMFLOAT3 ambientColor = {0.2f, 0.2f, 0.2f};
    XMFLOAT3 fogColor = {clearColor[0], clearColor[1], clearColor[2]};
    math::mat4f shadowMatrix = _shadowCamera.GetViewProjMatrix();
    math::mat4f inversedViewProj = math::matrixInverse(_viewCamera.GetViewProjMatrix());
    math::vec3f eyePosition = _viewCamera.GetPosition();

    auto* cbuffer = reinterpret_cast<sceneParamsConstantBuffer*>(_cbvScene);
    std::memcpy(cbuffer->lightPosition, &lightPos, sizeof(math::vec3f));
    std::memcpy(cbuffer->ambientColor, &ambientColor, sizeof(XMFLOAT3));
    std::memcpy(cbuffer->fogColor, &fogColor, sizeof(XMFLOAT3));
    // assume that we have box-shaped scene
    cbuffer->sceneSize = std::powf((float)_objects.size(), 0.333f);
    cbuffer->depthMapSize = static_cast<float>(depthMapSize);

    std::memcpy(cbuffer->shadowMatrix, shadowMatrix.arr, sizeof(math::mat4f));
    std::memcpy(cbuffer->inverseViewProj, inversedViewProj.arr, sizeof(math::mat4f));
    std::memcpy(cbuffer->camPosition, &eyePosition, sizeof(math::vec3f));
}

void SceneManager::CreateRootSignatures()
{
    CreateMRTPassRootSignature();
    CreateLightPassRootSignature();
    CreateLDRPassRootSignature();
    CreateIntensityPassRootSignature();

    if (_cmdLineOpts.shadow_pass)
        CreateDepthPassRootSignature();
}
