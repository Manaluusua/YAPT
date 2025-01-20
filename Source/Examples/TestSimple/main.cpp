//Entry point

#include <Renderer/Renderer.h>
#include <Gui/Gui.h>
#include <scene/Scene.h>
#include <AssetLoader/AssetLoader.h>
#include "TestScene.h"

#include <assert.h>

int main(int argc, char **argv)
{
#if defined(_DEBUG)
	for (int i = 0; i < argc; ++i)
	{
		if (strcmp(argv[i], "-debugHaltOnStartup") == 0)
		{
			while (!::IsDebuggerPresent())
			{
				::Sleep(100);
			}
		}
	}
#endif

	YAPT::Renderer* renderer = YAPT::createRenderer();
	YAPT::Gui* gui = YAPT::createGui();
	YAPT::Scene* scene = YAPT::createScene();

	bool success = gui->initialize(argc, argv, 1920, 1080);
	assert(success);

	YAPT::RendererInitializeConfig rendererConfig;
	rendererConfig.cache = YAPT::getDefaultRendererCacheProvider();
	rendererConfig.renderSurfaceHandle = gui->getRenderSurfaceHandle();
	success = renderer->initialize(rendererConfig);
	assert(success);
	success = scene->initialize(renderer);
	assert(success);
	

	TestScene* testScene = new TestScene(renderer, gui, scene);
	int res = gui->execute(scene, renderer);
	assert( res == 0);
	
	delete testScene;

	YAPT::destroyGui(gui);
	YAPT::destroyScene(scene);
	YAPT::destroyRenderer(renderer);
	return 0;
}
