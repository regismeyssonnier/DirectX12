//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include "stdafx.h"
#include "D3D12HelloFrameBuffering.h"

D3D12HelloFrameBuffering::D3D12HelloFrameBuffering(UINT width, UINT height, std::wstring name) :
    DXSample(width, height, name),
    m_frameIndex(0),
    m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
    m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
    m_fenceValues{},
    m_rtvDescriptorSize(0)
{
    forward = false;
    num_cubes = 100;
}

void D3D12HelloFrameBuffering::OnInit()
{
    
    LoadPipeline();
    LoadAssets();

    cam = new Camera(XMFLOAT4(0.0f, 2.0f, -4.0f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f));
}

// Load the rendering pipeline dependencies.
void D3D12HelloFrameBuffering::LoadPipeline()
{
    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();

            // Enable additional debug layers.
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    ComPtr<IDXGIFactory4> factory;
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

    if (m_useWarpDevice)
    {
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

        ThrowIfFailed(D3D12CreateDevice(
            warpAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_device)
            ));
    }
    else
    {
        ComPtr<IDXGIAdapter1> hardwareAdapter;
        GetHardwareAdapter(factory.Get(), &hardwareAdapter);

        ThrowIfFailed(D3D12CreateDevice(
            hardwareAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_device)
            ));
    }

    cube = new Cube(m_aspectRatio);

    for (int i = 0; i < num_cubes; i++) {
        tcube[i] = new Cube(m_aspectRatio);
    }

    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FrameCount;
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(factory->CreateSwapChainForHwnd(
        m_commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
        Win32Application::GetHwnd(),
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain
        ));

    // This sample does not support fullscreen transitions.
    ThrowIfFailed(factory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChain.As(&m_swapChain));
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Create descriptor heaps.
    {
        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = FrameCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));

        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        // Describe and create a constant buffer view (CBV) descriptor heap.
        // Flags indicate that this descriptor heap can be bound to the pipeline 
        // and that descriptors contained in it can be referenced by a root table.
       /* D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
        cbvHeapDesc.NumDescriptors = 1;
        cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap)));
        */
        cube->Init_CB(m_device);
        for (int i = 0; i < num_cubes; i++) {
            tcube[i]->Init_CB(m_device);
        }

        // create a depth stencil descriptor heap so we can get a pointer to the depth stencil buffer
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        HRESULT hr = m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsDescriptorHeap));
        if (FAILED(hr))
        {
            //Running = false;
            cerr << "CreateDescriptorHeap error" << endl;
        }


    }

    // Create frame resources.
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

        // Create a RTV and a command allocator for each frame.
        for (UINT n = 0; n < FrameCount; n++)
        {
            ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
            m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, m_rtvDescriptorSize);

            ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[n])));
        }
    }
}

