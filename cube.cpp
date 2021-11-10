#include "stdafx.h"
#include "cube.h"

Cube::Cube(float aratio): numindices(36), rotx(0.01f), roty(0.01f), rotz(0.01f), m_aspectRatio(aratio),
triangleVertices{ { { -0.25f * m_aspectRatio, 0.25f * m_aspectRatio, -0.25f * m_aspectRatio }, { 1.0f, 0.0f, 0.0f, 1.0f } },
{ { 0.25f * m_aspectRatio, 0.25f * m_aspectRatio, -0.25f * m_aspectRatio }, { 0.0f, 1.0f, 0.0f, 1.0f } },
{ { 0.25f * m_aspectRatio, -0.25f * m_aspectRatio, -0.25f * m_aspectRatio }, { 0.0f, 0.0f, 1.0f, 1.0f } },
{ { -0.25f * m_aspectRatio, -0.25f * m_aspectRatio, -0.25f * m_aspectRatio }, { 0.0f, 1.0f, 1.0f, 1.0f } },

{ { -0.25f * m_aspectRatio, 0.25f * m_aspectRatio, 0.25f * m_aspectRatio }, { 1.0f, 1.0f, 0.0f, 1.0f } },
{ { 0.25f * m_aspectRatio, 0.25f * m_aspectRatio, 0.25f * m_aspectRatio}, { 1.0f, 0.0f, 1.0f, 1.0f } },
{ { 0.25f * m_aspectRatio, -0.25f * m_aspectRatio, 0.25f * m_aspectRatio}, { 0.0f, 0.5f, 1.0f, 1.0f } },
{ { -0.25f * m_aspectRatio, -0.25f * m_aspectRatio, 0.25f * m_aspectRatio}, { 1.0f, 0.5f, 0.0f, 1.0f } } }
{
    

}

void Cube::Init_CB(ComPtr<ID3D12Device> &m_device) {

    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
    cbvHeapDesc.NumDescriptors = 1;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap)));

}

