#pragma once
#include "DxRes.hpp"
#include "Shader.h"
#include "Texture.h"
#include "../math/Matr.h"
#include "singletones/TransformSystem.h"
#include "Model.h"

namespace engine
{
	class PickableInstances
	{
	public:
		struct Instance
		{
			Vec3 instancePos;
			float instanceSize;
			Instance(){}
			explicit Instance(const Vec3 &pos, float size) : instancePos(pos), instanceSize(size)
			{}
		};

		struct GpuInstance
		{
			Vec3 instancePos;
			float instanceSize;
			GpuInstance(){}
			GpuInstance(const Instance& instance) : instancePos(instance.instancePos), instanceSize(instance.instanceSize)
			{}
		};

		std::vector<Instance> m_instances;
		VertexBuffer<GpuInstance> m_instanceBuffer;
		Shader m_shader;
		std::shared_ptr<Model> m_model;
		DxResPtr<ID3D11RasterizerState> m_rasterizerState;
		Texture m_texture;
	public:
		PickableInstances() {}
		void Init();
		void render();
		void updateInstanceBuffer();
	};
}