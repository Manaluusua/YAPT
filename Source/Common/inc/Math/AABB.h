#pragma once
#include "Math.h"
#include <limits.h>
namespace YAPT
{
	struct AABB
	{
		vec3 min;
		vec3 max;


		static AABB createEmpty()
		{
			constexpr float maxF = std::numeric_limits<float>::max();
			AABB aabb;
			aabb.max = vec3(-maxF, -maxF, -maxF);
			aabb.min = vec3(maxF, maxF, maxF);

			return aabb;
		}

		AABB() = default;

		AABB(const vec3& minimum, const vec3& maximum)
		{
			min = minimum;
			max = maximum;
		}

		void encapsulate(const vec3& p)
		{
			min = glm::min(p, min);
			max = glm::max(p, max);
		}

		void encapsulate(const AABB& b)
		{
			min = glm::min(b.min, min);
			max = glm::max(b.max, max);
		}

		bool isEmpty()
		{
			return glm::any(glm::lessThan(max, min));
		}

		vec3 center()
		{
			return (min + max) * 0.5f;
		}

		vec3 size()
		{
			return max - min;
		}

		vec3 extents()
		{
			return size() * 0.5f;
		}
	};

}