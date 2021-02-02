#ifndef EXAMPLES_TESTSCENE__H
#define EXAMPLES_TESTSCENE__H

#include "Renderer/Renderer.h"
#include "Gui/Gui.h"
#include "Scene/Scene.h"
#include <Common/RCObjectPtr.h>
#include <Renderer/Mesh.h>
#include <Scene/Model.h>

#include <vector>
class TestScene : public YAPT::SceneListener
{
public:
	TestScene(YAPT::Renderer* renderer, YAPT::Gui* gui, YAPT::Scene* scene);
	~TestScene();


	void beforeUpdate(const YAPT::SceneUpdateParameters& update);
private:

	void loadEnvMap();
	void loadMeshes();
	YAPT::RCObjectPtr<YAPT::Mesh> loadMesh(const char* path);

	void buildScene();

	void createCarPaintComparison();
	void createRoughnessComparison();
	void createMaterialComparisonScene();
	void createHeroShotScene();
	void createWhiteFurnaceTestScene();

	YAPT::RCObjectPtr<YAPT::Mesh> m_planeMesh;
	YAPT::RCObjectPtr<YAPT::Mesh> m_sphereMesh;
	YAPT::RCObjectPtr<YAPT::Mesh> m_torusMesh;
	YAPT::RCObjectPtr<YAPT::Mesh> m_bowlMesh;

	YAPT::Renderer* m_renderer;
	YAPT::Gui* m_gui;
	YAPT::Scene* m_scene;

	YAPT::RCObjectPtr<YAPT::Texture> m_skyCube;
	std::vector<YAPT::RCObjectPtr<YAPT::Model>> m_models;

};


#endif
