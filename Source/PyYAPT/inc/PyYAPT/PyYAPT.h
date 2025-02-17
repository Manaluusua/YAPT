#pragma once
#include"CommonDefines.h"
#include <Common/RCObject.h>


namespace YAPT
{
	class PyYAPT;
	PYTHON_MODULE_INTERFACE PyYAPT* createPythonModule();
	PYTHON_MODULE_INTERFACE void destroyPythonModule(PyYAPT* m);

	class PyYAPT
	{
	public:
		virtual void executeFile(const char* filePath) = 0;
	protected:
		virtual ~PyYAPT() {};
		PYTHON_MODULE_INTERFACE friend void destroyPythonModule(PyYAPT* renderer);
	};


}
