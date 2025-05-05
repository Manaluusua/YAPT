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


	void CRendererConfiguration::registerVariable(const char* name, RendererVariable* var)
	{
		std::string str(name);
		auto iter = m_vars.find(str);

		if (iter != m_vars.end())
		{
			if (iter->second->getType() != var->getType())
			{
				YAPT_LOG_FATAL_ERROR("Trying to register render configuration variable %s with different type than what is already registered", name);
			}
			delete var;
		}


		m_vars[str] = var;

	}

}