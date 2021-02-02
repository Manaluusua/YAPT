#pragma once

#include "CommonDefines.h"
#include <Renderer/WindowSurfaceDefinition.h>

namespace YAPT
{
	class Gui;
	class Renderer;
	class Scene;

	GUI_MODULE_INTERFACE Gui* createGui();
	GUI_MODULE_INTERFACE void destroyGui(Gui* window);

	class Gui
	{
	public:
		GUI_MODULE_INTERFACE virtual bool initialize(int argc, char** argv, size_t windowWidth, size_t windowHeight) = 0;
		GUI_MODULE_INTERFACE virtual int execute(Scene* scene, Renderer* renderer) = 0;
		GUI_MODULE_INTERFACE virtual YaptRenderSurfaceHandle getRenderSurfaceHandle() = 0;
	protected:
		GUI_MODULE_INTERFACE virtual ~Gui() {};


		GUI_MODULE_INTERFACE friend void destroyGui(Gui* window);
	};


}