// Load the sample assets.
void D3D12HelloFrameBuffering::LoadAssets()
{
    // Create an empty root signature.
    /* {
        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
        ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
    }*/

    // Create a root signature consisting of a descriptor table with a single CBV.
    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

        // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
        CD3DX12_ROOT_PARAMETER1 rootParameters[1];

        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);

        // Allow input layout and deny uneccessary access to certain pipeline stages.
        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
        ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
    }

    
   
    
    // Create the pipeline state, which includes compiling and loading shaders.
    {
         /*ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif

       // ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
       // ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"shaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));
        
        ThrowIfFailed(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
        ThrowIfFailed(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));
        */

        // Create the depth/stencil buffer


        D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
        depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
        depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

        D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
        depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
        depthOptimizedClearValue.DepthStencil.Stencil = 0;

        m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, m_width, m_height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthOptimizedClearValue,
            IID_PPV_ARGS(&depthStencilBuffer)
        );

        dsDescriptorHeap->SetName(L"Depth/Stencil Resource Heap");

        m_device->CreateDepthStencilView(depthStencilBuffer, &depthStencilDesc, dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

        
       /* // Define the vertex input layout.
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = TRUE;
        psoDesc.DepthStencilState.StencilEnable = TRUE;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

        // Stencil test parameters
        psoDesc.DepthStencilState.StencilReadMask = 0xFF;
        psoDesc.DepthStencilState.StencilWriteMask = 0xFF;

        // Stencil operations if pixel is front-facing
        psoDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        psoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_INCR;
        psoDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        psoDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

        // Stencil operations if pixel is back-facing
        psoDesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        psoDesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_DECR;
        psoDesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        psoDesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;
        ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
       */ 
    }
    
    
    cube->Init_Pipeline(m_rootSignature, m_pipelineState, m_device);
    for (int i = 0; i < num_cubes; i++) {
        tcube[i]->Init_Pipeline(m_rootSignature, m_pipelineState, m_device);
    }

    // Create the command list.
    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[m_frameIndex].Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));

    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    ThrowIfFailed(m_commandList->Close());

    cube->Translate(XMFLOAT3(2.0f, 0.0f, 0.0f));
    float y = 0.0f; int I = 0;
    for (int i = 0; i < num_cubes; i++) {
        tcube[i]->Translate(XMFLOAT3(2.0f + (float)I, y, 0.0f));
        I++;
        if (I == 10)
        {
            y++;
            I = 0;
        }
    }
    cube->Init_Asset(m_device);
    for (int i = 0; i < num_cubes; i++) {
        tcube[i]->Init_Asset(m_device);
    }

    // Create the vertex buffer.
    {
        // Define the geometry for a triangle.
        /*Vertex triangleVertices[] =
        {
            { { 0.0f, 0.25f * m_aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { { 0.25f, -0.25f * m_aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { -0.25f, -0.25f * m_aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
             { { -0.25f, 0.25f * m_aspectRatio, 0.0f }, { 1.0f, 0.0f, 1.0f, 1.0f } }
        };*/

       /* Vertex triangleVertices[] =
        {
            { { -0.25f * m_aspectRatio, 0.25f * m_aspectRatio, -0.25f * m_aspectRatio }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { { 0.25f * m_aspectRatio, 0.25f * m_aspectRatio, -0.25f * m_aspectRatio }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { 0.25f * m_aspectRatio, -0.25f * m_aspectRatio, -0.25f * m_aspectRatio }, { 0.0f, 0.0f, 1.0f, 1.0f } },
            { { -0.25f * m_aspectRatio, -0.25f * m_aspectRatio, -0.25f * m_aspectRatio }, { 0.0f, 1.0f, 1.0f, 1.0f } },

            { { -0.25f * m_aspectRatio, 0.25f * m_aspectRatio, 0.25f * m_aspectRatio }, { 1.0f, 1.0f, 0.0f, 1.0f } },
            { { 0.25f * m_aspectRatio, 0.25f * m_aspectRatio, 0.25f * m_aspectRatio}, { 1.0f, 0.0f, 1.0f, 1.0f } },
            { { 0.25f * m_aspectRatio, -0.25f * m_aspectRatio, 0.25f * m_aspectRatio}, { 0.0f, 0.5f, 1.0f, 1.0f } },
            { { -0.25f * m_aspectRatio, -0.25f * m_aspectRatio, 0.25f * m_aspectRatio}, { 1.0f, 0.5f, 0.0f, 1.0f } },
            
        };
        

        const UINT vertexBufferSize = sizeof(triangleVertices);

        // Note: using upload heaps to transfer static data like vert buffers is not 
        // recommended. Every time the GPU needs it, the upload heap will be marshalled 
        // over. Please read up on Default Heap usage. An upload heap is used here for 
        // code simplicity and because there are very few verts to actually transfer.
        ThrowIfFailed(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),//D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ, //D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_vertexBuffer)));

        // Copy the triangle data to the vertex buffer.
        UINT8* pVertexDataBegin;
        CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
        memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
        m_vertexBuffer->Unmap(0, nullptr);

        // Initialize the vertex buffer view.
        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.StrideInBytes = sizeof(Vertex);
        m_vertexBufferView.SizeInBytes = vertexBufferSize;

        DWORD iList[] = {
            //front
            0, 1, 2,
            0, 2, 3,

            //left
            0, 3, 7,
            0, 7, 4,

            //right
            1, 6, 2,
            1, 5, 6,

            //back
            4, 6, 5,
            4, 7, 6,

            //up
            0, 4, 1,
            4, 5, 1,

            //down
            2, 7, 3,
            2, 6, 7

        };

        const UINT indexBufferSize = sizeof(iList);
        numindices = indexBufferSize/sizeof(DWORD);
        

        // Note: using upload heaps to transfer static data like vert buffers is not 
        // recommended. Every time the GPU needs it, the upload heap will be marshalled 
        // over. Please read up on Default Heap usage. An upload heap is used here for 
        // code simplicity and because there are very few verts to actually transfer.
        ThrowIfFailed(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),//D3D12_HEAP_TYPE_UPLOAD),D3D12_HEAP_TYPE_READBACK
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ, //,D3D12_RESOURCE_STATE_COPY_DEST
            nullptr,
            IID_PPV_ARGS(&m_indexBuffer)));

        // Copy the triangle data to the vertex buffer.
        UINT8* pindexDataBegin;
        CD3DX12_RANGE readRange2(0, 0);        // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(m_indexBuffer->Map(0, &readRange2, reinterpret_cast<void**>(&pindexDataBegin)));
        memcpy(pindexDataBegin, iList, sizeof(iList));
        m_indexBuffer->Unmap(0, nullptr);

        // Initialize the vertex buffer view.
        m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
        m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
        m_indexBufferView.SizeInBytes = indexBufferSize;
        

        
     */   

    }

    // Create the constant buffer.
   /* {
        const UINT constantBufferSize = sizeof(SceneConstantBuffer);    // CB size is required to be 256-byte aligned.

        ThrowIfFailed(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_constantBuffer)));

        // Describe and create a constant buffer view.
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = constantBufferSize;
        m_device->CreateConstantBufferView(&cbvDesc, m_cbvHeap->GetCPUDescriptorHandleForHeapStart());

        // Map and initialize the constant buffer. We don't unmap this until the
        // app closes. Keeping things mapped for the lifetime of the resource is okay.
        CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBegin)));
        memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));
    }
    */
    rotx = 0.0f;
    roty = 0.0f;
    rotz = 0.0f;

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        ThrowIfFailed(m_device->CreateFence(m_fenceValues[m_frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        m_fenceValues[m_frameIndex]++;

        // Create an event handle to use for frame synchronization.
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        // Wait for the command list to execute; we are reusing the same command 
        // list in our main loop but for now, we just want to wait for setup to 
        // complete before continuing.
        WaitForGpu();
    }
}

// Update frame-based values.
void D3D12HelloFrameBuffering::OnUpdate()
{
    /*const float translationSpeed = 0.005f;
    const float offsetBounds = 1.25f;

    m_constantBufferData.offset.x += translationSpeed;
    if (m_constantBufferData.offset.x > offsetBounds)
    {
        m_constantBufferData.offset.x = -offsetBounds;
    }
    memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));*/

    // build projection and view matrix
    /*XMMATRIX tmpMat = XMMatrixPerspectiveFovLH(45.0f * (3.14f / 180.0f), (float)m_width / (float)m_height, 0.1f, 1000.0f);
    XMStoreFloat4x4(&cameraProjMat, tmpMat);*/

    cam->Set_Perspective(45.0f * (3.14f / 180.0f), m_width, m_height, 0.1f, 1000.0f);

    // set starting camera state
    /*cameraPosition = XMFLOAT4(0.0f, 2.0f, -4.0f, 0.0f);
    cameraTarget = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
    cameraUp = XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);*/

    if (forward) {
        cam->Go_Forward(0.1f);
    }
    if (back) {
        cam->Go_Back(0.1f);
    }
    if (left) {
        cam->Go_Left(0.1f);
    }
    if (right) {
        cam->Go_Right(0.1f);
    }

    /*XMVECTOR cPos = XMLoadFloat4(&cameraPosition);
    XMVECTOR cTarg = XMLoadFloat4(&cameraTarget);
    XMVECTOR cUp = XMLoadFloat4(&cameraUp);
    XMMATRIX tmpMat = XMMatrixLookAtLH(cPos, cTarg, cUp);
    XMStoreFloat4x4(&cameraViewMat, tmpMat);*/

    cam->Set_ViewMat();

    cube->Init_cam(cam->Get_CamPos(), cam->Get_CamTarget(), cam->Get_CamUp(), cam->Get_ProjMat(), cam->Get_ViewMat());
    cube->OnUpdate();

    for (int i = 0; i < num_cubes; i++) {
        tcube[i]->Init_cam(cam->Get_CamPos(), cam->Get_CamTarget(), cam->Get_CamUp(), cam->Get_ProjMat(), cam->Get_ViewMat());
        tcube[i]->OnUpdate();
    }

    /* build view matrix
    XMVECTOR cPos = XMLoadFloat4(&cameraPosition);
    XMVECTOR cTarg = XMLoadFloat4(&cameraTarget);
    XMVECTOR cUp = XMLoadFloat4(&cameraUp);
    tmpMat = XMMatrixLookAtLH(cPos, cTarg, cUp);
    XMStoreFloat4x4(&cameraViewMat, tmpMat);

    // set starting cubes position
    // first cube
    cube1Position = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f); // set cube 1's position
    XMVECTOR posVec = XMLoadFloat4(&cube1Position); // create xmvector for cube1's position

    tmpMat = XMMatrixTranslationFromVector(posVec); // create translation matrix from cube1's position vector
    XMStoreFloat4x4(&cube1RotMat, XMMatrixIdentity()); // initialize cube1's rotation matrix to identity matrix
    XMStoreFloat4x4(&cube1WorldMat, tmpMat); // store cube1's world matrix


    // create rotation matrices
    XMMATRIX rotXMat = XMMatrixRotationX(rotx);
    rotx += 0.01f;
    XMMATRIX rotYMat = XMMatrixRotationY(roty);
    roty += 0.01f;
    XMMATRIX rotZMat = XMMatrixRotationZ(rotz);
    rotz += 0.01f;

    // add rotation to cube1's rotation matrix and store it
    XMMATRIX rotMat = XMLoadFloat4x4(&cube1RotMat) * rotXMat * rotYMat * rotZMat;
    XMStoreFloat4x4(&cube1RotMat, rotMat);

    // create translation matrix for cube 1 from cube 1's position vector
    XMMATRIX translationMat = XMMatrixTranslationFromVector(XMLoadFloat4(&cube1Position));

    // create cube1's world matrix by first rotating the cube, then positioning the rotated cube
    XMMATRIX worldMat = rotMat * translationMat;

    // store cube1's world matrix
    XMStoreFloat4x4(&cube1WorldMat, worldMat);

    // update constant buffer for cube1
    // create the wvp matrix and store in constant buffer
    XMMATRIX viewMat = XMLoadFloat4x4(&cameraViewMat); // load view matrix
    XMMATRIX projMat = XMLoadFloat4x4(&cameraProjMat); // load projection matrix
    XMMATRIX wvpMat = XMLoadFloat4x4(&cube1WorldMat) * viewMat * projMat; // create wvp matrix
    XMMATRIX transposed = XMMatrixTranspose(wvpMat); // must transpose wvp matrix for the gpu
    XMStoreFloat4x4(&m_constantBufferData.wvpMat, transposed); // store transposed wvp matrix in constant buffer

    memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData)); */
    

}

// Render the scene.
void D3D12HelloFrameBuffering::OnRender()
{
    // Record all the commands we need to render the scene into the command list.
    PopulateCommandList();

    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Present the frame.
    ThrowIfFailed(m_swapChain->Present(1, 0));

    MoveToNextFrame();
}

void D3D12HelloFrameBuffering::OnDestroy()
{
    // Ensure that the GPU is no longer referencing resources that are about to be
    // cleaned up by the destructor.
    WaitForGpu();

    CloseHandle(m_fenceEvent);

    delete cube;
    for (int i = 0; i < num_cubes; i++) {
        delete tcube[i];
    }

    delete cam;
}

void D3D12HelloFrameBuffering::PopulateCommandList()
{
    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    ThrowIfFailed(m_commandAllocators[m_frameIndex]->Reset());

    // However, when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), m_pipelineState.Get()));

    // Set necessary state.
    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

   /* ID3D12DescriptorHeap* ppHeaps[] = {m_cbvHeap.Get()};
    m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    m_commandList->SetGraphicsRootDescriptorTable(0, m_cbvHeap->GetGPUDescriptorHandleForHeapStart());
    */
    cube->Init_GRD(m_commandList);
    for (int i = 0; i < num_cubes; i++) {
        tcube[i]->Init_GRD(m_commandList);
    }

    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    // Indicate that the back buffer will be used as a render target.
    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);

    // get a handle to the depth/stencil buffer
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    // set the render target for the output merger stage (the output of the pipeline)
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    //m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // clear the depth/stencil buffer
    m_commandList->ClearDepthStencilView(dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Record commands.
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

    // get a handle to the depth/stencil buffer
    //CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    // set the render target for the output merger stage (the output of the pipeline)
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    

    /*
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_commandList->IASetIndexBuffer(&m_indexBufferView);
    m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    m_commandList->DrawIndexedInstanced(numindices, numindices/3, 0, 0, 0);

    cout << "numindices :" << numindices << endl;*/

    cube->OnRender(m_commandList);
    for (int i = 0; i < num_cubes; i++) {
        tcube[i]->OnRender(m_commandList);
    }
    

    // Indicate that the back buffer will now be used to present.
    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    ThrowIfFailed(m_commandList->Close());
}

// Wait for pending GPU work to complete.
void D3D12HelloFrameBuffering::WaitForGpu()
{
    // Schedule a Signal command in the queue.
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValues[m_frameIndex]));

    // Wait until the fence has been processed.
    ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
    WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);

    // Increment the fence value for the current frame.
    m_fenceValues[m_frameIndex]++;
}

