#include <PyRendererVar.h>

#include <PyTexture.h>
#include <PyBuffer.h>

#include <pybind11/stl.h>

namespace YAPT
{
	DEFINE_BINDING_CLASS(PyRendererVar);

    template<typename VecType>
    std::vector<typename VecType::value_type> asVector(VecType v)
    {
        std::vector<typename VecType::value_type> vec;
        vec.reserve(v.length());
        for (size_t i = 0; i < v.length(); ++i)
        {
            vec.push_back(glm::value_ptr(v)[i]);
        }
        return vec;
        
    }


	PyRendererVar::PyRendererVar(RendererVariable* var)
        :m_rendererVar(var)
	{

	}
	PyRendererVar::~PyRendererVar()
	{

	}

    void PyRendererVar::setFromFloatArray(std::vector<float> floats)
    {
        switch (m_rendererVar->getType())
        {
        case RendererVariableType::FLOAT:
            if (floats.size() > 0)
            {
                m_rendererVar->set(floats[0]);
            }
            break;
        case RendererVariableType::VEC2:
            if (floats.size() > 1)
            {
                m_rendererVar->set(vec2p(floats[0], floats[1]));
            }
            break;
        case RendererVariableType::VEC3:
            if (floats.size() > 2)
            {
                m_rendererVar->set(vec3p(floats[0], floats[1], floats[2]));
            }
            break;
        case RendererVariableType::VEC4:
            if (floats.size() > 3)
            {
                m_rendererVar->set(vec4p(floats[0], floats[1], floats[2], floats[3]));
            }
            break;

        default:
            //report error? silent fail for now
            break;
        }
    }
    void PyRendererVar::setFromIntArray(std::vector<int32_t> ints)
    {
        switch (m_rendererVar->getType())
        {
        case RendererVariableType::INT:
            if (ints.size() > 0)
            {
                m_rendererVar->set(ints[0]);
            }
            break;
        case RendererVariableType::IVEC2:
            if (ints.size() > 1)
            {
                m_rendererVar->set(ivec2p(ints[0], ints[1]));
            }
            break;
        case RendererVariableType::IVEC3:
            if (ints.size() > 2)
            {
                m_rendererVar->set(ivec3p(ints[0], ints[1], ints[2]));
            }
            break;
        case RendererVariableType::IVEC4:
            if (ints.size() > 3)
            {
                m_rendererVar->set(ivec4p(ints[0], ints[1], ints[2], ints[3]));
            }
            break;

        default:
            //report error? silent fail for now
            break;
        }

    }

    void PyRendererVar::setTexture(const PyTexture& val)
    {
        m_rendererVar->set(val.getTexture());
    }
    void PyRendererVar::setBuffer(const PyBuffer& val)
    {
        m_rendererVar->set(val.getBuffer());
    }

    std::vector<float> PyRendererVar::getAsFloatArray()
    {
        switch (m_rendererVar->getType())
        {
        case RendererVariableType::FLOAT:
        {
            float val;
            m_rendererVar->get(val);
            return std::vector{ val };
            break;
        }
        case RendererVariableType::VEC2:
        {
            vec2p val;
            m_rendererVar->get(val);
            return asVector(val);
            break;
        }
        case RendererVariableType::VEC3:
        {
            vec3p val;
            m_rendererVar->get(val);
            return asVector(val);
            break;
        }
        case RendererVariableType::VEC4:
        {
            vec4p val;
            m_rendererVar->get(val);
            return asVector(val);
            break;
        }
        default:
            //report error? silent fail for now
            break;
        }
        return std::vector<float>();
    }
    std::vector<int32_t> PyRendererVar::getAsIntArray()
    {
        switch (m_rendererVar->getType())
        {
        case RendererVariableType::INT:
        {
            int32_t val;
            m_rendererVar->get(val);
            return std::vector{ val };
            break;
        }
        case RendererVariableType::IVEC2:
        {
            ivec2p val;
            m_rendererVar->get(val);
            return asVector(val);
            break;
        }
        case RendererVariableType::IVEC3:
        {
            ivec3p val;
            m_rendererVar->get(val);
            return asVector(val);
            break;
        }
        case RendererVariableType::IVEC4:
        {
            ivec4p val;
            m_rendererVar->get(val);
            return asVector(val);
            break;
        }
        default:
            //report error? silent fail for now
            break;
        }
        return std::vector<int32_t>();
    }

