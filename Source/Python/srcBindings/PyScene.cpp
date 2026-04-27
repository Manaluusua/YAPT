#pragma once
#include <PyScene.h>
#include <Scene/Scene.h>
#include <PyRenderer.h>
#include <PyCamera.h>
#include <PyRenderObject.h>

namespace YAPT
{
	DEFINE_BINDING_CLASS(PyScene)
	PyScene::PyScene()
		:m_scene(nullptr)
	{

	}
	PyScene::~PyScene()
	{
		shutdown();
	}

	void PyScene::init(PyRenderer* renderer)
	{
		if (m_scene)
		{
			shutdown();
		}
		m_scene = createScene();
		m_scene->initialize(renderer->getRenderer());
		m_elapsedTime = 0;
	}
	void PyScene::shutdown()
	{
		if (m_scene)
		{
			destroyScene(m_scene);
		}
		m_scene = nullptr;
	}

	void PyScene::update(float deltaTime)
	{
		m_elapsedTime += deltaTime;
		SceneUpdateParameters params
		{
			m_elapsedTime,
			deltaTime
		};
		m_scene->update(params);
	}

	PyRenderObject* PyScene::createRenderObject(const char* name)
	{
		RenderableObject* obj = m_scene->createRenderableObject();
		PyRenderObject* pyObj = new PyRenderObject(name, obj);
		obj->Release();
		return pyObj;
	}

	PySceneObject* PyScene::createSceneObject(const char* name)
	{
		SceneObject* obj = m_scene->createSceneObject();
		PySceneObject* pyObj = new PySceneObject(name, obj);
		obj->Release();
		return pyObj;
	}

	PyCamera* PyScene::getMainCamera()
	{
		return new PyCamera(m_scene->getMainCamera());
	}

	BINDING_FUNC(PyScene, m)
	{
		pybind11::class_<PyScene>(m, "Scene")
			.def(pybind11::init<>())
			.def("init", &PyScene::init)
			.def("shutdown", &PyScene::shutdown)
			.def("update", &PyScene::update)
			.def("createRenderObject", &PyScene::createRenderObject)
			.def("getMainCamera", &PyScene::getMainCamera);
	}
}