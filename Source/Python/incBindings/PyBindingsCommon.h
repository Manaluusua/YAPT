#pragma once

#include <pybind11/pybind11.h>

typedef void (*RegisterPythonClassFunc)(pybind11::module& m);
void addPythonRegisterFunc(RegisterPythonClassFunc, int typeID, int baseTypeID);
int getNextRegisterTypeID();

struct REGISTER_TYPE_DUMMY
{
	static int getRegisteredTypeId()
	{
		return -1;
	}
};

#define DECLARE_BINDING_CLASS(T)							\
	static void registerPythonBinding(pybind11::module& m); \
	static int getRegisteredTypeId(); 
#define DEFINE_BINDING_CLASS_WITH_BASE(T, Y)					\
	static int s_registeredTypeID##T = -1;						\
	int T::getRegisteredTypeId()								\
	{															\
		if(s_registeredTypeID##T == -1)							\
		{														\
			s_registeredTypeID##T = getNextRegisterTypeID();	\
		}														\
		return s_registeredTypeID##T;							\
	}															\
	struct registerPythonClassHelper##T							\
	{															\
		registerPythonClassHelper##T()							\
		{														\
			addPythonRegisterFunc(								\
				&T::registerPythonBinding,						\
				T::getRegisteredTypeId(),						\
				Y::getRegisteredTypeId());					\
		}														\
	};															\
	static registerPythonClassHelper##T s_registerType##T_DUMMY; 
#define DEFINE_BINDING_CLASS(T) DEFINE_BINDING_CLASS_WITH_BASE(T, REGISTER_TYPE_DUMMY)
#define BINDING_FUNC(T, PARAM_NAME) void T::registerPythonBinding(pybind11::module& PARAM_NAME)