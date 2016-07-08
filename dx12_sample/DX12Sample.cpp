#include "stdafx.h"

#include "DX12Sample.h"

#include "Math.h"
#include <utils/Shaders.h>
#include <3rdparty/DDSTextureLoader.h>
#include <utils/FeaturesCollector.h>

DX12Sample::DX12Sample(int windowWidth, int windowHeight, std::set<optTypes>& opts)
    : DXSample(windowWidth, windowHeight, L"HELLO YOPTA")
{
    for (auto& opt : opts)
    {
        switch (opt)
        {
        case disable_bundles:
            _cmdLineOpts.bundles = false;
            break;
        case disable_concurrency:
            _cmdLineOpts.threads = false;
            break;
        case disable_textures:
            _cmdLineOpts.textures = false;
            break;
        case disable_root_constants:
            _cmdLineOpts.root_constants = false;
            break;
        case disable_shadow_pass:
            _cmdLineOpts.shadow_pass = false;
            break;
        case enable_tessellation:
            _cmdLineOpts.tessellation = true;
            break;
        }
    }
}

DX12Sample::~DX12Sample()
{
    // explicitly here
    _sceneManager.reset();
}

void DX12Sample::OnInit()
{
#ifdef _DEBUG
    // Enable the D3D12 debug layer.
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
    }
#endif

    CreateDXGIFactory();
    CreateDevice();
    DumpFeatures();
    CreateCommandQueue();
    CreateSwapChain();

    _RTManager.reset(new RenderTargetManager(_device));
    CreateTexturesHeap();

    _sceneManager.reset(new SceneManager(_device,
                                         m_width,
                                         m_height,
                                         _cmdLineOpts,
                                         _texturesHeap,
                                         _cmdQueue,
                                         _swapChain,
                                         _RTManager.get(),
                                         _objectsInRow));

    CreateTextures();
    CreateObjects();
}

void DX12Sample::OnUpdate()
{
    static std::chrono::high_resolution_clock clock;
    static auto prevTime = clock.now();
    static int framesCount = 0;
    std::chrono::milliseconds diff = std::chrono::duration_cast<std::chrono::milliseconds>(clock.now() - prevTime);
    framesCount++;
    if (diff.count() > 1000)
    {
        std::wostringstream ss;
        ss << "FPS: " << framesCount;
        SetWindowText(m_hwnd, ss.str().c_str());
        prevTime = clock.now();
        framesCount = 0;
    }

    static float currentCounter = 0.0f;
    currentCounter += 0.01f;

    for (uint32_t objIndex = 0; objIndex < _drawObjectsCount; ++objIndex)
    {
        auto & object = _objects[objIndex];
        object->Rotation(currentCounter);
    }

    static int time = 0;
    time++;

    {
        auto shadowCamera = _sceneManager->GetShadowCamera();
        float rotation = time * 0.1f;
        float inclination = std::sinf(time * 0.01f) * 45.0f;
        shadowCamera->SetInclination(inclination);
        shadowCamera->SetRotation(rotation);
    }

    {
        auto viewCamera = _sceneManager->GetViewCamera();
        static float inclination = -30.0f, rotation = 0.0f;
        rotation += 0.05f;
        float radius = (std::sinf(time * 0.005f) + 1.0f) * _objectsInRow / 2 + _objectsInRow;
        viewCamera->SetInclination(inclination);
        viewCamera->SetRotation(rotation);
        viewCamera->SetRadius(radius);
    }
}

void DX12Sample::OnRender()
{
    _sceneManager->DrawAll();
}

void DX12Sample::OnDestroy()
{
}

bool DX12Sample::OnEvent(MSG msg)
{
    switch (msg.message)
    {
    case WM_KEYDOWN:
        if (msg.wParam == VK_ESCAPE)
            exit(0);
    }

    // DXSample does not check return value, so it will be false :)
    return false;
}

void DX12Sample::DumpFeatures()
{
    FeaturesCollector collector {_device};
    collector.CollectFeatures("deviceInfo.log");
}