// Prepare to render the next frame.
void D3D12HelloFrameBuffering::MoveToNextFrame()
{
    // Schedule a Signal command in the queue.
    const UINT64 currentFenceValue = m_fenceValues[m_frameIndex];
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), currentFenceValue));

    // Update the frame index.
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // If the next frame is not ready to be rendered yet, wait until it is ready.
    if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex])
    {
        ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
        WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
    }

    // Set the fence value for the next frame.
    m_fenceValues[m_frameIndex] = currentFenceValue + 1;
}


void D3D12HelloFrameBuffering::OnKeyDown(UINT8 key) {

    switch (key) {

    //Z
    case 0x5A:
        forward = true;
        break;

    //S
    case 0x53:
        back = true;
        break;
    
    //Q
    case 0x51:
        left = true;
        break;

    //D
    case 0x44:
        right = true;
        break;

    }
    
        


}

 void D3D12HelloFrameBuffering::OnKeyUp(UINT8 key) {

     switch (key) {

         //Z
     case 0x5A:
         forward = false;
         break;

         //S
     case 0x53:
         back = false;
         break;

         //Q
     case 0x51:
         left = false;
         break;
     
         //D
     case 0x44:
         right = false;
         break;

     case VK_ESCAPE:
         PostQuitMessage(0);

         break;

     }

}


 void D3D12HelloFrameBuffering::OnMouseMove(UINT8 key, int x, int y) {

     X = x;
     Y = y;

     if (X > X_PREC) {
         cam->RotationY(0.1f);
     }
     else if (X < X_PREC) {
         cam->RotationY(-0.1f);
     }

     X_PREC = X;

     if (Y > Y_PREC) {
         cam->RotationAxisUD(0.01f);
         cam->Set_DIRUP(true);
     }
     else if (Y < Y_PREC) {
         cam->RotationAxisUD(-0.01f);
         cam->Set_DIRUP(false);
     }

     Y_PREC = Y;
}



