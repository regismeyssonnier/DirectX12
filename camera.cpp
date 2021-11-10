#include "stdafx.h"
#include "camera.h"

Camera::Camera(XMFLOAT4 cpos, XMFLOAT4 ctar, XMFLOAT4 cup) :cameraPosition(cpos), cameraTarget(ctar), cameraUp(cup) {

	speed = 0.1f;
	Y_PREC = 0.0f;
	anglev = 0.0f;
	//monFlux = new ofstream("angle.txt");
}

void Camera::Set_Perspective(float fov, float m_width, float m_height, float _near, float _far) {
	XMMATRIX tmpMat = XMMatrixPerspectiveFovLH(fov, m_width / m_height, _near, _far);
	XMStoreFloat4x4(&cameraProjMat, tmpMat);

}

void Camera::Set_ViewMat() {
	XMVECTOR cPos = XMLoadFloat4(&cameraPosition);
	XMVECTOR cTarg = XMLoadFloat4(&cameraTarget);
	XMVECTOR cUp = XMLoadFloat4(&cameraUp);
	XMMATRIX tmpMat = XMMatrixLookAtLH(cPos, cTarg, cUp);
	XMStoreFloat4x4(&cameraViewMat, tmpMat);

}

void Camera::RotationY(float angle) {
	XMFLOAT4 torigin, tmp;
	
	torigin.x = cameraTarget.x - cameraPosition.x;
	torigin.y = cameraTarget.y - cameraPosition.y;
	torigin.z = cameraTarget.z - cameraPosition.z;
	
	const float maty[] = {cos(angle), 0.0f, sin(angle), 0.0f, 1.0f, 0.0f, -sin(angle), 0.0f, cos(angle) };

	tmp.x = maty[0] * torigin.x + maty[1] * torigin.y + maty[2] * torigin.z;
	tmp.y = maty[3] * torigin.x + maty[4] * torigin.y + maty[5] * torigin.z;
	tmp.z = maty[6] * torigin.x + maty[7] * torigin.y + maty[8] * torigin.z;

	tmp.x = tmp.x + cameraPosition.x;
	tmp.y = tmp.y + cameraPosition.y;
	tmp.z = tmp.z + cameraPosition.z;
	float n = sqrt(tmp.x * tmp.x + tmp.y * tmp.y + tmp.z * tmp.z);
	cameraTarget.x = tmp.x;
	cameraTarget.y = tmp.y;
	cameraTarget.z = tmp.z;
}

void Camera::Set_ProjMat(XMFLOAT4X4 pm) {
	this->cameraProjMat = pm;

}

void Camera::Set_ViewMat(XMFLOAT4X4 pv) {
	this->cameraViewMat = pv;

}

XMFLOAT4X4 Camera::Get_ProjMat() {
	return this->cameraProjMat;
}
XMFLOAT4X4 Camera::Get_ViewMat() {
	return this->cameraViewMat;
}

void Camera::Set_CamPos(XMFLOAT4 c) {
	this->cameraPosition = c;
}
void Camera::Set_CamTarget(XMFLOAT4 c) {
	this->cameraTarget = c;
}
void Camera::Set_CamUp(XMFLOAT4 c) {
	this->cameraUp = c;
}
XMFLOAT4 Camera::Get_CamPos() {
	return this->cameraPosition;
}
XMFLOAT4 Camera::Get_CamTarget() {
	return this->cameraTarget;
}
XMFLOAT4 Camera::Get_CamUp() {
	return this->cameraUp;
}

void Camera::Go_Forward(float f) {
	/*this->cameraPosition.z += f;
	this->cameraTarget.z += f;*/

	XMFLOAT4 dir;
	float tmpx, tmpz;

	dir.x = cameraTarget.x - cameraPosition.x;
	dir.z = cameraTarget.z - cameraPosition.z;

	tmpx = dir.x * speed;
	tmpz = dir.z * speed;
	cameraPosition.x += tmpx;
	cameraPosition.z += tmpz;
	cameraTarget.x += tmpx;
	cameraTarget.z += tmpz;

}

void Camera::Go_Back(float f) {
	/*this->cameraPosition.z -= f;
	this->cameraTarget.z -= f;*/

	XMFLOAT4 dir;
	float tmpx, tmpz;

	dir.x = cameraTarget.x - cameraPosition.x;
	dir.z = cameraTarget.z - cameraPosition.z;

	tmpx = dir.x * speed;
	tmpz = dir.z * speed;
	cameraPosition.x -= tmpx;
	cameraPosition.z -= tmpz;
	cameraTarget.x -= tmpx;
	cameraTarget.z -= tmpz;

}

