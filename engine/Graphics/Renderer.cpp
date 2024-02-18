#include "Renderer.h"
#include "../Util/Assets.h"
#include "ModelInfo.h"
#include "Camera.h"
#include "Scene.h"

#include "finders_interface.h"
#ifndef NDEBUG
#define ASSERT(a, ...) assert(a)
#else
#define ASSERT(a, ...) 
#endif

static const char* baseVertexShader = "#version 330\n\
layout(location = 0) in vec3 position;\n\
layout(location = 1) in vec3 normal;\n\
layout(location = 2) in vec2 uv1;\n\
layout(location = 3) in vec2 uv2;\n\
layout(location = 4) in vec4 joint;\n\
layout(location = 5) in vec4 weights;\n\
layout(location = 6) in vec4 color;\n\
uniform mat4 model;\n\
uniform mat4 view;\n\
uniform mat4 projection;\n\
uniform vec3 camPos;\n\
#define MAX_NUM_JOINTS 128\n\
layout (std140) uniform BoneData {\n\
	mat4 jointMatrix[MAX_NUM_JOINTS];\n\
	float jointCount;\n\
} bones;\n\
out vec3 worldPos;\n\
out vec3 fragNormal;\n\
out vec2 fragUV1;\n\
out vec2 fragUV2;\n\
out vec4 fragColor;\n\
void main(){\n\
	vec4 locPos;\n\
	if (bones.jointCount > 0.0) {\n\
		mat4 boneMat =\n\
			weights.x * bones.jointMatrix[int(joint.x)] +\n\
			weights.y * bones.jointMatrix[int(joint.y)] +\n\
			weights.z * bones.jointMatrix[int(joint.z)] +\n\
			weights.w * bones.jointMatrix[int(joint.w)];\n\
	\n\
		locPos = model * boneMat * vec4(position, 1.0);\n\
		fragNormal = normalize(transpose(inverse(mat3(model * boneMat))) * normal);\n\
	}\n\
	else {\n\
		locPos = model * vec4(position, 1.0);\n\
		fragNormal = normalize(transpose(inverse(mat3(model))) * normal);\n\
	}\n\
	worldPos = locPos.xyz / locPos.w;\n\
	fragUV1 = uv1;\n\
	fragUV2 = uv2;\n\
	fragColor = color;\n\
	gl_Position = projection * view * vec4(worldPos, 1.0);\n\
}";
static const char* baseFragmentShader = "#version 330\n\
in vec3 worldPos;\n\
in vec3 fragNormal;\n\
in vec2 fragUV1;\n\
in vec2 fragUV2;\n\
in vec4 fragColor;\n\
layout (std140) uniform Material {\n\
	vec4 baseColorFactor;\n\
	vec4 emissiveFactor;\n\
	vec4 diffuseFactor;\n\
	vec4 specularFactor;\n\
	int baseColorUV;\n\
	int normalUV;\n\
	int emissiveUV;\n\
	int aoUV;\n\
	int metallicRoughnessUV;\n\
	float roughnessFactor;\n\
	float metallicFactor;\n\
	float alphaMask;\n\
	float alphaCutoff;\n\
	float workflow;\n\
}mat;\n\
#define MAX_NUM_LIGHTS 20\n\
struct PointLight\n\
{\n\
	vec4 color;\n\
	vec4 pos;\n\
	float constant;\n\
	float linear;\n\
	float quadratic;\n\
	int projIdx;\n\
};\n\
struct DirectionalLight\n\
{\n\
	vec3 direction;\n\
	int isCascaded;\n\
	vec3 color;\n\
	int projIdx;\n\
};\n\
struct SpotLight\n\
{\n\
	vec4 color;\n\
	vec3 direction;\n\
	int projIdx;\n\
	vec3 pos;\n\
	float cutOff;\n\
};\n\
struct ProjectionData\n\
{\n\
	mat4 proj;\n\
	vec2 start;\n\
	vec2 end;\n\
};\n\
layout (std140) uniform LightData {\n\
	PointLight pointLights[MAX_NUM_LIGHTS];\n\
	DirectionalLight dirLights[MAX_NUM_LIGHTS];\n\
	SpotLight spotLights[MAX_NUM_LIGHTS];\n\
	ProjectionData projections[MAX_NUM_LIGHTS];\n\
	vec4 ambientColor;\n\
	int numPointLights;\n\
	int numDirLights;\n\
	int numSpotLights;\n\
	int numProjections;\n\
}lights;\n\
\n\
uniform samplerCube samplerIrradiance;\n\
uniform samplerCube prefilteredMap;\n\
uniform sampler2D samplerBRDFLUT;\n\
uniform sampler2D colorMap;\n\
uniform sampler2D normalMap;\n\
uniform sampler2D aoMap;\n\
uniform sampler2D emissiveMap;\n\
uniform sampler2D metallicRoughnessMap;\n\
uniform sampler2DShadow shadowMap;\n\
\n\
uniform mat4 model; \n\
uniform mat4 view; \n\
uniform mat4 projection; \n\
uniform vec3 camPos; \n\
\n\
uniform float prefilteredCubeMipLevels; \n\
uniform float scaleIBLAmbient;\n\
\n\
const float M_PI = 3.141592653589793;\n\
const float c_MinRoughness = 0.04;\n\
const float PBR_WORKFLOW_METALLIC_ROUGHNESS = 0.0;\n\
const float PBR_WORKFLOW_SPECULAR_GLOSINESS = 1.0f;\n\
\n\
struct PBRInfo\n\
{\n\
	float NdotL;					\n\
	float NdotV;					\n\
	float NdotH;					\n\
	float LdotH;					\n\
	float VdotH;					\n\
	float perceptualRoughness;		\n\
	float metalness;				\n\
	vec3 reflectance0;				\n\
	vec3 reflectance90;				\n\
	float alphaRoughness;			\n\
	vec3 diffuseColor;				\n\
	vec3 specularColor;				\n\
};\n\
vec3 getNormal()\n\
{\n\
\n\
	vec3 tangentNormal = texture(normalMap, mat.normalUV == 0 ? fragUV1 : fragUV2).xyz * 2.0f - 1.0f;\n\
\n\
	vec3 q1 = dFdx(worldPos);\n\
	vec3 q2 = dFdy(worldPos);\n\
	vec2 st1 = dFdx(fragUV1);\n\
	vec2 st2 = dFdy(fragUV1);\n\
\n\
	vec3 N = normalize(fragNormal);\n\
	vec3 T = normalize(q1 * st2.t - q2 * st1.t);\n\
	vec3 B = -normalize(cross(N, T));\n\
	mat3 TBN = mat3(T, B, N);\n\
\n\
	return normalize(TBN * tangentNormal);\n\
}\n\
vec3 getIBLContribution(PBRInfo pbrInputs, vec3 n, vec3 reflection)\n\
{\n\
	float lod = (pbrInputs.perceptualRoughness * prefilteredCubeMipLevels);\n\
	vec3 brdf = (texture(samplerBRDFLUT, vec2(pbrInputs.NdotV, 1.0f - pbrInputs.perceptualRoughness))).rgb;\n\
	vec3 diffuseLight = texture(samplerIrradiance, n).rgb;\n\
\n\
	vec3 specularLight = textureLod(prefilteredMap, reflection, lod).rgb;\n\
\n\
	vec3 diffuse = diffuseLight * pbrInputs.diffuseColor;\n\
	vec3 specular = specularLight * (pbrInputs.specularColor * brdf.x + brdf.y);\n\
\n\
	diffuse *= scaleIBLAmbient;\n\
	specular *= scaleIBLAmbient;\n\
\n\
	return diffuse + specular;\n\
}\n\
\n\
vec3 diffuse(PBRInfo pbrInputs)\n\
{\n\
	return pbrInputs.diffuseColor / M_PI;\n\
}\n\
vec3 specularReflection(PBRInfo pbrInputs)\n\
{\n\
	return pbrInputs.reflectance0 + (pbrInputs.reflectance90 - pbrInputs.reflectance0) * pow(clamp(1.0 - pbrInputs.VdotH, 0.0, 1.0), 5.0);\n\
}\n\
float geometricOcclusion(PBRInfo pbrInputs)\n\
{\n\
	float NdotL = pbrInputs.NdotL;\n\
	float NdotV = pbrInputs.NdotV;\n\
	float r = pbrInputs.alphaRoughness;\n\
	float attenuationL = 2.0 * NdotL / (NdotL + sqrt(r * r + (1.0 - r * r) * (NdotL * NdotL)));\n\
	float attenuationV = 2.0 * NdotV / (NdotV + sqrt(r * r + (1.0 - r * r) * (NdotV * NdotV)));\n\
	return attenuationL * attenuationV;\n\
}\
float microfacetDistribution(PBRInfo pbrInputs)\n\
{\n\
	float roughnessSq = pbrInputs.alphaRoughness * pbrInputs.alphaRoughness;\n\
	float f = (pbrInputs.NdotH * roughnessSq - pbrInputs.NdotH) * pbrInputs.NdotH + 1.0;\n\
	return roughnessSq / (M_PI * f * f);\n\
}\n\
float convertMetallic(vec3 diffuse, vec3 specular, float maxSpecular) {\n\
	float perceivedDiffuse = sqrt(0.299f * diffuse.r * diffuse.r + 0.587f * diffuse.g * diffuse.g + 0.114f * diffuse.b * diffuse.b);\n\
	float perceivedSpecular = sqrt(0.299f * specular.r * specular.r + 0.587f * specular.g * specular.g + 0.114f * specular.b * specular.b);\n\
	if (perceivedSpecular < c_MinRoughness) {\n\
		return 0.0;\n\
	}\n\
	float a = c_MinRoughness;\n\
	float b = perceivedDiffuse * (1.0f - maxSpecular) / (1.0f - c_MinRoughness) + perceivedSpecular - 2.0f * c_MinRoughness;\n\
	float c = c_MinRoughness - perceivedSpecular;\n\
	float D = max(b * b - 4.0 * a * c, 0.0f);\n\
	return clamp((-b + sqrt(D)) / (2.0f * a), 0.0f, 1.0f);\n\
}\n\
float calculateShadowValue(ProjectionData p, vec2 textureStep)\n\
{\n\
	float shadowVal = 0.0f;\n\
	vec4 shadowUV = p.proj * vec4(worldPos, 1.0f);\n\
	shadowUV /= shadowUV.w; shadowUV.xyz += 1.0f; shadowUV.xyz *= 0.5f;\n\
	shadowUV.xy *= (p.end - p.start); shadowUV.xy += p.start;\n\
	if(shadowUV.z >= 1.0f || shadowUV.z <= 0.0f) return 1.0f;\n\
	for(int x = -1; x <= 1; ++x)\n\
	{\n\
		for(int y = -1; y <= 1; ++y)\n\
		{\n\
			vec3 testPos = shadowUV.xyz + vec3(textureStep.x * float(x), textureStep.y * float(y), 0.0f);\n\
			if(testPos.x < p.start.x || testPos.y < p.start.y || testPos.x > p.end.x || testPos.y > p.end.y) shadowVal += 1.0f;\n\
			else shadowVal += texture(shadowMap, testPos);\n\
		}\n\
	}\n\
	shadowVal /= 9.0f;\n\
	return shadowVal;\n\
}\n\
\n\
out vec4 outColor;\n\
void main()\n\
{\n\
	float perceptualRoughness;\n\
	float metallic;\n\
	vec3 diffuseColor;\n\
	vec4 baseColor;\n\
\n\
	vec3 f0 = vec3(0.04f);\n\
\n\
	if (mat.baseColorUV > -1) {\n\
		baseColor = texture(colorMap, mat.baseColorUV == 0 ? fragUV1 : fragUV2) * mat.baseColorFactor;\n\
	}\n\
	else {\n\
		baseColor = mat.baseColorFactor;\n\
	}\n\
	if (baseColor.a < mat.alphaCutoff) {\n\
		discard;\n\
	}\n\
\n\
	if (mat.workflow == PBR_WORKFLOW_METALLIC_ROUGHNESS) {\n\
		perceptualRoughness = mat.roughnessFactor;\n\
		metallic = mat.metallicFactor;\n\
		if (mat.metallicRoughnessUV > -1) {\n\
			vec4 mrSample = texture(metallicRoughnessMap, mat.metallicRoughnessUV == 0 ? fragUV1 : fragUV2);\n\
			perceptualRoughness = mrSample.g * perceptualRoughness;\n\
			metallic = mrSample.b * metallic;\n\
		}\n\
		else {\n\
			perceptualRoughness = clamp(perceptualRoughness, c_MinRoughness, 1.0);\n\
			metallic = clamp(metallic, 0.0, 1.0);\n\
		}\n\
		if (mat.baseColorUV > -1) {\n\
			baseColor = texture(colorMap, mat.baseColorUV == 0 ? fragUV1 : fragUV2) * mat.baseColorFactor;\n\
		}\n\
		else {\n\
			baseColor = mat.baseColorFactor;\n\
		}\n\
	}\n\
\n\
	if (mat.workflow == PBR_WORKFLOW_SPECULAR_GLOSINESS) {\n\
		if (mat.metallicRoughnessUV > -1) {\n\
			perceptualRoughness = 1.0 - texture(metallicRoughnessMap, mat.metallicRoughnessUV == 0 ? fragUV1 : fragUV2).a;\n\
		}\n\
		else {\n\
			perceptualRoughness = 0.0;\n\
		}\n\
\n\
		const float epsilon = 1e-6;\n\
\n\
		vec4 diffuse = texture(colorMap, fragUV1);\n\
		vec3 specular = texture(metallicRoughnessMap, fragUV1).rgb;\n\
\n\
		float maxSpecular = max(max(specular.r, specular.g), specular.b);\n\
\n\
		// Convert metallic value from specular glossiness inputs\n\
		metallic = convertMetallic(diffuse.rgb, specular, maxSpecular);\n\
\n\
		vec3 baseColorDiffusePart = diffuse.rgb * ((1.0f - maxSpecular) / (1.0f - c_MinRoughness) / max(1.0f - metallic, epsilon)) * mat.diffuseFactor.rgb;\n\
		vec3 baseColorSpecularPart = specular - (vec3(c_MinRoughness) * (1.0f - metallic) * (1.0f / max(metallic, epsilon))) * mat.specularFactor.rgb;\n\
		baseColor = vec4(mix(baseColorDiffusePart, baseColorSpecularPart, metallic * metallic), diffuse.a);\n\
	}\n\
	baseColor *= fragColor;\n\
\n\
	diffuseColor = baseColor.rgb * (vec3(1.0f) - f0);\n\
	diffuseColor *= 1.0f - metallic;\n\
\n\
	float alphaRoughness = perceptualRoughness * perceptualRoughness;\n\
\n\
	vec3 specularColor = mix(f0, baseColor.rgb, metallic);\n\
\n\
	float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);\n\
\n\
	float reflectance90 = clamp(reflectance * 25.0f, 0.0f, 1.0f);\n\
	vec3 specularEnvironmentR0 = specularColor.rgb;\n\
	vec3 specularEnvironmentR90 = vec3(1.0f, 1.0f, 1.0f) * reflectance90;\n\
\n\
	vec3 n = (mat.normalUV > -1) ? getNormal() : normalize(fragNormal);\n\
	vec3 v = normalize(camPos - worldPos);    // Vector from surface point to camera\n\
	vec3 reflection = normalize(reflect(v, n));\n\
	reflection.y *= -1.0;\n\
	\n\
	float NdotV = clamp(abs(dot(n, v)), 0.001, 1.0);\n\
	PBRInfo pbrInputs = PBRInfo(\n\
		0.001,	// NdotL\n\
		NdotV,\n\
		0.0,	// NdotH\n\
		0.0,	// LdotH\n\
		0.0,	// VdotH\n\
		perceptualRoughness,\n\
		metallic,\n\
		specularEnvironmentR0,\n\
		specularEnvironmentR90,\n\
		alphaRoughness,\n\
		diffuseColor,\n\
		specularColor\n\
	);\n\
	vec3 color = lights.ambientColor.rgb * baseColor.rgb;\n\
	vec2 ts = vec2(1.0f) / vec2(textureSize(shadowMap, 0));\n\
	for(int i = 0; i < lights.numDirLights; i++)\n\
	{\n\
		vec3 l = normalize(-lights.dirLights[i].direction.xyz); // Vector from surface point to light\n\
		vec3 h = normalize(l+v);\n\
		pbrInputs.NdotL = clamp(dot(n, l), 0.001, 1.0);\n\
		pbrInputs.NdotH = clamp(dot(n, h), 0.0, 1.0);\n\
		pbrInputs.LdotH = clamp(dot(l, h), 0.0, 1.0);\n\
		pbrInputs.VdotH = clamp(dot(v, h), 0.0, 1.0);\n\
		float shadowVal = 1.0f;\n\
		if(lights.dirLights[i].projIdx > -1)\n\
		{\n\
			shadowVal = calculateShadowValue(lights.projections[lights.dirLights[i].projIdx], ts);\n\
		}\n\
		vec3 F = specularReflection(pbrInputs);\n\
		float G = geometricOcclusion(pbrInputs);\n\
		float D = microfacetDistribution(pbrInputs);\n\
		vec3 diffuseContrib = (1.0 - F) * diffuse(pbrInputs);\n\
		vec3 specContrib = F * G * D / (4.0 * pbrInputs.NdotL * pbrInputs.NdotV);\n\
		color += shadowVal * pbrInputs.NdotL * lights.dirLights[i].color.rgb * (diffuseContrib + specContrib);\n\
	}\n\
	for(int i = 0; i < lights.numPointLights; i++)\n\
	{\n\
		vec3 l = normalize(lights.pointLights[i].pos.xyz - worldPos);\n\
		vec3 h = normalize(l+v);\n\
		pbrInputs.NdotL = clamp(dot(n, l), 0.001, 1.0);\n\
		float shadowVal = 1.0f;\n\
		if(lights.pointLights[i].projIdx > -1)\n\
		{\n\
			vec3 ptw = normalize(worldPos - lights.pointLights[i].pos.xyz);\n\
			if(abs(ptw.x) > abs(ptw.y))\n\
			{\n\
				if(abs(ptw.x) > abs(ptw.z))\n\
				{\n\
					if(ptw.x < 0.0f) shadowVal = calculateShadowValue(lights.projections[lights.pointLights[i].projIdx], ts);\n\
					else shadowVal = calculateShadowValue(lights.projections[lights.pointLights[i].projIdx + 1], ts);\n\
				}\n\
				else\n\
				{\n\
					if(ptw.z < 0.0f) shadowVal = calculateShadowValue(lights.projections[lights.pointLights[i].projIdx + 4], ts);\n\
					else shadowVal = calculateShadowValue(lights.projections[lights.pointLights[i].projIdx + 5], ts);\n\
				}\n\
			}\n\
			else\n\
			{\n\
				if(abs(ptw.y) > abs(ptw.z))\n\
				{\n\
					if(ptw.y < 0.0f) shadowVal = calculateShadowValue(lights.projections[lights.pointLights[i].projIdx + 2], ts);\n\
					else shadowVal = calculateShadowValue(lights.projections[lights.pointLights[i].projIdx + 3], ts);\n\
				}\n\
				else\n\
				{\n\
					if(ptw.z < 0.0f) shadowVal = calculateShadowValue(lights.projections[lights.pointLights[i].projIdx + 4], ts);\n\
					else shadowVal = calculateShadowValue(lights.projections[lights.pointLights[i].projIdx + 5], ts);\n\
				}\n\
			}\n\
		}\n\
		pbrInputs.NdotH = clamp(dot(n, h), 0.0, 1.0);\n\
		pbrInputs.LdotH = clamp(dot(l, h), 0.0, 1.0);\n\
		pbrInputs.VdotH = clamp(dot(v, h), 0.0, 1.0);\n\
		vec3 F = specularReflection(pbrInputs);\n\
		float G = geometricOcclusion(pbrInputs);\n\
		float D = microfacetDistribution(pbrInputs);\n\
		vec3 diffuseContrib = (1.0 - F) * diffuse(pbrInputs);\n\
		vec3 specContrib = F * G * D / (4.0 * pbrInputs.NdotL * pbrInputs.NdotV);\n\
		color += pbrInputs.NdotL * shadowVal * lights.pointLights[i].color.rgb * (diffuseContrib + specContrib);\n\
	}\n\
	for(int i = 0; i < lights.numSpotLights; i++)\n\
	{\n\
		vec3 l = normalize(lights.spotLights[i].pos.xyz - worldPos);\n\
		float theta = dot(-l, normalize(lights.spotLights[i].direction.xyz));\n\
		if(theta > lights.spotLights[i].cutOff)\n\
		{\n\
			vec3 h = normalize(l+v);\n\
			pbrInputs.NdotL = clamp(dot(n, l), 0.001, 1.0);\n\
			pbrInputs.NdotH = clamp(dot(n, h), 0.0, 1.0);\n\
			pbrInputs.LdotH = clamp(dot(l, h), 0.0, 1.0);\n\
			pbrInputs.VdotH = clamp(dot(v, h), 0.0, 1.0);\n\
			vec3 F = specularReflection(pbrInputs);\n\
			float G = geometricOcclusion(pbrInputs);\n\
			float D = microfacetDistribution(pbrInputs);\n\
			vec3 diffuseContrib = (1.0 - F) * diffuse(pbrInputs);\n\
			vec3 specContrib = F * G * D / (4.0 * pbrInputs.NdotL * pbrInputs.NdotV);\n\
			float shadowVal = 1.0f;\n\
			if(lights.spotLights[i].projIdx > -1)\n\
			{\n\
				shadowVal = calculateShadowValue(lights.projections[lights.spotLights[i].projIdx], ts);\n\
			}\n\
			color += pbrInputs.NdotL * shadowVal * lights.spotLights[i].color.rgb * (diffuseContrib + specContrib);\n\
		}\n\
	}\n\
	\n\
	color += getIBLContribution(pbrInputs, n, reflection);\n\
\n\
	const float u_OcclusionStrength = 1.0f;\n\
	if (mat.aoUV > -1) {\n\
		float ao = texture(aoMap, (mat.aoUV == 0 ? fragUV1 : fragUV2)).r;\n\
		color = mix(color, color * ao, u_OcclusionStrength);\n\
	}\n\
\n\
	const float u_EmissiveFactor = 1.0f;\n\
	if (mat.emissiveUV > -1) {\n\
		vec3 emissive = texture(emissiveMap, mat.emissiveUV == 0 ? fragUV1 : fragUV2).rgb * u_EmissiveFactor;\n\
		color += emissive * fragColor.rgb;\n\
	}\n\
\n\
	outColor = vec4(color, baseColor.a);\n\
}\n\
";
static const char* baseGeometryFragmentShader = "#version 330\n\
in vec3 worldPos;\n\
in vec3 fragNormal;\n\
in vec2 fragUV1;\n\
in vec2 fragUV2;\n\
in vec4 fragColor;\n\
layout (std140) uniform Material {\n\
	vec4 baseColorFactor;\n\
	vec4 emissiveFactor;\n\
	vec4 diffuseFactor;\n\
	vec4 specularFactor;\n\
	int baseColorUV;\n\
	int normalUV;\n\
	int emissiveUV;\n\
	int aoUV;\n\
	int metallicRoughnessUV;\n\
	float roughnessFactor;\n\
	float metallicFactor;\n\
	float alphaMask;\n\
	float alphaCutoff;\n\
	float workflow;\n\
}mat;\n\
uniform sampler2D colorMap;\n\
\n\
uniform mat4 model; \n\
uniform mat4 view; \n\
uniform mat4 projection; \n\
uniform vec3 camPos; \n\
\n\
void main()\n\
{\n\
	vec4 baseColor;\n\
	if (mat.baseColorUV > -1) {\n\
		baseColor = texture(colorMap, mat.baseColorUV == 0 ? fragUV1 : fragUV2) * mat.baseColorFactor;\n\
	}\n\
	else {\n\
		baseColor = mat.baseColorFactor;\n\
	}\n\
	if (baseColor.a < mat.alphaCutoff) {\n\
		discard;\n\
	}\n\
}";
static const char* ssrPbrFragmentShader = "#version 330\n\
in vec3 worldPos;\n\
in vec3 fragNormal;\n\
in vec2 fragUV1;\n\
in vec2 fragUV2;\n\
in vec4 fragColor;\n\
layout (std140) uniform Material {\n\
	vec4 baseColorFactor;\n\
	vec4 emissiveFactor;\n\
	vec4 diffuseFactor;\n\
	vec4 specularFactor;\n\
	int baseColorUV;\n\
	int normalUV;\n\
	int emissiveUV;\n\
	int aoUV;\n\
	int metallicRoughnessUV;\n\
	float roughnessFactor;\n\
	float metallicFactor;\n\
	float alphaMask;\n\
	float alphaCutoff;\n\
	float workflow;\n\
}mat;\n\
\n\
const float c_MinRoughness = 0.04;\n\
const float PBR_WORKFLOW_METALLIC_ROUGHNESS = 0.0;\n\
const float PBR_WORKFLOW_SPECULAR_GLOSINESS = 1.0f;\n\
\n\
uniform sampler2D colorMap;\n\
uniform sampler2D normalMap;\n\
uniform sampler2D metallicRoughnessMap;\n\
\n\
uniform mat4 model; \n\
uniform mat4 view; \n\
uniform mat4 projection; \n\
uniform vec3 camPos; \n\
\n\
vec3 getNormal()\n\
{\n\
	vec3 tangentNormal = texture(normalMap, mat.normalUV == 0 ? fragUV1 : fragUV2).xyz * 2.0f - 1.0f;\n\
\n\
	vec3 q1 = dFdx(worldPos);\n\
	vec3 q2 = dFdy(worldPos);\n\
	vec2 st1 = dFdx(fragUV1);\n\
	vec2 st2 = dFdy(fragUV1);\n\
\n\
	vec3 N = normalize(fragNormal);\n\
	vec3 T = normalize(q1 * st2.t - q2 * st1.t);\n\
	vec3 B = -normalize(cross(N, T));\n\
	mat3 TBN = mat3(T, B, N);\n\
\n\
	return normalize(TBN * tangentNormal);\n\
}\n\
float convertMetallic(vec3 diffuse, vec3 specular, float maxSpecular) {\n\
	float perceivedDiffuse = sqrt(0.299f * diffuse.r * diffuse.r + 0.587f * diffuse.g * diffuse.g + 0.114f * diffuse.b * diffuse.b);\n\
	float perceivedSpecular = sqrt(0.299f * specular.r * specular.r + 0.587f * specular.g * specular.g + 0.114f * specular.b * specular.b);\n\
	if (perceivedSpecular < c_MinRoughness) {\n\
		return 0.0;\n\
	}\n\
	float a = c_MinRoughness;\n\
	float b = perceivedDiffuse * (1.0f - maxSpecular) / (1.0f - c_MinRoughness) + perceivedSpecular - 2.0f * c_MinRoughness;\n\
	float c = c_MinRoughness - perceivedSpecular;\n\
	float D = max(b * b - 4.0 * a * c, 0.0f);\n\
	return clamp((-b + sqrt(D)) / (2.0f * a), 0.0f, 1.0f);\n\
}\n\
layout(location = 0) out vec4 outNormalDepth;\n\
layout(location = 1) out vec4 outMetallic;\n\
void main()\n\
{\n\
	float metallic;\n\
	vec3 n = (mat.normalUV > -1) ? getNormal() : normalize(fragNormal);\n\
	if (mat.workflow == PBR_WORKFLOW_METALLIC_ROUGHNESS) {\n\
		metallic = mat.metallicFactor;\n\
		if (mat.metallicRoughnessUV > -1) {\n\
			vec4 mrSample = texture(metallicRoughnessMap, mat.metallicRoughnessUV == 0 ? fragUV1 : fragUV2);\n\
			metallic = mrSample.b * metallic;\n\
		}\n\
		else {\n\
			metallic = clamp(metallic, 0.0, 1.0);\n\
		}\n\
	}\n\
		vec4 diffuse = texture(colorMap, fragUV1);\n\
		vec3 specular = texture(metallicRoughnessMap, fragUV1).rgb;\n\
		float maxSpecular = max(max(specular.r, specular.g), specular.b);\n\
	if (mat.workflow == PBR_WORKFLOW_SPECULAR_GLOSINESS) {\n\
		metallic = convertMetallic(diffuse.rgb, specular, maxSpecular);\n\
	}\n\
	outNormalDepth = vec4(n, gl_FragCoord.z);\n\
	outMetallic = vec4(metallic);\n\
}";
static const char* outlinePbrVertexShader = "#version 330\n\
layout(location = 0) in vec3 position; \n\
layout(location = 1) in vec3 normal; \n\
layout(location = 2) in vec2 uv1; \n\
layout(location = 3) in vec2 uv2; \n\
layout(location = 4) in vec4 joint; \n\
layout(location = 5) in vec4 weights; \n\
layout(location = 6) in vec4 color; \n\
uniform mat4 model; \n\
uniform mat4 view; \n\
uniform mat4 projection; \n\
uniform vec3 camPos; \n\
#define MAX_NUM_JOINTS 128\n\
layout (std140) uniform BoneData {\n\
	mat4 jointMatrix[MAX_NUM_JOINTS];\n\
	float jointCount;\n\
} bones;\n\
out vec3 worldPos;\n\
out vec3 fragNormal;\n\
out vec2 fragUV1;\n\
out vec2 fragUV2;\n\
out vec4 fragColor;\n\
uniform float thickness = 0.04;\n\
void main(){\n\
	vec4 locPos;\n\
	if (bones.jointCount > 0.0) {\n\
		mat4 boneMat =\n\
			weights.x * bones.jointMatrix[int(joint.x)] +\n\
			weights.y * bones.jointMatrix[int(joint.y)] +\n\
			weights.z * bones.jointMatrix[int(joint.z)] +\n\
			weights.w * bones.jointMatrix[int(joint.w)];\n\
	\n\
		locPos = model * boneMat * vec4(position, 1.0);\n\
		fragNormal = normalize(transpose(inverse(mat3(model * boneMat))) * normal);\n\
	}\n\
	else {\n\
		locPos = model * vec4(position, 1.0);\n\
		fragNormal = normalize(transpose(inverse(mat3(model))) * normal);\n\
	}\n\
	locPos = thickness * vec4(fragNormal, 0.0f) + locPos;\n\
	worldPos = locPos.xyz / locPos.w;\n\
	fragUV1 = uv1;\n\
	fragUV2 = uv2;\n\
	fragColor = color;\n\
	gl_Position = projection * view * vec4(worldPos, 1.0);\n\
}";
static const char* solidColorPbrFragmentShader = "#version 330\n\
in vec3 worldPos;\n\
in vec3 fragNormal;\n\
in vec2 fragUV1;\n\
in vec2 fragUV2;\n\
in vec4 fragColor;\n\
\n\
uniform mat4 model; \n\
uniform mat4 view; \n\
uniform mat4 projection; \n\
uniform vec3 camPos; \n\
uniform vec4 color;\n\
\n\
layout(location = 0) out vec4 outColor;\n\
void main()\n\
{\n\
	outColor = color;\n\
}";











































