#include "PickableInstances.h"
#include "singletones/RasteriserStateManager.h"
#include "singletones/ModelManager.h"
#include "singletones/BlendStateManager.h"

namespace engine
{
	void PickableInstances::Init()
	{
		D3D11_INPUT_ELEMENT_DESC inputLayout[] =
		{
			{"POSITION", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},

			{"INSTANCE_POS", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
			{"INSTANCE_SIZE", 0, DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
		};

		m_shader.Init(L"engine/shaders/pickable.hlsl", inputLayout, 5, ShaderEnabling(true, false));
		m_rasterizerState = RasteriserStateManager::instance().GetRasteriserState("front_and_back");
		m_model = ModelManager::instance().GetUnitQuadModel();
		m_texture.Load(L"engine/assets/CHERRY.dds");
	}

	void PickableInstances::render()
	{
		if (m_instances.size() == 0)
			return;

		m_shader.Bind();
		m_model->m_indexBuffer.Bind();
		m_model->m_vertexBuffer.Bind(0);
		m_instanceBuffer.Bind(1);
		m_texture.Bind(0);

		engine::s_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		s_deviceContext->RSSetState(m_rasterizerState);
		s_deviceContext->DrawIndexedInstanced(m_model->m_meshes[0].indexNum, m_instanceBuffer.Size(), 0, 0, 0);
		s_deviceContext->RSSetState(nullptr);
	}

	void PickableInstances::updateInstanceBuffer()
	{
		if (m_instances.size() == 0)
			return;

		m_instanceBuffer.Init(m_instances.size(), D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE);
		auto mapping = m_instanceBuffer.Map();
		GpuInstance* dst = (GpuInstance*)mapping.pData;
		for (uint32_t i = 0; i < m_instances.size(); ++i)
		{
			dst[i] = GpuInstance(m_instances[i]);
		}
		m_instanceBuffer.Unmap();
	}
}