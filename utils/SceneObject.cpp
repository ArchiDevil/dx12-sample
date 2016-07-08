#include "stdafx.h"

#include "SceneObject.h"

#include "Types.h"

SceneObject::SceneObject(std::shared_ptr<MeshObject> meshObject,
                         ComPtr<ID3D12Device> pDevice,
                         ComPtr<ID3D12PipelineState> pPSO)
    : _meshObject(meshObject)
    , _device(pDevice)
{
    D3D12_HEAP_PROPERTIES heapProp = {D3D12_HEAP_TYPE_UPLOAD};
    D3D12_RESOURCE_DESC constantBufferDesc = {};
    constantBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    constantBufferDesc.Width = sizeof(perModelParamsConstantBuffer);
    constantBufferDesc.Height = 1;
    constantBufferDesc.MipLevels = 1;
    constantBufferDesc.SampleDesc.Count = 1;
    constantBufferDesc.DepthOrArraySize = 1;
    constantBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    pDevice->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &constantBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_constantBuffer));

    if (pPSO)
        CreateBundleList(pPSO);
}

void SceneObject::CreateBundleList(ComPtr<ID3D12PipelineState> pPSO)
{
    // Bundle allocator
    ThrowIfFailed(_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_PPV_ARGS(&_bundleCmdAllocator)));
    ThrowIfFailed(_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, _bundleCmdAllocator.Get(), pPSO.Get(), IID_PPV_ARGS(&_drawBundle)));

    Draw(_drawBundle);
    _drawBundle->Close();
    _useBundles = true;
}

SceneObject::~SceneObject()
{
}

void SceneObject::Draw(const ComPtr<ID3D12GraphicsCommandList> & pCmdList, bool bundleUsingOverride /*= false*/)
{
    if (_transformDirty)
        CalculateWorldMatrix();

    if (_useBundles && !bundleUsingOverride)
    {
        pCmdList->ExecuteBundle(_drawBundle.Get());
    }
    else
    {
        pCmdList->IASetPrimitiveTopology(_meshObject->TopologyType());
        pCmdList->IASetVertexBuffers(0, 1, &_meshObject->VertexBufferView());

        if (_meshObject->IndexBuffer())
        {
            pCmdList->IASetIndexBuffer(&_meshObject->IndexBufferView());
            pCmdList->DrawIndexedInstanced((UINT)_meshObject->IndicesCount(), 1, 0, 0, 0);
        }
        else
        {
            pCmdList->DrawInstanced((UINT)_meshObject->VerticesCount(), 1, 0, 0);
        }
    }
}

const XMMATRIX& SceneObject::GetWorldMatrix() const
{
    return _worldMatrix;
}

DirectX::XMFLOAT3 SceneObject::Position() const
{
    return _position;
}

void SceneObject::Position(DirectX::XMFLOAT3 val)
{
    _position = val;
    _transformDirty = true;
}

DirectX::XMFLOAT3 SceneObject::Scale() const
{
    return _scale;
}

void SceneObject::Scale(DirectX::XMFLOAT3 val)
{
    _scale = val;
    _transformDirty = true;
}

float SceneObject::Rotation() const
{
    return _rotation;
}

void SceneObject::Rotation(float val)
{
    _rotation = val;
    _transformDirty = true;
}

ComPtr<ID3D12Resource> SceneObject::GetConstantBuffer() const
{
    return _constantBuffer;
}

void SceneObject::CalculateWorldMatrix()
{
    XMMATRIX translationMatrix = XMMatrixTranslation(_position.x, _position.y, _position.z);
    XMMATRIX rotationMatrix = XMMatrixRotationX(_rotation);
    XMMATRIX scaleMatrix = XMMatrixScaling(_scale.x, _scale.y, _scale.z);

    _worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;

    perModelParamsConstantBuffer * bufPtr = nullptr;
    ThrowIfFailed(_constantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&bufPtr)));
    memcpy(bufPtr->worldMatrix, GetWorldMatrix().r, sizeof(XMMATRIX));
    _constantBuffer->Unmap(0, nullptr);

    _transformDirty = false;
}
