#include <PyRendererVar.h>

#include <PyTexture.h>
#include <PyBuffer.h>

namespace YAPT
{
	DEFINE_BINDING_CLASS(PyRendererVar);

	PyRendererVar::PyRendererVar(RendererVariable* var)
        :m_rendererVar(var)
	{

	}
	PyRendererVar::~PyRendererVar()
	{

	}

    void PyRendererVar::set(const float& val) { m_rendererVar->set(val); }
    void PyRendererVar::set(const uint32_t& val) { m_rendererVar->set(val); }
    void PyRendererVar::set(const int32_t& val) { m_rendererVar->set(val); }

    void PyRendererVar::set(const vec2p& val) { m_rendererVar->set(val); }
    void PyRendererVar::set(const vec3p& val) { m_rendererVar->set(val); }
    void PyRendererVar::set(const vec4p& val) { m_rendererVar->set(val); }

    void PyRendererVar::set(const ivec2p& val) { m_rendererVar->set(val); }
    void PyRendererVar::set(const ivec3p& val) { m_rendererVar->set(val); }
    void PyRendererVar::set(const ivec4p& val) { m_rendererVar->set(val); }

    void PyRendererVar::set(const PyTexture& val) { m_rendererVar->set(val.getTexture()); }
    void PyRendererVar::set(const PyBuffer& val) { m_rendererVar->set(val.getBuffer()); }

    bool PyRendererVar::get(float& val) { return m_rendererVar->get(val); }
    bool PyRendererVar::get(uint32_t& val) { return m_rendererVar->get(val); }
    bool PyRendererVar::get(int32_t& val) { return m_rendererVar->get(val); }

    bool PyRendererVar::get(vec2p& val) { return m_rendererVar->get(val); }
    bool PyRendererVar::get(vec3p& val) { return m_rendererVar->get(val); }
    bool PyRendererVar::get(vec4p& val) { return m_rendererVar->get(val); }

    bool PyRendererVar::get(ivec2p& val) { return m_rendererVar->get(val); }
    bool PyRendererVar::get(ivec3p& val) { return m_rendererVar->get(val); }
    bool PyRendererVar::get(ivec4p& val) { return m_rendererVar->get(val); }

    bool PyRendererVar::get(RCPtr<Texture>& val) { return m_rendererVar->get(val); }
    bool PyRendererVar::get(RCPtr<Buffer>& val) { return m_rendererVar->get(val); }

    bool PyRendererVar::getLimits(float& min, float& max) { return m_rendererVar->getLimits(min, max); }
    bool PyRendererVar::getLimits(uint32_t& min, uint32_t& max) { return m_rendererVar->getLimits(min, max); }
    bool PyRendererVar::getLimits(int32_t& min, int32_t& max) { return m_rendererVar->getLimits(min, max); }

    bool PyRendererVar::getLimits(vec2p& min, vec2p& max) { return m_rendererVar->getLimits(min, max); }
    bool PyRendererVar::getLimits(vec3p& min, vec3p& max) { return m_rendererVar->getLimits(min, max); }
    bool PyRendererVar::getLimits(vec4p& min, vec4p& max) { return m_rendererVar->getLimits(min, max); }

    bool PyRendererVar::getLimits(ivec2p& min, ivec2p& max) { return m_rendererVar->getLimits(min, max); }
    bool PyRendererVar::getLimits(ivec3p& min, ivec3p& max) { return m_rendererVar->getLimits(min, max); }
    bool PyRendererVar::getLimits(ivec4p& min, ivec4p& max) { return m_rendererVar->getLimits(min, max); }

    RendererVariableType PyRendererVar::getType() const { return m_rendererVar->getType(); }


