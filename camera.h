#pragma once
#include "DXSampleHelper.h"

using namespace DirectX;

class Camera {

public:
	Camera(XMFLOAT4 cpos, XMFLOAT4 ctar, XMFLOAT4 cup);
    void Set_Perspective(float fov, float m_width, float m_height, float _near, float _far);
    void Set_ViewMat();
    void Set_ProjMat(XMFLOAT4X4 pm);
    void Set_ViewMat(XMFLOAT4X4 pv);
    void RotationY(float angle);
    void RotationAxisUD(float angle);
    XMFLOAT4X4 Get_ProjMat();
    XMFLOAT4X4 Get_ViewMat();
    void Set_CamPos(XMFLOAT4 c);
    void Set_CamTarget(XMFLOAT4 c);
    void Set_CamUp(XMFLOAT4 c);
    XMFLOAT4 Get_CamPos();
    XMFLOAT4 Get_CamTarget();
    XMFLOAT4 Get_CamUp();
    void Set_DIRUP(bool d);
        

    void Go_Forward(float f);
    void Go_Back(float f);
    void Go_Left(float f);
    void Go_Right(float f);

private:
    XMFLOAT4X4 cameraProjMat; // this will store our projection matrix
    XMFLOAT4X4 cameraViewMat; // this will store our view matrix

    XMFLOAT4 cameraPosition; // this is our cameras position vector
    XMFLOAT4 cameraTarget; // a vector describing the point in space our camera is looking at
    XMFLOAT4 cameraUp;

    float speed;
    bool DIRUP;
    float Y_PREC;
    float anglev;

    ofstream *monFlux;

};