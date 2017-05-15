#include "stdafx.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

KDNode::KDNode() : KDNode({}, nullptr, nullptr, {})
{

}

KDNode::KDNode(DirectX::BoundingBox bbox, KDNode* left, KDNode* right, std::vector<Triangle*> triangles) : BBox(bbox), LeftChild(left), RightChild(right), Triangles(triangles)
{

}

KDNode::KDNode(const KDNode& other)
{
	this->BBox = other.BBox;
	this->LeftChild = other.LeftChild;
	this->RightChild = other.RightChild;
	this->Triangles = other.Triangles;
}

KDNode& KDNode::operator=(const KDNode& other)
{
	BBox = other.BBox;
	LeftChild = other.LeftChild;
	RightChild = other.RightChild;
	Triangles = other.Triangles;
	return *this;
}

KDNode::~KDNode()
{
	if (LeftChild != nullptr)
		delete LeftChild;
	if (RightChild != nullptr)
		delete RightChild;
}

KDNode* KDNode::Build(std::vector<Triangle*>& triangles, int depth)
{
	KDNode* node = new KDNode(BoundingBox(), nullptr, nullptr, triangles);

	if (triangles.size() == 0)
		return node;

	if (triangles.size() == 1)
	{
		node->BBox = triangles[0]->GetBoundingBox();
		node->LeftChild = new KDNode();
		node->RightChild = new KDNode();
		node->LeftChild->Triangles = std::vector<Triangle*>();
		node->RightChild->Triangles = std::vector<Triangle*>();
		return node;
	}

	node->BBox = triangles[0]->GetBoundingBox();

#pragma omp parallel for
	for (int i = 0; i < triangles.size(); ++i)
	{
#pragma omp critical ( BBOX_MERGE )
		node->BBox.CreateMerged(node->BBox, node->BBox, triangles[i]->GetBoundingBox());
	}

	Axis longestAxis = GetLongestAxis(node->BBox);
	float axisMedian = getMedian(triangles, longestAxis);

	std::vector<Triangle*> leftChildTriangles;
	std::vector<Triangle*> rightChildTriangles;
	
	int splittedTriangleCount = 0;
#pragma omp parallel for 
	for (int i = 0; i < triangles.size(); ++i)
	{
		auto vertices = triangles[i]->GetVertices();
		float x[]{ vertices[0].x, vertices[1].x, vertices[2].x };
		float y[]{ vertices[0].y, vertices[1].y, vertices[2].y };
		float z[]{ vertices[0].z, vertices[1].z, vertices[2].z };

		float* testVerticesPos;
		switch (longestAxis)
		{
		case Axis::X: testVerticesPos = &x[0]; break;
		case Axis::Y: testVerticesPos = &y[0]; break;
		case Axis::Z: testVerticesPos = &z[0]; break;
		}

		float t1 = testVerticesPos[0];
		float t2 = testVerticesPos[1];
		float t3 = testVerticesPos[2];

		bool fullyLeftOfMedian = testVerticesPos[0] < axisMedian && testVerticesPos[1] < axisMedian && testVerticesPos[2] < axisMedian;
		bool fullyRightOfMedian = testVerticesPos[0] > axisMedian && testVerticesPos[1] > axisMedian && testVerticesPos[2] > axisMedian;

		if (!fullyLeftOfMedian && !fullyRightOfMedian)
		{
#pragma omp critical ( TRIANGLE_ADD )
			{
				leftChildTriangles.push_back(triangles[i]);
				rightChildTriangles.push_back(triangles[i]);
			}
#pragma omp atomic
			++splittedTriangleCount;
		}
		else if (fullyLeftOfMedian)
		{
#pragma omp critical ( TRIANGLE_ADD )
			leftChildTriangles.push_back(triangles[i]);
		}
		else
		{
#pragma omp critical ( TRIANGLE_ADD )
			rightChildTriangles.push_back(triangles[i]);
		}
	}

	assert(leftChildTriangles.size() != 0);
	assert(rightChildTriangles.size() != 0);

	//if matchQuote (% of triangles) is satisfied AND size > minTriangles in the list, don't subdivide any more 
	float matchQuote = 0.5f;
	int minTriangles = 100;
	if ((float)splittedTriangleCount / leftChildTriangles.size() < matchQuote && (float)splittedTriangleCount / rightChildTriangles.size() < matchQuote
		&& leftChildTriangles.size() > minTriangles && rightChildTriangles.size() > minTriangles)
	{
		node->LeftChild = Build(leftChildTriangles, depth + 1);
		node->RightChild = Build(rightChildTriangles, depth + 1);
	}
	else
	{
		node->LeftChild = new KDNode();
		node->RightChild = new KDNode();
		node->LeftChild->Triangles = std::vector<Triangle*>();
		node->RightChild->Triangles = std::vector<Triangle*>();
	}

	return node;
}


