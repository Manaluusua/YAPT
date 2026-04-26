#pragma once
#include"CommonDefines.h"
#include <Common/RCObject.h>

namespace YAPT
{
	class Renderer;
	class Scene;
	class Camera;
	class RenderableObject;

	SCENE_MODULE_INTERFACE Scene* createScene();
	SCENE_MODULE_INTERFACE void destroyScene(Scene* scene);

	struct SceneUpdateParameters
	{
		double overallElapsedTimeInSeconds;
		float timeSinceLastFrameInSeconds;
	};

	class SceneListener
	{
	public:
		virtual void beforeUpdate(const SceneUpdateParameters& update) = 0;
	};

	class Scene
	{
	public:
		virtual bool initialize(Renderer* renderer) = 0;
		virtual void update(const SceneUpdateParameters& update) = 0;
		virtual Camera* getMainCamera() = 0;

		virtual RenderableObject* createRenderableObject() = 0;

		virtual void addSceneListener(SceneListener* l) = 0;
		virtual void removeSceneListener(SceneListener* l) = 0;

	protected:
		virtual ~Scene() {};

		SCENE_MODULE_INTERFACE friend void destroyScene(Scene* scene);

	};



}
