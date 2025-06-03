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

		AABB(vec3 minimum, vec3 maximum)
		{
			min = minimum;
			max = maximum;
		}


		bool isEmpty()
		{
			return glm::any(glm::lessThan(max, min));
		}

		vec3 center()
		{
			(min + max) * 0.5f;
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