#include "stdafx.h"

#include "GraphicsPipelineState.h"

GraphicsPipelineState::GraphicsPipelineState()
    : GraphicsPipelineState(RootSignature{}, {})
{
}

GraphicsPipelineState::GraphicsPipelineState(const RootSignature &pRootSignature, const D3D12_INPUT_LAYOUT_DESC & inputLayoutDesc)
{
    _description.pRootSignature = pRootSignature.GetInternal().Get();
    _description.BlendState = _defaultBlendDesc;
    _description.SampleMask = UINT_MAX;
    _description.RasterizerState = _defaultRasterizerDesc;
    _description.DepthStencilState = _defaultDepthStencilDesc;
    _description.InputLayout = inputLayoutDesc;
    _description.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    _description.SampleDesc = {1};
}

void GraphicsPipelineState::SetRootSignature(const RootSignature &pRootSignature)
{
    _description.pRootSignature = pRootSignature.GetInternal().Get();
}

void GraphicsPipelineState::SetInputLayout(const D3D12_INPUT_LAYOUT_DESC & inputLayoutDesc)
{
    _description.InputLayout = inputLayoutDesc;
}

void GraphicsPipelineState::SetShaderCode(ComPtr<ID3DBlob> pShaderCode, ShaderType type)
{
    switch (type)
    {
    case ShaderType::Vertex:
        _description.VS = {pShaderCode->GetBufferPointer(), pShaderCode->GetBufferSize()};
        break;
    case ShaderType::Pixel:
        _description.PS = {pShaderCode->GetBufferPointer(), pShaderCode->GetBufferSize()};
        break;
    case ShaderType::Geometry:
        _description.GS = {pShaderCode->GetBufferPointer(), pShaderCode->GetBufferSize()};
        break;
    case ShaderType::Hull:
        _description.HS = {pShaderCode->GetBufferPointer(), pShaderCode->GetBufferSize()};
        break;
    case ShaderType::Domain:
        _description.DS = {pShaderCode->GetBufferPointer(), pShaderCode->GetBufferSize()};
        break;
    case ShaderType::Compute:
        throw std::runtime_error("Compute shaders must be used with compute pipeline states.");
    default:
        throw std::runtime_error("Unknown shader type");
    }
}

void GraphicsPipelineState::SetRasterizerState(const D3D12_RASTERIZER_DESC & rasterizerDesc)
{
    _description.RasterizerState = rasterizerDesc;
}

void GraphicsPipelineState::SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC & depthStencilDesc)
{
    _description.DepthStencilState = depthStencilDesc;
}

void GraphicsPipelineState::SetRenderTargetFormats(const std::initializer_list<DXGI_FORMAT> & list)
{
    assert(list.size() < std::size(_description.RTVFormats));
    _description.NumRenderTargets = (UINT)list.size();
    std::copy(list.begin(), list.end(), _description.RTVFormats);
}

void GraphicsPipelineState::SetDepthStencilFormat(DXGI_FORMAT format)
{
    _description.DSVFormat = format;
}

void GraphicsPipelineState::SetPritimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE topology)
{
    _description.PrimitiveTopologyType = topology;
}

ComPtr<ID3D12PipelineState> GraphicsPipelineState::GetPSO()
{
    assert(_pso);
    return _pso;
}

void GraphicsPipelineState::Finalize(ComPtr<ID3D12Device> pDevice)
{
    ThrowIfFailed(pDevice->CreateGraphicsPipelineState(&_description, IID_PPV_ARGS(&_pso)));
}
