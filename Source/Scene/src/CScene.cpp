#include <Scene/Impl/CScene.h>
#include <Scene/Camera.h>
#include <Scene/Impl/CModel.h>
#include <Renderer/Renderer.h>

namespace YAPT
{
	Scene* createScene()
	{
		return new CScene();
	}
	void destroyScene(Scene* scene)
	{
		delete scene;
	}

	CScene::CScene()
		:m_defaultCamera(new Camera())
	{
		m_dirtyModels.reserve(256);
	}
	CScene::~CScene()
	{

	}

	bool CScene::initialize(Renderer* renderer)
	{
		m_renderer = renderer;
		m_renderer->prepare();
		return true;
	}

	void CScene::update(const SceneUpdateParameters& update)
	{
		for (SceneListener* l : m_listeners)
		{
			l->beforeUpdate(update);
		}


		for (CModel* model : m_dirtyModels)
		{
			model->refresh();
		}

		m_dirtyModels.clear();

		Camera* cam = getMainCamera();
		RenderParameters renderParams;
		renderParams.view = cam->getViewMatrix();
		renderParams.projection = cam->getProjectionMatrix();
		renderParams.nearFar = vec2(cam->getNearPlane(), cam->getFarPlance());
		renderParams.frameDeltaInSeconds = update.timeSinceLastFrameInSeconds;
		m_renderer->render(renderParams);

		//this should be moved to where an actual update starts. For now we prepare right after render call
		m_renderer->prepare();
	}

	Camera* CScene::getMainCamera()
	{
		return m_defaultCamera.get();
	}

	Model* CScene::createModel()
	{
		return new CModel(m_renderer, this);
	}

	void CScene::addSceneListener(SceneListener* l)
	{
		m_listeners.insert(l);
	}
	void CScene::removeSceneListener(SceneListener* l)
	{
		m_listeners.erase(l);
	}

	void CScene::needsRefresh(CModel* model)
	{
		m_dirtyModels.push_back(model);
	}

}