    BINDING_FUNC(PyRendererVar, m)
    {


        pybind11::enum_<RendererVariableType>(m, "RendererVariableType")
            .value("FLOAT", RendererVariableType::FLOAT)
            .value("UINT", RendererVariableType::UINT)
            .value("INT", RendererVariableType::INT)
            .value("VEC2", RendererVariableType::VEC2)
            .value("VEC3", RendererVariableType::VEC3)
            .value("VEC4", RendererVariableType::VEC4)
            .value("IVEC2", RendererVariableType::IVEC2)
            .value("IVEC3", RendererVariableType::IVEC3)
            .value("IVEC4", RendererVariableType::IVEC4)
            .value("TEXTURE", RendererVariableType::TEXTURE)
            .value("BUFFER", RendererVariableType::BUFFER);

        pybind11::class_<PyRendererVar>(m, "RendererVariable")
            .def("set", pybind11::overload_cast<const float&>(&PyRendererVar::set))
            .def("set", pybind11::overload_cast<const uint32_t&>(&PyRendererVar::set))
            .def("set", pybind11::overload_cast<const int32_t&>(&PyRendererVar::set))
            .def("set", pybind11::overload_cast<const vec2p&>(&PyRendererVar::set))
            .def("set", pybind11::overload_cast<const vec3p&>(&PyRendererVar::set))
            .def("set", pybind11::overload_cast<const vec4p&>(&PyRendererVar::set))
            .def("set", pybind11::overload_cast<const ivec2p&>(&PyRendererVar::set))
            .def("set", pybind11::overload_cast<const ivec3p&>(&PyRendererVar::set))
            .def("set", pybind11::overload_cast<const ivec4p&>(&PyRendererVar::set))
            .def("set", pybind11::overload_cast<const PyTexture&>(&PyRendererVar::set))
            .def("set", pybind11::overload_cast<const PyBuffer&>(&PyRendererVar::set))

            .def("get", [](PyRendererVar& self, float& val) { return self.get(val); })
            .def("get", [](PyRendererVar& self, uint32_t& val) { return self.get(val); })
            .def("get", [](PyRendererVar& self, int32_t& val) { return self.get(val); })
            .def("get", [](PyRendererVar& self, vec2p& val) { return self.get(val); })
            .def("get", [](PyRendererVar& self, vec3p& val) { return self.get(val); })
            .def("get", [](PyRendererVar& self, vec4p& val) { return self.get(val); })
            .def("get", [](PyRendererVar& self, ivec2p& val) { return self.get(val); })
            .def("get", [](PyRendererVar& self, ivec3p& val) { return self.get(val); })
            .def("get", [](PyRendererVar& self, ivec4p& val) { return self.get(val); })
            //.def("get", [](PyRendererVar& self, RCPtr<Texture>& val) { return self.get(val); })
            //.def("get", [](PyRendererVar& self, RCPtr<Buffer>& val) { return self.get(val); })

            .def("getLimits", [](PyRendererVar& self, float& min, float& max) { return self.getLimits(min, max); })
            .def("getLimits", [](PyRendererVar& self, uint32_t& min, uint32_t& max) { return self.getLimits(min, max); })
            .def("getLimits", [](PyRendererVar& self, int32_t& min, int32_t& max) { return self.getLimits(min, max); })
            .def("getLimits", [](PyRendererVar& self, vec2p& min, vec2p& max) { return self.getLimits(min, max); })
            .def("getLimits", [](PyRendererVar& self, vec3p& min, vec3p& max) { return self.getLimits(min, max); })
            .def("getLimits", [](PyRendererVar& self, vec4p& min, vec4p& max) { return self.getLimits(min, max); })
            .def("getLimits", [](PyRendererVar& self, ivec2p& min, ivec2p& max) { return self.getLimits(min, max); })
            .def("getLimits", [](PyRendererVar& self, ivec3p& min, ivec3p& max) { return self.getLimits(min, max); })
            .def("getLimits", [](PyRendererVar& self, ivec4p& min, ivec4p& max) { return self.getLimits(min, max); })

            .def("getType", &PyRendererVar::getType);


    }
}