#include "stdafx.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

Axis GetLongestAxis(DirectX::BoundingBox bb)
{
	Axis longestAxis;
	float longest = 0.f;
	if (bb.Extents.x > longest)
	{
		longestAxis = Axis::X;
		longest = bb.Extents.x;
	}	
	if (bb.Extents.y > longest)
	{
		longestAxis = Axis::Y;
		longest = bb.Extents.y;
	}	
	if (bb.Extents.z > longest)
	{
		longestAxis = Axis::Z;
		longest = bb.Extents.z;
	}
	return longestAxis;
}

HitResult::HitResult(bool isHit, float distance, DirectX::SimpleMath::Vector3 impactPoint) : IsHit(isHit), RayLength(distance), ImpactPoint(impactPoint){}

HitResult::HitResult() : HitResult(false, 0.f, Vector3::Zero) { }

HitResult::HitResult(const HitResult& other) : IsHit(other.IsHit), RayLength(other.RayLength), ImpactPoint(other.ImpactPoint) {}

HitResult::HitResult(HitResult&& other) : IsHit(std::move(other.IsHit)), RayLength(std::move(other.RayLength)), ImpactPoint(std::move(other.ImpactPoint)) {}

HitResult& HitResult::operator=(const HitResult& other)
{
	ImpactPoint = other.ImpactPoint;
	IsHit = other.IsHit;
	RayLength = other.RayLength;
	return *this;
}

DirectX::BoundingBox Triangle::GetBoundingBox()
{
	return m_boundingBox;
}

float Triangle::GetMidPoint(Axis axis) const
{
	switch (axis)
	{
	case Axis::X: return (m_v0.x + m_v1.x + m_v2.x) / 3.f;
	case Axis::Y: return (m_v0.y + m_v1.y + m_v2.y) / 3.f;
	case Axis::Z: return (m_v0.z + m_v1.z + m_v2.z) / 3.f;
	}
	return 0.f;
}

std::vector<DirectX::SimpleMath::Vector3> Triangle::GetVertices()
{
	return std::vector<DirectX::SimpleMath::Vector3>{m_v0, m_v1, m_v2};
}

bool Triangle::IsHit(const DirectX::SimpleMath::Ray& ray, HitResult& outHitResult)
{
	float rayDist;
	if (ray.Intersects(m_v0, m_v1, m_v2, rayDist))
	{
		outHitResult = HitResult(true, rayDist, ray.position + ray.direction*rayDist);
		return true;
	}
	return false;
}

Triangle Triangle::AverageOfTwo(const Triangle& first, const Triangle& second)
{
	Vector3 vertices[3]{ {}, {}, {} };
	for (int i = 0; i < 3; ++i)
	{
		vertices[i].x = (first[i].x + second[i].x) / 2.f;
		vertices[i].y = (first[i].y + second[i].y) / 2.f;
		vertices[i].z = (first[i].z + second[i].z) / 2.f;
	}
	return{ vertices[0], vertices[1], vertices[2] };
}

bool Triangle::LessThan(Triangle* left, Triangle* right, Axis axis)
{
	return left->GetMidPoint(axis) < right->GetMidPoint(axis);
}

Triangle::Triangle() : Triangle(Vector3::Zero, Vector3::Zero, Vector3::Zero) {}

Triangle::Triangle(Triangle&& other) noexcept : Triangle(other.m_v0, other.m_v1, other.m_v2){}

Triangle::Triangle(DirectX::SimpleMath::Vector3 p1, DirectX::SimpleMath::Vector3 p2, DirectX::SimpleMath::Vector3 p3) :m_v0(p1), m_v1(p2), m_v2(p3)
{
	createBoundingBox();
}

Triangle::Triangle(const Triangle& other) : m_v0(other.m_v0), m_v1(other.m_v1), m_v2(other.m_v2), m_boundingBox(other.m_boundingBox) {}

Vector3 Triangle::operator[](const int& index)const
{
	if (index >= 0 && index < 3)
	{
		switch (index)
		{
		case 0: return m_v0;
		case 1: return m_v1;
		case 2: return m_v2;
		}
	}
	return Vector3::Zero;
}

Triangle& Triangle::operator=(const Triangle& other)
{
	m_v0 = other.m_v0;
	m_v1 = other.m_v1;
	m_v2 = other.m_v2;
	m_boundingBox = other.m_boundingBox;
	return *this;
}

bool Triangle::operator==(const Triangle& other) const
{
	return m_v0 == other.m_v0 && m_v1 == other.m_v1 && m_v2 == other.m_v2;
}

bool Triangle::operator!=(const Triangle& other) const
{
	return !(*this == other);
}

void Triangle::createBoundingBox()
{
	std::vector<XMFLOAT3> vertices{ m_v0,m_v1,m_v2 };
	BoundingBox::CreateFromPoints(m_boundingBox, vertices.size(), &vertices[0], sizeof(XMFLOAT3));
}
