#include "Gui/Gui.h"
#include "Gui/GuiController.h"

namespace YAPT
{
	GUI_MODULE_INTERFACE Gui* createGui()
	{
		return new GuiController();
	}
	GUI_MODULE_INTERFACE void destroyGui(Gui* gui)
	{
		delete gui;
	}
	
}