const char* filterCubeVertShader = "#version 330\n\
layout (location = 0) in vec3 inPos;\n\
uniform mat4 mvp;\n\
out vec3 outUVW;\n\
void main()\n\
{\n\
	outUVW = inPos;\n\
	gl_Position = mvp * vec4(inPos.xyz, 1.0);\n\
}";
const char* filterIrradianceCubeFragmentShader = "#version 330\n\
in vec3 outUVW;\n\
out vec4 outColor;\n\
uniform samplerCube samplerEnv;\n\
uniform float deltaPhi;\n\
uniform float deltaTheta;\n\
#define PI 3.1415926535897932384626433832795\n\
void main()\n\
{\n\
	vec3 N = normalize(outUVW);\n\
	vec3 up = vec3(0.0, 1.0, 0.0);\n\
	vec3 right = normalize(cross(up, N));\n\
	up = cross(N, right);\n\
	const float TWO_PI = PI * 2.0;\n\
	const float HALF_PI = PI * 0.5;\n\
	vec3 color = vec3(0.0);\n\
	uint sampleCount = 0u;\n\
	for (float phi = 0.0; phi < TWO_PI; phi += deltaPhi) {\n\
		for (float theta = 0.0; theta < HALF_PI; theta += deltaTheta) {\n\
			vec3 tempVec = cos(phi) * right + sin(phi) * up;\n\
			vec3 sampleVector = cos(theta) * N + sin(theta) * tempVec;\n\
			color += texture(samplerEnv, sampleVector).rgb * cos(theta) * sin(theta);\n\
			sampleCount++;\n\
		}\n\
	}\n\
	outColor = vec4(PI * color / float(sampleCount), 1.0);\n\
}";
const char* prefilterEnvMapFragmentShader = "#version 330\n\
in vec3 outUVW;\n\
out vec4 outColor;\n\
uniform samplerCube samplerEnv;\n\
uniform float roughness;\n\
uniform uint numSamples;\n\
const float PI = 3.1415926536;\n\
float random(vec2 co)\n\
{\n\
	float a = 12.9898;\n\
	float b = 78.233;\n\
	float c = 43758.5453;\n\
	float dt = dot(co.xy, vec2(a, b));\n\
	float sn = mod(dt, 3.14);\n\
	return fract(sin(sn) * c);\n\
}\n\
vec2 hammersley2d(uint i, uint N)\n\
{\n\
	uint bits = (i << 16u) | (i >> 16u);\n\
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);\n\
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);\n\
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);\n\
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);\n\
	float rdi = float(bits) * 2.3283064365386963e-10;\n\
	return vec2(float(i) / float(N), rdi);\n\
}\n\
vec3 importanceSample_GGX(vec2 Xi, float roughness, vec3 normal)\n\
{\n\
	float alpha = roughness * roughness;\n\
	float phi = 2.0 * PI * Xi.x + random(normal.xz) * 0.1;\n\
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (alpha * alpha - 1.0) * Xi.y));\n\
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);\n\
	vec3 H = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);\n\
	vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);\n\
	vec3 tangentX = normalize(cross(up, normal));\n\
	vec3 tangentY = normalize(cross(normal, tangentX));\n\
	return normalize(tangentX * H.x + tangentY * H.y + normal * H.z);\n\
}\n\
float D_GGX(float dotNH, float roughness)\n\
{\n\
	float alpha = roughness * roughness;\n\
	float alpha2 = alpha * alpha;\n\
	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;\n\
	return (alpha2) / (PI * denom * denom);\n\
}\n\
vec3 prefilterEnvMap(vec3 R, float roughness)\n\
{\n\
	vec3 N = R;\n\
	vec3 V = R;\n\
	vec3 color = vec3(0.0);\n\
	float totalWeight = 0.0;\n\
	float envMapDim = float(textureSize(samplerEnv, 0).s);\n\
	for (uint i = 0u; i < numSamples; i++) {\n\
		vec2 Xi = hammersley2d(i, numSamples);\n\
		vec3 H = importanceSample_GGX(Xi, roughness, N);\n\
		vec3 L = 2.0 * dot(V, H) * H - V;\n\
		float dotNL = clamp(dot(N, L), 0.0, 1.0);\n\
		if (dotNL > 0.0) {\n\
			float dotNH = clamp(dot(N, H), 0.0, 1.0);\n\
			float dotVH = clamp(dot(V, H), 0.0, 1.0);\n\
			float pdf = D_GGX(dotNH, roughness) * dotNH / (4.0 * dotVH) + 0.0001;\n\
			float omegaS = 1.0 / (float(numSamples) * pdf);\n\
			float omegaP = 4.0 * PI / (6.0 * envMapDim * envMapDim);\n\
			float mipLevel = roughness == 0.0 ? 0.0 : max(0.5 * log2(omegaS / omegaP) + 1.0, 0.0f);\n\
			color += textureLod(samplerEnv, L, mipLevel).rgb * dotNL;\n\
			totalWeight += dotNL;\n\
		}\n\
	}\n\
	return (color / totalWeight);\n\
}\n\
void main()\n\
{\n\
	vec3 N = normalize(outUVW);\n\
	outColor = vec4(prefilterEnvMap(N, roughness), 1.0);\n\
}";
























































