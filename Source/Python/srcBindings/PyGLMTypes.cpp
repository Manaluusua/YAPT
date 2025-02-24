#include <PyGlmTypes.h>
#include <pybind11/stl.h>
#include <pybind11/operators.h>
#include <pybind11/numpy.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <Math/Math.h>


namespace YAPT
{
	DEFINE_BINDING_CLASS(PyGLMTypes);
	namespace py = pybind11;

	// Helper function to bind glm::vec types
	template <typename VecType, size_t N>
	void bind_vec(py::module& m, const char* name)
	{
		auto c = py::class_<VecType>(m, name);
		c.def(py::init<>());  // Default constructor
		c.def(py::init<typename VecType::value_type>());  // Scalar constructor
		c.def(py::init([](std::array<typename VecType::value_type, N> values)
		{
			VecType v;
			for (size_t i = 0; i < N; ++i) glm::value_ptr(v)[i] = values[i];
			return v;
		}));
		c.def(py::self + py::self);
		c.def(py::self - py::self);
		c.def(py::self * py::self);
		c.def(py::self / py::self);
		c.def(py::self * typename VecType::value_type());
		c.def(py::self / typename VecType::value_type());
		c.def("__getitem__", [](const VecType& v, size_t i)
			{
				if (i >= N) throw py::index_error();
				return glm::value_ptr(v)[i];
			});
		c.def("__setitem__", [](VecType& v, size_t i, typename VecType::value_type val)
			{
				if (i >= N) throw py::index_error();
				glm::value_ptr(v)[i] = val;
			});

		c.def("__repr__", [](const VecType& v)
			{
				std::string repr("vec( ");
				auto ptr = glm::value_ptr(v);
				for (size_t i = 0; i < N; ++i) 
				{ 
					repr += std::to_string(ptr[i]); 
					repr += " ";
				};
				repr += " )";
				return repr;
			});
	}


	BINDING_FUNC(PyGLMTypes, m)
	{
		// Bind vector types
		bind_vec<vec2p, 2>(m, "vec2");
		bind_vec<vec3p, 3>(m, "vec3");
		bind_vec<vec4p, 4>(m, "vec4");
		bind_vec<ivec2p, 2>(m, "ivec2");
		bind_vec<ivec3p, 3>(m, "ivec3");
		bind_vec<ivec4p, 4>(m, "ivec4");

	}
}