#pragma once

#include "stdafx.h"

enum class CommandListType
{
    Direct,
    Bundle,
    Compute,
    Copy
};

class CommandList
{
public:
    CommandList(CommandListType type, ComPtr<ID3D12Device> pDevice);
    CommandList(CommandListType type, ComPtr<ID3D12Device> pDevice, ComPtr<ID3D12PipelineState> pInitialState);

    void Reset();
    void Close();

    CommandListType GetType() const;
    ComPtr<ID3D12GraphicsCommandList> GetInternal() const;

private:
    ComPtr<ID3D12GraphicsCommandList>   _commandList = nullptr;
    ComPtr<ID3D12CommandAllocator>      _allocator = nullptr;
    ComPtr<ID3D12Device>                _device = nullptr;
    ComPtr<ID3D12PipelineState>         _initialState = nullptr;
    CommandListType                     _type = CommandListType::Direct;
};