static size_t generatePixels(std::vector<uint8_t>& pixels,
                             std::vector<D3D12_SUBRESOURCE_DATA>& mips_data,
                             const size_t width,
                             const size_t height,
                             const size_t depth,
                             const bool is_volumed = false)
{
    const int bytepp = 4;
    const size_t row_size_limit = 256 / bytepp;
    size_t generated_mips = 0;
    size_t pixels_generated = 0;

    if (!width || !height || !depth)
        return 0;

    const size_t align_width = (width > row_size_limit) ? width + (row_size_limit - width % row_size_limit) : row_size_limit;

    std::vector<size_t> mip_offsets;

    pixels.clear();
    mips_data.clear();

    pixels.resize(align_width * height * bytepp * 10 * depth, 0x0);

    uint32_t* p_pixels = (uint32_t*)pixels.data();
    uint64_t mip_offset = 0;

    size_t array_size = is_volumed ? 1 : depth;
    size_t max_depth = is_volumed ? depth : 1;

    for (size_t level = 0; level < array_size; ++level)
    {
        size_t mip_width = width;
        size_t mip_height = height;
        size_t mip_depth = max_depth;
        generated_mips = 0;

        // for each mip...
        while (true)
        {
            mip_offsets.push_back((uint64_t)p_pixels - (uint64_t)pixels.data());  // only for debug
            mips_data.push_back({(void*)p_pixels, (int64_t)(mip_width * bytepp), (int64_t)(mip_width * mip_height * bytepp)});

            for (size_t slice = 0; slice < mip_depth; ++slice)
            {
                size_t delimx = (mip_width >= 8) ? 8 : mip_width;
                size_t delimy = (mip_width >= 8) ? 8 : mip_height;

                for (UINT row = 0; row < mip_height; row++)
                {
                    for (UINT col = 0; col < mip_width; col++)
                    {
                        size_t xBlock = col * delimx / mip_width;
                        size_t yBlock = row * delimy / mip_height;

                        if (xBlock % 2 == yBlock % 2)
                        {
                            p_pixels[mip_width*row + col] = slice & 1 ? 0xFF'FF'FF'FF : 0xFF'C4'92'00;
                        }
                        else
                        {
                            p_pixels[mip_width*row + col] = slice & 1 ? 0xFF'C4'92'00 : 0xFF'FF'FF'FF;
                        }
                    }
                }
                p_pixels += mip_width * mip_height;
            }
            ++generated_mips;

            if (level == array_size - 1 && mip_width == 1 && mip_height == 1 && mip_depth == 1)
                break;

            size_t row_padding = 0;
            if (mip_width % row_size_limit)
            {
                row_padding = row_size_limit - mip_width % row_size_limit;
                p_pixels += row_padding * mip_height * mip_depth; // add row padding for alignment
            }

            if (((mip_width + row_padding) * mip_height * mip_depth) & (2 * row_size_limit - 1))
                p_pixels += row_size_limit;                       // add mip padding for alignment

            if (mip_width == 1 && mip_height == 1 && mip_depth == 1)
                break;

            mip_width = mip_width > 1 ? mip_width >> 1 : 1;
            mip_height = mip_height > 1 ? mip_height >> 1 : 1;
            mip_depth = mip_depth > 1 ? mip_depth >> 1 : 1;

            //if (width > 2*mip_width)
            //    break;
        }
    }
    pixels.resize((uint8_t*)p_pixels - pixels.data());
    return generated_mips;
}

