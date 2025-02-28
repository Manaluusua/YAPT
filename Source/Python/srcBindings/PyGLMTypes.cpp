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

	template<typename VecType, int N>
	struct BindComponentsHelper
	{
		void bind(py::class_<typename VecType>& c)
		{

		}
	};

	template<typename VecType>
	struct BindComponentsHelper<VecType, 4>
	{
		void bind(py::class_<typename VecType>& c)
		{
			c.def_readwrite("w", &VecType::w);
			BindComponentsHelper<typename VecType, 3>().bind(c);
		}
	};

	template<typename VecType>
	struct BindComponentsHelper<VecType, 3>
	{
		void bind(py::class_<typename VecType>& c)
		{
			c.def_readwrite("z", &VecType::z);
			BindComponentsHelper<typename VecType, 2>().bind(c);
		}
	};

	template<typename VecType>
	struct BindComponentsHelper<VecType, 2>
	{
		void bind(py::class_<typename VecType>& c)
		{
			c.def_readwrite("y", &VecType::y);
			BindComponentsHelper<typename VecType, 1>().bind(c);
		}
	};

	template<typename VecType>
	struct BindComponentsHelper<VecType, 1>
	{
		void bind(py::class_<typename VecType>& c)
		{
			c.def_readwrite("x", &VecType::x);
		}
	};



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

		BindComponentsHelper<typename VecType, N>().bind(c);

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

	template <typename MatType, size_t Rows, size_t Cols>
	void bind_mat(py::module& m, const char* name) {
		auto c = py::class_<MatType>(m, name);
		c.def(py::init<>()); 
		c.def(py::init([](std::array<std::array<MatType::value_type, Cols>, Rows> values)
			{
				MatType mat(1.0f); // Identity matrix
				for (size_t r = 0; r < Rows; ++r)
				{
					for (size_t c = 0; c < Cols; ++c)
					{
						mat[r][c] = values[r][c];
					}
				}
				return mat;
			}
		));
		c.def("__repr__", [](const MatType& m)
			{
				std::string repr = "<mat" + " [";
				for (size_t r = 0; r < Rows; ++r)
				{
					repr += "[";
					for (size_t c = 0; c < Cols; ++c)
					{
						repr += std::to_string(m[r][c]) + (c < Cols - 1 ? ", " : "");
					}
					repr += "]" + (r < Rows - 1 ? ", " : "");
				}
				return repr + "]>";
			});
		c.def("__getitem__", [](const MatType& m, size_t row)
			{
				if (row >= Rows) throw py::index_error();
				return py::array_t<typename MatType::value_type()>({ Cols }, { sizeof(typename MatType::value_type) }, glm::value_ptr(m[row]));
			});
		c.def("__setitem__", [](MatType& m, size_t row, py::array_t<typename MatType::value_type> values)
			{
				if (row >= Rows) throw py::index_error();
				auto buf = values.request();
				if (buf.size != Cols) throw py::value_error();
				typename MatType::value_type* ptr = static_cast<typename MatType::value_type*>(buf.ptr);
				for (size_t c = 0; c < Cols; ++c)
				{
					m[row][c] = ptr[c];
				}
				
			});
		c.def(py::self + py::self);
		c.def(py::self - py::self);
		c.def(py::self * py::self);
		c.def(py::self * typename MatType::value_type);
		c.def(py::self / typename MatType::value_type);
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