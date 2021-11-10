#pragma once
#include "DXSampleHelper.h"

using namespace DirectX;


using Microsoft::WRL::ComPtr;

class Cube {

public:
	Cube(float aratio);

    void Init_CB(ComPtr<ID3D12Device> &device);
    void Init_GRD(ComPtr<ID3D12GraphicsCommandList> &m_commandList);
    void Init_Pipeline(ComPtr<ID3D12RootSignature> &m_rootSignature, ComPtr<ID3D12PipelineState> &m_pipelineState, ComPtr<ID3D12Device>& m_device);
    void OnUpdate();
    void OnRender(ComPtr<ID3D12GraphicsCommandList> &m_commandList);
    void OnDestroy();
    void Init_Asset(ComPtr<ID3D12Device> &device);
    void Init_cam(XMFLOAT4 cameraPosition, XMFLOAT4 cameraTarget, XMFLOAT4 cameraUp, XMFLOAT4X4 cameraProjMat, XMFLOAT4X4 cameraViewMat);
    void Translate(XMFLOAT3 t);

private:

    struct Vertex
    {
        XMFLOAT3 position;
        XMFLOAT4 color;
    };

    struct SceneConstantBuffer
    {
        XMFLOAT4 offset;
        XMFLOAT4X4 wvpMat;
        float padding[44]; // Padding so the constant buffer is 256-byte aligned.
    };
    static_assert((sizeof(SceneConstantBuffer) % 256) == 0, "Constant Buffer size must be 256-byte aligned");


    float rotx;
    float roty;
    float rotz;
    int numindices;
    float m_aspectRatio;
    Vertex triangleVertices[8];
    //ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12DescriptorHeap> m_cbvHeap;

    // App resources.
    ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

    ComPtr<ID3D12Resource> m_indexBuffer;
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

    ID3D12Resource* depthStencilBuffer; // This is the memory for our depth buffer. it will also be used for a stencil buffer in a later tutorial
    ID3D12DescriptorHeap* dsDescriptorHeap; // This is a heap for our depth/stencil buffer descriptor

    ComPtr<ID3D12Resource> m_constantBuffer;
    SceneConstantBuffer m_constantBufferData;
    UINT8* m_pCbvDataBegin;

    //perspcetive
    XMFLOAT4X4 cameraProjMat; // this will store our projection matrix
    XMFLOAT4X4 cameraViewMat; // this will store our view matrix

    XMFLOAT4 cameraPosition; // this is our cameras position vector
    XMFLOAT4 cameraTarget; // a vector describing the point in space our camera is looking at
    XMFLOAT4 cameraUp;

    XMFLOAT4X4 cube1WorldMat; // our first cubes world matrix (transformation matrix)
    XMFLOAT4X4 cube1RotMat; // this will keep track of our rotation for the first cube
    XMFLOAT4 cube1Position; // our first cubes position in space
    

};