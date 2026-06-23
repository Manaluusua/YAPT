//Entry point

//#include <Renderer/Renderer.h>
//#include <Gui/Gui.h>
//#include <Scene/Scene.h>
//#include <AssetLoader/AssetLoader.h>
#include <assert.h>
#include <Python/Python.h>
#include <string>
#include <windows.h>
#include <array>
#include <Common/Logger.h>
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

	//should take from config? for now just hardcoded paths
	std::array paths =
	{
		"../../Source/Python/pythonScripts/default_bootstrap.py",
		"pythonScripts/default_bootstrap.py"
	};
	size_t validPathIndex = 0;
	for (; validPathIndex < paths.size(); ++validPathIndex)
	{
		FILE* fileHandle;
		fileHandle = fopen(paths[validPathIndex], "rb");
		bool success = fileHandle != nullptr;
		if (success)
		{
			fclose(fileHandle);
			break;
		}
		
	}
	
	if (validPathIndex < paths.size())
	{
		const char* pythonEntryFile = paths[validPathIndex];
		YAPT::Python* pythonModule = YAPT::createPythonModule();
		pythonModule->executeFile(pythonEntryFile);
		YAPT::destroyPythonModule(pythonModule);
	}
	else
	{
		YAPT_LOG_FATAL_ERROR("Unable to find python scripts");
	}
	
	return 0;
}
