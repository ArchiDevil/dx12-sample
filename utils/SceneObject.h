#pragma once

#include "stdafx.h"

#include <utils/MeshManager.h>

__declspec(align(16)) class SceneObject
{
public:
    SceneObject(std::shared_ptr<MeshObject> meshObject,
                ComPtr<ID3D12Device> pDevice,
                ComPtr<ID3D12PipelineState> pPSO);

    virtual ~SceneObject();

    void Draw(const ComPtr<ID3D12GraphicsCommandList> & pCmdList, bool bundleUsingOverride = false);

    const XMMATRIX& GetWorldMatrix() const;

    DirectX::XMFLOAT3 Position() const;
    void Position(DirectX::XMFLOAT3 val);

    DirectX::XMFLOAT3 Scale() const;
    void Scale(DirectX::XMFLOAT3 val);

    float RotationX() const;
    void RotationX(float val);
	void RotationY(float val);
	void RotationZ(float val);

    void * operator new(size_t i)
    {
        return _aligned_malloc(i, 16);
    }

    void operator delete(void* p)
    {
        _aligned_free(p);
    }

    ComPtr<ID3D12Resource> GetConstantBuffer() const;

private:
    void CreateBundleList(ComPtr<ID3D12PipelineState> pPSO);
    void CalculateWorldMatrix();

    std::shared_ptr<MeshObject>         _meshObject = nullptr;

    XMFLOAT3                            _position = {0.0f, 0.0f, 0.0f};
    XMFLOAT3                            _scale = {1.0f, 1.0f, 1.0f};
    float                               _rotationX = 0.0f;
	float                               _rotationY = 0.0f;
	float                               _rotationZ = 0.0f;
    XMMATRIX                            _worldMatrix = XMMatrixIdentity();

    ComPtr<ID3D12Resource>              _constantBuffer = nullptr;
    ComPtr<ID3D12Device>                _device = nullptr;
    ComPtr<ID3D12CommandAllocator>      _bundleCmdAllocator = nullptr;
    ComPtr<ID3D12GraphicsCommandList>   _drawBundle = nullptr;

    bool                                _useBundles = false;
    bool                                _transformDirty = true;
};
