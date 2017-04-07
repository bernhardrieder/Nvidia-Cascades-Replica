#pragma once
#include <SimpleMath.h>

class Camera
{
public:
	Camera();
	~Camera();

	// Get/Set world camera position.
	DirectX::SimpleMath::Vector3 GetPosition()const;
	void SetPosition(float x, float y, float z);
	void SetPosition(const DirectX::SimpleMath::Vector3& v);

	// Get camera basis vectors.
	DirectX::SimpleMath::Vector3 GetRight()const;
	DirectX::SimpleMath::Vector3 GetUp()const;
	DirectX::SimpleMath::Vector3 GetLook()const;

	// Get frustum properties.
	float GetNearZ()const;
	float GetFarZ()const;
	float GetAspect()const;
	float GetFovY()const;
	float GetFovX()const;

	// Get near and far plane dimensions in view space coordinates.
	float GetNearWindowWidth()const;
	float GetNearWindowHeight()const;
	float GetFarWindowWidth()const;
	float GetFarWindowHeight()const;

	// Set frustum.
	void SetLens(float fovY, float aspect, float zn, float zf);

	// Define camera space via LookAt parameters.
	void LookAt(const DirectX::SimpleMath::Vector3& pos, const DirectX::SimpleMath::Vector3& target, const DirectX::SimpleMath::Vector3& worldUp);

	//CREATED
	void SetOrientation(DirectX::SimpleMath::Quaternion quaternion);
	void LookAt(DirectX::SimpleMath::Vector3 worldUp, DirectX::SimpleMath::Vector3 look);

	// Get View/Proj matrices.
	DirectX::SimpleMath::Matrix GetView()const;
	DirectX::SimpleMath::Matrix GetProj()const;

	// Strafe/Walk the camera a distance d.
	void Strafe(float d);
	void Walk(float d);

	// Rotate the camera.
	void Pitch(float angle);
	void RotateY(float angle);

	// After modifying camera position/orientation, call to rebuild the view matrix.
	void UpdateViewMatrix();

private:
	// Camera coordinate system with coordinates relative to world space.
	DirectX::SimpleMath::Vector3 m_position = { 0.0f, 0.0f, 0.0f };
	DirectX::SimpleMath::Vector3 m_right = { 1.0f, 0.0f, 0.0f };
	DirectX::SimpleMath::Vector3 m_up = { 0.0f, 1.0f, 0.0f };
	DirectX::SimpleMath::Vector3 m_look = { 0.0f, 0.0f, 1.0f };

	// Cache frustum properties.
	float m_nearZ = 0.0f;
	float m_farZ = 0.0f;
	float m_aspect = 0.0f;
	float m_fovY = 0.0f;
	float m_nearWindowHeight = 0.0f;
	float m_farWindowHeight = 0.0f;

	bool m_viewDirty = true;

	// Cache View/Proj matrices.
	DirectX::SimpleMath::Matrix m_view = DirectX::SimpleMath::Matrix::Identity;
	DirectX::SimpleMath::Matrix m_proj = DirectX::SimpleMath::Matrix::Identity;
};
