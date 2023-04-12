#pragma once

#include <vector>
#include <assert.h>

namespace YAPT
{
	template<typename STATE>
	class ResourceStateTracker
	{
	public:

		void init(const STATE& state, size_t numberOfSubResources);
		
		void setSharedState(const STATE& state);
		void setStateForSubResource(size_t subresource, const STATE& state);

		size_t getNumberOfSubResources() const;
		STATE getStateForSubResource(size_t subresource);

		void checkSharedState();

		bool allSubResourcesShareState() const { return m_allSubresourcesShareState; }

	private:

		STATE m_sharedState;
		std::vector<STATE> m_statePerSubResource;
		bool m_allSubresourcesShareState;
	};


	//impl
	template<typename STATE>
	void ResourceStateTracker<STATE>::init(const STATE& state, size_t numberOfSubResources)
	{
		m_sharedState = state;
		if (numberOfSubResources > 1)
		{
			m_statePerSubResource.resize(numberOfSubResources, state);
		}
		else
		{
			m_statePerSubResource.clear();
		}
		m_allSubresourcesShareState = true;
	}

	template<typename STATE>
	void ResourceStateTracker<STATE>::setSharedState(const STATE& state)
	{
		if (!m_allSubresourcesShareState)
		{
			for (size_t i = 0; i < m_statePerSubResource.size(); ++i)
			{
				m_statePerSubResource[i] = state;
			}
		}

		m_allSubresourcesShareState = true;
		m_sharedState = state;

	}

	template<typename STATE>
	void ResourceStateTracker<STATE>::setStateForSubResource(size_t subresource, const STATE& state)
	{
		if (m_allSubresourcesShareState && m_sharedState == state) return;

		if (subresource == 0 && m_statePerSubResource.size() == 0)
		{
			m_sharedState = state;
		}
		else
		{
			assert(subresource < m_statePerSubResource.size());
			m_statePerSubResource[subresource] = state;
			m_allSubresourcesShareState = false;
		}
	}

	template<typename STATE>
	size_t ResourceStateTracker<STATE>::getNumberOfSubResources() const
	{
		return m_statePerSubResource.size() == 0 ? 1 : m_statePerSubResource.size();
	}


	template<typename STATE>
	STATE ResourceStateTracker<STATE>::getStateForSubResource(size_t subresource)
	{
		if (m_allSubresourcesShareState || (subresource == 0 && m_statePerSubResource.size() == 0))
		{
			return m_sharedState;
		}
		else
		{
			assert(subresource < m_statePerSubResource.size());
			return m_statePerSubResource[subresource];
		}
	}
	template<typename STATE>
	void ResourceStateTracker<STATE>::checkSharedState()
	{
		if (m_allSubresourcesShareState) return;
		if (m_statePerSubResource.size() == 0) return;

		bool stateShared = true;
		const STATE& state = m_statePerSubResource[0];
		for (int i = 1; i < m_statePerSubResource.size(); ++i)
		{
			if (m_statePerSubResource[i] != state)
			{
				stateShared = false;
				break;
			}
				
		}

		if (stateShared)
		{
			m_allSubresourcesShareState = true;
			m_sharedState = state;
		}
		
	}

}