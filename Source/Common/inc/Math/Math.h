#pragma once
#include <glm/glm.hpp>
#include <glm/common.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace YAPT
{
	typedef glm::vec<1, float> vec1;
	typedef glm::vec<2, float> vec2;
	typedef glm::vec<3, float> vec3;
	typedef glm::vec<4, float> vec4;
	typedef glm::mat<3, 3, float> mat3;
	typedef glm::mat<4, 4, float> mat4;
	typedef glm::quat quat;
	
	const static vec3 s_worldForward(0.f, 0.f, -1.f);
	const static vec3 s_worldUp(0.f, 1.0f, 0.f);
	const static vec3 s_worldRight(1.0f, 0.0f, 0.0f);
}

