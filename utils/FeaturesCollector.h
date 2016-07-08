#pragma once

#include "stdafx.h"

class FeaturesCollector
{
    ComPtr<ID3D12Device> _device = nullptr;

public:
    FeaturesCollector(ComPtr<ID3D12Device> pDevice)
        : _device(pDevice)
    {}

    void CollectFeatures(const std::string &fileName)
    {
        if (!_device)
            return;

        std::ostringstream ss;
        // architecture features
        D3D12_FEATURE_DATA_ARCHITECTURE arch = {};
        ThrowIfFailed(_device->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &arch, sizeof(D3D12_FEATURE_DATA_ARCHITECTURE)));
        ss << "Hardware and driver support for cache-coherent UMA: " << arch.CacheCoherentUMA << std::endl;
        ss << "Hardware and driver support for UMA: " << arch.UMA << std::endl;
        ss << "Tile-base rendering support: " << arch.TileBasedRenderer << std::endl;
        ss << std::endl;

        // d3d12 options
        D3D12_FEATURE_DATA_D3D12_OPTIONS opts = {};
        ThrowIfFailed(_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &opts, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS)));
        ss << "Conservative Rasterization Tier: " << opts.ConservativeRasterizationTier << std::endl;
        ss << "Cross Adapter Row Major Texture Supported: " << opts.CrossAdapterRowMajorTextureSupported << std::endl;
        ss << "Cross Node Sharing Tier: " << opts.CrossNodeSharingTier << std::endl;
        ss << "Double Precision Float Shader Operations: " << opts.DoublePrecisionFloatShaderOps << std::endl;
        ss << "Min Precision Support: " << opts.MinPrecisionSupport << std::endl;
        ss << "Output Merger Logic Operations: " << opts.OutputMergerLogicOp << std::endl;
        ss << "PS Specified Stencil Ref Supported: " << opts.PSSpecifiedStencilRefSupported << std::endl;
        ss << "Resource Binding Tier: " << opts.ResourceBindingTier << std::endl;
        ss << "Resource Heap Tier: " << opts.ResourceHeapTier << std::endl;
        ss << "ROVs Supported: " << opts.ROVsSupported << std::endl;
        ss << "Standard Swizzle 64KB Supported: " << opts.StandardSwizzle64KBSupported << std::endl;
        ss << "Tiled Resources Tier: " << opts.TiledResourcesTier << std::endl;
        ss << "Typed UAV Load Additional Formats: " << opts.TypedUAVLoadAdditionalFormats << std::endl;
        ss << "VP And RT Array Index From Any Shader Feeding Rasterizer Supported Without GS Emulation: "
            << opts.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation << std::endl;
        ss << std::endl;

        // virtual adressation support
        D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT vasupport = {};
        ThrowIfFailed(_device->CheckFeatureSupport(D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT, &vasupport, sizeof(D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT)));
        ss << "Max GPU Virtual Address Bits Per Process: " << vasupport.MaxGPUVirtualAddressBitsPerProcess << std::endl;
        ss << "Max GPU Virtual Address Bits Per Resource: " << vasupport.MaxGPUVirtualAddressBitsPerResource << std::endl;
        ss << std::endl;

        std::ofstream file {fileName};
        file << ss.str();
        file.close();
    }
};
