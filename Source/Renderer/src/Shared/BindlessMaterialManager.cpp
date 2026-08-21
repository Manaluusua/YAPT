#include <Renderer/Shared/BindlessMaterialManager.h>
#include <Renderer/Shared/CRenderer.h>

#include <Math/MathUtility.h>

namespace YAPT
{
	size_t MAT_BUFFER_INITIAL_SIZE = 128;
	BindlessMaterialManager::BindlessMaterialManager()
		:m_renderer(nullptr),
		m_gpuBuffer(ResourceUsageBits::RESOURCE_USAGE_COPY_DESTINATION | ResourceUsageBits::RESOURCE_USAGE_STORAGE_BUFFER)
	{

	}
	BindlessMaterialManager::~BindlessMaterialManager()
	{
		m_gpuBuffer.free();
	}

	void BindlessMaterialManager::init(CRenderer* renderer)
	{
		m_renderer = renderer;
		m_gpuBuffer.init(renderer->getGfxHandle());
		m_gpuBuffer.allocate(MAT_BUFFER_INITIAL_SIZE, "MaterialsBuffer");

		syncMaterialStateToGPU();

	}
	void BindlessMaterialManager::shutdown()
	{
		m_gpuBuffer.free();
	}

	void BindlessMaterialManager::update()
	{
		MaterialManager& matMngr = m_renderer->getMaterialManager();
		if (matMngr.hasChanges())
		{
			syncMaterialStateToGPU();
		}
	}

	void BindlessMaterialManager::syncMaterialStateToGPU()
	{
		MaterialManager& matMngr = m_renderer->getMaterialManager();

		//for now just rewrite everything on any change. TODO: do subupdates if this grows too big
		size_t sizeNeeded = matMngr.getActiveEntriesCount();
		if (m_gpuBuffer.getAllocatedEntryCount() < sizeNeeded)
		{
			m_gpuBuffer.free();
			m_gpuBuffer.allocate(sizeNeeded, "MaterialsBuffer");
		}

		if (matMngr.getHighestAllocatedIndex() >= m_materialIDToBufferIndex.size())
		{
			m_materialIDToBufferIndex.clear();
			m_materialIDToBufferIndex.resize(matMngr.getHighestAllocatedIndex() + 1, -1);
		}

		if (sizeNeeded == 0) return;

		char* gpuBuffer = m_gpuBuffer.map(0, sizeNeeded);

		auto& iter = matMngr.getMaterialIterator();
		size_t index = 0;
		MaterialInternal* mat = iter.getCurrent();
		while (mat != nullptr)
		{
			const MaterialParameters& matParams = mat->getMaterialParams();
			MaterialEntryGPU dst;

			m_materialIDToBufferIndex[mat->getID()] = index;

			float transparency = matParams.transparency;
			float metalness = matParams.metalness;

			dst.specAmountClearCoatAmountIORRoughness = vec4p(MathUtils::saturate(matParams.specularAmount),
				MathUtils::saturate(matParams.clearCoatAmount), matParams.clearCoatIOR, MathUtils::saturate(matParams.clearCoatRoughness));

			dst.albedoTransparency = vec4p(matParams.albedo, matParams.transparency);
			dst.specularMetalness = vec4p(matParams.specular, matParams.metalness);
			dst.absorptionDielectricIOR = vec4p(matParams.absorption, glm::clamp(matParams.dielectricIOR, 0.3f, 3.0f));
			dst.emissiveRoughness = vec4p(matParams.emissive, MathUtils::saturate(matParams.roughness));
			dst.anisotropy = MathUtils::saturate(matParams.anisotropy);
			dst.anisotropyRotation = matParams.anisotropyRotation;

			dst.materialMask = matParams.materialMask;
			dst.thinFilmThicknessNM = matParams.thinFilmThicknessNM;

			dst.cauchysCoefficients = matParams.cauchysCoeffs;
			dst.sheenAmount = matParams.sheenAmount;
			dst.emissionFocus = matParams.emissionFocus;
			dst.sheenColorRoughness = vec4p(matParams.sheenTint, matParams.sheenRoughness);
			dst.albedoTexIndexAndScale = uvec2p(matParams.albedoTex.textureIndex, matParams.albedoTex.packedScale);
			dst.normalTexIndexAndScale = uvec2p(matParams.normalTex.textureIndex, matParams.normalTex.packedScale);
			dst.ormTexIndexAndScale = uvec2p(matParams.ormTex.textureIndex, matParams.ormTex.packedScale);
			dst.emissiveTexIndexAndScale = uvec2p(matParams.emissiveTex.textureIndex, matParams.emissiveTex.packedScale);

			memcpy(gpuBuffer + m_gpuBuffer.getAlignedEntrySize() * index, &dst, sizeof(MaterialEntryGPU));

			++index;
			mat = iter.getNextValidEntry();
		}

		m_gpuBuffer.unmap();

	}
}