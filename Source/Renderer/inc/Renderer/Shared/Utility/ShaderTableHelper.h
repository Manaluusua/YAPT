#pragma once

#include <Renderer/Shared/GfxApi.h>

namespace YAPT
{
	class CRenderer;
	class ShaderTableHelper
	{
	public:
		ShaderTableHelper();
		~ShaderTableHelper();

		void init(CRenderer* renderer, RaytracePipelineStateHandle pso, size_t rayGenExtraConstantsSizeInBytes, size_t missExtraConstantsSizeInBytes, size_t hitGroupExtraConstantsSizeInBytes);
		void deinit();

		void resize(size_t rayGenEntries, size_t numberOfMissEntries, size_t numberOfHitgroupEntries);

		ShaderTableHandle getShaderTable() const { return m_shaderTableHandle; }
		ShaderTableEntry* appendRayGenUpdate();
		ShaderTableEntry* appendMissShaderUpdate();
		ShaderTableEntry* appendHitGroupUpdate();

		size_t getNumberOfRayGenEntries() const { return m_numberOfRayGenEntries; }
		size_t getNumberOfMissEntries() const { return m_numberOfMissEntries; }
		size_t getNumberOfHitGroupEntries() const { return m_numberOfHitGroupEntries; }

		void flush();

	private:
		CRenderer* m_renderer;
		RaytracePipelineStateHandle m_pso;
		ShaderTableHandle m_shaderTableHandle;
		size_t m_numberOfRayGenEntries;
		size_t m_numberOfMissEntries;
		size_t m_numberOfHitGroupEntries;

		size_t m_rayGenExtraConstantsSizeInBytes;
		size_t m_missExtraConstantsSizeInBytes;
		size_t m_hitGroupExtraConstantsSizeInBytes;

		std::vector<uint8_t> m_rayGenExtraData;
		std::vector<uint8_t> m_missExtraData;
		std::vector<uint8_t> m_hitGroupExtraData;

		std::vector<ShaderTableEntry> m_updatedRayGenEntries;
		std::vector<ShaderTableEntry> m_updatedMissEntries;
		std::vector<ShaderTableEntry> m_updatedHitGroupEntries;


	};

}