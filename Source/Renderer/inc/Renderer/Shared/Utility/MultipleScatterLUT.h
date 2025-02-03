#pragma once
#include <Gfx/GfxTypes.h>
#include <Renderer/Shared/Utility/RenderResourcesPool.h>
#include <Math/Math.h>

namespace YAPT
{
	class CRenderer;
	class MultiScatteringLUTs
	{
	public:
		MultiScatteringLUTs();
		~MultiScatteringLUTs();

		void initializeLUTStorage(CRenderer* r, RenderResourcesPool& pool);
		void initializeLUTContents();

		TextureViewHandle getSingleScatterDirectionalAlbedoNoFresnel() { return m_ssDirAlbedoNoFresnelLUT.textureView; }
		TextureViewHandle getSingleScatterAverageDirectionalAlbedoNoFresnel() { return m_ssAvgDirAlbedoNoFresnelLUT.textureView; }

		TextureViewHandle getSingleAndMultiScatterDirectionalAlbedo() { return m_ssmsDirAlbedoLUT.textureView; }
		TextureViewHandle getSingleAndMultiScatterAverageDirectionalAlbedo() { return m_ssmsAvgDirAlbedoLUT.textureView; }

		TextureViewHandle getDirectionalAlbedoSheen() { return m_dirAlbedoSheen.textureView; }

		TextureViewHandle getSingleScatterDirectionalAlbedoTranslucentToDenser() { return m_ssDirAlbedoTranslucentToDenserLUT.textureView; }
		TextureViewHandle getSingleScatterDirectionalAlbedoTranslucentToLighter() { return m_ssDirAlbedoTranslucentToLighterLUT.textureView; }
		TextureViewHandle getSingleScatterAverageAlbedoTranslucentToDenser() { return m_ssAvgAlbedoTranslucentToDenserLUT.textureView; }
		TextureViewHandle getSingleScatterAverageAlbedoTranslucentToLighter() { return m_ssAvgAlbedoTranslucentToLighterLUT.textureView; }
	private:
		struct TextureHandleAndView
		{
			TextureHandle texture;
			TextureViewHandle textureView;
		};

		void generateSingleScatterAlbedoLUTs(float* directionalAlbedoOUT, float* avgAlbedoOUT);
		void generateMultiScatterAlbedoLUTs(const float* directionalAlbedo,const float* avgAlbedo, float* directionalMultiScatterAlbedoOUT, float* avgMultiScatterAlbedoOUT);
		void generateTranslucentScatterAlbedoLUTs(float* directionalTranslucentScatterAlbedoDenserOUT, float* directionalTranslucentScatterAlbedoLighterOUT,
			float* avgTranslucentScatterAlbedoDenserOUT, float* avgTranslucentScatterAlbedoLighterOUT);

		void generateDirectionalAlbedoSheenLUT(float* directionalAlbedoOUT);

		bool loadSingleScatterAlbedoLUTs(float* directionalAlbedoOUT, float* avgAlbedoOUT);
		bool loadMultiScatterAlbedoLUTs(float* directionalMultiScatterAlbedoOUT, float* avgMultiScatterAlbedoOUT);
		bool loadTranslucentScatterAlbedoLUTs(float* directionalTranslucentScatterAlbedoDenserOUT, float* directionalTranslucentScatterAlbedoLighterOUT,
			float* avgTranslucentScatterAlbedoDenserOUT, float* avgTranslucentScatterAlbedoLighterOUT);
		bool loadDirectionalAlbedoSheenLUT(float* directionalAlbedoOUT);

		void storeSingleScatterAlbedoLUTs(const float* directionalAlbedoOUT, const float* avgAlbedoOUT);
		void storeMultiScatterAlbedoLUTs(const float* directionalMultiScatterAlbedoOUT, const float* avgMultiScatterAlbedoOUT);
		void storeTranslucentScatterAlbedoLUTs(const float* directionalTranslucentScatterAlbedoDenserOUT, const float* directionalTranslucentScatterAlbedoLighterOUT,
			const float* avgTranslucentScatterAlbedoDenserOUT, const float* avgTranslucentScatterAlbedoLighterOUT);
		void storeDirectionalAlbedoSheenLUT(float* directionalAlbedoOUT);

		bool tryToLoadFromfile(const char* filename, ResourceDimension dim, const glm::uvec3& expectedDimensions, float* dataOut);

		TextureHandleAndView m_ssDirAlbedoNoFresnelLUT;
		TextureHandleAndView m_ssAvgDirAlbedoNoFresnelLUT;

		TextureHandleAndView m_ssmsDirAlbedoLUT;
		TextureHandleAndView m_ssmsAvgDirAlbedoLUT;
		
		TextureHandleAndView m_dirAlbedoSheen;

		TextureHandleAndView m_ssDirAlbedoTranslucentToDenserLUT;
		TextureHandleAndView m_ssDirAlbedoTranslucentToLighterLUT;
		TextureHandleAndView m_ssAvgAlbedoTranslucentToDenserLUT;
		TextureHandleAndView m_ssAvgAlbedoTranslucentToLighterLUT;

		CRenderer* m_renderer;
	};
	

	
}