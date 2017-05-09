#pragma once
#include "stdafx.h"

enum class Axis : int
{
	X = 0, Y, Z
};

struct Triangle
{
public:
	DirectX::BoundingBox GetBoundingBox();
	float GetMidPoint(Axis axis) const;
	std::vector<DirectX::SimpleMath::Vector3> GetVertices();
	bool IsHit(const DirectX::SimpleMath::Ray& ray, struct HitResult& outHitResult);
	static Triangle AverageOfTwo(const Triangle& first, const Triangle&  second);
	static bool LessThan(Triangle* left, Triangle* right, Axis axis);

	Triangle();
	Triangle(Triangle&&) = delete;
	Triangle(const Triangle&);
	Triangle(DirectX::SimpleMath::Vector3 p1, DirectX::SimpleMath::Vector3 p2, DirectX::SimpleMath::Vector3 p3);
	Triangle& operator= (const Triangle& other);
	bool operator == (const Triangle& other) const;
	bool operator != (const Triangle& other) const;

private:
	DirectX::BoundingBox m_boundingBox;
	DirectX::SimpleMath::Vector3 m_v0, m_v1, m_v2;
	void createBoundingBox();
	DirectX::SimpleMath::Vector3 operator[](const int& index) const;
};

struct HitResult
{
	bool IsHit;
	float RayLength;
	DirectX::SimpleMath::Vector3 ImpactPoint;

	HitResult();
	HitResult(HitResult&&);
	HitResult(const HitResult&);
	HitResult(bool isHit, float distance, DirectX::SimpleMath::Vector3 impactPoint);
	HitResult& operator= (const HitResult& other);
};

Axis GetLongestAxis(DirectX::BoundingBox bb);