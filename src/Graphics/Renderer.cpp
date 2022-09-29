#include "Renderer.h"
#include "../Util/Assets.h"
#include "ModelInfo.h"
#include "Camera.h"

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
\n\
uniform samplerCube cubeMap;\n\
uniform sampler2D colorMap;\n\
uniform sampler2D normalMap;\n\
uniform sampler2D aoMap;\n\
uniform sampler2D emissiveMap;\n\
\n\
uniform mat4 model; \n\
uniform mat4 view; \n\
uniform mat4 projection; \n\
uniform vec3 camPos; \n\
\n\
out vec4 outColor;\n\
void main()\n\
{\n\
	outColor = fragColor;\n\
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

struct BaseProgramUniforms
{
	GLuint modelLoc;
	GLuint viewLoc;
	GLuint projLoc;
	GLuint camPosLoc;
	GLuint boneDataLoc;
};

enum BASE_SHADER_TEXTURE
{
	BASE_SHADER_TEXTURE_CUBEMAP,
	BASE_SHADER_TEXTURE_COLORMAP,
	BASE_SHADER_TEXTURE_NORMALMAP,
	BASE_SHADER_TEXTURE_AOMAP,
	BASE_SHADER_TEXTURE_EMISSIVEMAP,
	NUM_BASE_SHADER_TEXTURES,
};

struct CubemapRenderInfo
{
	GLuint vao;
	GLuint vtxBuf;
	GLuint program;
	GLuint viewProjLoc;
};

struct Renderer
{
	const CameraBase* currentCam;
	GLuint baseProgram;
	BaseProgramUniforms baseUniforms;
	CubemapRenderInfo cubemapInfo;

	GLuint defaultBoneData;
	GLuint whiteTexture;	// these are not owned by the renderer
	GLuint blackTexture;	// these are not owned by the renderer
};






static BaseProgramUniforms LoadBaseProgramUniformsLocationsFromProgram(GLuint program)
{
	BaseProgramUniforms un;
	un.modelLoc = glGetUniformLocation(program, "model");
	un.viewLoc = glGetUniformLocation(program, "view");
	un.projLoc = glGetUniformLocation(program, "projection");
	un.camPosLoc = glGetUniformLocation(program, "camPos");

	un.boneDataLoc = glGetUniformBlockIndex(program, "BoneData");
	glUniformBlockBinding(program, un.boneDataLoc, un.boneDataLoc);

	glUseProgram(program);
	GLint curTexture = glGetUniformLocation(program, "cubeMap");
	if (curTexture == -1) { LOG("failed to Get cubeMap Texture Loaction of GLTF shader program\n"); }
	glUniform1i(curTexture, BASE_SHADER_TEXTURE_CUBEMAP);
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


	return un;
}






struct Renderer* RE_CreateRenderer(struct AssetManager* assets)
{
	Renderer* renderer = new Renderer;
	renderer->currentCam = nullptr;

	// Create Material Defaults
	{
		CreateBoneDataDataFromAnimation(nullptr, &renderer->defaultBoneData);
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
		layout (location = 0) in vec3  aPos;\n\
		out vec3 TexCoords;\n\
		\
		uniform mat4 viewProj;\n\
		\
		void main(){\
			TexCoords = aPos;\
			vec4 pos = viewProj * vec4(aPos, 0.0);\n\
			gl_Position = pos.xyww;\
		}";
		static const char* cubemapFS = "#version 330\n\
		out vec4 FragColor;\n\
		\
		in vec3 TexCoords;\n\
		\
		uniform samplerCube skybox;\n\
		void main()\
		{\
			FragColor = texture(skybox, TexCoords);\n\
		}";
		static glm::vec3 cubeVertices[] = {
			{-1.0f, 1.0f, 1.0f},{-1.0f,-1.0f, 1.0f},{-1.0f,-1.0f,-1.0f},
			{-1.0f, 1.0f,-1.0f},{-1.0f,-1.0f,-1.0f},{1.0f, 1.0f,-1.0f},
			{1.0f,-1.0f,-1.0f},{-1.0f,-1.0f,-1.0f},{1.0f,-1.0f, 1.0f},
			{-1.0f,-1.0f,-1.0f},{1.0f,-1.0f,-1.0f},{1.0f, 1.0f,-1.0f},
			{-1.0f, 1.0f,-1.0f},{-1.0f, 1.0f, 1.0f},{-1.0f,-1.0f,-1.0f},
			{-1.0f,-1.0f,-1.0f},{-1.0f,-1.0f, 1.0f},{1.0f,-1.0f, 1.0f},
			{1.0f,-1.0f, 1.0f},{-1.0f,-1.0f, 1.0f},{-1.0f, 1.0f, 1.0f},
			{1.0f, 1.0f,-1.0f},{1.0f,-1.0f,-1.0f},{1.0f, 1.0f, 1.0f},
			{1.0f,-1.0f, 1.0f},{1.0f, 1.0f, 1.0f},{1.0f,-1.0f,-1.0f},
			{-1.0f, 1.0f,-1.0f},{1.0f, 1.0f,-1.0f},{1.0f, 1.0f, 1.0f},
			{-1.0f, 1.0f, 1.0f},{-1.0f, 1.0f,-1.0f},{1.0f, 1.0f, 1.0f},
			{1.0f,-1.0f, 1.0f},{-1.0f, 1.0f, 1.0f},{1.0f, 1.0f, 1.0f},
		};
		renderer->cubemapInfo.program = CreateProgram(cubemapVS, cubemapFS);
		renderer->cubemapInfo.viewProjLoc = glGetUniformLocation(renderer->cubemapInfo.program, "viewProj");
		glGenVertexArrays(1, &renderer->cubemapInfo.vao);
		glGenBuffers(1, &renderer->cubemapInfo.vtxBuf);
		glBindBuffer(GL_ARRAY_BUFFER, renderer->cubemapInfo.vtxBuf);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
		glUseProgram(renderer->cubemapInfo.program);
		GLint curTexture = glGetUniformLocation(renderer->cubemapInfo.program, "skybox");
		glUniform1i(curTexture, 0);

		glBindVertexArray(renderer->cubemapInfo.vao);

		glBindBuffer(GL_ARRAY_BUFFER, renderer->cubemapInfo.vtxBuf);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (const void*)0);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	return renderer;
}
void RE_CleanUpRenderer(struct Renderer* renderer)
{
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
	}
	delete renderer;
}

void RE_BeginScene(struct Renderer* renderer, struct Scene* scene)
{
	
}

void RE_SetCameraBase(struct Renderer* renderer, const struct CameraBase* camBase)
{
	renderer->currentCam = camBase;
}

void RE_RenderOpaque(struct Renderer* renderer, struct Scene* scene)
{
	assert(renderer->currentCam != nullptr, "The Camera base needs to be set before rendering");

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);



}
void RE_RenderTransparent(struct Renderer* renderer, struct Scene* scene)
{
	assert(renderer->currentCam != nullptr, "The Camera base needs to be set before rendering");

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