#include <algorithm>
#include "TriangleOctree.hpp"
#include "d3d.hpp"
#include <array>
#include "Mesh.h"

namespace engine
{
	const int TriangleOctree::PREFFERED_TRIANGLE_COUNT = 32;
	const float TriangleOctree::MAX_STRETCHING_RATIO = 1.05f;

	inline const Vec3& getPos(const Mesh& mesh, uint32_t triangleIndex, uint32_t vertexIndex)
	{
		uint32_t index = mesh.triangles.empty() ?
			triangleIndex * 3 + vertexIndex :
			mesh.triangles[triangleIndex].indices[vertexIndex];
		
		return mesh.vertices[index].position;
	}

	void TriangleOctree::initialize(const Mesh& mesh)
	{
		m_triangles.clear();
		m_triangles.shrink_to_fit();

		m_mesh = &mesh;
		m_children = nullptr;

		const Vec3 eps = { 1e-5f, 1e-5f, 1e-5f };
		m_box = m_initialBox = { mesh.box.min - eps, mesh.box.max + eps };

		for (uint32_t i = 0; i < mesh.triangles.size(); ++i)
		{
			const Vec3& V1 = getPos(mesh, i, 0);
			const Vec3& V2 = getPos(mesh, i, 1);
			const Vec3& V3 = getPos(mesh, i, 2);

			Vec3 P = (V1 + V2 + V3) / 3.f;

			bool inserted = addTriangle(i, V1, V2, V3, P);
			DEV_ASSERT(inserted);
		}
	}

	void TriangleOctree::initialize(const Mesh& mesh, const Box& parentBox, const Vec3& parentCenter, int octetIndex)
	{
		m_mesh = &mesh;
		m_children = nullptr;

		const float eps = 1e-5f;

		if (octetIndex % 2 == 0)
		{
			m_initialBox.min[0] = parentBox.min[0];
			m_initialBox.max[0] = parentCenter[0];
		}
		else
		{
			m_initialBox.min[0] = parentCenter[0];
			m_initialBox.max[0] = parentBox.max[0];
		}
		
		if (octetIndex % 4 < 2)
		{
			m_initialBox.min[1] = parentBox.min[1];
			m_initialBox.max[1] = parentCenter[1];
		}
		else
		{
			m_initialBox.min[1] = parentCenter[1];
			m_initialBox.max[1] = parentBox.max[1];
		}

		if (octetIndex < 4)
		{
			m_initialBox.min[2] = parentBox.min[2];
			m_initialBox.max[2] = parentCenter[2];
		}
		else
		{
			m_initialBox.min[2] = parentCenter[2];
			m_initialBox.max[2] = parentBox.max[2];
		}

		m_box = m_initialBox;
		Vec3 elongation = (MAX_STRETCHING_RATIO - 1.f) * m_box.size();

		if (octetIndex % 2 == 0) m_box.max[0] += elongation[0];
		else m_box.min[0] -= elongation[0];

		if (octetIndex % 4 < 2) m_box.max[1] += elongation[1];
		else m_box.min[1] -= elongation[1];

		if (octetIndex < 4) m_box.max[2] += elongation[2];
		else m_box.min[2] -= elongation[2];
	}

	bool TriangleOctree::addTriangle(uint32_t triangleIndex, const Vec3& V1, const Vec3& V2, const Vec3& V3, const Vec3& center)
	{
		if (!m_initialBox.contains(center) ||
			!m_box.contains(V1) ||
			!m_box.contains(V2) ||
			!m_box.contains(V3))
		{
			return false;
		}

		if (m_children == nullptr)
		{
			if (m_triangles.size() < PREFFERED_TRIANGLE_COUNT)
			{
				m_triangles.emplace_back(triangleIndex);
				return true;
			}
			else
			{
				Vec3 C = (m_initialBox.min + m_initialBox.max) / 2.f;

				m_children.reset(new std::array<TriangleOctree, 8>());
				for (int i = 0; i < 8; ++i)
				{
					(*m_children)[i].initialize(*m_mesh, m_initialBox, C, i);
				}

				std::vector<uint32_t> newTriangles;

				for (uint32_t index : m_triangles)
				{
					const Vec3& P1 = getPos(*m_mesh, index, 0);
					const Vec3& P2 = getPos(*m_mesh, index, 1);
					const Vec3& P3 = getPos(*m_mesh, index, 2);

					Vec3 P = (P1 + P2 + P3) / 3.f;

					int i = 0;
					for (; i < 8; ++i)
					{
						if ((*m_children)[i].addTriangle(index, P1, P2, P3, P))
							break;
					}

					if (i == 8)
						newTriangles.emplace_back(index);
				}

				m_triangles = std::move(newTriangles);
			}
		}

		int i = 0;
		for (; i < 8; ++i)
		{
			if ((*m_children)[i].addTriangle(triangleIndex, V1, V2, V3, center))
				break;
		}

		if (i == 8)
			m_triangles.emplace_back(triangleIndex);

		return true;
	}

	bool TriangleOctree::intersect(const ray& ray, MeshIntersection& nearest) const
	{
		float boxT = nearest.t;
		if (!ray.intersect(boxT, m_box))
			return false;

		return intersectInternal(ray, nearest);
	}

	bool TriangleOctree::intersectInternal(const ray& ray, MeshIntersection& nearest) const
	{
		bool found = false;

		for (uint32_t i = 0; i < m_triangles.size(); ++i)
		{
			const Vec3& V1 = getPos(*m_mesh, m_triangles[i], 0);
			const Vec3& V2 = getPos(*m_mesh, m_triangles[i], 1);
			const Vec3& V3 = getPos(*m_mesh, m_triangles[i], 2);

			if (ray.intersect(nearest, V1, V2, V3))
			{
				nearest.triangle = i;
				found = true;
			}
		}

		if (!m_children) return found;

		struct OctantIntersection
		{
			int index;
			float t;
		};

		std::array<OctantIntersection, 8> boxIntersections;

		for (int i = 0; i < 8; ++i)
		{
			if ((*m_children)[i].m_box.contains(ray.origin))
			{
				boxIntersections[i].index = i;
				boxIntersections[i].t = 0.f;
			}
			else
			{
				float boxT = nearest.t;
				if (ray.intersect(boxT, (*m_children)[i].m_box))
				{
					boxIntersections[i].index = i;
					boxIntersections[i].t = boxT;
				}
				else
				{
					boxIntersections[i].index = -1;
				}
			}
		}

		std::sort(boxIntersections.begin(), boxIntersections.end(),
			[](const OctantIntersection& A, const OctantIntersection& B) -> bool { return A.t < B.t; });

		for (int i = 0; i < 8; ++i)
		{
			if (boxIntersections[i].index < 0 || boxIntersections[i].t > nearest.t)
				continue;

			if ((*m_children)[boxIntersections[i].index].intersectInternal(ray, nearest))
			{
				found = true;
			}
		}

		return found;
	}
}