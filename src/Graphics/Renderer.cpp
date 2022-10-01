#include "Renderer.h"
#include "../Util/Assets.h"
#include "ModelInfo.h"
#include "Camera.h"
#include "Scene.h"

const char* baseVertexShader = "#version 330\n\
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
}\
";
const char* baseFragmentShader = "#version 330\n\
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
	float padding;\n\
};\n\
struct DirectionalLight\n\
{\n\
	vec4 direction;\n\
	vec4 color;\n\
};\n\
struct SpotLight\n\
{\n\
	vec4 color;\n\
	vec4 direction;\n\
	vec3 pos;\n\
	float cutOff;\n\
};\n\
layout (std140) uniform LightData {\n\
	PointLight pointLights[MAX_NUM_LIGHTS];\n\
	DirectionalLight dirLights[MAX_NUM_LIGHTS];\n\
	SpotLight spotLights[MAX_NUM_LIGHTS];\n\
	vec4 ambientColor;\n\
	int numPointLights;\n\
	int numDirLights;\n\
	int numSpotLights;\n\
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
	if (mat.alphaMask == 1.0f) {\n\
		if (mat.baseColorUV > -1) {\n\
			baseColor = texture(colorMap, mat.baseColorUV == 0 ? fragUV1 : fragUV2) * mat.baseColorFactor;\n\
		}\n\
		else {\n\
			baseColor = mat.baseColorFactor;\n\
		}\n\
		if (baseColor.a < mat.alphaCutoff) {\n\
			discard;\n\
		}\n\
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
	vec3 color = lights.ambientColor.rgb * diffuseColor;\n\
	for(int i = 0; i < lights.numDirLights; i++)\n\
	{\n\
		vec3 l = normalize(-lights.dirLights[i].direction.xyz); // Vector from surface point to light\n\
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
		color += pbrInputs.NdotL * lights.dirLights[i].color.rgb * (diffuseContrib + specContrib);\n\
	}\n\
	for(int i = 0; i < lights.numPointLights; i++)\n\
	{\n\
		vec3 l = normalize(lights.pointLights[i].pos.xyz - worldPos);\n\
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
		color += pbrInputs.NdotL * lights.pointLights[i].color.rgb * (diffuseContrib + specContrib);\n\
	}\n\
	for(int i = 0; i < lights.numSpotLights; i++)\n\
	{\n\
		vec3 l = normalize(lights.spotLights[i].pos.xyz - worldPos);\n\
		float theta = dot(-l, normalize(lights.spotLights[i].direction.xyz));\n\
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
		if(theta > lights.spotLights[i].cutOff)\n\
		{\n\
			color += pbrInputs.NdotL * lights.spotLights[i].color.rgb * (diffuseContrib + specContrib);\n\
		}\n\
		else\n\
		{\n\
			color += pbrInputs.NdotL * lights.spotLights[i].color.rgb * (specContrib);\n\
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

struct Renderer
{
	const CameraBase* currentCam;
	const EnvironmentData* currentEnvironmentData;
	GLuint currentLightData;
	GLuint baseProgram;
	BaseProgramUniforms baseUniforms;
	CubemapRenderInfo cubemapInfo;

	GLuint defaultBoneData;
	GLuint brdfLut;
	GLuint whiteTexture;	// these are not owned by the renderer
	GLuint blackTexture;	// these are not owned by the renderer

	uint32_t numObjs;
	struct SceneObject** objList;
};





static void BindMaterial(Renderer* r, Material* mat)
{
	if (mat)
	{
		glBindBufferBase(GL_UNIFORM_BUFFER, r->baseUniforms.materialDataLoc, mat->uniform);
		
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

	return un;
}






struct Renderer* RE_CreateRenderer(struct AssetManager* assets)
{
	Renderer* renderer = new Renderer;
	renderer->currentCam = nullptr;
	renderer->currentEnvironmentData = nullptr;
	renderer->currentLightData = 0;
	// Create Material Defaults
	{
		CreateBoneDataFromAnimation(nullptr, &renderer->defaultBoneData);
		renderer->whiteTexture = assets->textures[DEFAULT_WHITE_TEXTURE]->uniform;
		renderer->blackTexture = assets->textures[DEFAULT_BLACK_TEXTURE]->uniform;
	}
	// Create Main 3d shader
	{
		renderer->baseProgram = CreateProgram(baseVertexShader, baseFragmentShader);
		renderer->baseUniforms = LoadBaseProgramUniformsLocationsFromProgram(renderer->baseProgram);
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
			FragColor = texture(skybox, TexCoords);\n\
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

	renderer->brdfLut = CreateBRDFLut(512, 512);
	return renderer;
}
void RE_CleanUpRenderer(struct Renderer* renderer)
{
	glDeleteTextures(1, &renderer->brdfLut);
	glDeleteBuffers(1, &renderer->defaultBoneData);
	// delete base Shader
	{
		glDeleteProgram(renderer->baseProgram);
	}
	// Delete cubemap infos
	{
		glDeleteBuffers(1, &renderer->cubemapInfo.vtxBuf);
		glDeleteVertexArrays(1, &renderer->cubemapInfo.vao);
		glDeleteProgram(renderer->cubemapInfo.program);
		glDeleteProgram(renderer->cubemapInfo.irradianceFilterProgram);
	}
	delete renderer;
}

void RE_CreateEnvironment(struct Renderer* renderer, EnvironmentData* env)
{
	// SETTINGS
	constexpr float deltaPhi = (2.0f * float(M_PI)) / 180.0f;
	constexpr float deltaTheta = (0.5f * float(M_PI)) / 64.0f;
	constexpr uint32_t numSamples = 32;

	uint32_t numMipLevels = 1 + floor(log2(glm::max(env->width, env->height)));
	env->mipLevels = numMipLevels;
	GLuint framebuffer;
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);


	PerspectiveCamera perspectiveCam;
	memset(&perspectiveCam, 0, sizeof(PerspectiveCamera));
	perspectiveCam.width = env->width;
	perspectiveCam.height = env->height;

	RE_SetCameraBase(renderer, &perspectiveCam.base);
	RE_SetEnvironmentData(renderer, env);

	// CREATE IRRADIANCE MAP
	{
		glGenTextures(1, &env->irradianceMap);
		glBindTexture(GL_TEXTURE_CUBE_MAP, env->irradianceMap);
		for (unsigned int i = 0; i < 6; i++)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, env->width, env->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, env->irradianceMap, 0);

		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

		GLenum drawBuffers = GL_COLOR_ATTACHMENT0;
		glDrawBuffers(1, &drawBuffers);
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			LOG("FAILED TO CREATE FRAMEBUFFER OBJECT, STATUS: %d\n", status);
			return;
		}

		for (uint32_t i = 0; i < numMipLevels; i++)
		{
			glViewport(0, 0, glm::max(env->width >> i, 1u), glm::max(env->height >> i, 1u));
			perspectiveCam.yaw = 0.0f; CA_UpdatePerspectiveCamera(&perspectiveCam); perspectiveCam.base.view = glm::rotate(glm::mat4(1.0f), 3.14159f, glm::vec3(0.0f, 0.0f, 1.0f)) * perspectiveCam.base.view;
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, env->irradianceMap, i);
			RE_RenderIrradiance(renderer, deltaPhi, deltaTheta, CUBE_MAP_FRONT);


			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + 1, env->irradianceMap, i);
			perspectiveCam.yaw = 180.0f; CA_UpdatePerspectiveCamera(&perspectiveCam); perspectiveCam.base.view = glm::rotate(glm::mat4(1.0f), 3.14159f, glm::vec3(0.0f, 0.0f, 1.0f)) * perspectiveCam.base.view;
			RE_RenderIrradiance(renderer, deltaPhi, deltaTheta, CUBE_MAP_BACK);


			perspectiveCam.pitch = 90.0f; CA_UpdatePerspectiveCamera(&perspectiveCam); perspectiveCam.base.view = glm::rotate(glm::mat4(1.0f), 3.14159f / 2.0f, glm::vec3(0.0f, 0.0f, 1.0f)) * perspectiveCam.base.view;
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + 2, env->irradianceMap, i);
			RE_RenderIrradiance(renderer, deltaPhi, deltaTheta, CUBE_MAP_TOP);


			perspectiveCam.pitch = -90.0f; CA_UpdatePerspectiveCamera(&perspectiveCam); perspectiveCam.base.view = glm::rotate(glm::mat4(1.0f), -3.14159f / 2.0f, glm::vec3(0.0f, 0.0f, 1.0f)) * perspectiveCam.base.view;
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + 3, env->irradianceMap, i);
			RE_RenderIrradiance(renderer, deltaPhi, deltaTheta, CUBE_MAP_BOTTOM);


			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + 4, env->irradianceMap, i);
			perspectiveCam.yaw = 90.0f; perspectiveCam.pitch = 0.0f; CA_UpdatePerspectiveCamera(&perspectiveCam); perspectiveCam.base.view = glm::rotate(glm::mat4(1.0f), 3.14159f, glm::vec3(0.0f, 0.0f, 1.0f)) * perspectiveCam.base.view;
			RE_RenderIrradiance(renderer, deltaPhi, deltaTheta, CUBE_MAP_RIGHT);


			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + 5, env->irradianceMap, i);
			perspectiveCam.yaw = -90.0f; CA_UpdatePerspectiveCamera(&perspectiveCam); perspectiveCam.base.view = glm::rotate(glm::mat4(1.0f), 3.14159f, glm::vec3(0.0f, 0.0f, 1.0f)) * perspectiveCam.base.view;
			RE_RenderIrradiance(renderer, deltaPhi, deltaTheta, CUBE_MAP_LEFT);
		}
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
			perspectiveCam.yaw = 0.0f; CA_UpdatePerspectiveCamera(&perspectiveCam); perspectiveCam.base.view = glm::rotate(glm::mat4(1.0f), 3.14159f, glm::vec3(0.0f, 0.0f, 1.0f)) * perspectiveCam.base.view;
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X, env->prefilteredMap, i);
			RE_RenderPreFilterCubeMap(renderer, roughness, numSamples, CUBE_MAP_FRONT);


			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + 1, env->prefilteredMap, i);
			perspectiveCam.yaw = 180.0f; CA_UpdatePerspectiveCamera(&perspectiveCam); perspectiveCam.base.view = glm::rotate(glm::mat4(1.0f), 3.14159f, glm::vec3(0.0f, 0.0f, 1.0f)) * perspectiveCam.base.view;
			RE_RenderPreFilterCubeMap(renderer, roughness, numSamples, CUBE_MAP_BACK);


			perspectiveCam.pitch = 90.0f; CA_UpdatePerspectiveCamera(&perspectiveCam); perspectiveCam.base.view = glm::rotate(glm::mat4(1.0f), 3.14159f / 2.0f, glm::vec3(0.0f, 0.0f, 1.0f)) * perspectiveCam.base.view;
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + 2, env->prefilteredMap, i);
			RE_RenderPreFilterCubeMap(renderer, roughness, numSamples, CUBE_MAP_TOP);


			perspectiveCam.pitch = -90.0f; CA_UpdatePerspectiveCamera(&perspectiveCam); perspectiveCam.base.view = glm::rotate(glm::mat4(1.0f), -3.14159f / 2.0f, glm::vec3(0.0f, 0.0f, 1.0f)) * perspectiveCam.base.view;
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + 3, env->prefilteredMap, i);
			RE_RenderPreFilterCubeMap(renderer, roughness, numSamples, CUBE_MAP_BOTTOM);


			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + 4, env->prefilteredMap, i);
			perspectiveCam.yaw = 90.0f; perspectiveCam.pitch = 0.0f; CA_UpdatePerspectiveCamera(&perspectiveCam); perspectiveCam.base.view = glm::rotate(glm::mat4(1.0f), 3.14159f, glm::vec3(0.0f, 0.0f, 1.0f)) * perspectiveCam.base.view;
			RE_RenderPreFilterCubeMap(renderer, roughness, numSamples, CUBE_MAP_RIGHT);


			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + 5, env->prefilteredMap, i);
			perspectiveCam.yaw = -90.0f; CA_UpdatePerspectiveCamera(&perspectiveCam); perspectiveCam.base.view = glm::rotate(glm::mat4(1.0f), 3.14159f, glm::vec3(0.0f, 0.0f, 1.0f)) * perspectiveCam.base.view;
			RE_RenderPreFilterCubeMap(renderer, roughness, numSamples, CUBE_MAP_LEFT);
		}
	}

	glDeleteFramebuffers(1, &framebuffer);
}

