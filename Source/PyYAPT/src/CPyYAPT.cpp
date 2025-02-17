
#include <PyYAPT/Impl/CPyYAPT.h>
#include <Python.h>
namespace YAPT
{
	PyYAPT* createPythonModule()
	{
		return new CPyYAPT();
	}

	void destroyPythonModule(PyYAPT* m)
	{
		delete m;
	}

	
	CPyYAPT::CPyYAPT()
	{
		Py_Initialize();
	}
	CPyYAPT::~CPyYAPT()
	{
		Py_Finalize();
	}
	
	void CPyYAPT::executeFile(const char* filePath)
	{
		FILE* fileHandle;
		fileHandle = fopen(filePath, "rb");
		PyRun_SimpleFile(fileHandle, filePath);
		fclose(fileHandle);
	}


}