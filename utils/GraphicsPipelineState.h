#pragma once

#include "stdafx.h"
#include "Types.h"
#include "RootSignature.h"

class GraphicsPipelineState
{
public:
    GraphicsPipelineState();
    GraphicsPipelineState(const RootSignature &pRootSignature, const D3D12_INPUT_LAYOUT_DESC & inputLayoutDesc);

    void SetRootSignature(const RootSignature &pRootSignature);
    void SetInputLayout(const D3D12_INPUT_LAYOUT_DESC & inputLayoutDesc);
    void SetShaderCode(ComPtr<ID3DBlob> pShaderCode, ShaderType type);
    void SetRasterizerState(const D3D12_RASTERIZER_DESC & rasterizerDesc);
    void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC & depthStencilDesc);

    template<size_t N>
    void SetRenderTargetFormats(DXGI_FORMAT(&arr)[N])
    {
        static_assert(N < countof(_description.RTVFormats), "Wrong array size.");
        _description.NumRenderTargets = (UINT)N;
        for (size_t i = 0; i < N; ++i)
            _description.RTVFormats[i] = arr[i];
    }

    void SetRenderTargetFormats(const std::initializer_list<DXGI_FORMAT> & list);
    void SetDepthStencilFormat(DXGI_FORMAT format);
    void SetPritimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE topology);

    ComPtr<ID3D12PipelineState> GetPSO();
    void Finalize(ComPtr<ID3D12Device> pDevice);

private:
    D3D12_GRAPHICS_PIPELINE_STATE_DESC  _description = {};
    ComPtr<ID3D12PipelineState>         _pso = nullptr;

    const D3D12_DEPTH_STENCIL_DESC _defaultDepthStencilDesc = {
        TRUE,                       // DepthEnable
        D3D12_DEPTH_WRITE_MASK_ALL, // DepthWriteMask
        D3D12_COMPARISON_FUNC_LESS, // DepthFunc
        FALSE,                      // StencilEnable
        0,                          // StencilReadMask
        0,                          // StencilWriteMask
        {                           // FrontFace
            D3D12_STENCIL_OP_KEEP,          // StencilFailOp
            D3D12_STENCIL_OP_KEEP,          // StencilDepthFailOp
            D3D12_STENCIL_OP_KEEP,          // StencilPassOp
            D3D12_COMPARISON_FUNC_ALWAYS    // StencilFunc
        },
        {                           // BackFace
            D3D12_STENCIL_OP_KEEP,          // StencilFailOp
            D3D12_STENCIL_OP_KEEP,          // StencilDepthFailOp
            D3D12_STENCIL_OP_KEEP,          // StencilPassOp
            D3D12_COMPARISON_FUNC_ALWAYS    // StencilFunc
        },
    };

    const D3D12_RASTERIZER_DESC _defaultRasterizerDesc = {
        D3D12_FILL_MODE_SOLID,                      // FillMode
        D3D12_CULL_MODE_BACK,                       // CullMode
        FALSE,                                      // FrontCounterClockwise
        D3D12_DEFAULT_DEPTH_BIAS,                   // DepthBias
        D3D12_DEFAULT_DEPTH_BIAS_CLAMP,             // DepthBiasClamp
        D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,      // SlopeScaledDepthBias
        TRUE,                                       // DepthClipEnable
        FALSE,                                      // MultisampleEnable
        FALSE,                                      // AntialiasedLineEnable
        0,                                          // ForcedSampleCount
        D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF   // ConservativeRaster
    };

    const D3D12_RENDER_TARGET_BLEND_DESC _defaultRenderTargetBlendDesc = {
        FALSE,                          // BlendEnable
        FALSE,                          // LogicOpEnable
        D3D12_BLEND_ONE,                // SrcBlend
        D3D12_BLEND_ZERO,               // DestBlend
        D3D12_BLEND_OP_ADD,             // BlendOp
        D3D12_BLEND_ONE,                // SrcBlendAlpha
        D3D12_BLEND_ZERO,               // DestBlendAlpha
        D3D12_BLEND_OP_ADD,             // BlendOpAlpha
        D3D12_LOGIC_OP_NOOP,            // LogicOp
        D3D12_COLOR_WRITE_ENABLE_ALL,   // RenderTargetWriteMask
    };

    const D3D12_BLEND_DESC _defaultBlendDesc = {
        FALSE,
        FALSE,
        {
            _defaultRenderTargetBlendDesc,
            _defaultRenderTargetBlendDesc,
            _defaultRenderTargetBlendDesc,
            _defaultRenderTargetBlendDesc,
            _defaultRenderTargetBlendDesc,
            _defaultRenderTargetBlendDesc,
            _defaultRenderTargetBlendDesc,
            _defaultRenderTargetBlendDesc
        }
    };
};
