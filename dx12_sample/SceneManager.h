#pragma once

#include <utils/RenderTargetManager.h>
#include <utils/ComputePipelineState.h>
#include <utils/GraphicsPipelineState.h>
#include <utils/MeshManager.h>
#include <utils/RootSignature.h>
#include <utils/SceneObject.h>
#include <utils/CommandList.h>
#include <utils/Types.h>
#include <utils/Camera.h>

#include "stdafx.h"

class BlurEffect
{
public:
	void Init(UINT _screenWidth, UINT _screenHeight, float direction[], DXGI_FORMAT format,
				RenderTargetManager* _rtManager, ComPtr<ID3D12Device> _pDevice, std::shared_ptr<MeshManager>&  _meshManager, std::shared_ptr<RenderTarget> Input,
				std::shared_ptr<RenderTarget> Output, std::shared_ptr<RenderTarget> Depth, const std::wstring& BlurName = L"Blur");

	void DoBlur(std::unique_ptr<CommandList>&  CmdList);

	std::shared_ptr<RenderTarget> GetOut(void) { return m_Out; };
	std::shared_ptr<RenderTarget> GetInp(void) { return m_Inp; };

protected:
	struct BlurParams
	{
		float step[2];
	};

	UINT m_screenWidth = 1024;
	UINT m_screenHeight = 768;
	float m_direction[2];
	RenderTargetManager* m_rtManager = nullptr;
	std::shared_ptr<RenderTarget> m_Inp;
	std::shared_ptr<RenderTarget> m_Out;
	std::shared_ptr<RenderTarget> m_Depth;
	bool m_OutIsExternalRT;
	ComPtr<ID3D12Device> m_pDevice;
	ComPtr<ID3D12DescriptorHeap> m_customsHeap = nullptr;
	std::wstring m_Name = L"";
	DXGI_FORMAT m_Format;
	std::unique_ptr<GraphicsPipelineState>  m_PassState = nullptr;
	RootSignature  m_RootSignature;
	std::unique_ptr<SceneObject>  m_objScreenQuad = nullptr;
	std::shared_ptr<MeshManager>  m_meshManager = nullptr;
	ComPtr<ID3D12Resource>  m_cbvParams = nullptr;
	void*                m_cbvParamsMapped = nullptr;
};

class HBAO
{
public:
	void Init(UINT _screenWidth, UINT _screenHeight, const math::mat4f& ProjectionMatrix, DXGI_FORMAT format,
		RenderTargetManager* _rtManager, ComPtr<ID3D12Device> _pDevice, std::shared_ptr<MeshManager>&  _meshManager, CommandList& CmdList,
		std::shared_ptr<RenderTarget> Output, std::shared_ptr<RenderTarget> Depth, const std::wstring& BlurName = L"HBAO");
	void DoHBAO(std::unique_ptr<CommandList>&  CmdList);

	std::shared_ptr<RenderTarget> GetOut(void) { return m_AOBluredOut; };
	~HBAO() { if (m_texture_data != nullptr ) 	delete[] m_texture_data; }
protected:

	struct AOParams
	{
		float viewProjectionInvMatrix[4][4];
		float _screenInvWidth;
		float _screenInvHeight;
	};

	void CreateNoise(CommandList& CmdList);

	UINT m_screenWidth = 1024;
	UINT m_screenHeight = 768;
	UINT m_AOscreenWidth = m_screenWidth/2;
	UINT m_AOscreenHeight = m_screenHeight/2;
	RenderTargetManager* m_rtManager = nullptr;
	std::shared_ptr<RenderTarget> m_AOOut;
	std::shared_ptr<RenderTarget> m_AOBluredOut;
	std::shared_ptr<RenderTarget> m_Depth;
	bool m_OutIsExternalRT;
	ComPtr<ID3D12Device> m_pDevice;
	ComPtr<ID3D12DescriptorHeap> m_customsHeap = nullptr;
	std::wstring m_Name = L"";
	DXGI_FORMAT m_Format;
	std::unique_ptr<GraphicsPipelineState>  m_PassState = nullptr;
	RootSignature  m_RootSignature;
	std::unique_ptr<SceneObject>  m_objScreenQuad = nullptr;
	std::shared_ptr<MeshManager>  m_meshManager = nullptr;
	ComPtr<ID3D12Resource>  m_cbvParams = nullptr;
	void*                m_cbvParamsMapped = nullptr;
	ComPtr<ID3D12Resource>  _Noise;
	ComPtr<ID3D12Resource>  _NoiseUploadHeap;
	BlurEffect              BlurEffect_X;
	BlurEffect              BlurEffect_Y;
	float*                  m_texture_data = nullptr;
};

#include <assimp/scene.h>

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

    void SetBackgroundCubemap(const std::wstring& name);
	void ReLoadScene(const std::string& filename, float scale);
    void LoadScene(const std::string& filename, float scale);

    SceneObjectPtr CreateFilledCube();
    SceneObjectPtr CreateOpenedCube();
    SceneObjectPtr CreatePlane();

    void DrawAll();

    // only for texture creation!
    void ExecuteCommandLists(const CommandList & commandList);

    Graphics::Camera * GetViewCamera();
    Graphics::Camera * GetShadowCamera();

private:
    void CreateSceneNodeFromAssimpNode(const aiScene *scene, const aiNode *node, float scale);

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
	void CreateNoise();

    void FillViewProjMatrix();
    void FillSceneProperties();

    void PopulateDepthPassCommandList();
    void PopulateWorkerCommandLists();
    void PopulateClearPassCommandList();
    void PopulateLightPassCommandList();

    void WaitCurrentFrame();

    std::shared_ptr<MeshManager>                _meshManager = nullptr;

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
    std::atomic_bool                            _workerThreadExit = false;
    std::atomic_size_t                          _pendingWorkers = 0;
    std::mutex                                  _workerMutex {};
    std::atomic_uint32_t                        _drawObjectIndex = ~0x0;
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
    ComPtr<ID3D12Resource>                      _backgroundTexture;
    ComPtr<ID3D12Resource>                      _intermediateIntensityBuffer = nullptr;
    ComPtr<ID3D12Resource>                      _finalIntensityBuffer = nullptr;
    Graphics::Camera                            _viewCamera;
    Graphics::Camera                            _shadowCamera;
    RenderTargetManager *                       _rtManager = nullptr;
    ComPtr<ID3D12DescriptorHeap>                _texturesHeap = nullptr;
    ComPtr<ID3D12DescriptorHeap>                _customsHeap = nullptr;

    std::array<std::shared_ptr<RenderTarget>, 3>_mrtRts; // diffuse, normals, depth
    std::shared_ptr<RenderTarget>               _HDRRt;
    std::shared_ptr<DepthStencil>               _mrtDepth;
    std::shared_ptr<DepthStencil>               _shadowDepth;
    bool                                        _isFrameWaiting = false;

	HBAO                                        _HBAO_effect;
	const char*                                 _mesh_name;
    void CreateIntensityPassPSO();
    void CreateIntensityPassRootSignature();
};
