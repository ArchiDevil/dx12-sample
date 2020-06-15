#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers.
#endif

#include <windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <WinPixEventRuntime/pix3.h>
#include <processthreadsapi.h>

#include <string>
#include <chrono>
#include <vector>
#include <set>
#include <map>
#include <memory>
#include <sstream>
#include <mutex>
#include <fstream>
#include <thread>
#include <atomic>
#include <array>

#include <3rdparty/d3dx12.h>
#include <3rdparty/DDSTextureLoader.h>
#include <utils/DXSampleHelper.h>

using namespace DirectX;
using namespace Microsoft::WRL;
