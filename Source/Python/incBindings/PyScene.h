#pragma once
#include <PyBindingsCommon.h>
#include <Renderer/Light.h>

namespace YAPT
{
	class PyRenderer;
	class PyCamera;
	class Scene;
	class PyRenderObject;
	class PySceneObject;
	class PyLight;
	class PyScene
	{
	public:
		DECLARE_BINDING_CLASS(PyScene);
		PyScene();
		~PyScene();
		void init(PyRenderer* renderer);
		void shutdown();

		void update(float deltaTime);

		PyRenderObject* createRenderObject(const char* name);
		PySceneObject* createSceneObject(const char* name);
		PyLight* createLight(const char* name, LightType type);
		
		PyCamera* getMainCamera();

	private:
		Scene* m_scene;
		double m_elapsedTime;;
	};
}