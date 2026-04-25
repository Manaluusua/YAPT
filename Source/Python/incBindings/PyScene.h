#pragma once
#include <PyBindingsCommon.h>

namespace YAPT
{
	class PyRenderer;
	class PyCamera;
	class Scene;
	class PyRenderObject;
	class PySceneObject;
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
		
		PyCamera* getMainCamera();

	private:
		Scene* m_scene;
		double m_elapsedTime;;
	};
}