void DX12Sample::CreateTextures()
{
    CommandList uploadCommandList {CommandListType::Direct, _device};
    ID3D12GraphicsCommandList *pCmdList = uploadCommandList.GetInternal().Get();

    D3D12_HEAP_PROPERTIES defaultHeapProp = {D3D12_HEAP_TYPE_DEFAULT};
    D3D12_HEAP_PROPERTIES uploadHeapProp = {D3D12_HEAP_TYPE_UPLOAD};
    D3D12_CPU_DESCRIPTOR_HANDLE texturesHeapHandle = _texturesHeap->GetCPUDescriptorHandleForHeapStart();
    UINT CbvSrvUavHeapIncSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    std::vector<uint8_t> texturePixels;
    std::vector<D3D12_SUBRESOURCE_DATA> textureMips;

    size_t textureWidth = 255;
    size_t textureHeight = 255;
    size_t textureSlices = 30;

    // Have to create and fill vertex buffer
    size_t mipsCount = generatePixels(texturePixels, textureMips, textureWidth, textureHeight, textureSlices);
    size_t imagesGenerated = mipsCount * textureSlices;

    ComPtr<ID3D12Resource> textureUploadBuffer[6] = {};
    D3D12_RESOURCE_DESC textureResourceDesc = {};
    UINT64 uploadBufferSize = 0;
    D3D12_RESOURCE_DESC uploadBufferDesc = {
        D3D12_RESOURCE_DIMENSION_BUFFER,
        0,
        0,
        1,
        1,
        1,
        DXGI_FORMAT_UNKNOWN,
        1,
        0,
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        D3D12_RESOURCE_FLAG_NONE
    };
    D3D12_SHADER_RESOURCE_VIEW_DESC textureSRVDesc = {};

    {
        // BC6U texture - 2D
        ID3D12Resource * pTex_bc6 = nullptr;
        ID3D12Resource * pTex_bc6_upload = nullptr;
        ThrowIfFailed(CreateDDSTextureFromFile(_device.Get(), L"Assets/Textures/tex_bc6u.dds", 1024, true, &pTex_bc6, pCmdList, &pTex_bc6_upload, texturesHeapHandle));
        assert(pTex_bc6 && pTex_bc6_upload);
        _texture[4].Attach(pTex_bc6);
        _texture[4]->SetName(L"BC6u");
        textureUploadBuffer[4].Attach(pTex_bc6_upload);
        texturesHeapHandle.ptr += CbvSrvUavHeapIncSize;

        // BC6S texture
        ThrowIfFailed(CreateDDSTextureFromFile(_device.Get(), L"Assets/Textures/tex_bc6s.dds", 1024, true, &pTex_bc6, pCmdList, &pTex_bc6_upload, texturesHeapHandle));
        assert(pTex_bc6 && pTex_bc6_upload);
        _texture[5].Attach(pTex_bc6);
        _texture[5]->SetName(L"BC6s");
        textureUploadBuffer[5].Attach(pTex_bc6_upload);
        texturesHeapHandle.ptr += CbvSrvUavHeapIncSize;
    }

    {
        // *** TEX0 *** - 2D mips - default
        // To upload texture data we have to use temp resource to copy data from CPU to it, and then order cmd list to copy to destination texture
        textureResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        textureResourceDesc.Width = (UINT)textureWidth;
        textureResourceDesc.Height = (UINT)textureHeight;
        textureResourceDesc.MipLevels = (UINT16)mipsCount;
        textureResourceDesc.SampleDesc.Count = 1;
        textureResourceDesc.DepthOrArraySize = (UINT16)textureSlices;
        textureResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        textureResourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

        ThrowIfFailed(_device->CreateCommittedResource(&defaultHeapProp,
                                                       D3D12_HEAP_FLAG_NONE,
                                                       &textureResourceDesc,
                                                       D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                       IID_PPV_ARGS(&_texture[0])));

        uploadBufferSize = GetRequiredIntermediateSize(_texture[0].Get(), 0, (UINT)imagesGenerated);

        // Create the GPU upload buffer
        uploadBufferDesc.Width = uploadBufferSize;
        ThrowIfFailed(_device->CreateCommittedResource(&uploadHeapProp,
                                                       D3D12_HEAP_FLAG_NONE,
                                                       &uploadBufferDesc,
                                                       D3D12_RESOURCE_STATE_GENERIC_READ,
                                                       nullptr,
                                                       IID_PPV_ARGS(&textureUploadBuffer[0])));

        UpdateSubresources(pCmdList,
                           _texture[0].Get(),
                           textureUploadBuffer[0].Get(),
                           0,
                           0,
                           (UINT)imagesGenerated,
                           textureMips.data());

        // Add barrier to upload texture
        pCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_texture[0].Get(),
                                                                           D3D12_RESOURCE_STATE_COPY_DEST,
                                                                           D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        // Now we need to prepare SRV for Texture
        textureSRVDesc.Format = textureResourceDesc.Format;
        textureSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        textureSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        textureSRVDesc.Texture2D.MipLevels = (UINT16)mipsCount;
        textureSRVDesc.Texture2D.MostDetailedMip = 0;

        _texture[0]->SetName(L"TEXTURE2D with mips");

        _device->CreateShaderResourceView(_texture[0].Get(), &textureSRVDesc, texturesHeapHandle);
        texturesHeapHandle.ptr += CbvSrvUavHeapIncSize;
    }

    {
        // *** TEX1 *** - 2D mips array
        textureResourceDesc.Width = (UINT)textureWidth;
        textureResourceDesc.Height = (UINT)textureHeight;
        textureResourceDesc.MipLevels = (UINT16)mipsCount;
        textureResourceDesc.DepthOrArraySize = (UINT16)textureSlices;

        ThrowIfFailed(_device->CreateCommittedResource(&defaultHeapProp,
                                                       D3D12_HEAP_FLAG_NONE,
                                                       &textureResourceDesc,
                                                       D3D12_RESOURCE_STATE_COPY_DEST,
                                                       nullptr,
                                                       IID_PPV_ARGS(&_texture[1])));

        uploadBufferSize = GetRequiredIntermediateSize(_texture[1].Get(),
                                                       0,
                                                       (UINT)imagesGenerated);

        // Create the GPU upload buffer
        uploadBufferDesc.Width = uploadBufferSize;
        ThrowIfFailed(_device->CreateCommittedResource(&uploadHeapProp,
                                                       D3D12_HEAP_FLAG_NONE,
                                                       &uploadBufferDesc,
                                                       D3D12_RESOURCE_STATE_GENERIC_READ,
                                                       nullptr,
                                                       IID_PPV_ARGS(&textureUploadBuffer[1])));

        UpdateSubresources(pCmdList,
                           _texture[1].Get(),
                           textureUploadBuffer[1].Get(),
                           0,
                           0,
                           (UINT)imagesGenerated,
                           textureMips.data());

        // Add barrier to upload texture
        pCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_texture[1].Get(),
                                                                           D3D12_RESOURCE_STATE_COPY_DEST,
                                                                           D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        // Now we need to prepare SRV for Texture
        textureSRVDesc.Format = textureResourceDesc.Format;
        textureSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        textureSRVDesc.Texture2DArray.MipLevels = (UINT16)mipsCount;
        textureSRVDesc.Texture2DArray.MostDetailedMip = 0;
        textureSRVDesc.Texture2DArray.ArraySize = (UINT)textureSlices;

        _texture[1]->SetName(L"TEXTURE2D ARRAY with mips");

        _device->CreateShaderResourceView(_texture[1].Get(), &textureSRVDesc, texturesHeapHandle);
        texturesHeapHandle.ptr += CbvSrvUavHeapIncSize;
    }

    {
        // *** TEX2 *** - CubeMap mips array
        textureResourceDesc.Width = (UINT)textureWidth;
        textureResourceDesc.Height = (UINT)textureHeight;
        textureResourceDesc.MipLevels = (UINT16)mipsCount;
        textureResourceDesc.DepthOrArraySize = (UINT16)textureSlices;

        ThrowIfFailed(_device->CreateCommittedResource(&defaultHeapProp,
                                                       D3D12_HEAP_FLAG_NONE,
                                                       &textureResourceDesc,
                                                       D3D12_RESOURCE_STATE_COPY_DEST,
                                                       nullptr,
                                                       IID_PPV_ARGS(&_texture[2])));

        uploadBufferSize = GetRequiredIntermediateSize(_texture[2].Get(),
                                                       0,
                                                       (UINT)imagesGenerated);

        // Create the GPU upload buffer
        uploadBufferDesc.Width = uploadBufferSize;
        ThrowIfFailed(_device->CreateCommittedResource(&uploadHeapProp,
                                                       D3D12_HEAP_FLAG_NONE,
                                                       &uploadBufferDesc,
                                                       D3D12_RESOURCE_STATE_GENERIC_READ,
                                                       nullptr,
                                                       IID_PPV_ARGS(&textureUploadBuffer[2])));

        UpdateSubresources(pCmdList,
                           _texture[2].Get(),
                           textureUploadBuffer[2].Get(),
                           0,
                           0,
                           (UINT)imagesGenerated,
                           textureMips.data());

        // Add barrier to upload texture
        pCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_texture[2].Get(),
                                                                           D3D12_RESOURCE_STATE_COPY_DEST,
                                                                           D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        // Now we need to prepare SRV for Texture
        textureSRVDesc.Format = textureResourceDesc.Format;
        textureSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
        textureSRVDesc.TextureCubeArray.MipLevels = (UINT16)mipsCount;
        textureSRVDesc.TextureCubeArray.MostDetailedMip = 0;
        textureSRVDesc.TextureCubeArray.NumCubes = (UINT)textureSlices / 6;

        _texture[2]->SetName(L"CUBEMAP ARRAY with mips");

        _device->CreateShaderResourceView(_texture[2].Get(), &textureSRVDesc, texturesHeapHandle);
        texturesHeapHandle.ptr += CbvSrvUavHeapIncSize;
    }

    {
        // *** TEX3 *** - Texture 3D
        textureWidth = 32;
        textureHeight = 32;
        textureSlices = 32;

        // Have to create and fill vertex buffer
        mipsCount = generatePixels(texturePixels,
                                   textureMips,
                                   textureWidth,
                                   textureHeight,
                                   textureSlices,
                                   true);
        imagesGenerated = textureMips.size();

        textureResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
        textureResourceDesc.Width = (UINT)textureWidth;
        textureResourceDesc.Height = (UINT)textureHeight;
        textureResourceDesc.MipLevels = (UINT16)mipsCount;
        textureResourceDesc.DepthOrArraySize = (UINT16)textureSlices;

        ThrowIfFailed(_device->CreateCommittedResource(&defaultHeapProp,
                                                       D3D12_HEAP_FLAG_NONE,
                                                       &textureResourceDesc,
                                                       D3D12_RESOURCE_STATE_COPY_DEST,
                                                       nullptr,
                                                       IID_PPV_ARGS(&_texture[3])));

        uploadBufferSize = GetRequiredIntermediateSize(_texture[3].Get(),
                                                       0,
                                                       (UINT)imagesGenerated);

        // Create the GPU upload buffer
        uploadBufferDesc.Width = uploadBufferSize;
        ThrowIfFailed(_device->CreateCommittedResource(&uploadHeapProp,
                                                       D3D12_HEAP_FLAG_NONE,
                                                       &uploadBufferDesc,
                                                       D3D12_RESOURCE_STATE_GENERIC_READ,
                                                       nullptr,
                                                       IID_PPV_ARGS(&textureUploadBuffer[3])));

        UpdateSubresources(pCmdList,
                           _texture[3].Get(),
                           textureUploadBuffer[3].Get(),
                           0,
                           0,
                           (UINT)imagesGenerated,
                           textureMips.data());

        // Add barrier to upload texture
        pCmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(_texture[3].Get(),
                                                                           D3D12_RESOURCE_STATE_COPY_DEST,
                                                                           D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        // Now we need to prepare SRV for Texture
        textureSRVDesc.Format = textureResourceDesc.Format;
        textureSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        textureSRVDesc.Texture3D.MipLevels = (UINT16)mipsCount;
        textureSRVDesc.Texture3D.MostDetailedMip = 0;

        _texture[3]->SetName(L"TEXTURE3D with mips");

        _device->CreateShaderResourceView(_texture[3].Get(), &textureSRVDesc, texturesHeapHandle);
        texturesHeapHandle.ptr += CbvSrvUavHeapIncSize;
    }

    uploadCommandList.Close();
    _sceneManager->ExecuteCommandLists(uploadCommandList);
    _sceneManager->SetTextures(_texture);
}

void DX12Sample::CreateDXGIFactory()
{
    // create dx objects
    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&_DXFactory)));
}

