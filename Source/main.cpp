//Entry point

//#include <Renderer/Renderer.h>
//#include <Gui/Gui.h>
//#include <Scene/Scene.h>
//#include <AssetLoader/AssetLoader.h>
#include <assert.h>
#include <Python/Python.h>
#include <string>
#include <windows.h>
#if defined(_DEBUG)
#include <debugapi.h>
#endif

/*
int main(int argc, char **argv)
{
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

	int res = gui->execute(scene, renderer);
	assert(res == 0);
	
	YAPT::destroyGui(gui);
	YAPT::destroyScene(scene);
	YAPT::destroyRenderer(renderer);
	return 0;
}*/


int main(int argc, char** argv) 
{
	bool enableGPUCapture = false;
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
		else if (strcmp(argv[i], "-debugEnableGPUCapture") == 0)
		{
			enableGPUCapture = true;
		}
	}
#endif
	const char* pythonEntryFile = "pythonScripts/default_bootstrap.py";
	YAPT::Python* pythonModule = YAPT::createPythonModule();
	pythonModule->executeFile(pythonEntryFile);
	YAPT::destroyPythonModule(pythonModule);

	return 0;
}