static const char* quadFilterVertexShader = "#version 330\n\
out vec2 UV;\n\
out vec2 pos;\n\
void main()\n\
{\n\
	UV = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);\n\
	pos = UV * 2.0f - 1.0f;\n\
	gl_Position = vec4(pos, 0.0f, 1.0f);\n\
}";
static const char* blurFragmentShader = "#version 330\n\
in vec2 UV;\
in vec2 pos;\
uniform sampler2D tex;\
uniform float blurRadius;\
uniform int axis;\
uniform vec2 texUV;\
out vec4 outCol;\
void main()\
{\
    vec2 textureSize = vec2(textureSize(tex, 0));\
    float x,y,rr=blurRadius*blurRadius,d,w,w0;\
    vec2 p = 0.5 * (vec2(1.0, 1.0) + pos) * texUV;\
    vec4 col = vec4(0.0, 0.0, 0.0, 0.0);\
    w0 = 0.5135 / pow(blurRadius, 0.96);\n\
    if (axis == 0) for (d = 1.0 / textureSize.x, x = -blurRadius, p.x += x * d; x <= blurRadius; x++, p.x += d) { \n\
    w = w0 * exp((-x * x) / (2.0 * rr)); col += texture(tex, p) * w;\n\
 }\n\
    if (axis == 1) for (d = 1.0 / textureSize.y, y = -blurRadius, p.y += y * d; y <= blurRadius; y++, p.y += d) { \n\
    w = w0 * exp((-y * y) / (2.0 * rr)); col += texture(tex, p) * w; \n\
}\n\
    outCol = col;\
}";
static const char* bloomFragmentShader = "#version 330\n\
in vec2 UV;\
in vec2 pos;\
uniform sampler2D tex;\
uniform float blurRadius;\
uniform int axis;\
uniform float intensity;\
uniform float mipLevel;\
out vec4 outCol;\
void main()\
{\
    vec2 textureSize = vec2(textureSize(tex, int(mipLevel)));\
    float x,y,rr=blurRadius*blurRadius,d,w,w0;\
    vec2 p = 0.5 * (vec2(1.0, 1.0) + pos);\
    vec4 col = vec4(0.0, 0.0, 0.0, 0.0);\
    w0 = 0.5135 / pow(blurRadius, 0.96);\n\
    if (axis == 0) for (d = 1.0 / textureSize.x, x = -blurRadius, p.x += x * d; x <= blurRadius; x++, p.x += d) { w = w0 * exp((-x * x) / (2.0 * rr));\
            vec3 addCol = textureLod(tex, p, mipLevel).rgb;\
            vec3 remCol = vec3(1.0f, 1.0f, 1.0f) * intensity;\n\
            addCol = max(addCol - remCol, vec3(0.0f));\
            col += vec4(addCol, 0.0f) * w;\
        }\n\
    if (axis == 1) for (d = 1.0 / textureSize.y, y = -blurRadius, p.y += y * d; y <= blurRadius; y++, p.y += d) { w = w0 * exp((-y * y) / (2.0 * rr));\
            vec3 addCol = textureLod(tex, p, mipLevel).rgb;\
            vec3 remCol = vec3(1.0f, 1.0f, 1.0f) * intensity;\n\
            addCol = max(addCol - remCol, vec3(0.0f));\
            col += vec4(addCol, 0.0f) * w;\
        }\n\
    outCol = col;\
}";
static const char* copyFragmentShader = "#version 330\n\
in vec2 UV;\
in vec2 pos;\
uniform sampler2D tex;\
uniform float mipLevel;\
out vec4 outCol;\
void main()\
{\
    outCol = textureLod(tex, UV, mipLevel);\
}";
static const char* upsamplingFragmentShader = "#version 330\n\
in vec2 UV;\
in vec2 pos;\
uniform sampler2D tex;\
uniform float mipLevel;\
out vec4 outCol;\
void main()\
{\
    vec2 ts = vec2(1.0f) / vec2(textureSize(tex, int(mipLevel)));\
    vec3 c1 = textureLod(tex, UV + vec2(-ts.x, -ts.y), mipLevel).rgb;\
    vec3 c2 = 2.0f * textureLod(tex, UV + vec2(0.0f, -ts.y), mipLevel).rgb;\
    vec3 c3 = textureLod(tex, UV + vec2(ts.x, -ts.y), mipLevel).rgb;\
    vec3 c4 = 2.0f * textureLod(tex, UV + vec2(-ts.x, 0.0f), mipLevel).rgb;\
    vec3 c5 = 4.0f * textureLod(tex, UV + vec2(0.0f, 0.0f), mipLevel).rgb;\
    vec3 c6 = 2.0f * textureLod(tex, UV + vec2(ts.x, 0.0f), mipLevel).rgb;\
    vec3 c7 = textureLod(tex, UV + vec2(-ts.x, ts.y), mipLevel).rgb;\
    vec3 c8 = 2.0f * textureLod(tex, UV + vec2(0.0f, ts.y), mipLevel).rgb;\
    vec3 c9 = textureLod(tex, UV + vec2(ts.x, ts.y), mipLevel).rgb;\
    vec3 accum = 1.0f / 16.0f * (c1 + c2 + c3 + c4 + c5 + c6 + c7 + c8 + c9);\
    outCol = vec4(accum, 1.0f);\
}";
static const char* copyDualFragmentShader = "#version 330\n\
in vec2 UV;\
in vec2 pos;\
uniform sampler2D tex1;\
uniform sampler2D tex2;\
uniform float mipLevel1;\
uniform float mipLevel2;\
uniform float exposure;\
uniform float gamma;\
vec3 aces(vec3 x) {\
    const float a = 2.51;\
    const float b = 0.03;\
    const float c = 2.43;\
    const float d = 0.59;\
    const float e = 0.14;\
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);\
}\
vec3 filmic(vec3 x) {\
    vec3 X = max(vec3(0.0), x - 0.004);\
    vec3 result = (X * (6.2 * X + 0.5)) / (X * (6.2 * X + 1.7) + 0.06);\
    return pow(result, vec3(2.2));\
}\
vec3 Uncharted2Tonemap(vec3 color)\
{\
	float A = 0.15;\
	float B = 0.50;\
	float C = 0.10;\
	float D = 0.20;\
	float E = 0.02;\
	float F = 0.30;\
	float W = 11.2;\
	return ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;\
}\
vec4 tonemap(vec4 color)\
{\
	vec3 outcol = Uncharted2Tonemap(color.rgb * exposure);\
	outcol = outcol * (1.0f / Uncharted2Tonemap(vec3(11.2f)));\
	return vec4(pow(outcol, vec3(1.0f / gamma)), color.a);\
}\
out vec4 outCol;\
void main()\
{\
    vec4 c = tonemap(textureLod(tex1, UV, mipLevel1) + textureLod(tex2, UV, mipLevel2));\n\
    outCol = c;\
}";
static const char* ssrFragmentShader = "#version 330\n\
in vec2 UV;\n\
in vec2 pos;\n\
out vec4 outColor;\n\
uniform sampler2D textureFrame;\n\
uniform sampler2D textureNormal;\n\
uniform sampler2D textureMetallic;\n\
uniform sampler2D textureDepth;\n\
uniform mat4 proj;\n\
uniform mat4 invProj;\n\
uniform mat4 view;\n\
uniform mat4 invView;\n\
uniform float rayStep = 0.1f;\n\
uniform int iterationCount = 100;\n\
uniform float distanceBias = 0.03f;\n\
uniform bool enableSSR = true;\n\
uniform int sampleCount = 4;\n\
uniform bool isSamplingEnabled = false;\n\
uniform bool isExponentialStepEnabled = false;\n\
uniform bool isAdaptiveStepEnabled = false;\n\
uniform bool isBinarySearchEnabled = true;\n\
uniform bool debugDraw = false;\n\
uniform float samplingCoefficient = 0.001f;\n\
float random(vec2 uv) {\n\
	return fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453123); //simple random function\n\
}\n\
vec3 generatePositionFromDepth(vec2 texturePos, float depth) {\n\
	vec4 ndc = vec4((texturePos - 0.5) * 2, depth, 1.f);\n\
	vec4 inversed = invProj * ndc;// going back from projected\n\
	inversed /= inversed.w;\n\
	return inversed.xyz;\n\
}\n\
vec2 generateProjectedPosition(vec3 pos) {\n\
	vec4 samplePosition = proj * vec4(pos, 1.f);\n\
	samplePosition.xy = (samplePosition.xy / samplePosition.w) * 0.5 + 0.5;\n\
	return samplePosition.xy;\n\
}\n\
vec3 SSR(vec3 position, vec3 reflection) {\n\
	vec3 step = rayStep * reflection;\n\
	vec3 marchingPosition = position + step;\n\
	float delta;\n\
	float depthFromScreen;\n\
	vec2 screenPosition;\n\
	int i = 0;\n\
	for (; i < iterationCount; i++) {\n\
		screenPosition = generateProjectedPosition(marchingPosition);\n\
		depthFromScreen = abs(generatePositionFromDepth(screenPosition, texture(textureDepth, screenPosition).x).z);\n\
		delta = abs(marchingPosition.z) - depthFromScreen;\n\
		if (abs(delta) < distanceBias) {\n\
			vec3 color = vec3(1);\n\
			if (debugDraw)\n\
				color = vec3(0.5 + sign(delta) / 2, 0.3, 0.5 - sign(delta) / 2);\n\
			return texture(textureFrame, screenPosition).xyz * color;\n\
		}\n\
		if (isBinarySearchEnabled && delta > 0) {\n\
			break;\n\
		}\n\
		if (isAdaptiveStepEnabled) {\n\
			float directionSign = sign(abs(marchingPosition.z) - depthFromScreen);\n\
			step = step * (1.0 - rayStep * max(directionSign, 0.0));\n\
			marchingPosition += step * (-directionSign);\n\
		}\n\
		else {\n\
			marchingPosition += step;\n\
		}\n\
		if (isExponentialStepEnabled) {\n\
			step *= 1.05;\n\
		}\n\
	}\n\
	if (isBinarySearchEnabled) {\n\
		for (; i < iterationCount; i++) {\n\
			step *= 0.5;\n\
			marchingPosition = marchingPosition - step * sign(delta);\n\
			screenPosition = generateProjectedPosition(marchingPosition);\n\
			depthFromScreen = abs(generatePositionFromDepth(screenPosition, texture(textureDepth, screenPosition).x).z);\n\
			delta = abs(marchingPosition.z) - depthFromScreen;\n\
			if (abs(delta) < distanceBias) {\n\
				vec3 color = vec3(1);\n\
				if (debugDraw)\n\
					color = vec3(0.5 + sign(delta) / 2, 0.3, 0.5 - sign(delta) / 2);\n\
				return texture(textureFrame, screenPosition).xyz * color;\n\
			}\n\
		}\n\
	}\n\
	return vec3(0.0);\n\
}\n\
void main() {\n\
	vec3 position = generatePositionFromDepth(UV, texture(textureDepth, UV).x);\n\
	vec4 normal = view * vec4(normalize(texture(textureNormal, UV).xyz), 0.0);\n\
	float metallic = texture(textureMetallic, UV).r;\n\
	if (!enableSSR || metallic < 0.01) {\n\
		outColor = texture(textureFrame, UV);\n\
	}\n\
	else {\n\
		vec3 reflectionDirection = normalize(reflect(position, normalize(normal.xyz)));\n\
		if (isSamplingEnabled) {\n\
			vec3 firstBasis = normalize(cross(vec3(0.f, 0.f, 1.f), reflectionDirection));\n\
			vec3 secondBasis = normalize(cross(reflectionDirection, firstBasis));\n\
			vec4 resultingColor = vec4(0.f);\n\
			for (int i = 0; i < sampleCount; i++) {\n\
				vec2 coeffs = vec2(random(UV + vec2(0, i)) + random(UV + vec2(i, 0))) * samplingCoefficient;\n\
				vec3 reflectionDirectionRandomized = reflectionDirection + firstBasis * coeffs.x + secondBasis * coeffs.y;\n\
				vec3 tempColor = SSR(position, normalize(reflectionDirectionRandomized));\n\
				if (tempColor != vec3(0.f)) {\n\
					resultingColor += vec4(tempColor, 1.f);\n\
				}\n\
			}\n\
			if (resultingColor.w == 0) {\n\
				outColor = texture(textureFrame, UV);\n\
			}\n\
			else {\n\
				resultingColor /= resultingColor.w;\n\
				outColor = vec4(resultingColor.xyz, 1.f);\n\
			}\n\
		}\n\
		else {\n\
			outColor = vec4(SSR(position, normalize(reflectionDirection)), 1.f);\n\
			if (outColor.xyz == vec3(0.f)) {\n\
				outColor = texture(textureFrame, UV);\n\
			}\n\
		}\n\
	}\n\
}";
static const char* ssaoFragmentShader = "#version 330\n\
in vec2 UV;\n\
in vec2 pos;\n\
\n\
uniform sampler2D textureColor;\n\
uniform sampler2D textureDepth;\n\
uniform vec2 resolution;\n\
uniform float zNear;\n\
uniform float zFar;\n\
uniform float strength;\n\
uniform int samples;\n\
uniform float radius;\n\
\n\
float aoclamp = 0.125; //depth clamp - reduces haloing at screen edges\n\
bool noise = true; //use noise instead of pattern for sample dithering\n\
float noiseamount = 0.0002; //dithering amount\n\
float diffarea = 0.3; //self-shadowing reduction\n\
float gdisplace = 0.4; //gauss bell center //0.4\n\
bool mist = false; //use mist?\n\
float miststart = 0.0; //mist start\n\
float mistend = zFar; //mist end\n\
\n\
#define PI 3.14159265\n\
float width = resolution.x;\n\
float height = resolution.y;\n\
vec2 texCoord = UV;\n\
vec2 rand(vec2 coord)\n\
{\n\
	float noiseX = ((fract(1.0 - coord.s * (width / 2.0)) * 0.25) + (fract(coord.t * (height / 2.0)) * 0.75)) * 2.0 - 1.0;\n\
	float noiseY = ((fract(1.0 - coord.s * (width / 2.0)) * 0.75) + (fract(coord.t * (height / 2.0)) * 0.25)) * 2.0 - 1.0;\n\
	if (noise)\n\
	{\n\
		noiseX = clamp(fract(sin(dot(coord, vec2(12.9898, 78.233))) * 43758.5453), 0.0, 1.0) * 2.0 - 1.0;\n\
		noiseY = clamp(fract(sin(dot(coord, vec2(12.9898, 78.233) * 2.0)) * 43758.5453), 0.0, 1.0) * 2.0 - 1.0;\n\
	}\n\
	return vec2(noiseX, noiseY) * noiseamount;\n\
}\n\
float doMist()\n\
{\n\
	float zdepth = texture2D(textureDepth, texCoord.xy).x;\n\
	float depth = -zFar * zNear / (zdepth * (zFar - zNear) - zFar);\n\
	return clamp((depth - miststart) / mistend, 0.0, 1.0);\n\
}\n\
float readDepth(vec2 coord)\n\
{\n\
	if (UV.x < 0.0 || UV.y < 0.0) return 1.0;\n\
	else {\n\
		float z_b = texture2D(textureDepth, coord).x;\n\
		float z_n = 2.0 * z_b - 1.0;\n\
		return (2.0 * zNear) / (zFar + zNear - z_n * (zFar - zNear));\n\
	}\n\
}\n\
int compareDepthsFar(float depth1, float depth2) {\n\
	float garea = 2.0;\n\
	float diff = (depth1 - depth2) * 100.0;\n\
	if (diff < gdisplace)\n\
	{\n\
		return 0;\n\
	}\n\
	else {\n\
		return 1;\n\
	}\n\
}\n\
float compareDepths(float depth1, float depth2)\n\
{\n\
	float garea = 2.0;\n\
	float diff = (depth1 - depth2) * 100.0;\n\
	if (diff < gdisplace)\n\
	{\n\
		garea = diffarea;\n\
	}\n\
	float gauss = pow(2.7182, -2.0 * (diff - gdisplace) * (diff - gdisplace) / (garea * garea));\n\
	return gauss;\n\
}\n\
float calAO(float depth,float dw, float dh)\n\
{\n\
	float dd = (1.0 - depth) * radius;\n\
	float temp = 0.0;\n\
	float temp2 = 0.0;\n\
	float coordw = UV.x + dw * dd;\n\
	float coordh = UV.y + dh * dd;\n\
	float coordw2 = UV.x - dw * dd;\n\
	float coordh2 = UV.y - dh * dd;\n\
	vec2 coord = vec2(coordw, coordh);\n\
	vec2 coord2 = vec2(coordw2, coordh2);\n\
	float cd = readDepth(coord);\n\
	int far = compareDepthsFar(depth, cd);\n\
	temp = compareDepths(depth, cd);\n\
	if (far > 0)\n\
	{\n\
		temp2 = compareDepths(readDepth(coord2), depth);\n\
		temp += (1.0 - temp) * temp2;\n\
	}\n\
	return temp;\n\
}\n\
void main(void)\n\
{\n\
	vec2 noise = rand(texCoord);\n\
	float depth = readDepth(texCoord);\n\
	float w = (1.0 / width) / clamp(depth, aoclamp, 1.0) + (noise.x * (1.0 - noise.x));\n\
	float h = (1.0 / height) / clamp(depth, aoclamp, 1.0) + (noise.y * (1.0 - noise.y));\n\
	float pw = 0.0;\n\
	float ph = 0.0;\n\
	float ao = 0.0;\n\
	float dl = PI * (3.0 - sqrt(5.0));\n\
	float dz = 1.0 / float(samples);\n\
	float l = 0.0;\n\
	float z = 1.0 - dz / 2.0;\n\
	for (int i = 0; i < 64; i++)\n\
	{\n\
		if (i > samples) break;\n\
		float r = sqrt(1.0 - z);\n\
		pw = cos(l) * r;\n\
		ph = sin(l) * r;\n\
		ao += calAO(depth, pw * w, ph * h);\n\
		z = z - dz;\n\
		l = l + dl;\n\
	}\n\
	ao /= float(samples);\n\
	ao *= strength;\n\
	ao = 1.0 - ao;\n\
	if (mist)\n\
	{\n\
		ao = mix(ao, 1.0, doMist());\n\
	}\n\
	gl_FragColor = vec4(texture(textureColor, texCoord).rgb, 1.0f) * ao;\n\
}\n\
";















static GLuint CreateProgram(const char* vtxShader, const char* frgShader)
{
	GLuint out = glCreateProgram();

	GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vertShader, 1, &vtxShader, NULL);
	glShaderSource(fragShader, 1, &frgShader, NULL);

	glCompileShader(vertShader);

	GLint isCompiled = 0;
	glGetShaderiv(vertShader, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetShaderiv(vertShader, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		char* errorLog = new char[maxLength];
		glGetShaderInfoLog(vertShader, maxLength, &maxLength, &errorLog[0]);

		LOG("FAILED TO COMPILE VERTEXSHADER: %s\n", errorLog);
		delete[] errorLog;

		glDeleteShader(vertShader);
	}

	glCompileShader(fragShader);
	glGetShaderiv(fragShader, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetShaderiv(fragShader, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		char* errorLog = new char[maxLength];
		glGetShaderInfoLog(fragShader, maxLength, &maxLength, &errorLog[0]);

		LOG("FAILED TO COMPILE FRAGMENTSHADER: %s\n", errorLog);
		delete[] errorLog;

		glDeleteShader(fragShader);
	}


	glAttachShader(out, vertShader);
	glAttachShader(out, fragShader);

	glLinkProgram(out);
	glDetachShader(out, vertShader);
	glDetachShader(out, fragShader);
	glDeleteShader(vertShader);
	glDeleteShader(fragShader);

	return out;
}
static GLuint CreateBRDFLut(uint32_t width, uint32_t height)
{
	const char* vertShader = "#version 330\n\
	out vec2 outUV;\n\
	void main()\n\
	{\n\
		outUV = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);\n\
		gl_Position = vec4(outUV * 2.0f - 1.0f, 0.0f, 1.0f);\n\
	}";
	const char* fragShader = "#version 330\n\
	in vec2 outUV;\n\
	out vec4 outColor;\n\
	const uint NUM_SAMPLES = 1024u;\n\
	const float PI = 3.1415926536;\n\
	float random(vec2 co)\n\
	{\n\
		float a = 12.9898;\n\
		float b = 78.233;\n\
		float c = 43758.5453;\n\
		float dt = dot(co.xy, vec2(a, b));\n\
		float sn = mod(dt, 3.14);\n\
		return fract(sin(sn) * c);\n\
	}\n\
	vec2 hammersley2d(uint i, uint N)\n\
	{\n\
		uint bits = (i << 16u) | (i >> 16u);\n\
		bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);\n\
		bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);\n\
		bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);\n\
		bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);\n\
		float rdi = float(bits) * 2.3283064365386963e-10;\n\
		return vec2(float(i) / float(N), rdi);\n\
	}\n\
	vec3 importanceSample_GGX(vec2 Xi, float roughness, vec3 normal)\n\
	{\n\
		float alpha = roughness * roughness;\n\
		float phi = 2.0 * PI * Xi.x + random(normal.xz) * 0.1;\n\
		float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (alpha * alpha - 1.0) * Xi.y));\n\
		float sinTheta = sqrt(1.0 - cosTheta * cosTheta);\n\
		vec3 H = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);\n\
		vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);\n\
		vec3 tangentX = normalize(cross(up, normal));\n\
		vec3 tangentY = normalize(cross(normal, tangentX));\n\
		return normalize(tangentX * H.x + tangentY * H.y + normal * H.z);\n\
	}\n\
	float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)\n\
	{\n\
		float k = (roughness * roughness) / 2.0;\n\
		float GL = dotNL / (dotNL * (1.0 - k) + k);\n\
		float GV = dotNV / (dotNV * (1.0 - k) + k);\n\
		return GL * GV;\n\
	}\n\
	vec2 BRDF(float NoV, float roughness)\n\
	{\n\
		const vec3 N = vec3(0.0, 0.0, 1.0);\n\
		vec3 V = vec3(sqrt(1.0 - NoV * NoV), 0.0, NoV);\n\
		vec2 LUT = vec2(0.0);\n\
		for (uint i = 0u; i < NUM_SAMPLES; i++) {\n\
			vec2 Xi = hammersley2d(i, NUM_SAMPLES);\n\
			vec3 H = importanceSample_GGX(Xi, roughness, N);\n\
			vec3 L = 2.0 * dot(V, H) * H - V;\n\
			float dotNL = max(dot(N, L), 0.0);\n\
			float dotNV = max(dot(N, V), 0.0);\n\
			float dotVH = max(dot(V, H), 0.0);\n\
			float dotNH = max(dot(H, N), 0.0);\n\
			if (dotNL > 0.0) {\n\
				float G = G_SchlicksmithGGX(dotNL, dotNV, roughness);\n\
				float G_Vis = (G * dotVH) / (dotNH * dotNV);\n\
				float Fc = pow(1.0 - dotVH, 5.0);\n\
				LUT += vec2((1.0 - Fc) * G_Vis, Fc * G_Vis);\n\
			}\n\
		}\n\
		return LUT / float(NUM_SAMPLES);\n\
	}\n\
	void main()\n\
	{\n\
		outColor = vec4(BRDF(outUV.s, 1.0 - outUV.t), 0.0, 1.0);\n\
	}";

	GLuint program = CreateProgram(vertShader, fragShader);

	GLuint framebuffer;
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

	GLuint brdfLut;
	glGenTextures(1, &brdfLut);
	glBindTexture(GL_TEXTURE_2D, brdfLut);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLut, 0);

	GLenum drawBuffers = GL_COLOR_ATTACHMENT0;
	glDrawBuffers(1, &drawBuffers);
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		LOG("FAILED TO CREATE FRAMEBUFFER OBJECT, STATUS: %d\n", status);
		return 0;
	}


	glDisable(GL_DEPTH_TEST);
	glUseProgram(program);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glViewport(0, 0, width, height);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	glDeleteProgram(program);
	glDeleteFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return brdfLut;
}

struct BaseProgramUniforms
{
	GLuint modelLoc;
	GLuint viewLoc;
	GLuint projLoc;
	GLuint camPosLoc;
	GLuint boneDataLoc;
	GLuint materialDataLoc;
	GLuint prefilteredCubeMipLevelsLoc;
	GLuint scaleIBLAmbientLoc;
	GLuint lightDataLoc;
	struct BoundData
	{
		GLuint bBone = 0;
		GLuint bMaterial = 0;
		GLuint bPrefilter = 0;
		GLuint bIrradiance = 0;
		GLuint bBrdfLut = 0;
		GLuint bShadow = 0;
		GLuint bLight = 0;
		Material* bMat = nullptr;
	}bound;
};
struct BaseGeometryProgramUniforms
{
	GLuint modelLoc;
	GLuint viewLoc;
	GLuint projLoc;
	GLuint camPosLoc;
	GLuint boneDataLoc;
	GLuint materialDataLoc;
	struct BoundData
	{
		GLuint bBone = 0;
		GLuint bMaterial = 0;
		Material* bMat = nullptr;
	}bound;
};
struct BaseSSRProgramUniforms
{
	GLuint modelLoc;
	GLuint viewLoc;
	GLuint projLoc;
	GLuint camPosLoc;
	GLuint boneDataLoc;
	GLuint materialDataLoc;
	struct BoundData
	{
		GLuint bBone = 0;
		GLuint bMaterial = 0;
		Material* bMat = nullptr;
	}bound;
};
struct BaseOutlineProgramUniforms
{
	GLuint modelLoc;
	GLuint viewLoc;
	GLuint projLoc;
	GLuint camPosLoc;
	GLuint boneDataLoc;
	GLuint thicknessLoc;
	GLuint colorLoc;
	struct BoundData
	{
		GLuint bBone = 0;
	}bound;
};

enum BASE_SHADER_TEXTURE
{
	BASE_SHADER_TEXTURE_SAMPLER_IRRADIANCE,
	BASE_SHADER_TEXTURE_PREFILTERED_MAP,
	BASE_SHADER_TEXTURE_BRDFLUT,
	BASE_SHADER_TEXTURE_COLORMAP,
	BASE_SHADER_TEXTURE_NORMALMAP,
	BASE_SHADER_TEXTURE_AOMAP,
	BASE_SHADER_TEXTURE_EMISSIVEMAP,
	BASE_SHADER_TEXTURE_METALLIC_ROUGHNESS,
	BASE_SHADER_TEXTURE_SHADOW_MAP,
	NUM_BASE_SHADER_TEXTURES,
};

struct CubemapRenderInfo
{
	GLuint vao;
	GLuint vtxBuf;
	GLuint program;
	GLuint viewProjLoc;

	GLuint irradianceFilterProgram;
	GLuint irradianceFilterMVPLoc;
	GLuint irradianceFilterdeltaPhiLoc;
	GLuint irradianceFilterdeltaThetaLoc;

	GLuint preFilterProgram;
	GLuint preFilterMVPLoc;
	GLuint preFilterRoughness;
	GLuint preFilterNumSamples;
};

struct PBRRenderInfo
{
	GLuint baseProgram;
	BaseProgramUniforms baseUniforms;
	GLuint geomProgram;
	BaseGeometryProgramUniforms geomUniforms;
	GLuint ssrProgram;
	BaseSSRProgramUniforms ssrUniforms;
	GLuint outlineProgram;
	BaseOutlineProgramUniforms outlineUniforms;
	GLuint defaultBoneData;
	GLuint brdfLut;
	GLuint defMaterial;
};
struct PostProcessingRenderInfo
{
	enum class BLUR_AXIS
	{
		X_AXIS,
		Y_AXIS,
	};
	struct Blur
	{
		GLuint program;
		GLuint radiusLoc;
		GLuint axisLoc;
		GLuint textureUVLoc;
	}blur;
	struct Bloom
	{
		GLuint program;
		GLuint blurRadiusLoc;
		GLuint blurAxisLoc;
		GLuint intensityLoc;
		GLuint mipLevelLoc;
	}bloom;
	struct Copy
	{
		GLuint program;
		GLuint mipLevelLoc;
	}copy;
	struct DualCopy
	{
		GLuint program;
		GLuint mipLevel1Loc;
		GLuint mipLevel2Loc;
		GLuint exposureLoc;
		GLuint gammaLoc;
	}dualCopy;
	struct Upsampling
	{
		GLuint program;
		GLuint mipLevelLoc;
	}upsampling;
	struct ScreenSpaceReflection
	{
		GLuint program;
		GLuint projLoc;
		GLuint invProjLoc;
		GLuint viewLoc;
		GLuint invViewLoc;
		GLuint rayStepLoc;
		GLuint iterationCountLoc;
		GLuint distanceBiasLoc;
		GLuint enableSSRLoc;
		GLuint sampleCountLoc;
		GLuint isSamplingEnabledLoc;
		GLuint isExponentialStepEnabledLoc;
		GLuint isAdaptiveStepEnabledLoc;
		GLuint isBinarySearchEnabledLoc;
		GLuint debugDrawLoc;
		GLuint samplingCoefficientLoc;
	}ssr;
	struct ScreenSpaceAmbientOcclusion
	{
		GLuint program;
		GLuint resolutionLoc;
		GLuint zNearLoc;
		GLuint zFarLoc;
		GLuint strengthLoc;
		GLuint samplesLoc;
		GLuint radiusLoc;
	}ssao;
};

struct ProjectionData
{
	glm::mat4 proj;
	glm::vec2 start;
	glm::vec2 end;
};
struct LightData
{
	PointLight pointLights[MAX_NUM_LIGHTS];
	DirectionalLight dirLights[MAX_NUM_LIGHTS];
	SpotLight spotLights[MAX_NUM_LIGHTS];
	ProjectionData projections[MAX_NUM_LIGHTS];
	glm::vec4 ambientColor;
	int numPointLights;
	int numDirLights;
	int numSpotLights;
	int numProjections;
};

struct MappedRect
{
	uint16_t x;
	uint16_t y;
	uint16_t w;
	uint16_t h;
};

struct InternalDirLight
{
	DirShadowLight light;
	MappedRect map[4];
	bool useCascades;
	bool hasShadow;
	bool isActive;
};
struct InternalPointLight
{
	PointShadowLight light;
	MappedRect map[6];
	bool hasShadow;
	bool isActive;
};
struct InternalSpotLight
{
	SpotShadowLight light;
	MappedRect mapped;
	bool hasShadow;
	bool isActive;
};

struct ShadowLightGroup
{
	static constexpr uint32_t shadowTextureSize = 0x2000;
	GLuint fbo;
	GLuint texture;
	uint32_t numUsedProjections;
	ProjectionData projections[MAX_NUM_LIGHTS];
};

struct LightGroup
{
	ShadowLightGroup shadowGroup;
	glm::vec4 ambient;
	InternalDirLight dirLights[MAX_NUM_LIGHTS];
	InternalPointLight pointLights[MAX_NUM_LIGHTS];
	InternalSpotLight spotLights[MAX_NUM_LIGHTS];

	InternalDirLight* dirs[MAX_NUM_LIGHTS];
	InternalPointLight* points[MAX_NUM_LIGHTS];
	InternalSpotLight* spots[MAX_NUM_LIGHTS];

	uint32_t numDirLights;
	uint32_t numPointLights;
	uint32_t numSpotLights;

	GLuint uniform;
};


struct RendererBindInfo
{
	GLuint curProgram;
	GLuint curBoundVao;
	GLuint curBoundIbo;

};

struct Renderer
{
	const CameraBase* currentCam;
	const EnvironmentData* currentEnvironmentData;
	LightGroup* currentLightData;
	PBRRenderInfo pbrData;
	CubemapRenderInfo cubemapInfo;
	PostProcessingRenderInfo ppInfo;
	std::vector<LightGroup*> lightGroups;

