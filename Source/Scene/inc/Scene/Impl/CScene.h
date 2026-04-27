#pragma once 

#include <Scene/Scene.h>
#include <Common/RCObjectPtr.h>
#include <vector>
#include <unordered_set>
namespace YAPT
{
	class Camera;
	class CRenderableObject;
	class CScene : public Scene
	{
	public:
		CScene();
		virtual bool initialize(Renderer* renderer);
		virtual void update(const SceneUpdateParameters& update) final;
		virtual Camera* getMainCamera() final;
		virtual RenderableObject* createRenderableObject() final;
		virtual SceneObject* createSceneObject() final;

		virtual void addSceneListener(SceneListener* l) final;
		virtual void removeSceneListener(SceneListener* l) final;

		void needsRefresh(CRenderableObject* model);

	protected:
		Renderer* m_renderer;
		RCObjectPtr<Camera> m_defaultCamera;
		std::vector<CRenderableObject*> m_dirtyRenderables;
		std::unordered_set<SceneListener*> m_listeners;

		virtual ~CScene();
	};
}