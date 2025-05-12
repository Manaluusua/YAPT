#pragma once
#include <Math/Math.h>
#include <Renderer/Shared/Utility/GpuBufferHelper.h>
#include <Renderer/Shared/MaterialManager.h>

namespace YAPT
{
	struct MaterialEntryGPU
	{
		vec4p specAmountClearCoatAmountIORRoughness;
		vec4p albedoTransparency;
		vec4p specularMetalness;
		vec4p absorptionDielectricIOR;
		vec4p emissiveRoughness;

		float anisotropy;
		float anisotropyRotation;
		uint32_t materialMask;
		float thinFilmThickness;

		vec2p cauchysCoefficients;
		float sheenAmount;
		float pad0;


		vec4p sheenColorRoughness;
		uvec2p albedoTexIndexAndScale;
		uvec2p normalTexIndexAndScale;
		uvec2p ormTexIndexAndScale;
		uvec2p emissiveTexIndexAndScale;
	};

	class CRenderer;
	class BindlessMaterialManager
	{
	public:
		BindlessMaterialManager();
		~BindlessMaterialManager();

		void init(CRenderer* renderer);
		void update();
		void shutdown();
		size_t getEntryIndexForMaterialId(MaterialIndex id) { return m_materialIDToBufferIndex[id]; }

		BufferViewHandle getBufferViewHandle() const { return m_gpuBuffer.getBufferViewHandle(); }

	private:

		void syncMaterialStateToGPU();

		CRenderer* m_renderer;
		DynamicSizeGpuBufferHelper<MaterialEntryGPU> m_gpuBuffer;
		std::vector<size_t> m_materialIDToBufferIndex;
	};
}