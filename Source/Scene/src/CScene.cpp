#include <Scene/Impl/CScene.h>
#include <Scene/Camera.h>
#include <Scene/Impl/CRenderableObject.h>
#include <Scene/Impl/CLightObject.h>
#include <Renderer/Renderer.h>
#include <algorithm>

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
		m_dirtyRenderables.reserve(256);
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


		for (CRenderableObject* model : m_dirtyRenderables)
		{
			model->refresh();
		}

		m_dirtyRenderables.clear();

		for (CLightObject* light : m_dirtyLights)
		{
			light->refresh();
		}
		m_dirtyLights.clear();

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

	RenderableObject* CScene::createRenderableObject()
	{
		return new CRenderableObject(m_renderer, this);
	}
	SceneObject* CScene::createSceneObject()
	{
		return new SceneObject();
	}
	LightObject* CScene::createLightObject(LightType type)
	{
		return new CLightObject(m_renderer, this, type);
	}

	void CScene::addSceneListener(SceneListener* l)
	{
		m_listeners.insert(l);
	}
	void CScene::removeSceneListener(SceneListener* l)
	{
		m_listeners.erase(l);
	}

	void CScene::needsRefresh(CRenderableObject* model)
	{
		m_dirtyRenderables.push_back(model);
	}

	void CScene::needsRefresh(CLightObject* light)
	{
		m_dirtyLights.push_back(light);
	}

	void CScene::cancelRefresh(CLightObject* light)
	{
		m_dirtyLights.erase(std::remove(m_dirtyLights.begin(), m_dirtyLights.end(), light), m_dirtyLights.end());
	}

}