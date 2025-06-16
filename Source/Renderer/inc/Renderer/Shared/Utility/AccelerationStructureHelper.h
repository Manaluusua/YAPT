#pragma once

#include <Gfx/GfxApi.h>
#include <vector>
#include <Renderer/Shared/RenderObjectManager.h>

namespace YAPT
{
	class CRenderer;

	class AccelerationStructureHelper
	{
	public:


		struct DefaultPerInstanceParamsHandler
		{
			void operator()(size_t arrayIndex, RenderObjectId id, uint32_t& instanceIdOut, uint32_t& instanceMaskOut, size_t& hitGroupShaderTableOffset)
			{
				instanceIdOut = (uint32_t)id;
				instanceMaskOut = ~0;
				hitGroupShaderTableOffset = id;
			}
		};

		AccelerationStructureHelper();
		~AccelerationStructureHelper();

		void init(CRenderer* renderer);
		void deinit();

		void updateBottomLevelStructures(CommandBufferHandle commandBuffer);

		template<typename PerInstanceParamsHandler>
		void updateTopLevelStructures(CommandBufferHandle commandBuffer, PerInstanceParamsHandler& paramsHandler);

		TopLevelAccelerationStructureHandle getTopLevelAccelerationStructure() const { return m_tlasHandle; }
	private:
		
		struct MeshBLASArray
		{
			void pushToArrayAndClear(std::vector<BottomLevelAccelerationStructureHandle>& arr)
			{
				for (size_t k = 0; k < blasPerSubMesh.size(); ++k)
				{
					if (blasPerSubMesh[k] != YAPT_NULL_HANDLE)
					{
						arr.push_back(blasPerSubMesh[k]);
						blasPerSubMesh[k] = YAPT_NULL_HANDLE;
					}
				}
			}

			void resize(size_t size)
			{
				blasPerSubMesh.resize(size);
			}

			size_t fill(BottomLevelAccelerationStructureHandle* dataInput)
			{
				memcpy(blasPerSubMesh.data(), dataInput, sizeof(BottomLevelAccelerationStructureHandle) * blasPerSubMesh.size());
				return blasPerSubMesh.size();
			}


			std::vector<BottomLevelAccelerationStructureHandle> blasPerSubMesh;
		};



		bool doesTopLevelAccelerationStructureNeedRebuild();
		void fillInstanceTransform(const mat4& src, float dst[12]);

		struct
		{
			std::vector<AccelerationStructureGeometryDefinition> geomDefs;
			std::vector<AccelerationStructureInstanceDefinition> instanceDefs;
			std::vector<size_t> meshIndexOfBlas;
			std::vector<BottomLevelAccelerationStructureDefinition> blasDefs;
			std::vector<BottomLevelAccelerationStructureHandle> blasHandles;
			std::vector<BottomLevelAccelerationStructureHandle> blasHandlesToRelease;

			void clear()
			{
				geomDefs.clear();
				instanceDefs.clear();
				meshIndexOfBlas.clear();
				blasDefs.clear();
				blasHandles.clear();
				blasHandlesToRelease.clear();
			}

		} m_temporaryContainers;

		CRenderer* m_renderer;

		std::vector<MeshBLASArray> m_blasArrayPerMesh;
		TopLevelAccelerationStructureHandle m_tlasHandle;

		

		bool m_blasHadChanges;
		bool m_isFirstUpdate;

	};


	template<typename PerInstanceParamsHandler>
	void AccelerationStructureHelper::updateTopLevelStructures(CommandBufferHandle commandBuffer, PerInstanceParamsHandler& paramsHandler)
	{
		if (doesTopLevelAccelerationStructureNeedRebuild())
		{
			RenderObjectManager& mngr = m_renderer->getRenderObjectManager();

			std::vector<AccelerationStructureInstanceDefinition>& instanceDefs = m_temporaryContainers.instanceDefs;
			size_t numberOfRenderObjects = mngr.getNumberOfObjects();
			instanceDefs.reserve(numberOfRenderObjects);

			MeshIndex* meshes = mngr.getAllMeshes();
			mat4* transforms = mngr.getAllMatrices();
			const RenderObjectId* ids = mngr.getAllIds();

			for (size_t i = 0; i < numberOfRenderObjects; ++i)
			{
				size_t meshIndex = meshes[i];
				const MeshBLASArray& blasArray = m_blasArrayPerMesh[meshIndex];
				size_t instanceDefsOffset = instanceDefs.size();
				instanceDefs.resize(instanceDefs.size() + blasArray.blasPerSubMesh.size());

				for (size_t k = 0; k < blasArray.blasPerSubMesh.size(); ++k)
				{
					AccelerationStructureInstanceDefinition& def = instanceDefs[instanceDefsOffset + k];
					fillInstanceTransform(transforms[i], def.instanceToWorld);
					def.blas = blasArray.blasPerSubMesh[k];
					paramsHandler(i, ids[i], k,  def.instanceID, def.instanceMask, def.hitGroupShaderTableOffset);
				}
			}

			//if (instanceDefs.size() > 0)
			{
				if (m_tlasHandle != YAPT_NULL_HANDLE)
				{
					Gfx::destroyTopLevelAccelerationStructures(m_renderer->getGfxHandle(), &m_tlasHandle, 1);
				}

				TopLevelAccelerationStructureDefinition tlasDef;
				tlasDef.instanceDefinitions = instanceDefs.data();
				tlasDef.instanceDefinitionCount = instanceDefs.size();

				Gfx::allocateTopLevelAccelerationStructures(m_renderer->getGfxHandle(), &tlasDef, 1, &m_tlasHandle);
				Gfx::buildTopLevelAccelerationStructures(m_renderer->getGfxHandle(), commandBuffer, &m_tlasHandle, 1);
			}

		}
		m_isFirstUpdate = false;
	}
}