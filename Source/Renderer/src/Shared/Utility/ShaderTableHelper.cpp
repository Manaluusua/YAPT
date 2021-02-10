#include <Renderer/Shared/Utility/ShaderTableHelper.h>
#include <Renderer/Shared/CRenderer.h>
namespace YAPT
{
	ShaderTableHelper::ShaderTableHelper()
		:m_shaderTableHandle(YAPT_NULL_HANDLE),
		m_numberOfRayGenEntries(0),
		m_numberOfMissEntries(0),
		m_numberOfHitGroupEntries(0)
	{

	}
	ShaderTableHelper::~ShaderTableHelper()
	{

	}

	void ShaderTableHelper::init(CRenderer* renderer, RaytracePipelineStateHandle pso)
	{
		m_renderer = renderer;
		m_pso = pso;
		m_rayGenExtraConstantsSizeInBytes = 0;
		m_missExtraConstantsSizeInBytes = 0;
		m_hitGroupExtraConstantsSizeInBytes = 0;

	}
	void ShaderTableHelper::deinit()
	{
		if (m_shaderTableHandle != YAPT_NULL_HANDLE)
		{
			Gfx::destroyShaderTable(m_renderer->getGfxHandle(), m_shaderTableHandle);
			m_shaderTableHandle = YAPT_NULL_HANDLE;
		}
	}

	void ShaderTableHelper::resize(size_t rayGenEntries, size_t numberOfMissEntries, size_t numberOfHitgroupEntries)
	{
		if (m_shaderTableHandle != YAPT_NULL_HANDLE)
		{
			Gfx::destroyShaderTable(m_renderer->getGfxHandle(), m_shaderTableHandle);
			m_shaderTableHandle = YAPT_NULL_HANDLE;
		}

		m_numberOfRayGenEntries = rayGenEntries;
		m_numberOfMissEntries = numberOfMissEntries;
		m_numberOfHitGroupEntries = numberOfHitgroupEntries;


		m_rayGenExtraData.resize(m_numberOfRayGenEntries * m_rayGenExtraConstantsSizeInBytes);
		m_missExtraData.resize(m_numberOfMissEntries * m_missExtraConstantsSizeInBytes);
		m_hitGroupExtraData.resize(m_numberOfHitGroupEntries * m_hitGroupExtraConstantsSizeInBytes);

		m_shaderTableHandle = Gfx::createShaderTable(m_renderer->getGfxHandle(), m_pso, m_numberOfRayGenEntries, m_numberOfMissEntries, m_numberOfHitGroupEntries);
	}

	void ShaderTableHelper::flush()
	{
		Gfx::setShaderTableEntries(m_renderer->getGfxHandle(), m_shaderTableHandle, m_updatedRayGenEntries.data(), m_updatedRayGenEntries.size(),
			m_updatedMissEntries.data(), m_updatedMissEntries.size(),
			m_updatedHitGroupEntries.data(), m_updatedHitGroupEntries.size());

		m_updatedRayGenEntries.clear();
		m_updatedMissEntries.clear();
		m_updatedHitGroupEntries.clear();
	}


	ShaderTableEntry* ShaderTableHelper::appendRayGenUpdate()
	{
		m_updatedRayGenEntries.resize(m_updatedRayGenEntries.size() + 1);
		size_t index = m_updatedRayGenEntries.size() - 1;
		assert(index < m_numberOfRayGenEntries);
		//m_updatedRayGenEntries[index].shaderTableExtraData = m_rayGenExtraData.data() + index * m_rayGenExtraConstantsSizeInBytes;
		return &m_updatedRayGenEntries[index];
	}
	ShaderTableEntry* ShaderTableHelper::appendMissShaderUpdate()
	{
		m_updatedMissEntries.resize(m_updatedMissEntries.size() + 1);
		size_t index = m_updatedMissEntries.size() - 1;
		assert(index < m_numberOfMissEntries);
		//m_updatedMissEntries[index].shaderTableExtraData = m_missExtraData.data() + index * m_missExtraConstantsSizeInBytes;
		return &m_updatedMissEntries[index];
	}
	ShaderTableEntry* ShaderTableHelper::appendHitGroupUpdate()
	{
		m_updatedHitGroupEntries.resize(m_updatedHitGroupEntries.size() + 1);
		size_t index = m_updatedHitGroupEntries.size() - 1;
		assert(index < m_numberOfHitGroupEntries);
		//m_updatedHitGroupEntries[index].shaderTableExtraData = m_hitGroupExtraData.data() + index * m_hitGroupExtraConstantsSizeInBytes;
		return &m_updatedHitGroupEntries[index];
	}

}