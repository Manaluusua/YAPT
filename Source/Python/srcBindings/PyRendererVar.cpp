#include <PyRendererVar.h>

#include <PyTexture.h>
#include <PyBuffer.h>

#include <pybind11/stl.h>

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

    void PyRendererVar::setFromFloatArray(std::vector<float> floats)
    {
        RenderVariableFloat* floatVar = m_rendererVar->as<RenderVariableFloat>();
        if (floatVar != nullptr && floatVar->getComponentCount() <= floats.size())
        {
            floatVar->set(floats.data());
        }
    }
    void PyRendererVar::setFromIntArray(std::vector<int32_t> ints)
    {
        RenderVariableInt* intVar = m_rendererVar->as<RenderVariableInt>();
        if (intVar != nullptr && intVar->getComponentCount() <= ints.size())
        {
            intVar->set(ints.data());
        }

    }

    void PyRendererVar::setTexture(const PyTexture& val)
    {
        RenderVariableTexture* var = m_rendererVar->as<RenderVariableTexture>();
        if (var != nullptr)
        {
            var->set(val.getTexture());
        }
    }
    void PyRendererVar::setBuffer(const PyBuffer& val)
    {
        RenderVariableBuffer* var = m_rendererVar->as<RenderVariableBuffer>();
        if (var != nullptr)
        {
            var->set(val.getBuffer());
        }
    }

    size_t PyRendererVar::getComponentCount() const
    {
        RenderVariableFloat* floatVar = m_rendererVar->as<RenderVariableFloat>();
        if (floatVar != nullptr)
        {
            return floatVar->getComponentCount();
        }
        RenderVariableInt* intVar = m_rendererVar->as<RenderVariableInt>();
        if (intVar != nullptr)
        {
            return intVar->getComponentCount();
        }
        return 0;
    }

    std::vector<float> PyRendererVar::getAsFloatArray()
    {
        std::vector<float> ret;
        RenderVariableFloat* floatVar = m_rendererVar->as<RenderVariableFloat>();
        if (floatVar != nullptr)
        {
            ret = std::vector<float>(floatVar->get(), floatVar->get() + floatVar->getComponentCount());
        }
        return ret;
    }
    std::vector<int32_t> PyRendererVar::getAsIntArray()
    {
        std::vector<int32_t> ret;
        RenderVariableInt* intVar = m_rendererVar->as<RenderVariableInt>();
        if (intVar != nullptr)
        {
            ret = std::vector<int32_t>(intVar->get(), intVar->get() + intVar->getComponentCount());
        }
        return ret;
    }

    PyTexture* PyRendererVar::getTexture()
    {
        RenderVariableTexture* var = m_rendererVar->as<RenderVariableTexture>();
        if (var != nullptr && var->get().get())
        {
            return new PyTexture(var->get().get());
        }
        return nullptr;
    }
    PyBuffer* PyRendererVar::getBuffer()
    {
        RenderVariableBuffer* var = m_rendererVar->as<RenderVariableBuffer>();
        if (var != nullptr && var->get().get())
        {
            return new PyBuffer(var->get().get());
        }
        return nullptr;
    }

    std::array<std::vector<float>, 2> PyRendererVar::getLimitsFloat()
    {
        std::array<std::vector<float>, 2> limits;
        RenderVariableFloat* floatVar = m_rendererVar->as<RenderVariableFloat>();

        const float* min;
        const float* max;

        if (floatVar != nullptr && floatVar->getLimits(min, max))
        {
            size_t components = floatVar->getComponentCount();

            limits[0].resize(components);
            limits[1].resize(components);

            for (size_t i = 0; i < components; ++i)
            {
                limits[0][i] = min[i];
                limits[1][i] = max[i];
            }
        }

        return limits;
    }
    std::array<std::vector<int32_t>, 2> PyRendererVar::getLimitsInt()
    {
        std::array<std::vector<int32_t>, 2> limits;
        RenderVariableInt* intVar = m_rendererVar->as<RenderVariableInt>();

        const int32_t* min;
        const int32_t* max;

        if (intVar != nullptr && intVar->getLimits(min, max))
        {
            size_t components = intVar->getComponentCount();

            limits[0].resize(components);
            limits[1].resize(components);

            for (size_t i = 0; i < components; ++i)
            {
                limits[0][i] = min[i];
                limits[1][i] = max[i];
            }
        }

        return limits;
    }

    std::vector<const char*> PyRendererVar::getOptions()
    {
        std::vector<const char*> options;
        RenderVariableOptions* optionsVar = m_rendererVar->as<RenderVariableOptions>();
        if (optionsVar != nullptr)
        {

            options.resize(optionsVar->getOptionsCount());

            const char* const* optionsConst = optionsVar->getOptions();
            
            for (size_t i = 0; i < options.size(); ++i)
            {
                options[i] = optionsConst[i];
            }

            
        }

        return options;
    }

    void PyRendererVar::setSelectedOption(int32_t v)
    {
        RenderVariableOptions* optionsVar = m_rendererVar->as<RenderVariableOptions>();
        if (optionsVar != nullptr)
        {
            optionsVar->set(v);
        }

    }

    int32_t PyRendererVar::getSelectedOption()
    {
        RenderVariableOptions* optionsVar = m_rendererVar->as<RenderVariableOptions>();
        if (optionsVar != nullptr)
        {
            return optionsVar->get();
        }

        return -1;
    }

    RendererVariableType PyRendererVar::getType() const { return m_rendererVar->getType(); }


    BINDING_FUNC(PyRendererVar, m)
    {


        pybind11::enum_<RendererVariableType>(m, "RendererVariableType")
            .value("FLOAT", RendererVariableType::FLOAT)
            .value("INT", RendererVariableType::INT)
            .value("TEXTURE", RendererVariableType::TEXTURE)
            .value("BUFFER", RendererVariableType::BUFFER)
            .value("OPTIONS", RendererVariableType::OPTIONS);

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
            .def("getComponentCount", &PyRendererVar::getComponentCount)

            .def("getOptions", &PyRendererVar::getOptions)
            .def("getSelectedOption", &PyRendererVar::getSelectedOption)
            .def("setSelectedOption", &PyRendererVar::setSelectedOption)

            .def("getType", &PyRendererVar::getType);


    }
}