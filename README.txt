DX12 sample, revision 1.3, created 2015-09-23.

To build it, please execute `build.bat` file or use a usual CMake building procedure (generate cache and build ALL_BUILD target). It was tested on MSVS 2019 and 2022 versions.

dx12_sample executable accepts the following command line options:
    --disable_bundles               - Don't use bundle cmd lists
    --disable_concurrency           - Render from single thread
    --disable_root_constants        - Don't use in place root constants in RootSignature
    --disable_textures              - Don't use textures (no samplers, easier MRT shader, easier root signatures)
    --disable_shadow_pass           - Don't use shadow mapping (no depth pass, simple shader) for rendering
    --enable_tessellation           - Use easy displacement mapping on all meshes (enables hull/domain shader)

Best regards, ArchiDevil
