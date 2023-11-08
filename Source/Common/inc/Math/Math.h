#pragma once
#include <glm/glm.hpp>
#include <glm/common.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace YAPT
{
	//aligned
	typedef glm::mat<3, 3, float, glm::aligned_highp> mat3;
	typedef glm::mat<4, 4, float, glm::aligned_highp> mat4;
	typedef glm::qua<float, glm::aligned_highp> quat;
	typedef glm::vec<2, float, glm::aligned_highp> vec2;
	typedef glm::vec<3, float, glm::aligned_highp> vec3;
	typedef glm::vec<4, float, glm::aligned_highp> vec4;
	typedef glm::vec<2, unsigned int, glm::aligned_highp> uvec2;
	typedef glm::vec<3, unsigned int, glm::aligned_highp> uvec3;
	typedef glm::vec<4, unsigned int, glm::aligned_highp> uvec4;
	typedef glm::vec<2, int, glm::aligned_highp> ivec2;
	typedef glm::vec<3, int, glm::aligned_highp> ivec3;
	typedef glm::vec<4, int, glm::aligned_highp> ivec4;

	//tightly packed
	typedef glm::mat<3, 3, float, glm::packed_highp> mat3p;
	typedef glm::mat<4, 4, float, glm::packed_highp> mat4p;
	typedef glm::qua<float, glm::packed_highp> quatp;
	typedef glm::vec<2, float, glm::packed_highp> vec2p;
	typedef glm::vec<3, float, glm::packed_highp> vec3p;
	typedef glm::vec<4, float, glm::packed_highp> vec4p;
	typedef glm::vec<2, unsigned int, glm::packed_highp> uvec2p;
	typedef glm::vec<3, unsigned int, glm::packed_highp> uvec3p;
	typedef glm::vec<4, unsigned int, glm::packed_highp> uvec4p;
	typedef glm::vec<2, int, glm::packed_highp> ivec2p;
	typedef glm::vec<3, int, glm::packed_highp> ivec3p;
	typedef glm::vec<4, int, glm::packed_highp> ivec4p;
	
	const static vec3 s_worldForward(0.f, 0.f, -1.f);
	const static vec3 s_worldUp(0.f, 1.0f, 0.f);
	const static vec3 s_worldRight(1.0f, 0.0f, 0.0f);
}

