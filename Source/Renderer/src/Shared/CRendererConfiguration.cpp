#pragma once

#include <Renderer/Shared/CRendererConfiguration.h>
#include <Common/Logger.h>

namespace YAPT
{

	////////////////////////////////////////////////////// RendererConfiguration Impl ///////////////////////////////////////////////////////////////////////////
	CRendererConfiguration::CRendererConfiguration()
	{

	}
	CRendererConfiguration::~CRendererConfiguration()
	{
		clear();
	}


	RendererVariable* CRendererConfiguration::getRendererVariable(const char* name)
	{
		std::string str(name);

		auto iter = m_vars.find(str);
		if (iter != m_vars.end())
		{
			return iter->second;
		}
		else
		{
			return nullptr;
		}

	}

	size_t CRendererConfiguration::getNumberOfRendererVariables() const
	{
		return m_vars.size();
	}
	void CRendererConfiguration::queryRendererVariableNamesList(const char** listOut, size_t maxNumberOfEntries)
	{
		size_t index = 0;
		for (auto iter = m_vars.begin(); (iter != m_vars.end()) && (index < maxNumberOfEntries); ++iter, ++index)
		{
			listOut[index] = iter->first.c_str();
		}
	}

	void CRendererConfiguration::clear()
	{
		m_dirtyVariables.clear();
		for (auto iter = m_vars.begin(); iter != m_vars.end(); ++iter)
		{
			delete iter->second;
		}
		m_vars.clear();
	}

	void CRendererConfiguration::commitChanges()
	{
		for (size_t i = 0; i < m_dirtyVariables.size(); ++i)
		{
			m_dirtyVariables[i].callback(m_dirtyVariables[i].rendererVar);
		}
		m_dirtyVariables.clear();
	}

	void CRendererConfiguration::addDirtyVariable(CommitCallback& cb)
	{
		m_dirtyVariables.push_back(cb);
	}

	RendererVariable* CRendererConfiguration::registerVariable(const char* name, RendererVariableType type)
	{
		std::string str(name);
		auto iter = m_vars.find(str);

		RendererVariable* retVal = nullptr;
		if (iter != m_vars.end())
		{
			if (iter->second->getType() != type)
			{
				YAPT_LOG_FATAL_ERROR("Trying to register render configuration variable %s with different type than what is already registered", name);
				return retVal;
			}
			
		}
		else
		{
			switch (type)
			{
			case YAPT::RendererVariableType::FLOAT:
				retVal = new CRendererVariable<float, RendererVariableType::FLOAT>(this);
				break;
			case YAPT::RendererVariableType::UINT:
				retVal = new CRendererVariable<uint32_t, RendererVariableType::UINT>(this);
				break;
			case YAPT::RendererVariableType::INT:
				retVal = new CRendererVariable<int32_t, RendererVariableType::INT>(this);
				break;
			case YAPT::RendererVariableType::VEC2:
				retVal = new CRendererVariable<glm::vec2, RendererVariableType::VEC2>(this);
				break;
			case YAPT::RendererVariableType::VEC3:
				retVal = new CRendererVariable<glm::vec3, RendererVariableType::VEC3>(this);
				break;
			case YAPT::RendererVariableType::VEC4:
				retVal = new CRendererVariable<glm::vec4, RendererVariableType::VEC4>(this);
				break;
			case YAPT::RendererVariableType::IVEC2:
				retVal = new CRendererVariable<glm::ivec2, RendererVariableType::IVEC2>(this);
				break;
			case YAPT::RendererVariableType::IVEC3:
				retVal = new CRendererVariable<glm::ivec3, RendererVariableType::IVEC3>(this);
				break;
			case YAPT::RendererVariableType::IVEC4:
				retVal = new CRendererVariable<glm::ivec4, RendererVariableType::IVEC4>(this);
				break;
			case YAPT::RendererVariableType::TEXTURE:
				retVal = new CRendererVariable<RCPtr<Texture>, RendererVariableType::TEXTURE>(this);
				break;
			case YAPT::RendererVariableType::BUFFER:
				retVal = new CRendererVariable<RCPtr<Buffer>, RendererVariableType::BUFFER>(this);
				break;
			default:
				YAPT_LOG_FATAL_ERROR("Tried to instantiate unknown renderer variable type");
				break;
			}
		}

		if (retVal)
		{
			m_vars[str] = retVal;
		}

		return retVal;
	}

}