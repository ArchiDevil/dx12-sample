#include "stdafx.h"
#include "DX12Sample.h"

#include <shellapi.h>
#include <iostream>

#ifdef _DEBUG
int main(int argc, char * argv[])
#else // _DEBUG
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#endif
{
    HINSTANCE localInstance = GetModuleHandle(0);

#ifndef _DEBUG
    int argc = 0;
    wchar_t ** argv = CommandLineToArgvW(GetCommandLine(), &argc);
#endif

    const std::map<std::wstring, optTypes> argumentToString = {
        { L"--disable_bundles",               disable_bundles },
        { L"--disable_concurrency",           disable_concurrency },
        { L"--disable_root_constants",        disable_root_constants },
        { L"--disable_textures",              disable_textures },
        { L"--disable_shadow_pass",           disable_shadow_pass },
        { L"--enable_tessellation",           enable_tessellation},
        { L"--legacy_swapchain",              legacy_swapchain },
	    { L"--mesh_name",                     mesh_name }
    };

    std::set<optTypes> arguments {};
	char mesh[256]="\0";

    for (int i = 1; i < argc; ++i)
    {
#ifdef _DEBUG
        auto iter = argumentToString.find(std::wstring(argv[i], argv[i] + strlen(argv[i])));
#else
        auto iter = argumentToString.find(argv[i]);
#endif
		if (iter != argumentToString.end())
		{
			if (iter->second == optTypes::mesh_name)
			{
#ifdef _DEBUG
				strcpy_s(mesh, argv[++i] );
#else
				std::wstring _mesh = argv[++i];
				strcpy_s(mesh, std::string(_mesh.begin(), _mesh.end()).c_str());
#endif
			}
			else
			{
				arguments.insert(iter->second);
			}
		}
        else
        {
            std::wcout << L"Usage: dx12_sample <opts>" << std::endl;
            std::wcout << L"where <opts>:" << std::endl;
            for (auto & pair : argumentToString)
            {
                std::wcout << L"\t" << pair.first << std::endl;
            }

            return -1;
        }
    }

    try
    {
        DX12Sample app = {1280, 720, arguments, mesh };
        return app.Run(localInstance, SW_SHOW);
    }
    catch (const std::exception& e)
    {
        MessageBoxA(NULL, e.what(), "Exception", NULL);
        return 1;
    }
}
