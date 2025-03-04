#pragma once
#include"CommonDefines.h"
#include <Gfx/WindowSurfaceDefinition.h>
#include <Renderer/Material.h>
#include <Renderer/Mesh.h>
#include <Math/Math.h>
#include <Renderer/RenderObject.h>
#include <Renderer/RendererConfiguration.h>
#include <Renderer/RendererCacheProvider.h>

namespace YAPT
{
	class Renderer;
	RENDERER_MODULE_INTERFACE Renderer* createRenderer();
	RENDERER_MODULE_INTERFACE void destroyRenderer(Renderer* renderer);


	
	struct RendererInitializeConfig
	{
		RendererCacheProvider* cache = nullptr;
		YaptRenderSurfaceHandle renderSurfaceHandle;
		bool enableGPUDebugCapture;
	};

	struct RenderParameters
	{
		mat4 view;
		mat4 projection;
		vec2 nearFar;
		float frameDeltaInSeconds;

	};
	
	class Renderer
	{
	public:

		virtual bool initialize(const RendererInitializeConfig& config) = 0;

		virtual bool setRenderOutputToSurface(const WindowSurfaceDefinition& windowSurface) = 0;
		virtual void resetRenderOutput() = 0;

		virtual void prepare() = 0;
		virtual void render(const RenderParameters& renderParams) = 0;

		virtual Texture* createTexture(const char* name, ResourceDimension dimensions, ResourceFormat format, ResourceUsage resourceUsage, uint32_t width, uint32_t height, uint32_t mips = 1, uint32_t depthOrSlices = 1) = 0;
		virtual Buffer* createBuffer(const char* name, ResourceUsage resourceUsage, size_t size) = 0;

		/**
			Creates a mesh with given layouts. Buffers bound to the mesh are assumed to be in the same order (ie. buffer bound to index 0 has the layout of the first layout in the list passed here).
		*/
		virtual Mesh* createMesh(const VertexBufferLayout* layouts, size_t numberOfVertexBufferLayouts, size_t vertexCount) = 0;
		virtual Material* createMaterial() = 0;
		virtual RenderObject* createRenderObject() = 0;

		virtual RendererConfiguration* getRendererConfiguration() = 0;

	protected:
		virtual ~Renderer() {};

		RENDERER_MODULE_INTERFACE friend void destroyRenderer(Renderer* renderer);
	};


	
}
