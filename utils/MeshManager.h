#pragma once

#include "stdafx.h"

class MeshObject
{
public:
    MeshObject(const std::vector<uint8_t>& vertex_data,
               size_t stride,
               const std::vector<uint32_t>& index_data,
               ComPtr<ID3D12Device> pDevice,
               D3D_PRIMITIVE_TOPOLOGY topology);

    MeshObject(MeshObject&& right) noexcept = default;
    MeshObject& operator=(MeshObject&& right) noexcept = default;

    const ComPtr<ID3D12Resource>& VertexBuffer() const;
    const D3D12_VERTEX_BUFFER_VIEW& VertexBufferView() const;
    const ComPtr<ID3D12Resource>& IndexBuffer() const;
    const D3D12_INDEX_BUFFER_VIEW& IndexBufferView() const;
    D3D_PRIMITIVE_TOPOLOGY TopologyType() const;
    size_t VerticesCount() const;
    size_t IndicesCount() const;

private:
    size_t                      _verticesCount = 0;
    size_t                      _indicesCount = 0;

    ComPtr<ID3D12Resource>      _vertexBuffer = nullptr;
    D3D12_VERTEX_BUFFER_VIEW    _vertexBufferView = {};

    ComPtr<ID3D12Resource>      _indexBuffer = nullptr;
    D3D12_INDEX_BUFFER_VIEW     _indexBufferView = {};

    D3D_PRIMITIVE_TOPOLOGY      _topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ComPtr<ID3D12Device>        _device = nullptr;
};

class MeshManager
{
public:
    MeshManager(bool tessellationEnabled, ComPtr<ID3D12Device> device);
    ~MeshManager();

    std::shared_ptr<MeshObject> LoadMesh(const std::string& filename);
    std::shared_ptr<MeshObject> CreateCube();
    std::shared_ptr<MeshObject> CreateEmptyCube();
    std::shared_ptr<MeshObject> CreateSphere();
    std::shared_ptr<MeshObject> CreatePlane();
    std::shared_ptr<MeshObject> CreateScreenQuad();

private:
    std::vector<std::shared_ptr<MeshObject>> _meshes;
    ComPtr<ID3D12Device>        _device = nullptr;
    bool                        _tessellationEnabled = false;

    std::shared_ptr<MeshObject> _screenQuad = nullptr;
    std::shared_ptr<MeshObject> _plane = nullptr;
    std::shared_ptr<MeshObject> _cube = nullptr;
    std::shared_ptr<MeshObject> _emptyCube = nullptr;

};