	GLuint whiteTexture;	// these are not owned by the renderer
	GLuint blackTexture;	// these are not owned by the renderer

	uint32_t cmdlistCapacity;
	uint32_t numCmds;
	uint32_t firstTransparentCmd;
	RenderCommand** drawCmds;

	RendererBindInfo bindInfo;
};





static ShadowLightGroup* AddShadowLightGroup(LightGroup* data)
{
	if (!data->shadowGroup.fbo)
	{
		memset(&data->shadowGroup, 0, sizeof(ShadowLightGroup));

		glGenFramebuffers(1, &data->shadowGroup.fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, data->shadowGroup.fbo);

		glGenTextures(1, &data->shadowGroup.texture);
		glBindTexture(GL_TEXTURE_2D, data->shadowGroup.texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, ShadowLightGroup::shadowTextureSize, ShadowLightGroup::shadowTextureSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		
		GLfloat borderColor = 10.0f;
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, &borderColor);


		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);	// THIS HERE IS WIERD TAKE A LOOK AT IT LATER MAYBE
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);	// THIS HERE IS WIERD TAKE A LOOK AT IT LATER MAYBE
		
	
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, data->shadowGroup.texture, 0);

		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			LOG("FAILED TO CREATE INTERMEDIATE FRAMEBUFFER\n");
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	return &data->shadowGroup;
}
static void PackShadowLights(LightGroup* g)
{
	using namespace rectpack2D;
	constexpr bool allow_flip = false;
	const auto runtime_flipping_mode = rectpack2D::flipping_option::DISABLED;
	using spaces_type = rectpack2D::empty_spaces<allow_flip, rectpack2D::default_empty_spaces>;
	using rect_type = output_rect_t<spaces_type>;
	auto report_successful = [](rect_type&) { return callback_result::CONTINUE_PACKING; };
	auto report_unsuccessful = [](rect_type&) { return callback_result::ABORT_PACKING; };
	const auto max_side = ShadowLightGroup::shadowTextureSize;
	const auto discard_step = -4;
	std::vector<rect_type> rectangles;


	for (uint32_t i = 0; i < g->numDirLights; i++)
	{
		const InternalDirLight& l = *g->dirs[i];
		if (l.hasShadow)
		{
			if (l.useCascades)
			{
				for (int j = 0; j < 4; j++)
				{
					rectangles.emplace_back(0, 0, l.map[j].w, l.map[j].h);
				}
			}
			else
			{
				rectangles.emplace_back(0, 0, l.map->w, l.map->h);
			}
		}
	}
	for (uint32_t i = 0; i < g->numPointLights; i++)
	{
		const InternalPointLight& l = *g->points[i];
		if (l.hasShadow)
		{
			for (int j = 0; j < 6; j++)
			{
				rectangles.emplace_back(0, 0, l.map[j].w, l.map[j].h);
			}
		}
	}
	for (uint32_t i = 0; i < g->numSpotLights; i++)
	{
		const InternalSpotLight& l = *g->spots[i];
		if (l.hasShadow)
		{
			rectangles.emplace_back(0, 0, l.mapped.w, l.mapped.h);
		}
	}

	auto report_result = [&rectangles](const rect_wh& result_size) {
		LOG("shadow map bin: %d, %d\n", result_size.w, result_size.h);
		for (const auto& r : rectangles) {
			LOG("shadow_rectangles: %d %d %d %d\n", r.x, r.y, r.w, r.h);
		}
	};
	const auto result_size = find_best_packing<spaces_type>(rectangles, make_finder_input(max_side, discard_step, report_successful, report_unsuccessful, runtime_flipping_mode));
	report_result(result_size);

	int curRecIdx = 0;
	for (uint32_t i = 0; i < g->numDirLights; i++)
	{
		InternalDirLight& l = *g->dirs[i];
		if (l.hasShadow)
		{
			if (l.useCascades)
			{
				for (int j = 0; j < 4; j++)
				{
					const auto& r = rectangles.at(curRecIdx);
					l.map[j].x = r.x; l.map[j].y = r.y; l.map[j].w = r.w; l.map[j].h = r.h;
					curRecIdx++;
				}
			}
			else
			{
				const auto& r = rectangles.at(curRecIdx);
				l.map->x = r.x; l.map->y = r.y; l.map->w = r.w; l.map->h = r.h;
				curRecIdx++;
			}
		}
	}
	for (uint32_t i = 0; i < g->numPointLights; i++)
	{
		InternalPointLight& l = *g->points[i];
		if (l.hasShadow)
		{
			for (int j = 0; j < 6; j++)
			{
				const auto& r = rectangles.at(curRecIdx);
				l.map[j].x = r.x; l.map[j].y = r.y; l.map[j].w = r.w; l.map[j].h = r.h;
				curRecIdx++;
			}
		}
	}
	for (uint32_t i = 0; i < g->numSpotLights; i++)
	{
		InternalSpotLight& l = *g->spots[i];
		if (l.hasShadow)
		{
			const auto& r = rectangles.at(curRecIdx);
			l.mapped.x = r.x; l.mapped.y = r.y; l.mapped.w = r.w; l.mapped.h = r.h;
			curRecIdx++;
		}
	}
}
static void CheckShadowGroupAndDeleteIfEmpty(LightGroup* g)
{
	if (g->shadowGroup.numUsedProjections > 0) return;

	glDeleteTextures(1, &g->shadowGroup.texture);
	glDeleteFramebuffers(1, &g->shadowGroup.fbo);
	g->shadowGroup.texture = 0;
	g->shadowGroup.fbo = 0;
}



static void BindMaterial(Renderer* r, Material* mat)
{
	if (r->pbrData.baseUniforms.bound.bMat == mat) return;
	r->pbrData.baseUniforms.bound.bMat = mat;
	if (mat)
	{
		glBindBufferBase(GL_UNIFORM_BUFFER, r->pbrData.baseUniforms.materialDataLoc, mat->uniform);
		
		glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_COLORMAP);
		if (mat->tex.baseColor) glBindTexture(GL_TEXTURE_2D, mat->tex.baseColor->uniform);
		else glBindTexture(GL_TEXTURE_2D, r->whiteTexture);
		glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_NORMALMAP);
		if (mat->tex.normal) glBindTexture(GL_TEXTURE_2D, mat->tex.normal->uniform);
		else glBindTexture(GL_TEXTURE_2D, r->whiteTexture);
		glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_AOMAP);
		if (mat->tex.ao) glBindTexture(GL_TEXTURE_2D, mat->tex.ao->uniform);
		else glBindTexture(GL_TEXTURE_2D, r->whiteTexture);
		glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_EMISSIVEMAP);
		if (mat->tex.emissive) glBindTexture(GL_TEXTURE_2D, mat->tex.emissive->uniform);
		else glBindTexture(GL_TEXTURE_2D, r->whiteTexture);
		glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_METALLIC_ROUGHNESS);
		if (mat->tex.emissive) glBindTexture(GL_TEXTURE_2D, mat->tex.metallicRoughness->uniform);
		else glBindTexture(GL_TEXTURE_2D, r->whiteTexture);
	}
	else
	{
		glBindBufferBase(GL_UNIFORM_BUFFER, r->pbrData.baseUniforms.materialDataLoc, r->pbrData.defMaterial);
		glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_COLORMAP);
		glBindTexture(GL_TEXTURE_2D, r->whiteTexture);
		glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_NORMALMAP);
		glBindTexture(GL_TEXTURE_2D, r->whiteTexture);
		glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_AOMAP);
		glBindTexture(GL_TEXTURE_2D, r->whiteTexture);
		glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_EMISSIVEMAP);
		glBindTexture(GL_TEXTURE_2D, r->whiteTexture);
		glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_METALLIC_ROUGHNESS);
		glBindTexture(GL_TEXTURE_2D, r->whiteTexture);
	}
}
static void BindMaterialGeometry(Renderer* r, Material* mat)
{
	if (r->pbrData.geomUniforms.bound.bMat == mat) return;
	r->pbrData.geomUniforms.bound.bMat = mat;
	if (mat)
	{
		glBindBufferBase(GL_UNIFORM_BUFFER, r->pbrData.geomUniforms.materialDataLoc, mat->uniform);
		glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_COLORMAP);
		if (mat->tex.baseColor) glBindTexture(GL_TEXTURE_2D, mat->tex.baseColor->uniform);
		else glBindTexture(GL_TEXTURE_2D, r->whiteTexture);
	}
	else
	{
		glBindBufferBase(GL_UNIFORM_BUFFER, r->pbrData.geomUniforms.materialDataLoc, r->pbrData.defMaterial);
		glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_COLORMAP);
		glBindTexture(GL_TEXTURE_2D, r->whiteTexture);
	}
}
static void BindMaterialSSR(Renderer* r, Material* mat)
{
	if (r->pbrData.ssrUniforms.bound.bMat == mat) return;
	r->pbrData.ssrUniforms.bound.bMat = mat;
	if (mat)
	{
		glBindBufferBase(GL_UNIFORM_BUFFER, r->pbrData.ssrUniforms.materialDataLoc, mat->uniform);
		glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_COLORMAP);
		if (mat->tex.baseColor) glBindTexture(GL_TEXTURE_2D, mat->tex.baseColor->uniform);
		else glBindTexture(GL_TEXTURE_2D, r->whiteTexture);
		glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_NORMALMAP);
		if (mat->tex.normal) glBindTexture(GL_TEXTURE_2D, mat->tex.normal->uniform);
		else glBindTexture(GL_TEXTURE_2D, r->whiteTexture);
		glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_METALLIC_ROUGHNESS);
		if (mat->tex.metallicRoughness) glBindTexture(GL_TEXTURE_2D, mat->tex.metallicRoughness->uniform);
		else glBindTexture(GL_TEXTURE_2D, r->whiteTexture);
	}
	else
	{
		glBindBufferBase(GL_UNIFORM_BUFFER, r->pbrData.ssrUniforms.materialDataLoc, r->pbrData.defMaterial);
		glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_COLORMAP);
		glBindTexture(GL_TEXTURE_2D, r->whiteTexture);
		glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_NORMALMAP);
		glBindTexture(GL_TEXTURE_2D, r->whiteTexture);
		glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_METALLIC_ROUGHNESS);
		glBindTexture(GL_TEXTURE_2D, r->whiteTexture);
	}
}
static BaseProgramUniforms LoadBaseProgramUniformsLocationsFromProgram(GLuint program)
{
	BaseProgramUniforms un;
	un.modelLoc = glGetUniformLocation(program, "model");
	un.viewLoc = glGetUniformLocation(program, "view");
	un.projLoc = glGetUniformLocation(program, "projection");
	un.camPosLoc = glGetUniformLocation(program, "camPos");

	un.prefilteredCubeMipLevelsLoc = glGetUniformLocation(program, "prefilteredCubeMipLevels");
	un.scaleIBLAmbientLoc = glGetUniformLocation(program, "scaleIBLAmbient");

	un.boneDataLoc = glGetUniformBlockIndex(program, "BoneData");
	glUniformBlockBinding(program, un.boneDataLoc, un.boneDataLoc);
	un.materialDataLoc = glGetUniformBlockIndex(program, "Material");
	glUniformBlockBinding(program, un.materialDataLoc, un.materialDataLoc);
	un.lightDataLoc = glGetUniformBlockIndex(program, "LightData");
	glUniformBlockBinding(program, un.lightDataLoc, un.lightDataLoc);



	glUseProgram(program);
	GLint curTexture = glGetUniformLocation(program, "samplerIrradiance");
	if (curTexture == -1) { LOG("failed to Get samplerIrradiance Texture Loaction of GLTF shader program\n"); }
	glUniform1i(curTexture, BASE_SHADER_TEXTURE_SAMPLER_IRRADIANCE);
	curTexture = glGetUniformLocation(program, "prefilteredMap");
	if (curTexture == -1) { LOG("failed to Get prefilteredMap Texture Loaction of GLTF shader program\n"); }
	glUniform1i(curTexture, BASE_SHADER_TEXTURE_PREFILTERED_MAP);
	curTexture = glGetUniformLocation(program, "samplerBRDFLUT");
	if (curTexture == -1) { LOG("failed to Get samplerBRDFLUT Texture Loaction of GLTF shader program\n"); }
	glUniform1i(curTexture, BASE_SHADER_TEXTURE_BRDFLUT);
	curTexture = glGetUniformLocation(program, "colorMap");
	if (curTexture == -1) { LOG("failed to Get colorMap Texture Loaction of GLTF shader program\n"); }
	glUniform1i(curTexture, BASE_SHADER_TEXTURE_COLORMAP);
	curTexture = glGetUniformLocation(program, "normalMap");
	if (curTexture == -1) { LOG("failed to Get normalMap Texture Loaction of GLTF shader program\n"); }
	glUniform1i(curTexture, BASE_SHADER_TEXTURE_NORMALMAP);
	curTexture = glGetUniformLocation(program, "aoMap");
	if (curTexture == -1) { LOG("failed to Get aoMap Texture Loaction of GLTF shader program\n"); }
	glUniform1i(curTexture, BASE_SHADER_TEXTURE_AOMAP);
	curTexture = glGetUniformLocation(program, "emissiveMap");
	if (curTexture == -1) { LOG("failed to Get emissiveMap Texture Loaction of GLTF shader program\n"); }
	glUniform1i(curTexture, BASE_SHADER_TEXTURE_EMISSIVEMAP);
	curTexture = glGetUniformLocation(program, "metallicRoughnessMap");
	if (curTexture == -1) { LOG("failed to Get metallicRoughnessMap Texture Loaction of GLTF shader program\n"); }
	glUniform1i(curTexture, BASE_SHADER_TEXTURE_METALLIC_ROUGHNESS);
	curTexture = glGetUniformLocation(program, "shadowMap");
	if (curTexture == -1) { LOG("failed to Get shadowMap Texture Loaction of GLTF shader program\n"); }
	glUniform1i(curTexture, BASE_SHADER_TEXTURE_SHADOW_MAP);

	return un;
}
static BaseGeometryProgramUniforms LoadBaseGeometryProgramUniformLocationsFromProgram(GLuint program)
{
	BaseGeometryProgramUniforms un;
	un.modelLoc = glGetUniformLocation(program, "model");
	un.viewLoc = glGetUniformLocation(program, "view");
	un.projLoc = glGetUniformLocation(program, "projection");
	un.camPosLoc = glGetUniformLocation(program, "camPos");

	un.boneDataLoc = glGetUniformBlockIndex(program, "BoneData");
	glUniformBlockBinding(program, un.boneDataLoc, un.boneDataLoc);
	un.materialDataLoc = glGetUniformBlockIndex(program, "Material");
	glUniformBlockBinding(program, un.materialDataLoc, un.materialDataLoc);

	glUseProgram(program);
	GLuint curTexture = glGetUniformLocation(program, "colorMap");
	if (curTexture == -1) { LOG("failed to Get colorMap Texture Loaction of GLTF shader program\n"); }
	glUniform1i(curTexture, BASE_SHADER_TEXTURE_COLORMAP);
	return un;
}
static BaseSSRProgramUniforms LoadBaseSSRProgramUniformLocationsFromProgram(GLuint program)
{
	BaseSSRProgramUniforms un;
	un.modelLoc = glGetUniformLocation(program, "model");
	un.viewLoc = glGetUniformLocation(program, "view");
	un.projLoc = glGetUniformLocation(program, "projection");
	un.camPosLoc = glGetUniformLocation(program, "camPos");

	un.boneDataLoc = glGetUniformBlockIndex(program, "BoneData");
	glUniformBlockBinding(program, un.boneDataLoc, un.boneDataLoc);
	un.materialDataLoc = glGetUniformBlockIndex(program, "Material");
	glUniformBlockBinding(program, un.materialDataLoc, un.materialDataLoc);

	glUseProgram(program);
	GLuint curTexture = glGetUniformLocation(program, "colorMap");
	if (curTexture == -1) { LOG("failed to Get colorMap Texture Loaction of GLTF shader program\n"); }
	glUniform1i(curTexture, BASE_SHADER_TEXTURE_COLORMAP);
	curTexture = glGetUniformLocation(program, "normalMap");
	if (curTexture == -1) { LOG("failed to Get normalMap Texture Loaction of GLTF shader program\n"); }
	glUniform1i(curTexture, BASE_SHADER_TEXTURE_NORMALMAP);
	curTexture = glGetUniformLocation(program, "metallicRoughnessMap");
	if (curTexture == -1) { LOG("failed to Get metallicRoughnessMap Texture Loaction of GLTF shader program\n"); }
	glUniform1i(curTexture, BASE_SHADER_TEXTURE_METALLIC_ROUGHNESS);
	return un;
}
static BaseOutlineProgramUniforms LoadBaseOutlineProgramUniformLocationsFromProgram(GLuint program)
{
	BaseOutlineProgramUniforms un;
	un.modelLoc = glGetUniformLocation(program, "model");
	un.viewLoc = glGetUniformLocation(program, "view");
	un.projLoc = glGetUniformLocation(program, "projection");
	un.camPosLoc = glGetUniformLocation(program, "camPos");
	un.thicknessLoc = glGetUniformLocation(program, "thickness");
	un.colorLoc = glGetUniformLocation(program, "color");
	un.boneDataLoc = glGetUniformBlockIndex(program, "BoneData");
	glUniformBlockBinding(program, un.boneDataLoc, un.boneDataLoc);
	return un;
}
static void LoadPostProcessingRenderInfo(PostProcessingRenderInfo* info)
{
	info->blur.program = CreateProgram(quadFilterVertexShader, blurFragmentShader);
	info->bloom.program = CreateProgram(quadFilterVertexShader, bloomFragmentShader);
	info->copy.program = CreateProgram(quadFilterVertexShader, copyFragmentShader);
	info->dualCopy.program = CreateProgram(quadFilterVertexShader, copyDualFragmentShader);
	info->upsampling.program = CreateProgram(quadFilterVertexShader, upsamplingFragmentShader);
	info->ssr.program = CreateProgram(quadFilterVertexShader, ssrFragmentShader);
	info->ssao.program = CreateProgram(quadFilterVertexShader, ssaoFragmentShader);

	info->blur.radiusLoc = glGetUniformLocation(info->blur.program, "blurRadius");
	info->blur.axisLoc = glGetUniformLocation(info->blur.program, "axis");
	info->blur.textureUVLoc = glGetUniformLocation(info->blur.program, "texUV");

	info->bloom.blurRadiusLoc = glGetUniformLocation(info->bloom.program, "blurRadius");
	info->bloom.blurAxisLoc = glGetUniformLocation(info->bloom.program, "axis");
	info->bloom.intensityLoc = glGetUniformLocation(info->bloom.program, "intensity");
	info->bloom.mipLevelLoc = glGetUniformLocation(info->bloom.program, "mipLevel");

	info->copy.mipLevelLoc = glGetUniformLocation(info->copy.program, "mipLevel");

	info->dualCopy.mipLevel1Loc = glGetUniformLocation(info->dualCopy.program, "mipLevel1");
	info->dualCopy.mipLevel2Loc = glGetUniformLocation(info->dualCopy.program, "mipLevel2");
	info->dualCopy.exposureLoc = glGetUniformLocation(info->dualCopy.program, "exposure");
	info->dualCopy.gammaLoc = glGetUniformLocation(info->dualCopy.program, "gamma");

	glUseProgram(info->dualCopy.program);
	GLuint idx = glGetUniformLocation(info->dualCopy.program, "tex1");
	glUniform1i(idx, 0);
	idx = glGetUniformLocation(info->dualCopy.program, "tex2");
	glUniform1i(idx, 1);

	info->upsampling.mipLevelLoc = glGetUniformLocation(info->upsampling.program, "mipLevel");



	info->ssr.projLoc = glGetUniformLocation(info->ssr.program, "proj");
	info->ssr.invProjLoc = glGetUniformLocation(info->ssr.program, "invProj");
	info->ssr.viewLoc = glGetUniformLocation(info->ssr.program, "view");
	info->ssr.invViewLoc = glGetUniformLocation(info->ssr.program, "invView");
	info->ssr.rayStepLoc = glGetUniformLocation(info->ssr.program, "rayStep");
	info->ssr.iterationCountLoc = glGetUniformLocation(info->ssr.program, "iterationCount");
	info->ssr.distanceBiasLoc = glGetUniformLocation(info->ssr.program, "distanceBias");
	info->ssr.enableSSRLoc = glGetUniformLocation(info->ssr.program, "enableSSR");
	info->ssr.sampleCountLoc = glGetUniformLocation(info->ssr.program, "sampleCount");
	info->ssr.isSamplingEnabledLoc = glGetUniformLocation(info->ssr.program, "isSamplingEnabled");
	info->ssr.isExponentialStepEnabledLoc = glGetUniformLocation(info->ssr.program, "isExponentialStepEnabled");
	info->ssr.isAdaptiveStepEnabledLoc = glGetUniformLocation(info->ssr.program, "isAdaptiveStepEnabled");
	info->ssr.isBinarySearchEnabledLoc = glGetUniformLocation(info->ssr.program, "isBinarySearchEnabled");
	info->ssr.debugDrawLoc = glGetUniformLocation(info->ssr.program, "debugDraw");
	info->ssr.samplingCoefficientLoc = glGetUniformLocation(info->ssr.program, "samplingCoefficient");

	glUseProgram(info->ssr.program);
	idx = glGetUniformLocation(info->ssr.program, "textureFrame");
	glUniform1i(idx, 0);
	idx = glGetUniformLocation(info->ssr.program, "textureNormal");
	glUniform1i(idx, 1);
	idx = glGetUniformLocation(info->ssr.program, "textureMetallic");
	glUniform1i(idx, 2);
	idx = glGetUniformLocation(info->ssr.program, "textureDepth");
	glUniform1i(idx, 3);


	info->ssao.resolutionLoc = glGetUniformLocation(info->ssao.program, "resolution");
	info->ssao.zNearLoc = glGetUniformLocation(info->ssao.program, "zNear");
	info->ssao.zFarLoc = glGetUniformLocation(info->ssao.program, "zFar");
	info->ssao.strengthLoc = glGetUniformLocation(info->ssao.program, "strength");
	info->ssao.samplesLoc = glGetUniformLocation(info->ssao.program, "samples");
	info->ssao.radiusLoc = glGetUniformLocation(info->ssao.program, "radius");

	glUseProgram(info->ssao.program);
	idx = glGetUniformLocation(info->ssao.program, "textureColor");
	glUniform1i(idx, 0);
	idx = glGetUniformLocation(info->ssao.program, "textureDepth");
	glUniform1i(idx, 1);

}
static void CleanUpPostProcessingRenderInfo(PostProcessingRenderInfo* info)
{
	glDeleteProgram(info->blur.program);
	glDeleteProgram(info->bloom.program);
	glDeleteProgram(info->copy.program);
	glDeleteProgram(info->dualCopy.program);
	glDeleteProgram(info->upsampling.program);
	glDeleteProgram(info->ssr.program);
	glDeleteProgram(info->ssao.program);
}




