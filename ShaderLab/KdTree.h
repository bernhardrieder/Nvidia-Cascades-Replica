#pragma once
#include "stdafx.h"

class KDNode
{
public:
	DirectX::BoundingBox BBox;
	KDNode* LeftChild;
	KDNode* RightChild;
	std::vector<Triangle*> Triangles;

	KDNode();
	KDNode(DirectX::BoundingBox bbox, KDNode* left, KDNode* right, std::vector<Triangle*> triangles);
	KDNode(KDNode&&) = delete;
	KDNode(const KDNode&);
	KDNode& operator= (const KDNode&);
	~KDNode();
	static KDNode* Build(std::vector<Triangle*>& triangles, int depth);
	static HitResult RayCast(KDNode* root, const DirectX::SimpleMath::Ray& ray);
	static int GetKdTreeDepth(const KDNode& root);

private:
	static float getMedian(std::vector<Triangle*>& triangles, Axis axis);
	static void getKdTreeDepth(const KDNode& root, int& outDepth);
	static bool isHit(KDNode* node, const DirectX::SimpleMath::Ray& ray, float& tmin, HitResult& outHitResult);
};
