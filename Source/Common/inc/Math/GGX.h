#pragma once
#include <Common/CommonUtilities.h>
#include <Math/Math.h>
#include <Math/Fresnel.h>
#include <Math/MathUtility.h>


namespace YAPT
{
	using MathUtils::PI;
	vec3p refr(float etaR, const vec3p& wm, const vec3p& wo)
	{
		float c = dot(wm, wo);
		float temp = 1 + etaR * (MathUtils::sqr(c) - 1.f);
		if (temp < 0) //TIR
		{
			return vec3p(0.f);
		}

		temp = etaR * c - glm::sign(wo.y) * sqrt(temp);
		return normalize(temp * wm - etaR * wo);
	}


	float jReflection(const vec3p& wi, const vec3p& wm)
	{
		return 1.f / SAFE_DIVISOR(4 * MathUtils::saturate(dot(wi, wm)));
	}


	float jRefraction(float eta, const vec3p& wo, const vec3p& wm, const vec3p& wi)
	{ 

		float dotMO = dot(wo, wm);
		float dotMI = abs(dot(wi, wm));

		float denom = MathUtils::sqr(dotMO + eta * dotMI);
		return MathUtils::sqr(eta) * dotMI / SAFE_DIVISOR(denom);
	}

	inline float smithLambdaGGX(vec3p d, float ax, float ay)
	{
		float s = 1.0f + (d.x * d.x * ax * ax + d.z * d.z * ay * ay) / (d.y * d.y);
		return (sqrt(s) - 1) * 0.5f;
	}


	inline float G1GGX(vec3p wo, vec3p wm, float ax, float ay)
	{
		float lambda = smithLambdaGGX(wo, ax, ay);
		return MathUtils::step(0, dot(wo, wm)) / (1.0f + lambda);
	}

	//Height correlated G2
	inline float G2GGX(vec3p wo, vec3p wi, vec3p wm, float ax, float ay)
	{
		float lambdaI = smithLambdaGGX(wi, ax, ay);
		float lambdaO = smithLambdaGGX(wo, ax, ay);

		float nom = MathUtils::step(0, dot(wo, wm)); //* MathUtils::step(0, dot(wi, wm));
		float denom = 1 + lambdaI + lambdaO;

		return nom / denom;
	}
	 

	inline float DGGX(vec3p wm, float ax, float ay)
	{
		ax = max(ax, 0.000001f);
		ay = max(ay, 0.000001f);
		float denom = (MathUtils::sqr(wm.x / SAFE_DIVISOR(ax)) + MathUtils::sqr(wm.z / SAFE_DIVISOR(ay)) + MathUtils::sqr(wm.y));
		denom *= denom;
		denom *= ax * ay * PI;
		return (1.f / SAFE_DIVISOR(denom));

	}


	inline float DVGGX(vec3p wo, vec3p wm, float ax, float ay)
	{
		float D = DGGX(wm, ax, ay);
		float G1 = G1GGX(wo, wm, ax, ay);
		return (G1 * abs(dot(wo, wm)) * D) / SAFE_DIVISOR(abs(wo.y));
	}

	inline vec3p sampleNormalsGGX(float ax, float ay, float u1, float u2)
	{
		vec3p t1 = vec3p(1.f, 0.0f, 0.0f);
		vec3p t2 = vec3p(0.f, 0.f, 1.f);
		vec3p wm = sqrt(u1 / SAFE_DIVISOR(1.f - u1)) * (ax * cos(2.f * PI * u2) * t1 + ay * sin(2.f * PI * u2) * t2);
		wm += vec3p(0.f, 1.f, 0.f);
		return normalize(wm);
	}

	//Sampling the GGX Distribution of Visible Normals by Eric Heitz
	//Difference here to the article is that we use y up, not z up
	inline vec3p sampleVisibleNormalsGGX(vec3p wo, float ax, float ay, float u1, float u2)
	{

		// Section 3.2: transforming the view direction to the hemisphere configuration
		vec3p Vh = normalize(vec3p(ax * wo.x, ay * wo.z, wo.y));
		// Section 4.1: orthonormal basis (with special case if cross product is zero)
		float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
		vec3p T1 = lensq > 0 ? vec3p(-Vh.y, Vh.x, 0) * (1.f / sqrt(lensq)) : vec3p(1, 0, 0);
		vec3p T2 = cross(Vh, T1);
		// Section 4.2: parameterization of the projected area
		float r = sqrt(u1);
		float phi = 2.0f * PI * u2;
		float t1 = r * cos(phi);
		float t2 = r * sin(phi);
		float s = 0.5f * (1.0f + Vh.z);
		t2 = (1.0f - s) * sqrt(1.0f - t1 * t1) + s * t2;
		// Section 4.3: reprojection onto hemisphere
		vec3p Nh = t1 * T1 + t2 * T2 + sqrt(max(0.f, 1.f - t1 * t1 - t2 * t2)) * Vh;
		// Section 3.4: transforming the normal back to the ellipsoid configuration
		vec3p Ne = normalize(vec3p(ax * Nh.x, max(0.0f, Nh.z), ay * Nh.y));


		return Ne;
	}
	  
          
	vec3p sampleWM(const vec3p& wo, float ax, float ay, float u1, float u2)
	{
		//return sampleVisibleNormalsGGX(wo, ax, ay, u1, u2);
		return sampleNormalsGGX(ax, ay, u1, u2);
	}
	  
