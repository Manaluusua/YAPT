#pragma once 

#include <Scene/Scene.h>
#include <Common/RCObjectPtr.h>
#include <vector>
#include <unordered_set>
namespace YAPT
{
	class Camera;
	class CModel;
	class CScene : public Scene
	{
	public:
		CScene();
		virtual bool initialize(Renderer* renderer);
		virtual void update(const SceneUpdateParameters& update) final;
		virtual Camera* getMainCamera() final;
		virtual Model* createModel() final;

		virtual void addSceneListener(SceneListener* l) final;
		virtual void removeSceneListener(SceneListener* l) final;

		void needsRefresh(CModel* model);

	protected:
		Renderer* m_renderer;
		RCObjectPtr<Camera> m_defaultCamera;
		std::vector<CModel*> m_dirtyModels;
		std::unordered_set<SceneListener*> m_listeners;

		virtual ~CScene();
	};
}