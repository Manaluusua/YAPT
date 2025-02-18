
#include <Python/Impl/CPython.h>
#include <pybind11/pybind11.h>
namespace YAPT
{
	Python* createPythonModule()
	{
		return new CPython();
	}

	void destroyPythonModule(Python* m)
	{
		delete m;
	}

	
	CPython::CPython()
	{
		Py_Initialize();
	}
	CPython::~CPython()
	{
		Py_Finalize();
	}
	
	void CPython::executeFile(const char* filePath)
	{

		FILE* fileHandle;
		fileHandle = fopen(filePath, "rb");
		PyRun_SimpleFile(fileHandle, filePath);
		fclose(fileHandle);
	}


}