    PyTexture* PyRendererVar::getTexture()
    {
        RCPtr<Texture> tex;
        m_rendererVar->get(tex);
        if (tex != nullptr)
        {
            return new PyTexture(tex);
        }
        return nullptr;
    }
    PyBuffer* PyRendererVar::getBuffer()
    {
        RCPtr<Buffer> b;
        m_rendererVar->get(b);
        if (b != nullptr)
        {
            return new PyBuffer(b);
        }
        return nullptr;
    }

    std::array<std::vector<float>, 2> PyRendererVar::getLimitsFloat()
    {
        std::array<std::vector<float>, 2> limits;
        switch (m_rendererVar->getType())
        {
        case RendererVariableType::FLOAT:
        {
            float val0, val1;
            m_rendererVar->getLimits(val0, val1);
            limits[0] = std::vector{ val0 };
            limits[1] = std::vector{ val1 };
            break;
        }
        case RendererVariableType::IVEC2:
        {
            vec2p val0, val1;
            m_rendererVar->getLimits(val0, val1);
            limits[0] = asVector(val0);
            limits[1] = asVector(val1);
            break;
        }
        case RendererVariableType::IVEC3:
        {
            vec3p val0, val1;
            m_rendererVar->getLimits(val0, val1);
            limits[0] = asVector(val0);
            limits[1] = asVector(val1);
            break;
        }
        case RendererVariableType::IVEC4:
        {
            vec4p val0, val1;
            m_rendererVar->getLimits(val0, val1);
            limits[0] = asVector(val0);
            limits[1] = asVector(val1);
            break;
        }
        default:
            //report error? silent fail for now
            break;
        }
        return limits;
    }
    std::array<std::vector<int>, 2> PyRendererVar::getLimitsInt()
    {
        std::array<std::vector<int>, 2> limits;
        switch (m_rendererVar->getType())
        {
        case RendererVariableType::INT:
        {
            int32_t val0, val1;
            m_rendererVar->getLimits(val0, val1);
            limits[0] = std::vector{ val0 };
            limits[1] = std::vector{ val1 };
            break;
        }
        case RendererVariableType::IVEC2:
        {
            ivec2p val0, val1;
            m_rendererVar->getLimits(val0, val1);
            limits[0] = asVector(val0);
            limits[1] = asVector(val1);
            break;
        }
        case RendererVariableType::IVEC3:
        {
            ivec3p val0, val1;
            m_rendererVar->getLimits(val0, val1);
            limits[0] = asVector(val0);
            limits[1] = asVector(val1);
            break;
        }
        case RendererVariableType::IVEC4:
        {
            ivec4p val0, val1;
            m_rendererVar->getLimits(val0, val1);
            limits[0] = asVector(val0);
            limits[1] = asVector(val1);
            break;
        }
        default:
            //report error? silent fail for now
            break;
        }
        return limits;
    }

    RendererVariableType PyRendererVar::getType() const { return m_rendererVar->getType(); }


    BINDING_FUNC(PyRendererVar, m)
    {


        pybind11::enum_<RendererVariableType>(m, "RendererVariableType")
            .value("FLOAT", RendererVariableType::FLOAT)
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
            .def("setFromFloatArray", &PyRendererVar::setFromFloatArray)
            .def("setFromIntArray", &PyRendererVar::setFromIntArray)
            .def("setTexture", &PyRendererVar::setTexture)
            .def("setBuffer", &PyRendererVar::setBuffer)

            .def("getAsFloatArray", &PyRendererVar::getAsFloatArray)
            .def("getAsIntArray", &PyRendererVar::getAsIntArray)
            .def("getTexture", &PyRendererVar::getTexture)
            .def("getBuffer", &PyRendererVar::getBuffer)

            .def("getLimitsFloat", &PyRendererVar::getLimitsFloat)
            .def("getLimitsInt", &PyRendererVar::getLimitsInt)

            .def("getType", &PyRendererVar::getType);


    }
}