void DX12Sample::CreateDevice()
{
    // ComPtr<IDXGIAdapter1> hardwareAdapter;
    // GetHardwareAdapter(pDXFactory.Get(), &hardwareAdapter);
    ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_device)));
}

void DX12Sample::CreateCommandQueue()
{
    // Create commandQueue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_cmdQueue)));
}

void DX12Sample::CreateSwapChain()
{
    // Create swapChain in factory using CmdQueue
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = _swapChainBuffersCount;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.Width = m_width;
    swapChainDesc.BufferDesc.Height = m_height;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.OutputWindow = m_hwnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = TRUE;

    IDXGISwapChain* pTmpSwapChain = nullptr;
    ThrowIfFailed(_DXFactory->CreateSwapChain(_cmdQueue.Get(), &swapChainDesc, &pTmpSwapChain));
    _swapChain.Attach(reinterpret_cast<IDXGISwapChain3*>(pTmpSwapChain));
}

void DX12Sample::CreateTexturesHeap()
{
    // Create Heap for the Texture
    D3D12_DESCRIPTOR_HEAP_DESC texHeapDesc = {};
    texHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    texHeapDesc.NumDescriptors = 100;
    texHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(_device->CreateDescriptorHeap(&texHeapDesc, IID_PPV_ARGS(&_texturesHeap)));
}

void DX12Sample::CreateObjects()
{
    for (size_t i = 0; i < _drawObjectsCount; ++i)
    {
        if (i % 2 == 0)
            _objects.push_back(_sceneManager->CreateFilledCube());
        else
            _objects.push_back(_sceneManager->CreateOpenedCube());
    }

    for (int i = 0; i < _drawObjectsCount; ++i)
    {
        uint32_t plane = i / (_objectsInRow * _objectsInRow);
        uint32_t row = i / _objectsInRow % _objectsInRow;
        uint32_t col = i % _objectsInRow;

        auto & object = _objects[i];

        // Shift x and y position to place scene at screen center.
        float x_coord = _objDistance * col - (_objectsInRow - 1) * _objDistance / 2.0f;
        float y_coord = _objDistance * plane - (_objectsInRow - 1) * _objDistance / 2.0f;
        float z_coord = _objDistance * row - (_objectsInRow - 1) * _objDistance / 2.0f;

        object->Position({x_coord, y_coord, z_coord});
    }

    _plane = _sceneManager->CreatePlane();
    _plane->Position({0.0f, -(float)_objectsInRow, 0.0f});
    _plane->Scale({_objectsInRow*_objectsInRow, 1.0f, _objectsInRow*_objectsInRow});
}
