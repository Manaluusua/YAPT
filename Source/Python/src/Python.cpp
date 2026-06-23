
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
		FILE* fileHandle = fopen(filePath, "rb");

		if (fileHandle)
		{
			PyObject* mainModule = PyImport_AddModule("__main__");
			PyObject* globals = PyModule_GetDict(mainModule);

			PyObject* pyFile = PyUnicode_FromString(filePath);
			PyDict_SetItemString(globals, "__file__", pyFile);
			Py_DECREF(pyFile);

			PyRun_FileExFlags(
				fileHandle,
				filePath,
				Py_file_input,
				globals,
				globals,
				1,      // close file when done
				nullptr // flags
			);
		}
	}


}