void Cube::Init_GRD(ComPtr<ID3D12GraphicsCommandList> &m_commandList) {

    ID3D12DescriptorHeap* ppHeaps[] = { m_cbvHeap.Get() };
    m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    m_commandList->SetGraphicsRootDescriptorTable(0, m_cbvHeap->GetGPUDescriptorHandleForHeapStart());

}
void Cube::Init_Pipeline(ComPtr<ID3D12RootSignature> &m_rootSignature, ComPtr<ID3D12PipelineState> &m_pipelineState, ComPtr<ID3D12Device>& m_device) {

    /************************Pipeline*************************************/

   /* D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
    cbvHeapDesc.NumDescriptors = 1;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap)));
    
    */


    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
    // Enable better shader debugging with the graphics debugging tools.
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif

    ThrowIfFailed(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
    ThrowIfFailed(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

    // Define the vertex input layout.
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


}

void Cube::Init_Asset(ComPtr<ID3D12Device> &m_device){

    /**********************Asset********************************/

    /*Vertex triangleVertices[] =
    {
        { { -0.25f * m_aspectRatio, 0.25f * m_aspectRatio, -0.25f * m_aspectRatio }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { 0.25f * m_aspectRatio, 0.25f * m_aspectRatio, -0.25f * m_aspectRatio }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { { 0.25f * m_aspectRatio, -0.25f * m_aspectRatio, -0.25f * m_aspectRatio }, { 0.0f, 0.0f, 1.0f, 1.0f } },
        { { -0.25f * m_aspectRatio, -0.25f * m_aspectRatio, -0.25f * m_aspectRatio }, { 0.0f, 1.0f, 1.0f, 1.0f } },

        { { -0.25f * m_aspectRatio, 0.25f * m_aspectRatio, 0.25f * m_aspectRatio }, { 1.0f, 1.0f, 0.0f, 1.0f } },
        { { 0.25f * m_aspectRatio, 0.25f * m_aspectRatio, 0.25f * m_aspectRatio}, { 1.0f, 0.0f, 1.0f, 1.0f } },
        { { 0.25f * m_aspectRatio, -0.25f * m_aspectRatio, 0.25f * m_aspectRatio}, { 0.0f, 0.5f, 1.0f, 1.0f } },
        { { -0.25f * m_aspectRatio, -0.25f * m_aspectRatio, 0.25f * m_aspectRatio}, { 1.0f, 0.5f, 0.0f, 1.0f } },

    };*/

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
    numindices = indexBufferSize / sizeof(DWORD);


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

    {
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

    

}

void Cube::Init_cam(XMFLOAT4 cameraPosition, XMFLOAT4 cameraTarget, XMFLOAT4 cameraUp, XMFLOAT4X4 cameraProjMat, XMFLOAT4X4 cameraViewMat) {

    this->cameraPosition = cameraPosition;
    this->cameraTarget = cameraTarget;
    this->cameraUp = cameraUp;
    this->cameraProjMat = cameraProjMat;
    this->cameraViewMat = cameraViewMat;

}

void Cube::OnUpdate() {

    const float offsetBounds = 1.25f;

    //m_constantBufferData.offset.x += 0.01f;
    /*if (m_constantBufferData.offset.x > offsetBounds)
    {
        m_constantBufferData.offset.x = -offsetBounds;
    }*/
    //memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));


    // build view matrix
    /*XMVECTOR cPos = XMLoadFloat4(&cameraPosition);
    XMVECTOR cTarg = XMLoadFloat4(&cameraTarget);
    XMVECTOR cUp = XMLoadFloat4(&cameraUp);
    XMMATRIX tmpMat = XMMatrixLookAtLH(cPos, cTarg, cUp);
    XMStoreFloat4x4(&cameraViewMat, tmpMat);*/

    // set starting cubes position
    // first cube
    cube1Position = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f); // set cube 1's position
    XMVECTOR posVec = XMLoadFloat4(&cube1Position); // create xmvector for cube1's position

    XMMATRIX tmpMat = XMMatrixTranslationFromVector(posVec); // create translation matrix from cube1's position vector
    XMStoreFloat4x4(&cube1RotMat, XMMatrixIdentity()); // initialize cube1's rotation matrix to identity matrix
    XMStoreFloat4x4(&cube1WorldMat, tmpMat); // store cube1's world matrix


    // create rotation matrices
   // XMMATRIX rotXMat = XMMatrixRotationX(rotx);
  // rotx += 0.01f;
  //  XMMATRIX rotYMat = XMMatrixRotationY(roty); // XMMatrixRotationAxis(XMVectorSet(2.0f, 1.0f, 0.0f, 0.0f), roty);
 //   roty += 0.00f;
  //  XMMATRIX rotZMat = XMMatrixRotationZ(rotz);
  //  rotz += 0.01f;

    // add rotation to cube1's rotation matrix and store it
   // XMMATRIX rotMat = XMLoadFloat4x4(&cube1RotMat) * rotYMat;// *rotXMat* rotYMat* rotZMat;
    //XMStoreFloat4x4(&cube1RotMat, rotMat);
    XMStoreFloat4x4(&cube1RotMat, XMMatrixIdentity());

    // create translation matrix for cube 1 from cube 1's position vector
    XMMATRIX translationMat = XMMatrixTranslationFromVector(XMLoadFloat4(&cube1Position));

    // create cube1's world matrix by first rotating the cube, then positioning the rotated cube
    XMMATRIX worldMat = /*rotMat */ translationMat;

    // store cube1's world matrix
    XMStoreFloat4x4(&cube1WorldMat, worldMat);

    // update constant buffer for cube1
    // create the wvp matrix and store in constant buffer
    XMMATRIX viewMat = XMLoadFloat4x4(&cameraViewMat); // load view matrix
    XMMATRIX projMat = XMLoadFloat4x4(&cameraProjMat); // load projection matrix
    XMMATRIX wvpMat = XMLoadFloat4x4(&cube1WorldMat) * viewMat * projMat; // create wvp matrix
    XMMATRIX transposed = XMMatrixTranspose(wvpMat); // must transpose wvp matrix for the gpu
    XMStoreFloat4x4(&m_constantBufferData.wvpMat, transposed); // store transposed wvp matrix in constant buffer

    memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));

}

void Cube::OnRender(ComPtr<ID3D12GraphicsCommandList> &m_commandList) {

    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_commandList->IASetIndexBuffer(&m_indexBufferView);
    m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    m_commandList->DrawIndexedInstanced(numindices, numindices / 3, 0, 0, 0);

}

void Cube::OnDestroy() {


}


void Cube::Translate(XMFLOAT3 t) {

    for (int i = 0; i < 8; i++) {
        triangleVertices[i].position.x += t.x;
        triangleVertices[i].position.y += t.y;
        triangleVertices[i].position.z += t.z;

    }

}