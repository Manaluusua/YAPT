#pragma once
#include <PyBindingsCommon.h>

namespace YAPT
{
	class PyRenderer;
	class Scene;
	class PyScene
	{
	public:
		DECLARE_BINDING_CLASS(PyScene);
		PyScene();
		~PyScene();
		void init(PyRenderer* renderer);
		void shutdown();

		void update(float deltaTime);

	private:
		Scene* m_scene;
		double m_elapsedTime;;
	};
}