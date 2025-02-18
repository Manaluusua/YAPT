#pragma once

#include <pybind11/pybind11.h>

typedef void (*RegisterPythonClassFunc)(pybind11::module& m);
void addPythonRegisterFunc(RegisterPythonClassFunc);


#define DECLARE_BINDING_CLASS(T) static void registerPythonBinding(pybind11::module& m);
#define DEFINE_BINDING_CLASS(T) struct registerPythonClassHelper##T { registerPythonClassHelper##T() {addPythonRegisterFunc(&T::registerPythonBinding);}}; static registerPythonClassHelper##T s_registerType##T_DUMMY;
#define BINDING_FUNC(T, PARAM_NAME) void T::registerPythonBinding(pybind11::module& PARAM_NAME)