	float pdfWM(const vec3p& wo, const vec3p& wm, float ax, float ay)
	{ 
		//return DVGGX(wo, wm, ax, ay);
		return DGGX(wm, ax, ay) * MathUtils::saturate(wm.y);
	}
	            
	 
	vec3p evaluateGGXConductor(const vec3p& etaR, const vec3p& etaK, float ax, float ay, const vec3p& wo, const vec3p& wi)
	{
		vec3p wm = glm::normalize(wo + wi);
		if (wi.y < 0.0f || dot(wi, wm) < 0.f)
		{
			return vec3p(0.f);
		}

		vec3p F = fresnelDielectricConductor(etaR, etaK, dot(wo, wm));
		float G2 = G2GGX(wo, wi, wm, ax, ay);
		float D = DGGX(wm, ax, ay);

		return F * G2 * D / (4.f * wo.y * wi.y);
	}



	vec3p getWMTranslucent(const vec3p& wo, const vec3p& wi, float etaR)
	{
		vec3p wm;
		bool isReflected = wi.y >= 0.f;
		if (isReflected)
		{
			wm = normalize(wo + wi);
		}
		else
		{
			if (etaR == 1.f)
			{
				etaR = 1.0001f;
			}

			wm = normalize(wo + wi * etaR);
			if (etaR > 1.f)
			{
				wm *= -1.f;
			}
		}

		return wm;
	}
	      
	void sampleGGXDielectricTranslucent(float etaR, float ax, float ay, const vec3p& wo, const vec3p& sample, vec3p& sampleDirOut)
	{
		vec3p wm;
		vec3p wi;

		wm = sampleWM(wo, ax, ay, sample.x, sample.y);


		if (etaR == 1.f)
		{
			etaR = 1.0001f;
		}

		float F = fresnelDielectricDielectric2(etaR, dot(wo, wm));

		if (dot(wo, wm) < 0.f)
		{
			sampleDirOut = vec3p(0.f);
			return;
		}

		float invEta = 1.f / etaR;
		  
		if (sample.z <= F)
		{
			wi = reflect(-wo, wm);
			if (wi.y <= 0.f)
			{
				wi = vec3p(0.f);
			}
		}
		else
		{    
			wi = glm::refract(-wo, wm, invEta);
		}
     
		sampleDirOut = wi;
	}

	float pdfGGXDielectricTranslucent(float etaR, const vec3p& wo, const vec3p& wi, float ax, float ay)
	{
		vec3p wm;
		bool isReflected = wi.y >= 0.f;

		wm = getWMTranslucent(wo, wi, etaR);

		if (glm::dot(wo, wm) <= 0.f) return 0.f;

		float F = fresnelDielectricDielectric2(etaR, dot(wo, wm));

		float pdf = pdfWM(wo, wm, ax, ay);
	
		if (isReflected)
		{
			pdf *= F * jReflection(wo, wm);
		}
		else
		{
			pdf *= (1.f - F) * jRefraction(etaR, wo, wm, wi);

		}

		return pdf;
		                        
		  
	}

	vec3p evaluateGGXDielectricTranslucent(float etaR, float ax, float ay, const vec3p& wo, const vec3p& wi)
	{
		bool isReflected = wi.y >= 0.f;
		vec3p wm = getWMTranslucent(wo, wi, etaR);

		vec3p weight;
		 
		float VdotH = MathUtils::saturate(dot(wo, wm));

		vec3p F(fresnelDielectricDielectric2(etaR, VdotH));
		float G2 = G2GGX(wo, wi, wm, ax, ay);
		float D = DGGX(wm, ax, ay);
		   
		if (isReflected)
		{
			weight = F * G2 * D / (4.f * wo.y * wi.y);
			 
		}
		else
		{
			if (dot(wo, wm) * dot(wi, wm) > 0) return vec3p(0.f);
			         
			weight = (1.f - F) * G2 * D * VdotH * jRefraction(etaR, wo, wm, wi) / SAFE_DIVISOR(abs(wo.y * wi.y));

		}
		return weight;
	}



}