#include <PyBindingsCommon.h>
#include <vector>

static std::vector<RegisterPythonClassFunc> s_registerPythonClassFuncs;


void addPythonRegisterFunc(RegisterPythonClassFunc f)
{
    s_registerPythonClassFuncs.push_back(f);
}
  
PYBIND11_MODULE(py_yapt, m)
{
    m.doc() = "YAPT Python bindings module"; 

	for each (auto registerFunc in s_registerPythonClassFuncs)
	{
		registerFunc(m);
	}

}