struct Renderer* RE_CreateRenderer(struct AssetManager* assets)
{
	Renderer* renderer = new Renderer;
	renderer->currentCam = nullptr;
	renderer->currentEnvironmentData = nullptr;
	renderer->currentLightData = 0;
	renderer->cmdlistCapacity = 0;
	renderer->drawCmds = 0;
	renderer->numCmds = 0;
	renderer->firstTransparentCmd = 0;
	// Create Material Defaults
	{
		BoneData boneData = {};
		glGenBuffers(1, &renderer->pbrData.defaultBoneData);
		glBindBuffer(GL_UNIFORM_BUFFER, renderer->pbrData.defaultBoneData);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(BoneData), &boneData, GL_STATIC_DRAW);

		renderer->whiteTexture = assets->textures[DEFAULT_WHITE_TEXTURE]->uniform;
		renderer->blackTexture = assets->textures[DEFAULT_BLACK_TEXTURE]->uniform;

		MaterialData defaultMaterial; memset(&defaultMaterial, 0, sizeof(defaultMaterial));
		defaultMaterial.emissiveFactor = glm::vec4(1.0f);
		defaultMaterial.specularFactor = glm::vec4(0.0f);
		defaultMaterial.diffuseFactor = glm::vec4(0.0f);
		defaultMaterial.emissiveUV = 0;
		defaultMaterial.normalUV = -1;
		defaultMaterial.baseColorUV = -1;
		defaultMaterial.aoUV = -1;
		defaultMaterial.metallicRoughnessUV = -1;

		glGenBuffers(1, &renderer->pbrData.defMaterial);
		glBindBuffer(GL_UNIFORM_BUFFER, renderer->pbrData.defMaterial);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(defaultMaterial), &defaultMaterial, GL_STATIC_DRAW);

	}
	// Create Main 3d shader
	{
		renderer->pbrData.baseProgram = CreateProgram(baseVertexShader, baseFragmentShader);
		renderer->pbrData.baseUniforms = LoadBaseProgramUniformsLocationsFromProgram(renderer->pbrData.baseProgram);

		renderer->pbrData.geomProgram = CreateProgram(baseVertexShader, baseGeometryFragmentShader);
		renderer->pbrData.geomUniforms = LoadBaseGeometryProgramUniformLocationsFromProgram(renderer->pbrData.geomProgram);

		renderer->pbrData.ssrProgram = CreateProgram(baseVertexShader, ssrPbrFragmentShader);
		renderer->pbrData.ssrUniforms = LoadBaseSSRProgramUniformLocationsFromProgram(renderer->pbrData.ssrProgram);

		renderer->pbrData.outlineProgram = CreateProgram(outlinePbrVertexShader, solidColorPbrFragmentShader);
		renderer->pbrData.outlineUniforms = LoadBaseOutlineProgramUniformLocationsFromProgram(renderer->pbrData.outlineProgram);

	}
	// CREATE SKYBOX SHADER DATA
	{
		static const char* cubemapVS = "#version 330\n\
		layout (location = 0) in vec3 aPos;\n\
		out vec3 TexCoords;\n\
		uniform mat4 viewProj;\n\
		void main(){\
			TexCoords = aPos;\
			vec4 pos = viewProj * vec4(aPos, 0.0);\n\
			gl_Position = pos.xyww;\
		}";
		static const char* cubemapFS = "#version 330\n\
		in vec3 TexCoords;\n\
		out vec4 FragColor;\n\
		uniform samplerCube skybox;\n\
		void main()\
		{\
			FragColor = textureLod(skybox, TexCoords, 0);\n\
		}";
		static glm::vec3 cubeVertices[] = {
			{-1.0f, 1.0f, 1.0f},{-1.0f,-1.0f, 1.0f},{-1.0f,-1.0f,-1.0f},
			{-1.0f, 1.0f,-1.0f},{-1.0f, 1.0f, 1.0f},{-1.0f,-1.0f,-1.0f},

			{1.0f,-1.0f,-1.0f},{-1.0f,-1.0f,-1.0f},{1.0f,-1.0f, 1.0f},
			{-1.0f,-1.0f,-1.0f},{-1.0f,-1.0f, 1.0f},{1.0f,-1.0f, 1.0f},

			{1.0f,-1.0f, 1.0f},{-1.0f,-1.0f, 1.0f},{-1.0f, 1.0f, 1.0f},
			{1.0f,-1.0f, 1.0f},{-1.0f, 1.0f, 1.0f},{1.0f, 1.0f, 1.0f},

			{-1.0f,-1.0f,-1.0f},{1.0f,-1.0f,-1.0f},{1.0f, 1.0f,-1.0f},
			{-1.0f, 1.0f,-1.0f},{-1.0f,-1.0f,-1.0f},{1.0f, 1.0f,-1.0f},

			{1.0f, 1.0f,-1.0f},{1.0f,-1.0f,-1.0f},{1.0f, 1.0f, 1.0f},
			{1.0f,-1.0f, 1.0f},{1.0f, 1.0f, 1.0f},{1.0f,-1.0f,-1.0f},

			{-1.0f, 1.0f, 1.0f},{-1.0f, 1.0f,-1.0f},{1.0f, 1.0f, 1.0f},
			{-1.0f, 1.0f,-1.0f},{1.0f, 1.0f,-1.0f},{1.0f, 1.0f, 1.0f},
		};
		renderer->cubemapInfo.program = CreateProgram(cubemapVS, cubemapFS);
		renderer->cubemapInfo.viewProjLoc = glGetUniformLocation(renderer->cubemapInfo.program, "viewProj");
		glUseProgram(renderer->cubemapInfo.program);
		GLint curTexture = glGetUniformLocation(renderer->cubemapInfo.program, "skybox");
		glUniform1i(curTexture, 0);


		renderer->cubemapInfo.irradianceFilterProgram = CreateProgram(filterCubeVertShader, filterIrradianceCubeFragmentShader);
		renderer->cubemapInfo.irradianceFilterMVPLoc = glGetUniformLocation(renderer->cubemapInfo.irradianceFilterProgram, "mvp");
		renderer->cubemapInfo.irradianceFilterdeltaPhiLoc = glGetUniformLocation(renderer->cubemapInfo.irradianceFilterProgram, "deltaPhi");
		renderer->cubemapInfo.irradianceFilterdeltaThetaLoc = glGetUniformLocation(renderer->cubemapInfo.irradianceFilterProgram, "deltaTheta");

		renderer->cubemapInfo.preFilterProgram = CreateProgram(filterCubeVertShader, prefilterEnvMapFragmentShader);
		renderer->cubemapInfo.preFilterMVPLoc = glGetUniformLocation(renderer->cubemapInfo.preFilterProgram, "mvp");
		renderer->cubemapInfo.preFilterRoughness = glGetUniformLocation(renderer->cubemapInfo.preFilterProgram, "roughness");
		renderer->cubemapInfo.preFilterNumSamples = glGetUniformLocation(renderer->cubemapInfo.preFilterProgram, "numSamples");
		
		glGenVertexArrays(1, &renderer->cubemapInfo.vao);
		glGenBuffers(1, &renderer->cubemapInfo.vtxBuf);
		glBindBuffer(GL_ARRAY_BUFFER, renderer->cubemapInfo.vtxBuf);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

		glBindVertexArray(renderer->cubemapInfo.vao);

		glBindBuffer(GL_ARRAY_BUFFER, renderer->cubemapInfo.vtxBuf);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (const void*)0);
		glEnableVertexAttribArray(0);

		glBindVertexArray(0);

	}

	LoadPostProcessingRenderInfo(&renderer->ppInfo);


	renderer->pbrData.brdfLut = CreateBRDFLut(512, 512);
	return renderer;
}
void RE_DestroyRenderer(struct Renderer* renderer)
{
	CleanUpPostProcessingRenderInfo(&renderer->ppInfo);

	glDeleteTextures(1, &renderer->pbrData.brdfLut);
	glDeleteBuffers(1, &renderer->pbrData.defaultBoneData);
	// delete base Shader
	{
		glDeleteProgram(renderer->pbrData.baseProgram);
		glDeleteProgram(renderer->pbrData.geomProgram);
	}
	// Delete cubemap infos
	{
		glDeleteBuffers(1, &renderer->cubemapInfo.vtxBuf);
		glDeleteVertexArrays(1, &renderer->cubemapInfo.vao);
		glDeleteProgram(renderer->cubemapInfo.program);
		glDeleteProgram(renderer->cubemapInfo.irradianceFilterProgram);
	}
	if (renderer->drawCmds) delete[] renderer->drawCmds;
	renderer->drawCmds = nullptr;
	delete renderer;
}

void RE_CreateEnvironment(struct Renderer* renderer, EnvironmentData* env, uint32_t irrWidth, uint32_t irrHeight)
{
	// SETTINGS
	constexpr float deltaPhi = (2.0f * float(M_PI)) / 180.0f;
	constexpr float deltaTheta = (0.5f * float(M_PI)) / 64.0f;
	constexpr uint32_t numSamples = 32;

	static constexpr glm::vec3 cubeDirs[6] = {
		{ 1.0f,  0.0f,  0.0f},
		{-1.0f,  0.0f,  0.0f},
		{ 0.0f,  1.0f,  0.0f},
		{ 0.0f, -1.0f,  0.0f},
		{ 0.0f,  0.0f,  1.0f},
		{ 0.0f,  0.0f, -1.0f},

	};

	uint32_t numMipLevels = 1 + static_cast<uint32_t>(floor(log2(glm::max(env->width, env->height))));
	env->mipLevels = numMipLevels;
	env->irradianceMapWidth = irrWidth;
	env->irradianceMapHeight = irrHeight;
	GLuint framebuffer;
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);


	PerspectiveCamera perspectiveCam;
	memset(&perspectiveCam, 0, sizeof(PerspectiveCamera));
	CA_InitPerspectiveCamera(&perspectiveCam, { 0.0f, 0.0f, 0.0f }, 90.0f, static_cast<float>(env->width), static_cast<float>(env->height));

	RE_SetCameraBase(renderer, &perspectiveCam.base);
	RE_SetEnvironmentData(renderer, env);

	// CREATE IRRADIANCE MAP
	{
		glGenTextures(1, &env->irradianceMap);
		glBindTexture(GL_TEXTURE_CUBE_MAP, env->irradianceMap);
		for (unsigned int i = 0; i < 6; i++)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, irrWidth, irrHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, env->irradianceMap, 0);


		GLenum drawBuffers = GL_COLOR_ATTACHMENT0;
		glDrawBuffers(1, &drawBuffers);
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			LOG("FAILED TO CREATE FRAMEBUFFER OBJECT, STATUS: %d\n", status);
			return;
		}

		glViewport(0, 0, irrWidth, irrHeight);
		CA_UpdatePerspectiveCamera(&perspectiveCam, cubeDirs[0]); perspectiveCam.base.view = glm::rotate(glm::mat4(1.0f), 3.14159f, glm::vec3(0.0f, 0.0f, 1.0f)) * perspectiveCam.base.view;
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, env->irradianceMap, 0);
		RE_RenderIrradiance(renderer, deltaPhi, deltaTheta, CUBE_MAP_FRONT);


		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + 1, env->irradianceMap, 0);
		CA_UpdatePerspectiveCamera(&perspectiveCam, cubeDirs[1]); perspectiveCam.base.view = glm::rotate(glm::mat4(1.0f), 3.14159f, glm::vec3(0.0f, 0.0f, 1.0f)) * perspectiveCam.base.view;
		RE_RenderIrradiance(renderer, deltaPhi, deltaTheta, CUBE_MAP_BACK);


		CA_UpdatePerspectiveCamera(&perspectiveCam, cubeDirs[2]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + 2, env->irradianceMap, 0);
		RE_RenderIrradiance(renderer, deltaPhi, deltaTheta, CUBE_MAP_TOP);


		CA_UpdatePerspectiveCamera(&perspectiveCam, cubeDirs[3]); perspectiveCam.base.view = glm::rotate(glm::mat4(1.0f), 3.14159f, glm::vec3(0.0f, 0.0f, 1.0f)) * perspectiveCam.base.view;
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + 3, env->irradianceMap, 0);
		RE_RenderIrradiance(renderer, deltaPhi, deltaTheta, CUBE_MAP_BOTTOM);


		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + 4, env->irradianceMap, 0);
		CA_UpdatePerspectiveCamera(&perspectiveCam, cubeDirs[4]); perspectiveCam.base.view = glm::rotate(glm::mat4(1.0f), 3.14159f, glm::vec3(0.0f, 0.0f, 1.0f)) * perspectiveCam.base.view;
		RE_RenderIrradiance(renderer, deltaPhi, deltaTheta, CUBE_MAP_RIGHT);


		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + 5, env->irradianceMap, 0);
		CA_UpdatePerspectiveCamera(&perspectiveCam, cubeDirs[5]); perspectiveCam.base.view = glm::rotate(glm::mat4(1.0f), 3.14159f, glm::vec3(0.0f, 0.0f, 1.0f)) * perspectiveCam.base.view;
		RE_RenderIrradiance(renderer, deltaPhi, deltaTheta, CUBE_MAP_LEFT);
		
	}

	// CREATE PREFILTER MAP
	{
		glGenTextures(1, &env->prefilteredMap);
		glBindTexture(GL_TEXTURE_CUBE_MAP, env->prefilteredMap);
		for (unsigned int i = 0; i < 6; i++)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, env->width, env->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, env->prefilteredMap, 0);

		GLenum drawBuffers = GL_COLOR_ATTACHMENT0;
		glDrawBuffers(1, &drawBuffers);
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			LOG("FAILED TO CREATE FRAMEBUFFER OBJECT, STATUS: %d\n", status);
			return;
		}

		for (uint32_t i = 0; i < numMipLevels; i++)
		{
			float roughness = (float)i / (float)(numMipLevels - 1);
			glViewport(0, 0, glm::max(env->width >> i, 1u), glm::max(env->height >> i, 1u));
			CA_UpdatePerspectiveCamera(&perspectiveCam, cubeDirs[0]); perspectiveCam.base.view = glm::rotate(glm::mat4(1.0f), 3.14159f, glm::vec3(0.0f, 0.0f, 1.0f)) * perspectiveCam.base.view;
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, env->prefilteredMap, i);
			RE_RenderPreFilterCubeMap(renderer, roughness, numSamples, CUBE_MAP_FRONT);


			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + 1, env->prefilteredMap, i);
			CA_UpdatePerspectiveCamera(&perspectiveCam, cubeDirs[1]); perspectiveCam.base.view = glm::rotate(glm::mat4(1.0f), 3.14159f, glm::vec3(0.0f, 0.0f, 1.0f)) * perspectiveCam.base.view;
			RE_RenderPreFilterCubeMap(renderer, roughness, numSamples, CUBE_MAP_BACK);


			CA_UpdatePerspectiveCamera(&perspectiveCam, cubeDirs[2]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + 2, env->prefilteredMap, i);
			RE_RenderPreFilterCubeMap(renderer, roughness, numSamples, CUBE_MAP_TOP);


			CA_UpdatePerspectiveCamera(&perspectiveCam, cubeDirs[3]); perspectiveCam.base.view = glm::rotate(glm::mat4(1.0f), 3.14159f, glm::vec3(0.0f, 0.0f, 1.0f)) * perspectiveCam.base.view;
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + 3, env->prefilteredMap, i);
			RE_RenderPreFilterCubeMap(renderer, roughness, numSamples, CUBE_MAP_BOTTOM);


			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + 4, env->prefilteredMap, i);
			CA_UpdatePerspectiveCamera(&perspectiveCam, cubeDirs[4]); perspectiveCam.base.view = glm::rotate(glm::mat4(1.0f), 3.14159f, glm::vec3(0.0f, 0.0f, 1.0f)) * perspectiveCam.base.view;
			RE_RenderPreFilterCubeMap(renderer, roughness, numSamples, CUBE_MAP_RIGHT);


			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + 5, env->prefilteredMap, i);
			CA_UpdatePerspectiveCamera(&perspectiveCam, cubeDirs[5]); perspectiveCam.base.view = glm::rotate(glm::mat4(1.0f), 3.14159f, glm::vec3(0.0f, 0.0f, 1.0f)) * perspectiveCam.base.view;
			RE_RenderPreFilterCubeMap(renderer, roughness, numSamples, CUBE_MAP_LEFT);
		}
	}

	glDeleteFramebuffers(1, &framebuffer);
}
void RE_CleanUpEnvironment(EnvironmentData* env)
{
	glDeleteTextures(1, &env->irradianceMap);
	glDeleteTextures(1, &env->prefilteredMap);
	env->irradianceMap = 0;
	env->prefilteredMap = 0;
}




void RE_CreateAntialiasingData(AntialiasingRenderData* data, uint32_t width, uint32_t height, uint32_t numSamples)
{
	data->width = width;
	data->height = height;
	data->msaaCount = numSamples;
	if (numSamples > 0)
	{

		// only initialize depth if it is asked to finish the antialiased data (thats what i call it now)
		data->intermediateDepth = 0;
		data->intermediateFbo = 0;
		data->intermediateTexture = 0;

		glGenFramebuffers(1, &data->fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, data->fbo);

		glGenTextures(1, &data->texture);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, data->texture);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, numSamples, GL_RGBA16F, width, height, GL_TRUE);
		glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, data->texture, 0);

		glGenTextures(1, &data->depth);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, data->depth);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, numSamples, GL_DEPTH24_STENCIL8, width, height, GL_TRUE);
		glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, data->depth, 0);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			LOG("FAILED TO CREATE ANTIALIASING FRAMEBUFFER\n");
		}
	}
	else
	{
		glGenFramebuffers(1, &data->intermediateFbo);
		glBindFramebuffer(GL_FRAMEBUFFER, data->intermediateFbo);

		glGenTextures(1, &data->intermediateTexture);
		glBindTexture(GL_TEXTURE_2D, data->intermediateTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, data->width, data->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, data->intermediateTexture, 0);

		glGenTextures(1, &data->intermediateDepth);
		glBindTexture(GL_TEXTURE_2D, data->intermediateDepth);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, data->width, data->height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, data->intermediateDepth, 0);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			LOG("FAILED TO CREATE INTERMEDIATE FRAMEBUFFER\n");
		}
		data->fbo = data->intermediateFbo;
		data->texture = data->intermediateTexture;
		data->depth = data->intermediateDepth;
	}
}
void RE_FinishAntialiasingData(AntialiasingRenderData* data)
{
	if (data->msaaCount > 0)
	{
		if (!data->intermediateFbo)
		{
			glGenFramebuffers(1, &data->intermediateFbo);
			glBindFramebuffer(GL_FRAMEBUFFER, data->intermediateFbo);

			glGenTextures(1, &data->intermediateTexture);
			glBindTexture(GL_TEXTURE_2D, data->intermediateTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, data->width, data->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, data->intermediateTexture, 0);

			glGenTextures(1, &data->intermediateDepth);
			glBindTexture(GL_TEXTURE_2D, data->intermediateDepth);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, data->width, data->height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, data->intermediateDepth, 0);

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
				LOG("FAILED TO CREATE INTERMEDIATE FRAMEBUFFER\n");
			}
		}
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, data->intermediateFbo);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, data->fbo);
		glBlitFramebuffer(0, 0, data->width, data->height, 0, 0, data->width, data->height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glBlitFramebuffer(0, 0, data->width, data->height, 0, 0, data->width, data->height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	}
}
void RE_CopyAntialiasingDataToFBO(AntialiasingRenderData* data, GLuint dstFbo, uint32_t dstWidth, uint32_t dstHeight)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, data->fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFbo);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glBlitFramebuffer(0, 0, data->width, data->height, 0, 0, dstWidth, dstHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBlitFramebuffer(0, 0, data->width, data->height, 0, 0, dstWidth, dstHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}
void RE_CleanUpAntialiasingData(AntialiasingRenderData* data)
{
	if (data->msaaCount > 0)
	{
		if (data->fbo) glDeleteFramebuffers(1, &data->fbo);
		if (data->texture) glDeleteTextures(1, &data->texture);
		if (data->depth) glDeleteTextures(1, &data->depth);
	}
	if (data->intermediateFbo && data->intermediateTexture && data->intermediateDepth)
	{
		glDeleteFramebuffers(1, &data->intermediateFbo);
		glDeleteTextures(1, &data->intermediateTexture);
		glDeleteTextures(1, &data->depth);
	}
	data->intermediateFbo = 0;
	data->intermediateDepth = 0;
	data->intermediateTexture = 0;
	data->fbo = 0;
	data->texture = 0;
	data->depth = 0;
}

void RE_CreatePostProcessingRenderData(PostProcessingRenderData* data, uint32_t width, uint32_t height)
{
	data->width = width;
	data->height = height;

	data->blurRadius = 4.0f;		// set standard value for blur
	data->bloomIntensity = 1.0f;	// set standard value for bloom
	data->exposure = 4.5f;			// set standard value for exposure
	data->gamma = 1.5f;				// set standard value for gamme


	data->numPPFbos = 1 + static_cast<uint32_t>(floor(log2(glm::max(data->width, data->height))));
	int curX = width; int curY = height;
	for (int i = 1; i < data->numPPFbos; i++)
	{
		curX = glm::max((curX >> 1), 1);
		curY = glm::max((curY >> 1), 1);
		if (curX < 16 && curY < 16 && false)
		{
			data->numPPFbos = i;
			break;
		}
	}
	data->numPPFbos = glm::min(data->numPPFbos, MAX_BLOOM_MIPMAPS);
	{
		data->ppFBOs1 = new GLuint[data->numPPFbos];
		glGenFramebuffers(data->numPPFbos, data->ppFBOs1);

		glGenTextures(1, &data->ppTexture1);
		glBindTexture(GL_TEXTURE_2D, data->ppTexture1);
		glTexStorage2D(GL_TEXTURE_2D, data->numPPFbos, GL_RGBA16F, width, height);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		for (int i = 0; i < data->numPPFbos; i++)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, data->ppFBOs1[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, data->ppTexture1, i);
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
				LOG("FAILED  TO CREATE FRAMEBUFFER\n");
			}
		}
	}
	{
		data->ppFBOs2 = new GLuint[data->numPPFbos];
		glGenFramebuffers(data->numPPFbos, data->ppFBOs2);

		glGenTextures(1, &data->ppTexture2);
		glBindTexture(GL_TEXTURE_2D, data->ppTexture2);
		glTexStorage2D(GL_TEXTURE_2D, data->numPPFbos, GL_RGBA16F, width, height);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		for (int i = 0; i < data->numPPFbos; i++)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, data->ppFBOs2[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, data->ppTexture2, i);
			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
				LOG("FAILED  TO CREATE FRAMEBUFFER\n");
			}
		}

	}

	glGenFramebuffers(1, &data->intermediateFbo);
	glBindFramebuffer(GL_FRAMEBUFFER, data->intermediateFbo);
	glGenTextures(1, &data->intermediateTexture);
	glBindTexture(GL_TEXTURE_2D, data->intermediateTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, data->intermediateTexture, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		LOG("FAILED  TO CREATE FRAMEBUFFER\n");
	}

}
void RE_CleanUpPostProcessingRenderData(PostProcessingRenderData* data)
{
	if(data->intermediateTexture) glDeleteTextures(1, &data->intermediateTexture);
	if (data->intermediateFbo) glDeleteFramebuffers(1, &data->intermediateFbo);
	if(data->ppTexture1) glDeleteTextures(1, &data->ppTexture1);
	if(data->ppTexture2) glDeleteTextures(1, &data->ppTexture2);
	if (data->ppFBOs1)
	{
		glDeleteFramebuffers(data->numPPFbos, data->ppFBOs1);
		delete[] data->ppFBOs1;
	}
	if (data->ppFBOs2)
	{
		glDeleteFramebuffers(data->numPPFbos, data->ppFBOs2);
		delete[] data->ppFBOs2;
	}
	data->numPPFbos = 0;
	data->ppFBOs1 = nullptr;
	data->ppFBOs2 = nullptr;
	data->ppTexture1 = 0;
	data->ppTexture2 = 0;
	data->intermediateTexture = 0;
}

