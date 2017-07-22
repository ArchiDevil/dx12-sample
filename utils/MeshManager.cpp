#include "stdafx.h"

#include "MeshManager.h"

#include "Types.h"

static const std::vector<geometryVertex> vertices =
{
    // back face +Z
    {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f,  1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f,  1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
    {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f,  1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
    {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f,  1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},

    // front face -Z
    {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},

    // bottom face -Y
    {{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},

    // top face +Y
    {{-0.5f,  0.5f,  0.5f}, {0.0f,  1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
    {{ 0.5f,  0.5f,  0.5f}, {0.0f,  1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {0.0f,  1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {0.0f,  1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},

    // left face -X
    {{-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
    {{-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
    {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},

    // right face +X
    {{0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
    {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
    {{0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},

    // back faces

    // back face +Z
    {{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
    {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
    {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},

    // front face -Z
    {{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},

    // bottom face -Y
    {{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
    {{-0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},

    // top face +Y
    {{-0.5f,  0.5f,  0.5f}, {0.0f,  -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
    {{ 0.5f,  0.5f,  0.5f}, {0.0f,  -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {0.0f,  -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {0.0f,  -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},

    // left face -X
    {{-0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
    {{-0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},

    // right face +X
    {{0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
    {{0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
    {{0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
};

MeshObject::MeshObject(const std::vector<uint8_t>& vertex_data,
                       size_t stride,
                       const std::vector<uint32_t>& index_data,
                       ComPtr<ID3D12Device> pDevice,
                       D3D_PRIMITIVE_TOPOLOGY topology)
    : _indicesCount(index_data.size())
    , _device(pDevice)
    , _topology(topology)
    , _verticesCount(vertex_data.size() / stride)
{
    D3D12_HEAP_PROPERTIES heapProp = {D3D12_HEAP_TYPE_UPLOAD};

    D3D12_RESOURCE_DESC vertexBufferDesc = {};
    vertexBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    vertexBufferDesc.Width = vertex_data.size();
    vertexBufferDesc.Height = 1;
    vertexBufferDesc.MipLevels = 1;
    vertexBufferDesc.SampleDesc.Count = 1;
    vertexBufferDesc.DepthOrArraySize = 1;
    vertexBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ThrowIfFailed(pDevice->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_vertexBuffer)));

    void * bufPtr = nullptr;
    ThrowIfFailed(_vertexBuffer->Map(0, nullptr, &bufPtr));
    std::memcpy(bufPtr, vertex_data.data(), vertex_data.size());
    _vertexBuffer->Unmap(0, nullptr);

    // creating view describing how to use vertex buffer for GPU
    _vertexBufferView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
    _vertexBufferView.SizeInBytes = (UINT)vertex_data.size();
    _vertexBufferView.StrideInBytes = (UINT)stride;

    if (!index_data.empty())
    {
        D3D12_RESOURCE_DESC indexBufferDesc = {};
        indexBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        indexBufferDesc.Width = index_data.size() * sizeof(uint32_t);
        indexBufferDesc.Height = 1;
        indexBufferDesc.MipLevels = 1;
        indexBufferDesc.SampleDesc.Count = 1;
        indexBufferDesc.DepthOrArraySize = 1;
        indexBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        ThrowIfFailed(pDevice->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &indexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_indexBuffer)));

        ThrowIfFailed(_indexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&bufPtr)));
        std::memcpy(bufPtr, index_data.data(), index_data.size() * sizeof(uint32_t));
        _indexBuffer->Unmap(0, nullptr);

        // creating view describing how to use vertex buffer for GPU
        _indexBufferView.BufferLocation = _indexBuffer->GetGPUVirtualAddress();
        _indexBufferView.SizeInBytes = (UINT)(index_data.size() * sizeof(uint32_t));
        _indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    }
}

const Microsoft::WRL::ComPtr<ID3D12Resource>& MeshObject::VertexBuffer() const
{
    return _vertexBuffer;
}

const D3D12_VERTEX_BUFFER_VIEW& MeshObject::VertexBufferView() const
{
    return _vertexBufferView;
}

const Microsoft::WRL::ComPtr<ID3D12Resource>& MeshObject::IndexBuffer() const
{
    return _indexBuffer;
}

const D3D12_INDEX_BUFFER_VIEW& MeshObject::IndexBufferView() const
{
    return _indexBufferView;
}

D3D_PRIMITIVE_TOPOLOGY MeshObject::TopologyType() const
{
    return _topology;
}

size_t MeshObject::VerticesCount() const
{
    return _verticesCount;
}

size_t MeshObject::IndicesCount() const
{
    return _indicesCount;
}

//////////////////////////////////////////////////////////////////////////

MeshManager::MeshManager(bool tessellationEnabled, ComPtr<ID3D12Device> device)
    : _tessellationEnabled(tessellationEnabled)
    , _device(device)
{
}

std::shared_ptr<MeshObject> MeshManager::LoadMesh(const std::string& filename)
{
    throw std::runtime_error("Not implemented yet");
}

std::shared_ptr<MeshObject> MeshManager::CreateCube()
{
    if (_cube)
        return _cube;

    static const std::vector<uint32_t> indices =
    {
        //back
        0, 2, 1,
        1, 2, 3,
        //front +4
        4, 5, 7,
        4, 7, 6,
        //left +8
        9, 8, 10,
        10, 8, 11,
        //right +12
        13, 14, 12,
        14, 15, 12,
        //front +16
        17, 16, 19,
        18, 17, 19,
        //back +20
        21, 23, 20,
        22, 23, 21
    };

    if (_tessellationEnabled)
    {
        _cube = std::make_shared<MeshObject>(std::vector<uint8_t>{(uint8_t*)vertices.data(), (uint8_t*)(vertices.data() + vertices.size())},
                                                        sizeof(geometryVertex),
                                                        indices,
                                                        _device,
                                                        D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
    }
    else
    {
        _cube = std::make_shared<MeshObject>(std::vector<uint8_t>{(uint8_t*)vertices.data(), (uint8_t*)(vertices.data() + vertices.size())},
                                                        sizeof(geometryVertex),
                                                        indices,
                                                        _device,
                                                        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    return _cube;
}

std::shared_ptr<MeshObject> MeshManager::CreateEmptyCube()
{
    if (_emptyCube)
        return _emptyCube;

    static const std::vector<uint32_t> indices =
    {
        //back
        0, 2, 1,
        1, 2, 3,
        24 + 0, 24 + 1, 24 + 2,
        24 + 1, 24 + 3, 24 + 2,
        //front
        4, 5, 7,
        4, 7, 6,
        24 + 4, 24 + 7, 24 + 5,
        24 + 4, 24 + 6, 24 + 7,
        // right
        17, 16, 19,
        18, 17, 19,
        24 + 16, 24 + 17, 24 + 19,
        24 + 17, 24 + 18, 24 + 19,
        // left
        21, 23, 20,
        22, 23, 21,
        24 + 23, 24 + 21, 24 + 20,
        24 + 23, 24 + 22, 24 + 21
    };

    if (_tessellationEnabled)
    {
        _emptyCube = std::make_shared<MeshObject>(std::vector<uint8_t>{(uint8_t*)vertices.data(), (uint8_t*)(vertices.data() + vertices.size())},
                                                  sizeof(geometryVertex),
                                                  indices,
                                                  _device,
                                                  D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
    }
    else
    {
        _emptyCube = std::make_shared<MeshObject>(std::vector<uint8_t>{(uint8_t*)vertices.data(), (uint8_t*)(vertices.data() + vertices.size())},
                                                  sizeof(geometryVertex),
                                                  indices,
                                                  _device,
                                                  D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    return _emptyCube;
}

std::shared_ptr<MeshObject> MeshManager::CreateSphere()
{
    throw std::runtime_error("Not implemented yet");
}

std::shared_ptr<MeshObject> MeshManager::CreatePlane()
{
    if (_plane)
        return _plane;

    static const std::vector<geometryVertex> vertices =
    {
        {{-0.5f, 0.0f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
        {{0.5f, 0.0f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
    };

    static const std::vector<uint32_t> indices =
    {
        0, 1, 2,
        0, 2, 3
    };

    if (_tessellationEnabled)
    {
        _plane = std::make_shared<MeshObject>(std::vector<uint8_t>{(uint8_t*)vertices.data(), (uint8_t*)(vertices.data() + vertices.size())},
                                              sizeof(geometryVertex),
                                              indices,
                                              _device,
                                              D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
    }
    else
    {
        _plane = std::make_shared<MeshObject>(std::vector<uint8_t>{(uint8_t*)vertices.data(), (uint8_t*)(vertices.data() + vertices.size())},
                                              sizeof(geometryVertex),
                                              indices,
                                              _device,
                                              D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    return _plane;
}

std::shared_ptr<MeshObject> MeshManager::CreateScreenQuad()
{
    if (_screenQuad)
        return _screenQuad;

    static std::vector<screenQuadVertex> sqVertices =
    {
        {{-1.0f,  1.0f}, {0.0f, 0.0f}},
        {{ 1.0f,  1.0f}, {1.0f, 0.0f}},
        {{ 1.0f, -1.0f}, {1.0f, 1.0f}},
        {{-1.0f,  1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f}, {1.0f, 1.0f}},
        {{-1.0f, -1.0f}, {0.0f, 1.0f}},
    };

    _screenQuad = std::make_shared<MeshObject>(std::vector<uint8_t>{(uint8_t*)sqVertices.data(), (uint8_t*)(sqVertices.data() + sqVertices.size())},
                                               sizeof(screenQuadVertex),
                                               std::vector<uint32_t>{},
                                               _device,
                                               D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    return _screenQuad;
}