HitResult KDNode::RayCast(KDNode* root, const DirectX::SimpleMath::Ray& ray)
{
	float tmin = (std::numeric_limits<float>::max)();
	HitResult outHitResult;
	KDNode::isHit(root, ray, tmin, outHitResult);
	return outHitResult;
}

bool KDNode::isHit(KDNode* node, const SimpleMath::Ray& ray, float& smallestRayLength, HitResult& outHitResult)
{
	float rayDist;
	if (ray.Intersects(node->BBox, rayDist))
	{
		bool wasTriangleHit = false;

		//if either child still has triangles, recurse down both sides and check for intersections
		if (node->LeftChild->Triangles.size() > 0 || node->RightChild->Triangles.size() > 0)
		{
			bool hitLeft = isHit(node->LeftChild, ray, smallestRayLength, outHitResult);
			bool hitRight = isHit(node->RightChild, ray, smallestRayLength, outHitResult);

			return hitLeft || hitRight;
		}
		else
		{
			//we have reached a leaf
#pragma omp parallel for 
			for (int i = 0; i < node->Triangles.size(); ++i)
			{
				HitResult hitResult;
				if (node->Triangles[i]->IsHit(ray, hitResult))
				{
#pragma omp critical ( HIT_COMPARISON )
					{
						if (hitResult.RayLength < smallestRayLength)
						{
							wasTriangleHit = true;
							smallestRayLength = hitResult.RayLength;
							outHitResult = hitResult;
						}
					}
				}
			}
			return wasTriangleHit;
		}
	}
	return false;
}

int KDNode::GetKdTreeDepth(const KDNode& root)
{
	int kdTreeDepth = -1;
	KDNode::getKdTreeDepth(root, kdTreeDepth);
	return kdTreeDepth;
}

float KDNode::getMedian(std::vector<Triangle*>& triangles, Axis axis)
{
	std::vector<size_t> medianIndexes;
	if (triangles.size() % 2)
	{
		//size is odd and has exactly one median 
		medianIndexes.push_back((size_t)((triangles.size() + 1) / 2));
	}
	else
	{
		//size is even and has 2 different medians!! -> calculate mathematical median
		medianIndexes.push_back((size_t)(triangles.size() / 2));
		medianIndexes.push_back((size_t)((triangles.size() / 2) +1));
	}

	std::function<bool(Triangle*, Triangle*)> comp = std::bind(&Triangle::LessThan, std::placeholders::_1, std::placeholders::_2, axis);
	for (auto& medianIndex : medianIndexes)
	{
		--medianIndex; //median would start at 1!
		std::nth_element(triangles.begin(), triangles.begin() + medianIndex, triangles.end(), comp);
	}

	if (medianIndexes.size() == 1)
		return triangles[medianIndexes[0]]->GetMidPoint(axis);
	else
		return (triangles[medianIndexes[0]]->GetMidPoint(axis) + triangles[medianIndexes[1]]->GetMidPoint(axis)) / 2.f;
}

void KDNode::getKdTreeDepth(const KDNode& root, int& outDepth)
{
	++outDepth;
	int tmpLeft = outDepth;
	int tmpRight = outDepth;

	if (root.LeftChild != nullptr)
		getKdTreeDepth(*(root.LeftChild), tmpLeft);
	if (root.RightChild != nullptr)
		getKdTreeDepth(*(root.RightChild), tmpRight);

	outDepth = tmpLeft;
	if (tmpLeft < tmpRight)
		outDepth = tmpRight;
}
