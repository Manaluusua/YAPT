#include <pybind11/pybind11.h>
#include <Renderer/Renderer.h>

PYBIND11_MODULE(PyYAPT, m)
{
    m.doc() = "YAPT Python bindings module"; 
    m.def("createRenderer", &YAPT::createRenderer);
    m.def("destroyRenderer", &YAPT::createRenderer); 

        
}