void RE_CreateScreenSpaceReflectionRenderData(ScreenSpaceReflectionRenderData* data, uint32_t width, uint32_t height)
{
	data->width = width;
	data->height = height;
	glGenFramebuffers(1, &data->fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, data->fbo);
	
	glGenTextures(1, &data->normalTexture);
	glBindTexture(GL_TEXTURE_2D, data->normalTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &data->metallicTexture);
	glBindTexture(GL_TEXTURE_2D, data->metallicTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


	glGenTextures(1, &data->depthTexture);
	glBindTexture(GL_TEXTURE_2D, data->depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, data->depthTexture, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		LOG("FAILED TO CREATE INTERMEDIATE FRAMEBUFFER\n");
	}

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, data->depthTexture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, data->normalTexture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, data->metallicTexture, 0);

	GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(1, drawBuffers);
	
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		LOG("FAILED TO CREATE FRAMEBUFFER\n");
	}

}
void RE_CleanUpScreenSpaceReflectionRenderData(ScreenSpaceReflectionRenderData* data)
{
	if (data->normalTexture) glDeleteTextures(1, &data->normalTexture);
	if (data->metallicTexture) glDeleteTextures(1, &data->metallicTexture);
	if (data->depthTexture) glDeleteTextures(1, &data->depthTexture);
	if (data->fbo) glDeleteFramebuffers(1, &data->fbo);

	memset(data, 0, sizeof(ScreenSpaceReflectionRenderData));
}







struct LightGroup* RELI_AddLightGroup(struct Renderer* renderer)
{
	LightGroup* out = new LightGroup;
	memset(out, 0, sizeof(LightGroup));
	renderer->lightGroups.push_back(out);

	LightData emptyLightData{};


	glGenBuffers(1, &out->uniform);
	glBindBuffer(GL_UNIFORM_BUFFER, out->uniform);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(LightData), &emptyLightData, GL_STATIC_DRAW);
	return out;
}
void RELI_RemoveLightGroup(struct Renderer* renderer, struct LightGroup* group)
{
	if (group->shadowGroup.fbo)
	{
		glDeleteFramebuffers(1, &group->shadowGroup.fbo);
		glDeleteTextures(1, &group->shadowGroup.texture);
		group->shadowGroup.fbo = 0;
		group->shadowGroup.texture = 0;
		group->numSpotLights = 0;
	}
	glDeleteBuffers(1, &group->uniform);
	
	delete group;
	for (int i = 0; i < renderer->lightGroups.size(); i++)
	{
		if (renderer->lightGroups.at(i) == group)
		{
			renderer->lightGroups.erase(renderer->lightGroups.begin() + i);
			break;
		}
	}
}
void RELI_Update(struct LightGroup* group, const CameraBase* relativeCam)
{
	LightData data; memset(&data, 0, sizeof(LightData));
	uint32_t curProjIdx = 0;
	for (uint32_t i = 0; i < group->numDirLights; i++)
	{
		InternalDirLight& l = *group->dirs[i];
		data.dirLights[i] = l.light.light;
		if (l.hasShadow)
		{
			const float xStart = (float)l.map[0].x / (float)ShadowLightGroup::shadowTextureSize;
			const float yStart = (float)l.map[0].y / (float)ShadowLightGroup::shadowTextureSize;
			const float xEnd = (float)l.map[0].w / (float)ShadowLightGroup::shadowTextureSize + xStart;
			const float yEnd = (float)l.map[0].h / (float)ShadowLightGroup::shadowTextureSize + yStart;
			glm::mat4 mat;
			if (relativeCam)
			{
				CameraBase output;
				float splitDepth = 0.0f;
				CA_CreateOrthoTightFit(relativeCam, &output, l.light.light.direction, 0.0f, 0.125f, &splitDepth);
				mat = output.proj * output.view;
				l.light.pos = output.pos;
			}
			else
			{
				mat = CA_CreateOrthoView(l.light.pos, l.light.light.direction, 30.0f, 30.0f, 0.1f, 1000.0f);
			}
			data.projections[curProjIdx].proj = mat;
			data.projections[curProjIdx].start = { xStart, yStart };
			data.projections[curProjIdx].end = { xEnd, yEnd };
			group->shadowGroup.projections[curProjIdx] = data.projections[curProjIdx];
			l.light.light.projIdx = curProjIdx;
			data.dirLights[i].projIdx = curProjIdx;
			curProjIdx++;
		}
		else
		{
			data.dirLights[i].projIdx = -1;
		}
	}
	for (uint32_t i = 0; i < group->numPointLights; i++)
	{
		InternalPointLight& l = *group->points[i];
		data.pointLights[i] = l.light.light;
		if (l.hasShadow)
		{
			l.light.light.projIdx = curProjIdx;
			data.pointLights[i].projIdx = curProjIdx;
			for (int j = 0; j < 6; j++)
			{
				static constexpr glm::vec3 pDirs[6] = { 
					{-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f},
					{0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
					{0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 1.0f}
				};
				const float xStart = (float)l.map[j].x / (float)ShadowLightGroup::shadowTextureSize;
				const float yStart = (float)l.map[j].y / (float)ShadowLightGroup::shadowTextureSize;
				const float xEnd = (float)l.map[j].w / (float)ShadowLightGroup::shadowTextureSize + xStart;
				const float yEnd = (float)l.map[j].h / (float)ShadowLightGroup::shadowTextureSize + yStart;
				const glm::mat4 mat = CA_CreatePerspectiveView(l.light.light.pos, pDirs[j], static_cast<float>(M_PI_2), l.map[j].w, l.map[j].h, 0.1f, 1000.0f);
				data.projections[curProjIdx].proj = mat;
				data.projections[curProjIdx].start = { xStart, yStart };
				data.projections[curProjIdx].end = { xEnd, yEnd };
				group->shadowGroup.projections[curProjIdx] = data.projections[curProjIdx];
				curProjIdx++;
			}
		}
		else
		{
			data.pointLights[i].projIdx = -1;
		}
	}
	for (uint32_t i = 0; i < group->numSpotLights; i++)
	{
		InternalSpotLight& l = *group->spots[i];
		data.spotLights[i] = l.light.light;
		if (l.hasShadow)
		{
			const float xStart = (float)l.mapped.x / (float)ShadowLightGroup::shadowTextureSize;
			const float yStart = (float)l.mapped.y / (float)ShadowLightGroup::shadowTextureSize;
			const float xEnd = (float)l.mapped.w / (float)ShadowLightGroup::shadowTextureSize + xStart;
			const float yEnd = (float)l.mapped.h / (float)ShadowLightGroup::shadowTextureSize + yStart;
			const glm::mat4 mat = CA_CreatePerspectiveView(l.light.light.pos, l.light.light.direction, static_cast<float>(M_PI) - l.light.light.cutOff, l.mapped.w, l.mapped.h, 0.1f, 1000.0f);
			data.projections[curProjIdx].proj = mat;
			data.projections[curProjIdx].start = { xStart, yStart };
			data.projections[curProjIdx].end = { xEnd, yEnd };
			group->shadowGroup.projections[curProjIdx] = data.projections[curProjIdx];
			l.light.light.projIdx = curProjIdx;
			data.spotLights[i].projIdx = curProjIdx;
			curProjIdx++;
		}
		else
		{
			data.spotLights[i].projIdx = -1;
		}
	}

	data.numPointLights = group->numPointLights;
	data.numDirLights = group->numDirLights;
	data.numSpotLights = group->numSpotLights;

	data.ambientColor = group->ambient;

	glBindBuffer(GL_UNIFORM_BUFFER, group->uniform);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(LightData), &data, GL_STATIC_DRAW);
}

void RELI_SetAmbientLightColor(struct LightGroup* group, const glm::vec3& col)
{
	group->ambient = { col.r, col.g, col.b, 1.0f };
}

DirectionalLight* RELI_AddDirectionalLight(struct LightGroup* group)
{
	if (group->numDirLights >= MAX_NUM_LIGHTS) return nullptr;
	
	for (int i = 0; i < MAX_NUM_LIGHTS; i++)
	{
		if (!group->dirLights[i].isActive)
		{
			DirectionalLight* l = &group->dirLights[i].light.light;
			group->dirLights[i].isActive = true;
			group->dirs[group->numDirLights] = &group->dirLights[i];
			group->numDirLights++;
			return l;
		}
	}
	return nullptr;
}
void RELI_RemoveDirectionalLight(struct LightGroup* group, DirectionalLight* light)
{
	const uintptr_t idx = ((uintptr_t)light - (uintptr_t)&group->dirLights[0]) / sizeof(InternalDirLight);
	if (idx < MAX_NUM_LIGHTS && group->dirLights[idx].isActive)
	{
		InternalDirLight* l = &group->dirLights[idx];
		memset(l, 0, sizeof(InternalDirLight));
		InternalDirLight* remainingList[MAX_NUM_LIGHTS]; memset(remainingList, 0, sizeof(void*) * MAX_NUM_LIGHTS);
		int curListIdx = 0;
		for (uint32_t i = 0; i < group->numDirLights; i++)
		{
			if (group->dirs[i] == l)
			{
				group->dirs[i] = nullptr;
			}
			else
			{
				remainingList[curListIdx] = group->dirs[i];
				curListIdx++;
			}
		}
		memcpy(group->dirs, remainingList, sizeof(void*) * MAX_NUM_LIGHTS);
		group->numDirLights--;
	}
}

PointLight* RELI_AddPointLight(struct LightGroup* group)
{
	if (group->numPointLights >= MAX_NUM_LIGHTS) return nullptr;
	for (int i = 0; i < MAX_NUM_LIGHTS; i++)
	{
		if (!group->pointLights[i].isActive)
		{
			PointLight* l = &group->pointLights[i].light.light;
			group->pointLights[i].hasShadow = false;
			group->pointLights[i].isActive = true;
			group->points[group->numPointLights] = &group->pointLights[i];
			group->numPointLights++;
			return l;
		}
	}
	return nullptr;
}
void RELI_RemovePointLight(struct LightGroup* group, PointLight* light)
{
	const uintptr_t idx = ((uintptr_t)light - (uintptr_t)&group->pointLights[0]) / sizeof(InternalPointLight);
	if (idx < MAX_NUM_LIGHTS && group->pointLights[idx].isActive)
	{
		InternalPointLight* l = &group->pointLights[idx];
		memset(l, 0, sizeof(InternalPointLight));
		InternalPointLight* remainingList[MAX_NUM_LIGHTS]; memset(remainingList, 0, sizeof(void*) * MAX_NUM_LIGHTS);
		int curListIdx = 0;
		for (uint32_t i = 0; i < group->numDirLights; i++)
		{
			if (group->points[i] == l)
			{
				group->points[i] = nullptr;
			}
			else
			{
				remainingList[curListIdx] = group->points[i];
				curListIdx++;
			}
		}
		memcpy(group->dirs, remainingList, sizeof(void*) * MAX_NUM_LIGHTS);
		group->numPointLights--;
	}
}

SpotLight* RELI_AddSpotLight(struct LightGroup* group)
{
	if (group->numSpotLights >= MAX_NUM_LIGHTS) return nullptr;
	for (int i = 0; i < MAX_NUM_LIGHTS; i++)
	{
		if (!group->spotLights[i].isActive)
		{
			SpotLight* l = &group->spotLights[i].light.light;
			group->spotLights[i].hasShadow = false;
			group->spotLights[i].isActive = true;
			group->spots[group->numSpotLights] = &group->spotLights[i];
			group->numSpotLights++;
			return l;
		}
	}
	return nullptr;
}
void RELI_RemoveSpotLight(struct LightGroup* group, SpotLight* light)
{
	const uintptr_t idx = ((uintptr_t)light - (uintptr_t)&group->spotLights[0]) / sizeof(InternalSpotLight);
	if (idx < MAX_NUM_LIGHTS && group->spotLights[idx].isActive)
	{
		InternalSpotLight* l = &group->spotLights[idx];
		memset(l, 0, sizeof(InternalSpotLight));
		InternalSpotLight* remainingList[MAX_NUM_LIGHTS]; memset(remainingList, 0, sizeof(void*) * MAX_NUM_LIGHTS);
		int curListIdx = 0;
		for (uint32_t i = 0; i < group->numDirLights; i++)
		{
			if (group->spots[i] == l)
			{
				group->spots[i] = nullptr;
			}
			else
			{
				remainingList[curListIdx] = group->spots[i];
				curListIdx++;
			}
		}
		memcpy(group->dirs, remainingList, sizeof(void*) * MAX_NUM_LIGHTS);
		group->numSpotLights--;
	}
}

DirShadowLight* RELI_AddDirectionalShadowLight(struct LightGroup* group, uint16_t shadowWidth, uint16_t shadowHeight, bool useCascade)
{
	if (shadowWidth > 0x1000 || shadowHeight > 0x1000 || group->numDirLights >= MAX_NUM_LIGHTS) return nullptr;
	if (!group->shadowGroup.fbo)
	{
		AddShadowLightGroup(group);
	}
	if (useCascade) {
        if ((group->shadowGroup.numUsedProjections + 4) >= MAX_NUM_LIGHTS) {
            return nullptr;
        }
    }
	else if ((group->shadowGroup.numUsedProjections + 1) >= MAX_NUM_LIGHTS) {
        return nullptr;
    }
	
	
	DirShadowLight* light = nullptr;
	for (int i = 0; i < MAX_NUM_LIGHTS; i++)
	{
		if (!group->dirLights[i].isActive)
		{
			light = &group->dirLights[i].light;
			group->dirLights[i].hasShadow = true;
			group->dirLights[i].isActive = true;
			group->dirs[group->numDirLights] = &group->dirLights[i];
			for (int j = 0; j < 4; j++)
			{
				group->dirLights[i].map[j] = { 0, 0, (uint16_t)shadowWidth, (uint16_t)shadowHeight };
			}
			group->numDirLights++;
			if (useCascade)
			{
				group->shadowGroup.numUsedProjections += 4;
			}
			else
			{
				group->shadowGroup.numUsedProjections++;
			}
			break;
		}
	}

	PackShadowLights(group);
	return light;
}
void RELI_RemoveDirectionalShadowLight(struct LightGroup* group, DirShadowLight* light)
{
	const uintptr_t idx = ((uintptr_t)light - (uintptr_t)&group->dirLights[0]) / sizeof(InternalDirLight);
	if (idx < MAX_NUM_LIGHTS && group->dirLights[idx].isActive)
	{
		InternalDirLight* l = &group->dirLights[idx];
		if (l->useCascades)
		{
			group->shadowGroup.numUsedProjections -= 4;
		}
		else
		{
			group->shadowGroup.numUsedProjections -= 1;
		}
		memset(l, 0, sizeof(InternalDirLight));
		InternalDirLight* remainingList[MAX_NUM_LIGHTS]; memset(remainingList, 0, sizeof(void*) * MAX_NUM_LIGHTS);
		int curListIdx = 0;
		for (uint32_t i = 0; i < group->numDirLights; i++)
		{
			if (group->dirs[i] == l)
			{
				group->dirs[i] = nullptr;
			}
			else
			{
				remainingList[curListIdx] = group->dirs[i];
				curListIdx++;
			}
		}
		memcpy(group->dirs, remainingList, sizeof(void*) * MAX_NUM_LIGHTS);
		group->numDirLights--;
	}
	CheckShadowGroupAndDeleteIfEmpty(group);
}
SpotShadowLight* RELI_AddSpotShadowLight(struct LightGroup* group, uint16_t shadowWidth, uint16_t shadowHeight)
{
	if (shadowWidth > 0x1000 || shadowHeight > 0x1000 || group->numDirLights >= MAX_NUM_LIGHTS || (group->shadowGroup.numUsedProjections + 1) >= MAX_NUM_LIGHTS) return nullptr;
	if (!group->shadowGroup.fbo)
	{
		AddShadowLightGroup(group);
	}

	SpotShadowLight* light = nullptr;
	for (int i = 0; i < MAX_NUM_LIGHTS; i++)
	{
		if (!group->spotLights[i].isActive)
		{
			light = &group->spotLights[i].light;
			group->spotLights[i].hasShadow = true;
			group->spotLights[i].isActive = true;
			group->spots[group->numSpotLights] = &group->spotLights[i];
			group->spotLights[i].mapped = { 0, 0, (uint16_t)shadowWidth, (uint16_t)shadowHeight };

			group->numSpotLights++;
			group->shadowGroup.numUsedProjections++;
			break;
		}
	}

	PackShadowLights(group);
	return light;
}
void RELI_RemoveSpotShadowLight(struct LightGroup* group, SpotShadowLight* light)
{
	const uintptr_t idx = ((uintptr_t)light - (uintptr_t)&group->spotLights[0]) / sizeof(InternalSpotLight);
	if (idx < MAX_NUM_LIGHTS && group->spotLights[idx].isActive)
	{
		InternalSpotLight* l = &group->spotLights[idx];
		group->shadowGroup.numUsedProjections -= 1;
		memset(l, 0, sizeof(InternalSpotLight));

		InternalSpotLight* remainingList[MAX_NUM_LIGHTS]; memset(remainingList, 0, sizeof(void*) * MAX_NUM_LIGHTS);
		int curListIdx = 0;
		for (uint32_t i = 0; i < group->numSpotLights; i++)
		{
			if (group->spots[i] == l)
			{
				group->spots[i] = nullptr;
			}
			else
			{
				remainingList[curListIdx] = group->spots[i];
				curListIdx++;
			}
		}
		memcpy(group->spots, remainingList, sizeof(void*) * MAX_NUM_LIGHTS);
		group->numSpotLights--;
	}
	CheckShadowGroupAndDeleteIfEmpty(group);
}
PointShadowLight* RELI_AddPointShadowLight(struct LightGroup* group, uint16_t shadowWidth, uint16_t shadowHeight)
{
	if (shadowWidth > 0x1000 || shadowHeight > 0x1000 || group->numPointLights >= MAX_NUM_LIGHTS || (group->shadowGroup.numUsedProjections + 6) >= MAX_NUM_LIGHTS) return nullptr;
	if (!group->shadowGroup.fbo)
	{
		AddShadowLightGroup(group);
	}

	PointShadowLight* light = nullptr;
	for (int i = 0; i < MAX_NUM_LIGHTS; i++)
	{
		if (!group->pointLights[i].isActive)
		{
			light = &group->pointLights[i].light;
			group->pointLights[i].hasShadow = true;
			group->pointLights[i].isActive = true;
			group->points[group->numPointLights] = &group->pointLights[i];
			for(int j = 0; j < 6; j++)
				group->pointLights[i].map[j] = {0, 0, (uint16_t)shadowWidth, (uint16_t)shadowHeight};

			group->numPointLights++;
			group->shadowGroup.numUsedProjections += 6;
			break;
		}
	}

	PackShadowLights(group);
	return light;
}
void RELI_RemovePointShadowLight(struct LightGroup* group, PointShadowLight* light)
{
	const uintptr_t idx = ((uintptr_t)light - (uintptr_t)&group->pointLights[0]) / sizeof(InternalPointLight);
	if (idx < MAX_NUM_LIGHTS && group->pointLights[idx].isActive)
	{
		InternalPointLight* l = &group->pointLights[idx];
		group->shadowGroup.numUsedProjections -= 6;
		memset(l, 0, sizeof(InternalPointLight));

		InternalPointLight* remainingList[MAX_NUM_LIGHTS]; memset(remainingList, 0, sizeof(void*) * MAX_NUM_LIGHTS);
		int curListIdx = 0;
		for (uint32_t i = 0; i < group->numSpotLights; i++)
		{
			if (group->points[i] == l)
			{
				group->points[i] = nullptr;
			}
			else
			{
				remainingList[curListIdx] = group->points[i];
				curListIdx++;
			}
		}
		memcpy(group->points, remainingList, sizeof(void*) * MAX_NUM_LIGHTS);
		group->numPointLights--;
	}
	CheckShadowGroupAndDeleteIfEmpty(group);
}












