#include "OpaqueInstances.h"
#include "singletones/LightSystem.h"
#include "singletones/ShadowManager.h"
static ID3D11ShaderResourceView* SRVnullptr[1] = { nullptr };
namespace engine
{
	void OpaqueInstances::Init()
	{
		D3D11_INPUT_ELEMENT_DESC inputLayout[] =
		{
			{"POSITION", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TANGENT", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"BITANGENT", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},

			{"TRANSFORM_X", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"TRANSFORM_Y", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"TRANSFORM_Z", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"TRANSFORM_W", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"MESH_ID", 0, DXGI_FORMAT::DXGI_FORMAT_R16_UINT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"GRAVITY", 0, DXGI_FORMAT::DXGI_FORMAT_R32_UINT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		};
		m_shader.Init(L"engine/shaders/opaque.hlsl", inputLayout, 11, ShaderEnabling(true, false));
		m_constantBuffer.Init(D3D11_USAGE::D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
		m_materialConstantBuffer.Init(D3D11_USAGE::D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

		ShadowManager::instance().m_pointLightOpaqueShadowShader.Init(L"engine/shaders/point_light_shadow.hlsl", inputLayout, 9, ShaderEnabling(false, true));
		ShadowManager::instance().m_directionalLightOpaqueShadowShader.Init(L"engine/shaders/directional_light_shadow.hlsl", inputLayout, 9, ShaderEnabling(false, true));
	}

	void OpaqueInstances::updateInstanceBuffers()
	{
		uint32_t totalInstances = 0;
		for (auto& modelInstances : m_modelInstances)
			for (auto& meshInstances : modelInstances.meshInstances)
				for (auto& materialInstances : meshInstances.materialInstances)
					totalInstances += uint32_t(materialInstances.instances.size());

		if (totalInstances == 0)
		{
			m_instanceBuffer.Init(0, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE);
			return;
		}

		m_instanceBuffer.Init(totalInstances, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE);

		auto mapping = m_instanceBuffer.Map();
		GpuInstance* dst = static_cast<GpuInstance*>(mapping.pData);

		uint32_t copiedNum = 0;
		for (auto& modelInstances : m_modelInstances)
		{
			for (uint32_t meshIndex = 0; meshIndex < modelInstances.meshInstances.size(); ++meshIndex)
			{
				for (auto& materialInstances : modelInstances.meshInstances[meshIndex].materialInstances)
				{
					auto& instances = materialInstances.instances;

					uint32_t numModelInstances = instances.size();
					for (uint32_t index = 0; index < numModelInstances; ++index)
					{
						dst[copiedNum++] = GpuInstance(instances[index]);
					}
				}
			}
		}

		m_instanceBuffer.Unmap();
	}

	void OpaqueInstances::render()
	{
		if (m_instanceBuffer.Size() == 0)
			return;

		s_deviceContext->PSSetShaderResources(8, 1, &ShadowManager::instance().m_srvPointLightShadow.ptr());
		s_deviceContext->PSSetShaderResources(9, 1, &ShadowManager::instance().m_srvDirectionalLightShadow.ptr());

		m_shader.Bind();
		m_instanceBuffer.Bind(1);
		m_constantBuffer.BindVS(1);
		m_materialConstantBuffer.BindPS(2);


		engine::s_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		uint32_t renderedInstances = 0;
		for (auto& modelInstances : m_modelInstances)
		{
			if (modelInstances.meshInstances.empty()) continue;

			modelInstances.model->m_vertexBuffer.Bind(0);
			modelInstances.model->m_indexBuffer.Bind();

			for (uint32_t meshIndex = 0; meshIndex < modelInstances.meshInstances.size(); ++meshIndex)
			{
				Mesh& mesh = modelInstances.model->m_meshes[meshIndex];

				m_constantBuffer.Update(mesh.meshToModelMatrix);

				for (auto& materialInstances : modelInstances.meshInstances[meshIndex].materialInstances)
				{
					if (materialInstances.instances.empty()) continue;

					auto& material = materialInstances.material;
					material.Bind();

					m_materialConstantBuffer.Update(material.m_materialConstantBuffer);

					uint32_t numInstances = uint32_t(materialInstances.instances.size());
					engine::s_deviceContext->DrawIndexedInstanced(mesh.indexNum, numInstances, mesh.indexOffset, mesh.vertexOffset, renderedInstances);
					renderedInstances += numInstances;
				}
			}
		}
		engine::s_deviceContext->PSSetShaderResources(8, 1, SRVnullptr);
		engine::s_deviceContext->PSSetShaderResources(9, 1, SRVnullptr);
	}

	void OpaqueInstances::renderSceneDepthToCubemaps()
	{
		uint32_t pointLightNum = LightSystem::instance().getPointLights().size();
		if (pointLightNum == 0)
			return;

		if (m_instanceBuffer.Size() == 0)
			return;

		m_instanceBuffer.Bind(1);
		m_constantBuffer.BindVS(1);

		ShadowManager::instance().m_pointLightOpaqueShadowShader.Bind();
		ShadowManager::instance().m_pointLightShadowBuffer.BindGS(2);

		engine::s_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		uint32_t renderedInstances = 0;
		for (auto& modelInstances : m_modelInstances)
		{
			if (modelInstances.meshInstances.empty()) continue;

			modelInstances.model->m_vertexBuffer.Bind(0);
			modelInstances.model->m_indexBuffer.Bind();

			for (uint32_t meshIndex = 0; meshIndex < modelInstances.meshInstances.size(); ++meshIndex)
			{
				Mesh& mesh = modelInstances.model->m_meshes[meshIndex];

				m_constantBuffer.Update(mesh.meshToModelMatrix);

				for (auto& materialInstances : modelInstances.meshInstances[meshIndex].materialInstances)
				{
					if (materialInstances.instances.empty()) continue;

					uint32_t numInstances = uint32_t(materialInstances.instances.size());
					for (uint32_t i = 0; i < pointLightNum; ++i)
					{
						ShadowManager::instance().m_pointLightShadowBuffer.Update(ShadowManager::PointLightShadowBuffer(i));
						engine::s_deviceContext->DrawIndexedInstanced(mesh.indexNum, numInstances, mesh.indexOffset, mesh.vertexOffset, renderedInstances);
					}
					renderedInstances += numInstances;
				}
			}
		}
	}
	void OpaqueInstances::renderSceneDepthForDirectionalLights()
	{
		uint32_t directionalLightNum = LightSystem::instance().getDirectionalLights().size();

		if (directionalLightNum == 0)
			return;

		if (m_instanceBuffer.Size() == 0)
			return;

		m_instanceBuffer.Bind(1);
		m_constantBuffer.BindVS(1);

		ShadowManager::instance().m_directionalLightOpaqueShadowShader.Bind();
		ShadowManager::instance().m_directionalLightShadowBuffer.BindGS(2);

		engine::s_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		uint32_t renderedInstances = 0;
		for (auto& modelInstances : m_modelInstances)
		{
			if (modelInstances.meshInstances.empty()) continue;

			modelInstances.model->m_vertexBuffer.Bind(0);
			modelInstances.model->m_indexBuffer.Bind();

			for (uint32_t meshIndex = 0; meshIndex < modelInstances.meshInstances.size(); ++meshIndex)
			{
				Mesh& mesh = modelInstances.model->m_meshes[meshIndex];

				m_constantBuffer.Update(mesh.meshToModelMatrix);

				for (auto& materialInstances : modelInstances.meshInstances[meshIndex].materialInstances)
				{
					if (materialInstances.instances.empty()) continue;

					uint32_t numInstances = uint32_t(materialInstances.instances.size());
					for (uint32_t i = 0; i < directionalLightNum; ++i)
					{
						ShadowManager::instance().m_directionalLightShadowBuffer.Update(ShadowManager::DirectionalLightShadowBuffer(i));
						engine::s_deviceContext->DrawIndexedInstanced(mesh.indexNum, numInstances, mesh.indexOffset, mesh.vertexOffset, renderedInstances);
					}
					renderedInstances += numInstances;
				}
			}
		}
	}
}