void RE_BeginScene(struct Renderer* renderer, struct SceneObject** objList, uint32_t num)
{
	renderer->numObjs = num;
	renderer->objList = objList;
}

void RE_SetCameraBase(struct Renderer* renderer, const struct CameraBase* camBase)
{
	renderer->currentCam = camBase;
}
void RE_SetEnvironmentData(struct Renderer* renderer, const EnvironmentData* data)
{
	renderer->currentEnvironmentData = data;
}
void RE_SetLightData(struct Renderer* renderer, GLuint lightUniform)
{
	renderer->currentLightData = lightUniform;
}


void RE_RenderIrradiance(struct Renderer* renderer, float deltaPhi, float deltaTheta, CUBE_MAP_SIDE side)
{
	assert(renderer->currentCam != nullptr, "The Camera base needs to be set before rendering");
	assert(renderer->currentEnvironmentData != nullptr, "The Camera base needs to be set before rendering");

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
	assert(renderer->currentCam != nullptr, "The Camera base needs to be set before rendering");
	assert(renderer->currentEnvironmentData != nullptr, "The Camera base needs to be set before rendering");

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





void RE_RenderOpaque(struct Renderer* renderer)
{
	assert(renderer->currentCam != nullptr, "The Camera base needs to be set before rendering");
	assert((renderer->numObjs != 0 && renderer->objList != nullptr) || renderer->numObjs == 0, "Need to Call Begin Scene Before Rendering");
	assert(renderer->currentEnvironmentData != 0, "Need to Set Current Environment Before Rendering");
	assert(renderer->currentLightData != 0, "Need to Set Current Light Data Before Rendering");
	if (renderer->numObjs == 0) return;
	
	glUseProgram(renderer->baseProgram);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);

	glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_PREFILTERED_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, renderer->currentEnvironmentData->prefilteredMap);
	glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_SAMPLER_IRRADIANCE);
	glBindTexture(GL_TEXTURE_CUBE_MAP, renderer->currentEnvironmentData->irradianceMap);
	glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_BRDFLUT);
	glBindTexture(GL_TEXTURE_2D, renderer->brdfLut);


	glUniform3fv(renderer->baseUniforms.camPosLoc, 1, (const GLfloat*)&renderer->currentCam->pos);
	glUniformMatrix4fv(renderer->baseUniforms.viewLoc, 1, GL_FALSE, (const GLfloat*)&renderer->currentCam->view);
	glUniformMatrix4fv(renderer->baseUniforms.projLoc, 1, GL_FALSE, (const GLfloat*)&renderer->currentCam->proj);

	glUniform1f(renderer->baseUniforms.prefilteredCubeMipLevelsLoc, renderer->currentEnvironmentData->mipLevels);
	glUniform1f(renderer->baseUniforms.scaleIBLAmbientLoc, 1.0f);

	glBindBufferBase(GL_UNIFORM_BUFFER, renderer->baseUniforms.lightDataLoc, renderer->currentLightData);
	glBindBufferBase(GL_UNIFORM_BUFFER, renderer->baseUniforms.boneDataLoc, renderer->defaultBoneData);
	
	for (uint32_t i = 0; i < renderer->numObjs; i++)
	{
		SceneObject* obj = renderer->objList[i];
		if (obj->model && (obj->flags & SCENE_OBJECT_FLAG_VISIBLE))
		{
			glm::mat4 mat = obj->transform * obj->model->baseTransform;
			glUniformMatrix4fv(renderer->baseUniforms.modelLoc, 1, GL_FALSE, (const GLfloat*)&mat);
			
			glBindVertexArray(obj->model->vao);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->model->indexBuffer);
			for (uint32_t j = 0; j < obj->model->numMeshes; j++)
			{
				Mesh* m = &obj->model->meshes[j];
				BindMaterial(renderer, m->material);
				glDrawElements(GL_TRIANGLES, m->numInd, GL_UNSIGNED_INT, (void*)(m->startIdx * sizeof(uint32_t)));
			}
		}
	}


}
void RE_RenderTransparent(struct Renderer* renderer)
{
	assert(renderer->currentCam != nullptr, "The Camera base needs to be set before rendering");
	assert((renderer->numObjs != 0 && renderer->objList != nullptr) || renderer->numObjs == 0, "Need to Call Begin Scene Before Rendering");
	assert(renderer->currentEnvironmentData != 0, "Need to Set Current Environment Before Rendering");
	assert(renderer->currentLightData != 0, "Need to Set Current Light Data Before Rendering");
	if (renderer->numObjs == 0) return;

	glUseProgram(renderer->baseProgram);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_PREFILTERED_MAP);
	glBindTexture(GL_TEXTURE_2D, renderer->currentEnvironmentData->environmentMap);
	glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_SAMPLER_IRRADIANCE);
	glBindTexture(GL_TEXTURE_2D, renderer->currentEnvironmentData->irradianceMap);
	glActiveTexture(GL_TEXTURE0 + BASE_SHADER_TEXTURE_BRDFLUT);
	glBindTexture(GL_TEXTURE_2D, renderer->brdfLut);


	glUniform3fv(renderer->baseUniforms.camPosLoc, 1, (const GLfloat*)&renderer->currentCam->pos);
	glUniformMatrix4fv(renderer->baseUniforms.viewLoc, 1, GL_FALSE, (const GLfloat*)&renderer->currentCam->view);
	glUniformMatrix4fv(renderer->baseUniforms.projLoc, 1, GL_FALSE, (const GLfloat*)&renderer->currentCam->proj);

	glUniform1f(renderer->baseUniforms.prefilteredCubeMipLevelsLoc, renderer->currentEnvironmentData->mipLevels);
	glUniform1f(renderer->baseUniforms.scaleIBLAmbientLoc, 1.0f);

	glBindBufferBase(GL_UNIFORM_BUFFER, renderer->baseUniforms.boneDataLoc, renderer->defaultBoneData);


	for (uint32_t i = 0; i < renderer->numObjs; i++)
	{
		SceneObject* obj = renderer->objList[i];
		if (obj->model && (obj->flags & SCENE_OBJECT_FLAG_VISIBLE))
		{
			glm::mat4 mat = obj->transform * obj->model->baseTransform;
			glUniformMatrix4fv(renderer->baseUniforms.modelLoc, 1, GL_FALSE, (const GLfloat*)&mat);

			glBindVertexArray(obj->model->vao);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->model->indexBuffer);

			for (uint32_t j = 0; j < obj->model->numMeshes; j++)
			{
				Mesh* m = &obj->model->meshes[j];
				BindMaterial(renderer, m->material);
				glDrawElements(GL_TRIANGLES, m->numInd, GL_UNSIGNED_INT, (void*)(m->startIdx * sizeof(uint32_t)));
			}
		}
	}


}
void RE_RenderCubeMap(struct Renderer* renderer, GLuint cubemap)
{
	assert(renderer->currentCam != nullptr, "The Camera base needs to be set before rendering");
	
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
	renderer->numObjs = 0xFFFFFFFF;
	renderer->objList = nullptr;

	renderer->currentCam = nullptr;
	renderer->currentEnvironmentData = nullptr;
	renderer->currentLightData = 0;

}