static bool AABBOverlapp(const AABB& frustum, const AABB& aabb)
{
	if (aabb.max.x >= frustum.min.x && frustum.max.x >= aabb.min.x &&
		aabb.max.y >= frustum.min.y && frustum.max.y >= aabb.min.y &&
		aabb.max.z >= frustum.min.z && frustum.max.z >= aabb.min.z)
	{
		return true;
	}
	return false;
}
static bool IsInViewFrustrum(const AABB& frustum, const glm::mat4& modelMat, const AABB& aabb)
{
	glm::vec4 positions[8] = {
		modelMat * glm::vec4(aabb.min.x, aabb.min.y, aabb.min.z, 1.0f),
		modelMat * glm::vec4(aabb.max.x, aabb.min.y, aabb.min.z, 1.0f),
		modelMat * glm::vec4(aabb.min.x, aabb.max.y, aabb.min.z, 1.0f),
		modelMat * glm::vec4(aabb.max.x, aabb.max.y, aabb.min.z, 1.0f),
		modelMat * glm::vec4(aabb.min.x, aabb.min.y, aabb.max.z, 1.0f),
		modelMat * glm::vec4(aabb.max.x, aabb.min.y, aabb.max.z, 1.0f),
		modelMat * glm::vec4(aabb.min.x, aabb.max.y, aabb.max.z, 1.0f),
		modelMat * glm::vec4(aabb.max.x, aabb.max.y, aabb.max.z, 1.0f),
	};
	glm::vec3 min = glm::vec3(FLT_MAX);
	glm::vec3 max = glm::vec3(-FLT_MAX);
	for (int i = 0; i < 8; i++)
	{
		positions[i] /= positions[i].w;
		min = glm::min(min, glm::vec3(positions[i].x, positions[i].y, positions[i].z));
		max = glm::max(max, glm::vec3(positions[i].x, positions[i].y, positions[i].z));
	}
	return AABBOverlapp(frustum, { min, max });
}
static AABB GenerateAABB(const glm::mat4& invViewProj)
{
	glm::vec4 positions[8] = {
		invViewProj * glm::vec4( 1.0f,  1.0f,  1.0f, 1.0f),
		invViewProj * glm::vec4(-1.0f,  1.0f,  1.0f, 1.0f),
		invViewProj * glm::vec4( 1.0f, -1.0f,  1.0f, 1.0f),
		invViewProj * glm::vec4(-1.0f, -1.0f,  1.0f, 1.0f),
		invViewProj * glm::vec4( 1.0f,  1.0f, -1.0f, 1.0f),
		invViewProj * glm::vec4(-1.0f,  1.0f, -1.0f, 1.0f),
		invViewProj * glm::vec4( 1.0f, -1.0f, -1.0f, 1.0f),
		invViewProj * glm::vec4(-1.0f, -1.0f, -1.0f, 1.0f),
	};
	glm::vec3 min = glm::vec3(FLT_MAX);
	glm::vec3 max = glm::vec3(-FLT_MAX);
	for (int i = 0; i < 8; i++)
	{
		positions[i] /= positions[i].w;
		min = glm::min(min, glm::vec3(positions[i].x, positions[i].y, positions[i].z));
		max = glm::max(max, glm::vec3(positions[i].x, positions[i].y, positions[i].z));
	}
	return { min, max };
}



void RE_BeginScene(struct Renderer* renderer, struct Scene* scene)
{
	renderer->bindInfo.curBoundIbo = 0;
	renderer->bindInfo.curBoundVao = 0;
	renderer->bindInfo.curProgram = 0;

	renderer->pbrData.geomUniforms.bound.bBone = 0;
	renderer->pbrData.geomUniforms.bound.bMaterial = 0;
	renderer->pbrData.geomUniforms.bound.bMat = 0;

	memset(&renderer->pbrData.baseUniforms.bound, 0, sizeof(renderer->pbrData.baseUniforms.bound));

	size_t numRenderables = 0;
	Renderable** r = SC_GetAllRenderables(scene, &numRenderables);

	uint32_t numCommands = 0;
	for (uint32_t i = 0; i < numRenderables; i++)
	{
		if (!(r[i]->GetFlags() & INACTIVE))
		{
			numCommands += r[i]->GetNumCommands();
		}
	}
	if (renderer->cmdlistCapacity <= numCommands)
	{
		renderer->cmdlistCapacity = numCommands + 100;
		if (renderer->drawCmds) delete[] renderer->drawCmds;
		renderer->drawCmds = new RenderCommand*[renderer->cmdlistCapacity];
	}
	renderer->firstTransparentCmd = 0;


	RenderCommand** curCmdList = renderer->drawCmds;
	numCommands = 0;
	// FILL OPAQUE OBJECTS
	for (uint32_t i = 0; i < numRenderables; i++)
	{
		if (!(r[i]->GetFlags() & INACTIVE))
		{
			uint32_t added = r[i]->FillRenderCommandsOpaque(curCmdList);
			curCmdList += added;
			numCommands += added;
			renderer->firstTransparentCmd += added;
		}
	}
	uint32_t curOffset = renderer->firstTransparentCmd;
	// FILL TRANSPARENT OBJECTS
	for (uint32_t i = 0; i < numRenderables; i++)
	{
		if (!(r[i]->GetFlags() & INACTIVE))
		{
			uint32_t added = r[i]->FillRenderCommandsTransparent(curCmdList);
			curCmdList += added;
			numCommands += added;
		}
	}
	renderer->numCmds = numCommands;
}

void RE_SetCameraBase(struct Renderer* renderer, const struct CameraBase* camBase)
{
	renderer->currentCam = camBase;
}
void RE_SetEnvironmentData(struct Renderer* renderer, const EnvironmentData* data)
{
	renderer->currentEnvironmentData = data;
}
void RE_SetLightData(struct Renderer* renderer, LightGroup* group)
{
	renderer->currentLightData = group;
}

void RE_RenderShadows(struct Renderer* renderer)
{
	ASSERT(renderer->currentCam != nullptr, "The Camera base needs to be set before rendering");
	ASSERT(renderer->numCmds != 0, "Need to Call Begin Scene Before Rendering");
	ASSERT(renderer->currentLightData != 0, "Need to Set Current Light Data Before Rendering");
	LightGroup* g = renderer->currentLightData;
	if (!g->shadowGroup.fbo) return;

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(2.0f, 4.0f);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);

	glBindFramebuffer(GL_FRAMEBUFFER, g->shadowGroup.fbo);
	glClearDepthf(1.0f);
	glClear(GL_DEPTH_BUFFER_BIT);
	
	CameraBase curLightCam;
	curLightCam.view = glm::mat4(1.0f);

	SceneRenderData data;
	data.renderer = renderer;
	data.env = renderer->currentEnvironmentData;
	data.lightGroupUniform = renderer->currentLightData->uniform;
	data.cam = &curLightCam;
	data.shadowMapTexture = 0;



	static const auto stdRender = [renderer, &data](const AABB& frustum)
	{
		for (uint32_t i = 0; i < renderer->numCmds; i++)
		{
			RenderCommand* obj = renderer->drawCmds[i];
			
			if (AABBOverlapp(frustum, obj->GetBound()))
			{
				obj->DrawShadow(&data);
			}
		}
	};
	for (uint32_t i = 0; i < g->numDirLights; i++)
	{
		if (g->dirs[i]->hasShadow)
		{
			const InternalDirLight& l = *g->dirs[i];
			if (l.light.light.projIdx < 0) continue;

			glViewport(l.map[0].x, l.map[0].y, l.map[0].w, l.map[0].h);
			curLightCam.pos = l.light.pos;
			curLightCam.proj = g->shadowGroup.projections[l.light.light.projIdx].proj;

			const glm::mat4 inv = glm::inverse(g->shadowGroup.projections[l.light.light.projIdx].proj);
			stdRender(GenerateAABB(inv));
		}
	}
	for (uint32_t i = 0; i < g->numPointLights; i++)
	{
		if (g->points[i]->hasShadow)
		{
			const InternalPointLight& l = *g->points[i];
			if (l.light.light.projIdx < 0) continue;

			curLightCam.pos = l.light.light.pos;

			for (int j = 0; j < 6; j++)
			{
				glViewport(l.map[j].x, l.map[j].y, l.map[j].w, l.map[j].h);
				curLightCam.proj = g->shadowGroup.projections[l.light.light.projIdx + j].proj;

				const glm::mat4 inv = glm::inverse(g->shadowGroup.projections[l.light.light.projIdx + j].proj);
				stdRender(GenerateAABB(inv));
			}

		}
	}
	for (uint32_t i = 0; i < g->numSpotLights; i++)
	{
		if (g->spots[i]->hasShadow)
		{
			const InternalSpotLight& l = *g->spots[i];
			if (l.light.light.projIdx < 0) continue;

			glViewport(l.mapped.x, l.mapped.y, l.mapped.w, l.mapped.h);
			curLightCam.pos = l.light.light.pos;
			curLightCam.proj = g->shadowGroup.projections[l.light.light.projIdx].proj;

			const glm::mat4 inv = glm::inverse(g->shadowGroup.projections[l.light.light.projIdx].proj);
			stdRender(GenerateAABB(inv));
		}
	}

	glDisable(GL_POLYGON_OFFSET_FILL);
}

void RE_RenderIrradiance(struct Renderer* renderer, float deltaPhi, float deltaTheta, CUBE_MAP_SIDE side)
{
	ASSERT(renderer->currentCam != nullptr, "The Camera base needs to be set before rendering");
	ASSERT(renderer->currentEnvironmentData != nullptr, "The Camera base needs to be set before rendering");

	glUseProgram(renderer->cubemapInfo.irradianceFilterProgram);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	glm::mat4 mvp = renderer->currentCam->proj * renderer->currentCam->view;

	glUniform1f(renderer->cubemapInfo.irradianceFilterdeltaPhiLoc, deltaPhi);
	glUniform1f(renderer->cubemapInfo.irradianceFilterdeltaThetaLoc, deltaTheta);
	glUniformMatrix4fv(renderer->cubemapInfo.irradianceFilterMVPLoc, 1, GL_FALSE, (const GLfloat*)&mvp);


	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, renderer->currentEnvironmentData->environmentMap);

	glBindVertexArray(renderer->cubemapInfo.vao);
	switch (side)
	{
	case CUBE_MAP_BACK:
		glDrawArrays(GL_TRIANGLES, 0, 6);
		break;
	case CUBE_MAP_BOTTOM:
		glDrawArrays(GL_TRIANGLES, 6, 6);
		break;
	case CUBE_MAP_RIGHT:
		glDrawArrays(GL_TRIANGLES, 12, 6);
		break;
	case CUBE_MAP_LEFT:
		glDrawArrays(GL_TRIANGLES, 18, 6);
		break;
	case CUBE_MAP_FRONT:
		glDrawArrays(GL_TRIANGLES, 24, 6);
		break;
	case CUBE_MAP_TOP:
		glDrawArrays(GL_TRIANGLES, 30, 6);
		break;
	};
}
void RE_RenderPreFilterCubeMap(struct Renderer* renderer, float roughness, uint32_t numSamples, CUBE_MAP_SIDE side)
{
	ASSERT(renderer->currentCam != nullptr, "The Camera base needs to be set before rendering");
	ASSERT(renderer->currentEnvironmentData != nullptr, "The Camera base needs to be set before rendering");

	glUseProgram(renderer->cubemapInfo.preFilterProgram);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	glm::mat4 mvp = renderer->currentCam->proj * renderer->currentCam->view;

	glUniform1f(renderer->cubemapInfo.preFilterRoughness, roughness);
	glUniform1ui(renderer->cubemapInfo.preFilterNumSamples, numSamples);
	glUniformMatrix4fv(renderer->cubemapInfo.preFilterMVPLoc, 1, GL_FALSE, (const GLfloat*)&mvp);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, renderer->currentEnvironmentData->environmentMap);

	glBindVertexArray(renderer->cubemapInfo.vao);

	switch (side)
	{
	case CUBE_MAP_BACK:
		glDrawArrays(GL_TRIANGLES, 0, 6);
		break;
	case CUBE_MAP_BOTTOM:
		glDrawArrays(GL_TRIANGLES, 6, 6);
		break;
	case CUBE_MAP_RIGHT:
		glDrawArrays(GL_TRIANGLES, 12, 6);
		break;
	case CUBE_MAP_LEFT:
		glDrawArrays(GL_TRIANGLES, 18, 6);
		break;
	case CUBE_MAP_FRONT:
		glDrawArrays(GL_TRIANGLES, 24, 6);
		break;
	case CUBE_MAP_TOP:
		glDrawArrays(GL_TRIANGLES, 30, 6);
		break;
	};
}



void RE_RenderGeometry(struct Renderer* renderer)
{
	ASSERT(renderer->currentCam != nullptr, "The Camera base needs to be set before rendering");
	ASSERT(renderer->numCmds != 0, "Need to Call Begin Scene Before Rendering");
    if (renderer->numCmds == 0) return;

	
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);


	const AABB frustum = GenerateAABB(glm::inverse(renderer->currentCam->proj * renderer->currentCam->view));

	SceneRenderData data;
	data.renderer = renderer;
	data.env = renderer->currentEnvironmentData;
	data.lightGroupUniform = 0;
	data.cam = renderer->currentCam;
	data.shadowMapTexture = 0;

	for (uint32_t i = 0; i < renderer->firstTransparentCmd; i++)
	{
		RenderCommand* cmd = renderer->drawCmds[i];
		
		if (AABBOverlapp(frustum, cmd->GetBound()))
		{
			cmd->DrawGeom(&data);
		}
	}
}

void RE_RenderOpaque(struct Renderer* renderer)
{
	ASSERT(renderer->currentCam != nullptr, "The Camera base needs to be set before rendering");
	ASSERT(renderer->numCmds != 0, "Need to Call Begin Scene Before Rendering");
	ASSERT(renderer->currentEnvironmentData != 0, "Need to Set Current Environment Before Rendering");
	ASSERT(renderer->currentLightData != 0, "Need to Set Current Light Data Before Rendering");
	if (renderer->numCmds == 0) return;
	
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);
	
	SceneRenderData data;
	data.renderer = renderer;
	data.env = renderer->currentEnvironmentData;
	data.lightGroupUniform = renderer->currentLightData->uniform;
	data.cam = renderer->currentCam;
	data.shadowMapTexture = renderer->currentLightData->shadowGroup.texture;

	const AABB frustum = GenerateAABB(glm::inverse(renderer->currentCam->proj * renderer->currentCam->view));

	for (uint32_t i = 0; i < renderer->firstTransparentCmd; i++)
	{
		RenderCommand* cmd = renderer->drawCmds[i];

		if (AABBOverlapp(frustum, cmd->GetBound()))
		{
			cmd->DrawOpaque(&data);
		}
		
	}
}
void RE_RenderAdditionalOpaque(struct Renderer* renderer)
{
	ASSERT(renderer->currentCam != nullptr, "The Camera base needs to be set before rendering");
	ASSERT(renderer->numCmds != 0, "Need to Call Begin Scene Before Rendering");
	ASSERT(renderer->currentEnvironmentData != 0, "Need to Set Current Environment Before Rendering");
	ASSERT(renderer->currentLightData != 0, "Need to Set Current Light Data Before Rendering");
	if (renderer->numCmds == 0) return;

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);

	SceneRenderData data;
	data.renderer = renderer;
	data.env = renderer->currentEnvironmentData;
	data.lightGroupUniform = renderer->currentLightData->uniform;
	data.cam = renderer->currentCam;
	data.shadowMapTexture = renderer->currentLightData->shadowGroup.texture;

	const AABB frustum = GenerateAABB(glm::inverse(renderer->currentCam->proj * renderer->currentCam->view));

	for (uint32_t i = 0; i < renderer->firstTransparentCmd; i++)
	{
		RenderCommand* cmd = renderer->drawCmds[i];

		if (AABBOverlapp(frustum, cmd->GetBound()))
		{
			cmd->DrawOpaque(&data);
		}

	}
}
void RE_RenderTransparent(struct Renderer* renderer)
{
	ASSERT(renderer->currentCam != nullptr, "The Camera base needs to be set before rendering");
	ASSERT(renderer->numCmds != 0, "Need to Call Begin Scene Before Rendering");
	ASSERT(renderer->currentEnvironmentData != 0, "Need to Set Current Environment Before Rendering");
	ASSERT(renderer->currentLightData != 0, "Need to Set Current Light Data Before Rendering");
	if (renderer->numCmds == renderer->firstTransparentCmd) return;

	const glm::mat4 viewProj = renderer->currentCam->proj * renderer->currentCam->view;
	const glm::mat4 invViewProj = glm::inverse(viewProj);
	
	// FILL OBJECTS DISTANCE FROM CAMERA
	for (uint32_t i = renderer->firstTransparentCmd; i < renderer->numCmds; i++)
	{
		const glm::vec3& min = renderer->drawCmds[i]->GetBound().min;
		const glm::vec3& max = renderer->drawCmds[i]->GetBound().max;
		glm::vec4 positions[8] = {
			renderer->currentCam->view * glm::vec4(min.x, min.y, min.z, 1.0f),
			renderer->currentCam->view * glm::vec4(max.x, min.y, min.z, 1.0f),
			renderer->currentCam->view * glm::vec4(min.x, max.y, min.z, 1.0f),
			renderer->currentCam->view * glm::vec4(max.x, max.y, min.z, 1.0f),
			renderer->currentCam->view * glm::vec4(min.x, min.y, max.z, 1.0f),
			renderer->currentCam->view * glm::vec4(max.x, min.y, max.z, 1.0f),
			renderer->currentCam->view * glm::vec4(min.x, max.y, max.z, 1.0f),
			renderer->currentCam->view * glm::vec4(max.x, max.y, max.z, 1.0f),
		};
		float zDepth = FLT_MAX;
		for (uint32_t j = 0; j < 8; j++)
		{
			positions[j] /= positions[j].w;
			zDepth = glm::min(zDepth, positions[j].z);
		}
		renderer->drawCmds[i]->SetZDepth(zDepth);
	}

	// SORT TRANSPARENT OBJECTS
	std::sort(&renderer->drawCmds[renderer->firstTransparentCmd], &renderer->drawCmds[renderer->numCmds], [](RenderCommand* d1, RenderCommand* d2)
	{
			return d2->GetZDepth() > d1->GetZDepth();
	});

	const AABB frustum = GenerateAABB(invViewProj);

	SceneRenderData data;
	data.renderer = renderer;
	data.env = renderer->currentEnvironmentData;
	data.lightGroupUniform = renderer->currentLightData->uniform;
	data.cam = renderer->currentCam;
	data.shadowMapTexture = renderer->currentLightData->shadowGroup.texture;

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	for (uint32_t i = renderer->firstTransparentCmd; i < renderer->numCmds; i++)
	{
		RenderCommand* cmd = renderer->drawCmds[i];
		
		if (AABBOverlapp(frustum, cmd->GetBound()))
		{
			cmd->DrawTransparent(&data);
		}
	}

}


void RE_RenderOutline(struct Renderer* renderer, const Renderable* obj, const glm::vec4& color, float thickness)
{
	ASSERT(renderer->currentCam != nullptr, "The Camera base needs to be set before rendering");
	ASSERT(renderer->currentEnvironmentData != 0, "Need to Set Current Environment Before Rendering");
	ASSERT(renderer->currentLightData != 0, "Need to Set Current Light Data Before Rendering");

	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glStencilMask(0xFF);

	glClear(GL_STENCIL_BUFFER_BIT);

	glStencilFunc(GL_ALWAYS, 1, 0xFF);


	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);

	SceneRenderData data;
	data.renderer = renderer;
	data.env = renderer->currentEnvironmentData;
	data.lightGroupUniform = renderer->currentLightData->uniform;
	data.cam = renderer->currentCam;
	data.shadowMapTexture = renderer->currentLightData->shadowGroup.texture;

	const size_t numCmds = obj->GetNumCommands();
	for (uint32_t i = 0; i < numCmds; i++)
	{
		const RenderCommand* cmd = obj->GetRenderCommand(i);
		cmd->DrawGeom(&data);
	}
	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	glStencilMask(0x00);
	
	glUseProgram(renderer->pbrData.outlineProgram);
	renderer->bindInfo.curProgram = renderer->pbrData.outlineProgram;

	glUniform3fv(renderer->pbrData.outlineUniforms.camPosLoc, 1, (const GLfloat*)&renderer->currentCam->pos);
	glUniformMatrix4fv(renderer->pbrData.outlineUniforms.viewLoc, 1, GL_FALSE, (const GLfloat*)&renderer->currentCam->view);
	glUniformMatrix4fv(renderer->pbrData.outlineUniforms.projLoc, 1, GL_FALSE, (const GLfloat*)&renderer->currentCam->proj);

	glUniform1f(renderer->pbrData.outlineUniforms.thicknessLoc, thickness);
	glUniform4fv(renderer->pbrData.outlineUniforms.colorLoc, 1, (const GLfloat*)&color);


	for (uint32_t i = 0; i < numCmds; i++)
	{
		const RenderCommand* cmd = obj->GetRenderCommand(i);
		cmd->DrawOutline(&data);
	}
	glDisable(GL_STENCIL_TEST);
}

void RE_RenderCubeMap(struct Renderer* renderer, GLuint cubemap)
{
	ASSERT(renderer->currentCam != nullptr, "The Camera base needs to be set before rendering");
	
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	glUseProgram(renderer->cubemapInfo.program);
	

	glm::mat4 viewProj = renderer->currentCam->proj * renderer->currentCam->view;
	glUniformMatrix4fv(renderer->cubemapInfo.viewProjLoc, 1, GL_FALSE, (const GLfloat*)&viewProj);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);

	glBindVertexArray(renderer->cubemapInfo.vao);
	glDrawArrays(GL_TRIANGLES, 0, 36);
}




void RE_EndScene(struct Renderer* renderer)
{
	renderer->numCmds = 0;

	renderer->currentCam = nullptr;
	renderer->currentEnvironmentData = nullptr;
	renderer->currentLightData = 0;

}


























