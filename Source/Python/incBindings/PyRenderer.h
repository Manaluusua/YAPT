#pragma once
#include <PyBindingsCommon.h>
#include <PyRendererVar.h>
#include <Windows.h>
#include <PyMesh.h>
namespace YAPT
{
	class Renderer;
	class PyRendererVar;
	class PyRendererCache;
	class ResourceAllocationPool;
	class PyTexture;
	class PyBuffer;
	class PyMaterial;
	
	class PyRenderer
	{
	public:
		DECLARE_BINDING_CLASS(PyRenderer);

		PyRenderer();
		~PyRenderer();

		void init(PyRendererCache* cache, uintptr_t windowHandle, bool enableGPUTrace);
		void shutdown();
		
		void setRenderOutputToSurface(size_t width, size_t height, uintptr_t windowHandle);
		void resetRenderOutput();

		PyTexture* createTexture(const char* name, ResourceDimension dimensions, ResourceFormat format, ResourceUsage resourceUsage, uint32_t width, uint32_t height, uint32_t mips = 1, uint32_t depthOrSlices = 1);
		PyBuffer* createBuffer(const char* name, ResourceUsage resourceUsage, size_t size);

		PyMaterial* createMaterial(const char* name);
		PyMesh* createMesh(const char* name, const std::vector<PyVertexBufferLayout>& layouts, size_t vertexCount);

		PyRendererVar getRendererVariable(const char* varName);
		std::vector<const char*> getAllRendererVariableNames();

		inline Renderer* getRenderer() { return m_renderer; }
	private:
		Renderer* m_renderer;

		
	};
}

