DX12 sample, revision 1.3, created 2015-09-23.

In order to build the sample on Windows10 machine, you have to manually update Target Platform Version in sample project properties in VS2015 to 10.X.XXXX.

dx12_sample executable accepts following command line options in any order:
    --disable_bundles               - Don't use bundle cmd lists
    --disable_concurrency           - Render from single thread
    --disable_root_constants        - Don't use in place root constants in RootSignature
    --disable_textures              - Don't use textures (no samplers, easier MRT shader, easier root signatures)
    --disable_shadow_pass           - Don't use shadow mapping (no depth pass, simple shader) for rendering
    --enable_tessellation           - Use easy displacement mapping on all meshes (enables hull/domain shader)

Best regards,