void RE_RenderScreenSpaceReflection(struct Renderer* renderer, const ScreenSpaceReflectionRenderData* ssrData, GLuint srcTexture, GLuint targetFBO, uint32_t targetWidth, uint32_t targetHeight)
{
	ASSERT(renderer->currentCam != nullptr, "The Camera base needs to be set before rendering");
	ASSERT(renderer->numCmds != 0, "Need to Call Begin Scene Before Rendering");
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);


	glBindFramebuffer(GL_FRAMEBUFFER, ssrData->fbo);
	glViewport(0, 0, ssrData->width, ssrData->height);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepthf(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	SceneRenderData data;
	data.renderer = renderer;
	data.env = nullptr;
	data.lightGroupUniform = 0;
	data.cam = renderer->currentCam;
	data.shadowMapTexture = 0;


	const AABB frustum = GenerateAABB(glm::inverse(renderer->currentCam->proj * renderer->currentCam->view));
	for (uint32_t i = 0; i < renderer->numCmds; i++)
	{
		RenderCommand* cmd = renderer->drawCmds[i];
		
		if (AABBOverlapp(frustum, cmd->GetBound()))
		{
			cmd->DrawSSR(&data);
		}
	}

	glm::mat4 invView = glm::inverse(renderer->currentCam->view);
	glm::mat4 invProj = glm::inverse(renderer->currentCam->proj);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glDisable(GL_DEPTH_TEST);

	glBindFramebuffer(GL_FRAMEBUFFER, targetFBO);
	glViewport(0, 0, targetWidth, targetHeight);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(renderer->ppInfo.ssr.program);


	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, srcTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, ssrData->normalTexture);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, ssrData->metallicTexture);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, ssrData->depthTexture);

	const PostProcessingRenderInfo::ScreenSpaceReflection& ssr = renderer->ppInfo.ssr;
	//uniform mat4 proj; \n\
	//uniform mat4 invProj; \n\
	//uniform mat4 view; \n\
	//uniform mat4 invView; \n\
	//uniform float rayStep = 0.1f; \n\
	//uniform int iterationCount = 100; \n\
	//uniform float distanceBias = 0.03f; \n\
	//uniform bool enableSSR = true; \n\
	//uniform int sampleCount = 4; \n\
	//uniform bool isSamplingEnabled = false; \n\
	//uniform bool isExponentialStepEnabled = false; \n\
	//uniform bool isAdaptiveStepEnabled = false; \n\
	//uniform bool isBinarySearchEnabled = true; \n\
	//uniform bool debugDraw = false; \n\
	//uniform float samplingCoefficient = 0.001f; \n\

	glUniformMatrix4fv(ssr.projLoc, 1, GL_FALSE, (GLfloat*)&renderer->currentCam->proj);
	glUniformMatrix4fv(ssr.invProjLoc, 1, GL_FALSE, (GLfloat*)&invProj);
	glUniformMatrix4fv(ssr.viewLoc, 1, GL_FALSE, (GLfloat*)&renderer->currentCam->view);
	glUniformMatrix4fv(ssr.invViewLoc, 1, GL_FALSE, (GLfloat*)&invView);

	glDrawArrays(GL_TRIANGLES, 0, 3);
}
void RE_RenderScreenSpaceAmbientOcclusion(struct Renderer* renderer, GLuint srcColorTexture, GLuint srcDepthTexture, uint32_t srcWidth, uint32_t srcHeight, GLuint targetFBO, uint32_t targetWidth, uint32_t targetHeight)
{
	ASSERT(renderer->currentCam != nullptr, "The Camera base needs to be set before rendering");
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	glBindFramebuffer(GL_FRAMEBUFFER, targetFBO);
	glViewport(0, 0, targetWidth, targetHeight);

	glUseProgram(renderer->ppInfo.ssao.program);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, srcColorTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, srcDepthTexture);

	float nearClipping = renderer->currentCam->proj[3][2] / (renderer->currentCam->proj[2][2] - 1.0f);
	float farClipping = renderer->currentCam->proj[3][2] / (renderer->currentCam->proj[2][2] + 1.0f);

	glUniform2f(renderer->ppInfo.ssao.resolutionLoc, static_cast<GLfloat>(srcWidth), static_cast<GLfloat>(srcHeight));
	glUniform1f(renderer->ppInfo.ssao.zNearLoc, nearClipping);
	glUniform1f(renderer->ppInfo.ssao.zFarLoc, farClipping);
	glUniform1f(renderer->ppInfo.ssao.strengthLoc, 1.0f);
	glUniform1i(renderer->ppInfo.ssao.samplesLoc, 64);
	glUniform1f(renderer->ppInfo.ssao.radiusLoc, 0.01f);

	glDrawArrays(GL_TRIANGLES, 0, 3);

}
void RE_RenderPostProcessingBloom(struct Renderer* renderer, const PostProcessingRenderData* ppData, GLuint srcTexture, uint32_t srcWidth, uint32_t srcHeight, GLuint targetFBO, uint32_t targetWidth, uint32_t targetHeight)
{
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	glm::ivec2 fboSizes[MAX_BLOOM_MIPMAPS];
	{	// Bloom src image into the pptexture1
		glBindFramebuffer(GL_FRAMEBUFFER, ppData->ppFBOs2[0]);
		glViewport(0, 0, ppData->width, ppData->height);
		glUseProgram(renderer->ppInfo.bloom.program);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, srcTexture);


		glUniform1f(renderer->ppInfo.bloom.blurRadiusLoc, ppData->blurRadius);
		glUniform1i(renderer->ppInfo.bloom.blurAxisLoc, (int)PostProcessingRenderInfo::BLUR_AXIS::X_AXIS);
		glUniform1f(renderer->ppInfo.bloom.intensityLoc, ppData->bloomIntensity);
		glUniform1f(renderer->ppInfo.bloom.mipLevelLoc, 0);

		glDrawArrays(GL_TRIANGLES, 0, 3);

		glBindFramebuffer(GL_FRAMEBUFFER, ppData->ppFBOs1[0]);
		glViewport(0, 0, ppData->width, ppData->height);

		glBindTexture(GL_TEXTURE_2D, ppData->ppTexture2);

		glUniform1f(renderer->ppInfo.bloom.intensityLoc, 0.0f);
		glUniform1i(renderer->ppInfo.bloom.blurAxisLoc, (int)PostProcessingRenderInfo::BLUR_AXIS::Y_AXIS);

		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
	fboSizes[0] = { ppData->width, ppData->height };
	int curSizeX = ppData->width; int curSizeY = ppData->height;
	{	// BLUR AND DOWNSAMPLE EACH ITERATION
		for (int i = 1; i < ppData->numPPFbos; i++)
		{
			curSizeX = std::max(curSizeX >> 1, 1); curSizeY = std::max(curSizeY >> 1, 1);
			fboSizes[i] = { curSizeX, curSizeY };
			glBindFramebuffer(GL_FRAMEBUFFER, ppData->ppFBOs2[i]);
			glViewport(0, 0, curSizeX, curSizeY);

			glBindTexture(GL_TEXTURE_2D, ppData->ppTexture1);

			glUniform1f(renderer->ppInfo.bloom.blurRadiusLoc, ppData->blurRadius);
			glUniform1i(renderer->ppInfo.bloom.blurAxisLoc, (int)PostProcessingRenderInfo::BLUR_AXIS::X_AXIS);
			glUniform1f(renderer->ppInfo.bloom.intensityLoc, 0.0f);
			glUniform1f(renderer->ppInfo.bloom.mipLevelLoc, static_cast<GLfloat>(i - 1));

			glDrawArrays(GL_TRIANGLES, 0, 3);

			glBindFramebuffer(GL_FRAMEBUFFER, ppData->ppFBOs1[i]);

			glBindTexture(GL_TEXTURE_2D, ppData->ppTexture2);

			glUniform1f(renderer->ppInfo.bloom.blurRadiusLoc, ppData->blurRadius / 2.0f);
			glUniform1f(renderer->ppInfo.bloom.intensityLoc, 0.0f);
			glUniform1f(renderer->ppInfo.bloom.mipLevelLoc, static_cast<float>(i));
			glUniform1i(renderer->ppInfo.bloom.blurAxisLoc, (int)PostProcessingRenderInfo::BLUR_AXIS::Y_AXIS);
			glDrawArrays(GL_TRIANGLES, 0, 3);
		}
	}
	{	// UPSAMPLE FROM LOWER MIPMAP TO HIGHER
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		glUseProgram(renderer->ppInfo.upsampling.program);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, ppData->ppTexture1);
		for (int i = ppData->numPPFbos - 2; i >= 0; i--)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, ppData->ppFBOs1[i]);
			glViewport(0, 0, fboSizes[i].x, fboSizes[i].y);
			glUniform1f(renderer->ppInfo.upsampling.mipLevelLoc, static_cast<GLfloat>(i + 1));
			glDrawArrays(GL_TRIANGLES, 0, 3);
		}
	}

	{	// COPY FINAL IMAGE TO OUTPUT FBO AND TONEMAP
		glDisable(GL_BLEND);
		glBindFramebuffer(GL_FRAMEBUFFER, targetFBO);
		glViewport(0, 0, targetWidth, targetHeight);
		glUseProgram(renderer->ppInfo.dualCopy.program);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, ppData->ppTexture1);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, srcTexture);

		glUniform1f(renderer->ppInfo.dualCopy.mipLevel1Loc, 0);
		glUniform1f(renderer->ppInfo.dualCopy.mipLevel2Loc, 0);
		glUniform1f(renderer->ppInfo.dualCopy.exposureLoc, ppData->exposure);
		glUniform1f(renderer->ppInfo.dualCopy.gammaLoc, ppData->gamma);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
}
void RE_RenderPostProcessingToneMap(struct Renderer* renderer, const PostProcessingRenderData* ppData, GLuint srcTexture, GLuint targetFBO, uint32_t targetWidth, uint32_t targetHeight)
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glBindFramebuffer(GL_FRAMEBUFFER, targetFBO);
	glViewport(0, 0, targetWidth, targetHeight);
	glUseProgram(renderer->ppInfo.dualCopy.program);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, renderer->blackTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, srcTexture);

	glUniform1f(renderer->ppInfo.dualCopy.mipLevel1Loc, 0);
	glUniform1f(renderer->ppInfo.dualCopy.mipLevel2Loc, 0);
	glUniform1f(renderer->ppInfo.dualCopy.exposureLoc, ppData->exposure);
	glUniform1f(renderer->ppInfo.dualCopy.gammaLoc, ppData->gamma);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}
















void PBRRenderCommand::DrawShadow(SceneRenderData* data) const
{
	if(renderFlags & PBR_RENDER_FLAGS::PBR_RENDER_SHADOW)
		DrawGeom(data);
}
void PBRRenderCommand::DrawGeom(SceneRenderData* data) const
{
	if (!(renderFlags & PBR_RENDER_GEOMETRY)) return;
	BaseGeometryProgramUniforms& un = data->renderer->pbrData.geomUniforms;
	if (data->renderer->bindInfo.curProgram != data->renderer->pbrData.geomProgram) {
		glUseProgram(data->renderer->pbrData.geomProgram);
		data->renderer->bindInfo.curProgram = data->renderer->pbrData.geomProgram;
		data->renderer->bindInfo.curBoundVao = 0;
		data->renderer->bindInfo.curBoundIbo = 0;
	}
	glUniform3fv(un.camPosLoc, 1, (const GLfloat*)&data->cam->pos);
	glUniformMatrix4fv(un.projLoc, 1, GL_FALSE, (const GLfloat*)&data->cam->proj);

	glUniformMatrix4fv(un.viewLoc, 1, GL_FALSE, (const GLfloat*)&data->cam->view);
	glUniformMatrix4fv(un.modelLoc, 1, GL_FALSE, (const GLfloat*)&transform);
	if (data->renderer->bindInfo.curBoundVao != vao) 
	{
		glBindVertexArray(vao);
		data->renderer->bindInfo.curBoundVao = vao;
	}
	if (indexBuffer && data->renderer->bindInfo.curBoundIbo != indexBuffer) 
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
		data->renderer->bindInfo.curBoundIbo = indexBuffer;
	}
	if (animUniform) {
		if (un.bound.bBone != animUniform)
		{
			glBindBufferBase(GL_UNIFORM_BUFFER, un.boneDataLoc, animUniform);
			un.bound.bBone = animUniform;
		}
	}
	else if(un.bound.bBone != data->renderer->pbrData.defaultBoneData) 
	{
		glBindBufferBase(GL_UNIFORM_BUFFER, un.boneDataLoc, data->renderer->pbrData.defaultBoneData);
		un.bound.bBone = data->renderer->pbrData.defaultBoneData;
	}

	BindMaterialGeometry(data->renderer, material);
	GLenum renderMode = (primFlags & PRIMITIVE_FLAG_LINE) ? GL_LINES : GL_TRIANGLES;
	if (primFlags & PRIMITIVE_FLAG_NO_INDEX_BUFFER)
	{
		glDrawArrays(renderMode, startIdx, numIdx);
	}
	else
	{
		glDrawElements(renderMode, numIdx, GL_UNSIGNED_INT, (void*)(startIdx * sizeof(uint32_t)));
	}
}
void PBRRenderCommand::DrawOpaque(SceneRenderData* data) const
{
	if (!(renderFlags & PBR_RENDER_OPAQUE)) { return; }
	BaseProgramUniforms& un = data->renderer->pbrData.baseUniforms;

	
	if (data->renderer->bindInfo.curProgram != data->renderer->pbrData.baseProgram)
	{
		glUseProgram(data->renderer->pbrData.baseProgram);
		data->renderer->bindInfo.curProgram = data->renderer->pbrData.baseProgram;
		data->renderer->bindInfo.curBoundVao = 0;
		data->renderer->bindInfo.curBoundIbo = 0;
	}
	if (un.bound.bPrefilter != data->env->prefilteredMap)
	{
		glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_PREFILTERED_MAP);
		glBindTexture(GL_TEXTURE_CUBE_MAP, data->env->prefilteredMap);
		un.bound.bPrefilter = data->env->prefilteredMap;
	}
	if (un.bound.bIrradiance != data->env->irradianceMap)
	{
		glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_SAMPLER_IRRADIANCE);
		glBindTexture(GL_TEXTURE_CUBE_MAP, data->env->irradianceMap);
		un.bound.bIrradiance = data->env->irradianceMap;
	}
	if (un.bound.bBrdfLut != data->renderer->pbrData.brdfLut)
	{
		glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_BRDFLUT);
		glBindTexture(GL_TEXTURE_2D, data->renderer->pbrData.brdfLut);
		un.bound.bBrdfLut = data->renderer->pbrData.brdfLut;
	}
	if (data->shadowMapTexture)
	{
		if (un.bound.bShadow != data->shadowMapTexture)
		{
			glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_SHADOW_MAP);
			glBindTexture(GL_TEXTURE_2D, data->shadowMapTexture);
			un.bound.bShadow = data->shadowMapTexture;
		}
	}
	else if(un.bound.bShadow != data->renderer->whiteTexture)
	{
		glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_SHADOW_MAP);
		glBindTexture(GL_TEXTURE_2D, data->renderer->whiteTexture);
		un.bound.bShadow = data->renderer->whiteTexture;
	}
	glUniform3fv(un.camPosLoc, 1, (const GLfloat*)&data->cam->pos);
	glUniformMatrix4fv(un.viewLoc, 1, GL_FALSE, (const GLfloat*)&data->cam->view);
	glUniformMatrix4fv(un.projLoc, 1, GL_FALSE, (const GLfloat*)&data->cam->proj);
	
	glUniform1f(un.prefilteredCubeMipLevelsLoc, static_cast<GLfloat>(data->env->mipLevels));
	glUniform1f(un.scaleIBLAmbientLoc, 1.0f);
	if (un.bound.bLight != data->lightGroupUniform)
	{
		glBindBufferBase(GL_UNIFORM_BUFFER, un.lightDataLoc, data->lightGroupUniform);
		un.bound.bLight = data->lightGroupUniform;
	}

	if (data->renderer->bindInfo.curBoundVao != vao) {
		glBindVertexArray(vao);
		data->renderer->bindInfo.curBoundVao = vao;
	}
	if (indexBuffer && data->renderer->bindInfo.curBoundIbo != indexBuffer) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
		data->renderer->bindInfo.curBoundIbo = indexBuffer;
	}
	glUniformMatrix4fv(un.modelLoc, 1, GL_FALSE, (const GLfloat*)&transform);
	
	if (animUniform)
	{
		if (un.bound.bBone != animUniform)
		{
			glBindBufferBase(GL_UNIFORM_BUFFER, un.boneDataLoc, animUniform);
			un.bound.bBone = animUniform;
		}
	}
	else if (un.bound.bBone != data->renderer->pbrData.defaultBoneData)
	{
		glBindBufferBase(GL_UNIFORM_BUFFER, un.boneDataLoc, data->renderer->pbrData.defaultBoneData);
		un.bound.bBone = data->renderer->pbrData.defaultBoneData;
	}
	
	BindMaterial(data->renderer, material);
	GLenum renderMode = (primFlags & PRIMITIVE_FLAG_LINE) ? GL_LINES : GL_TRIANGLES;
	if (primFlags & PRIMITIVE_FLAG_NO_INDEX_BUFFER)
	{
		glDrawArrays(renderMode, startIdx, numIdx);
	}
	else
	{
		glDrawElements(renderMode, numIdx, GL_UNSIGNED_INT, (void*)(startIdx * sizeof(uint32_t)));
	}
}
void PBRRenderCommand::DrawTransparent(SceneRenderData* data) const
{
	if (!(renderFlags & PBR_RENDER_TRANSPARENT)) return;
	BaseProgramUniforms& un = data->renderer->pbrData.baseUniforms;
	if (data->renderer->bindInfo.curProgram != data->renderer->pbrData.baseProgram)
	{
		glUseProgram(data->renderer->pbrData.baseProgram);
		data->renderer->bindInfo.curProgram = data->renderer->pbrData.baseProgram;
	}
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (un.bound.bPrefilter != data->env->prefilteredMap)
	{
		glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_PREFILTERED_MAP);
		glBindTexture(GL_TEXTURE_CUBE_MAP, data->env->prefilteredMap);
		un.bound.bPrefilter = data->env->prefilteredMap;
	}
	if (un.bound.bIrradiance != data->env->irradianceMap)
	{
		glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_SAMPLER_IRRADIANCE);
		glBindTexture(GL_TEXTURE_CUBE_MAP, data->env->irradianceMap);
		un.bound.bIrradiance = data->env->irradianceMap;
	}
	if (un.bound.bBrdfLut != data->renderer->pbrData.brdfLut)
	{
		glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_BRDFLUT);
		glBindTexture(GL_TEXTURE_2D, data->renderer->pbrData.brdfLut);
		un.bound.bBrdfLut = data->renderer->pbrData.brdfLut;
	}

	glUniform3fv(un.camPosLoc, 1, (const GLfloat*)&data->cam->pos);
	glUniformMatrix4fv(un.projLoc, 1, GL_FALSE, (const GLfloat*)&data->cam->proj);

	glUniformMatrix4fv(un.viewLoc, 1, GL_FALSE, (const GLfloat*)&data->cam->view);
	glUniformMatrix4fv(un.modelLoc, 1, GL_FALSE, (const GLfloat*)&transform);

	if (data->renderer->bindInfo.curBoundVao != vao) {
		glBindVertexArray(vao);
		data->renderer->bindInfo.curBoundVao = vao;
	}
	if (indexBuffer && data->renderer->bindInfo.curBoundIbo != indexBuffer) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
		data->renderer->bindInfo.curBoundIbo = indexBuffer;
	}

	if (animUniform)
	{
		if (un.bound.bBone != animUniform)
		{
			glBindBufferBase(GL_UNIFORM_BUFFER, un.boneDataLoc, animUniform);
			un.bound.bBone = animUniform;
		}
	}
	else if (un.bound.bBone != data->renderer->pbrData.defaultBoneData) {
		glBindBufferBase(GL_UNIFORM_BUFFER, un.boneDataLoc, data->renderer->pbrData.defaultBoneData);
		un.bound.bBone = data->renderer->pbrData.defaultBoneData;
	}

	BindMaterial(data->renderer, material);
	GLenum renderMode = (primFlags & PRIMITIVE_FLAG_LINE) ? GL_LINES : GL_TRIANGLES;
	if (primFlags & PRIMITIVE_FLAG_NO_INDEX_BUFFER)
	{
		glDrawArrays(renderMode, startIdx, numIdx);
	}
	else
	{
		glDrawElements(renderMode, numIdx, GL_UNSIGNED_INT, (void*)(startIdx * sizeof(uint32_t)));
	}
}
void PBRRenderCommand::DrawSSR(SceneRenderData* data) const
{
	if (!(renderFlags & PBR_RENDER_SSR)) return;
	BaseSSRProgramUniforms& un = data->renderer->pbrData.ssrUniforms;
	if (data->renderer->bindInfo.curProgram != data->renderer->pbrData.ssrProgram)
	{
		glUseProgram(data->renderer->pbrData.ssrProgram);
		data->renderer->bindInfo.curProgram = data->renderer->pbrData.ssrProgram;
		data->renderer->bindInfo.curBoundVao = 0;
		data->renderer->bindInfo.curBoundIbo = 0;
	}
	glUniform3fv(un.camPosLoc, 1, (const GLfloat*)&data->cam->pos);
	glUniformMatrix4fv(un.viewLoc, 1, GL_FALSE, (const GLfloat*)&data->cam->view);
	glUniformMatrix4fv(un.projLoc, 1, GL_FALSE, (const GLfloat*)&data->cam->proj);

	if (data->renderer->bindInfo.curBoundVao != vao) {
		glBindVertexArray(vao);
		data->renderer->bindInfo.curBoundVao = vao;
	}
	if (indexBuffer && data->renderer->bindInfo.curBoundIbo != indexBuffer) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
		data->renderer->bindInfo.curBoundIbo = indexBuffer;
	}
	if (animUniform)
	{
		if (un.bound.bBone != animUniform)
		{
			glBindBufferBase(GL_UNIFORM_BUFFER, un.boneDataLoc, animUniform);
			un.bound.bBone = animUniform;
		}
	}
	else if(un.bound.bBone != data->renderer->pbrData.defaultBoneData)
	{
		glBindBufferBase(GL_UNIFORM_BUFFER, un.boneDataLoc, data->renderer->pbrData.defaultBoneData);
		un.bound.bBone = data->renderer->pbrData.defaultBoneData;
	}

	glUniformMatrix4fv(un.modelLoc, 1, GL_FALSE, (const GLfloat*)&transform);
	BindMaterialSSR(data->renderer, material);
	GLenum renderMode = (primFlags & PRIMITIVE_FLAG_LINE) ? GL_LINES : GL_TRIANGLES;
	if (primFlags & PRIMITIVE_FLAG_NO_INDEX_BUFFER)
	{
		glDrawArrays(renderMode, startIdx, numIdx);
	}
	else
	{
		glDrawElements(renderMode, numIdx, GL_UNSIGNED_INT, (void*)(startIdx * sizeof(uint32_t)));
	}
}
void PBRRenderCommand::DrawOutline(SceneRenderData* data) const
{
	BaseOutlineProgramUniforms& un = data->renderer->pbrData.outlineUniforms;
	if (animUniform)
	{
		glBindBufferBase(GL_UNIFORM_BUFFER, un.boneDataLoc, animUniform);
		un.bound.bBone = animUniform;
	}
	else
	{
		glBindBufferBase(GL_UNIFORM_BUFFER, un.boneDataLoc, data->renderer->pbrData.defaultBoneData);
		un.bound.bBone = data->renderer->pbrData.defaultBoneData;
	}
	
	glUniformMatrix4fv(un.modelLoc, 1, GL_FALSE, (GLfloat*)&transform);
	GLenum renderMode = (primFlags & PRIMITIVE_FLAG_LINE) ? GL_LINES : GL_TRIANGLES;
	if (primFlags & PRIMITIVE_FLAG_NO_INDEX_BUFFER)
	{
		glDrawArrays(renderMode, startIdx, numIdx);
	}
	else
	{
		glDrawElements(renderMode, numIdx, GL_UNSIGNED_INT, (void*)(startIdx * sizeof(uint32_t)));
	}
}