void Camera::Go_Left(float f) {
	/*this->cameraPosition.x -= f;
	this->cameraTarget.x -= f;*/

	XMFLOAT4 V1, V2, res;
	float tmpx, tmpz;

	V1.x = cameraTarget.x - cameraPosition.x;
	V1.y = 0.0f;
	V1.z = cameraTarget.z - cameraPosition.z;

	V2 = XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);

	res.x = (V1.y * V2.z) - (V1.z * V2.y);
	//res.y = (V1.z * V2.x) - (V1.x * V2.z);
	res.z = (V1.x * V2.y) - (V1.y * V2.x);

	tmpx = res.x * speed;
	tmpz = res.z * speed;

	cameraPosition.x += tmpx;
	cameraPosition.z += tmpz;
	cameraTarget.x += tmpx;
	cameraTarget.z += tmpz;

}

void Camera::Go_Right(float f) {
	/*this->cameraPosition.x += f;
	this->cameraTarget.x += f;*/

	XMFLOAT4 V1, V2, res;
	float tmpx, tmpz;

	V1.x = cameraTarget.x - cameraPosition.x;
	V1.y = 0.0f;
	V1.z = cameraTarget.z - cameraPosition.z;

	V2 = XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);

	res.x = (V1.y * V2.z) - (V1.z * V2.y);
	//res.y = (V1.z * V2.x) - (V1.x * V2.z);
	res.z = (V1.x * V2.y) - (V1.y * V2.x);

	tmpx = res.x * speed;
	tmpz = res.z * speed;

	cameraPosition.x -= tmpx;
	cameraPosition.z -= tmpz;
	cameraTarget.x -= tmpx;
	cameraTarget.z -= tmpz;


}

void Camera::Set_DIRUP(bool d) {
	DIRUP = d;
}

void Camera::RotationAxisUD(float angle) {

	

	anglev += angle;
	//*monFlux << anglev << endl;
	if ((anglev > -1.5f) && (anglev < 1.5f))
	{
		

		XMFLOAT4 V1, V2, res;
		float tmpx, tmpz;

		V1.x = cameraTarget.x - cameraPosition.x;
		V1.y = 0.0f;
		V1.z = cameraTarget.z - cameraPosition.z;

		V2 = XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);

		res.x = (V1.y * V2.z) - (V1.z * V2.y);
		res.y =  (V1.z * V2.x) - (V1.x * V2.z);
		res.z = (V1.x * V2.y) - (V1.y * V2.x);
		res.w = 0.0f;

		/*float s = sqrt(res.x * res.x + res.y * res.y + res.z * res.z);
		res.x /= s;
		res.y /= s;
		res.z /= s;*/

		XMVECTOR axis = XMLoadFloat4(&res);

		XMMATRIX matr;// = XMMatrixRotationAxis(axis, angle);
		XMFLOAT4X4 m;
		//XMStoreFloat4x4(&m, matr);

		//if(Y_PREC)

		
	
		XMVECTOR qrot = XMQuaternionRotationAxis(axis, angle);
		matr = XMMatrixRotationQuaternion(qrot);
		XMStoreFloat4x4(&m, matr);

		XMFLOAT4 rs;
		XMFLOAT4 torigin;

		torigin.x = cameraTarget.x - cameraPosition.x;
		torigin.y = cameraTarget.y - cameraPosition.y;
		torigin.z = cameraTarget.z - cameraPosition.z;

		rs.x = m._11 * torigin.x + m._12 * torigin.y + m._13 * torigin.z;
		rs.y = m._21 * torigin.x + m._22 * torigin.y + m._23 * torigin.z;
		rs.z = m._31 * torigin.x + m._32 * torigin.y + m._33 * torigin.z;

		rs.x = rs.x + cameraPosition.x;
		rs.y = rs.y + cameraPosition.y;
		rs.z = rs.z + cameraPosition.z;

		/*float y = rs.y - Y_PREC;
		if ((DIRUP && (y > 0.0f)) || (!DIRUP && (y < 0.0f))) {
			cameraTarget.y -= y;
		}
		else
		{
			cameraTarget.y = rs.y;

		}*/


		cameraTarget.y = rs.y;
		//cameraTarget.x = rs.x;
		//cameraTarget.y = rs.y;
		//cameraTarget.z = rs.z;

		Y_PREC = cameraTarget.y;

	}
	else
	{
		anglev -= angle;
	}

}