#pragma once
#include"CommonDefines.h"
#include <Common/RCObject.h>


namespace YAPT
{
	class Python;
	PYTHON_MODULE_INTERFACE Python* createPythonModule();
	PYTHON_MODULE_INTERFACE void destroyPythonModule(Python* m);

	class Python
	{
	public:
		virtual void executeFile(const char* filePath) = 0;
	protected:
		virtual ~Python() {};
		PYTHON_MODULE_INTERFACE friend void destroyPythonModule(Python* renderer);
	};


}
