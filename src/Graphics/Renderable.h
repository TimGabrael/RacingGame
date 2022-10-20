#pragma once
#include "GLIncludes.h"
#include "Camera.h"
#include "ModelInfo.h"




struct SceneRenderData
{
	struct Renderer* renderer;
	const CameraBase* cam;
	const struct EnvironmentData* env;
	GLuint lightGroupUniform;
	GLuint shadowMapTexture;
};

struct RenderCommand
{
	virtual void DrawShadow(SceneRenderData* data) const = 0;
	virtual void DrawGeom(SceneRenderData* data) const = 0;
	virtual void DrawOpaque(SceneRenderData* data) const = 0;
	virtual void DrawTransparent(SceneRenderData* data) const = 0;
	virtual void DrawSSR(SceneRenderData* data) const = 0;
	virtual void DrawOutline(SceneRenderData* data) const = 0;

	virtual bool IsTransparent() const = 0;

	virtual const AABB& GetBound() const = 0;
	virtual uint32_t GetSize() const = 0;
	virtual float GetZDepth() const = 0;
	virtual void SetZDepth(float zDepth) = 0;
};

// THE DEFINITIONS ARE IN Renderer.cpp
struct PBRRenderCommand : public RenderCommand
{
	enum PBR_RENDER_FLAGS
	{
		PBR_RENDER_SHADOW = 1,
		PBR_RENDER_GEOMETRY = 2,
		PBR_RENDER_OPAQUE = 4,
		PBR_RENDER_TRANSPARENT = 8,
		PBR_RENDER_SSR = 16,
	};
	GLuint vao = 0;
	GLuint indexBuffer = 0;
	GLuint animUniform = 0;
	uint32_t startIdx = 0;
	uint32_t numIdx = 0;
	uint32_t primFlags = 0;
	AABB bound = { };
	glm::mat4 transform = glm::mat4(1.0f);
	Material* material = 0;
	float zDepth = 0;
	uint32_t renderFlags = 0;
	virtual void DrawShadow(SceneRenderData* data) const override;
	virtual void DrawGeom(SceneRenderData* data) const override;
	virtual void DrawOpaque(SceneRenderData* data) const override;
	virtual void DrawTransparent(SceneRenderData* data) const override;
	virtual void DrawSSR(SceneRenderData* data) const override;
	virtual void DrawOutline(SceneRenderData* data) const override;

	virtual bool IsTransparent() const override { return (renderFlags & PBR_RENDER_TRANSPARENT); }

	virtual const AABB& GetBound() const override { return bound; };
	virtual uint32_t GetSize() const override { return sizeof(PBRRenderCommand); };
	virtual float GetZDepth() const override { return zDepth; };
	virtual void SetZDepth(float zDepth) override { this->zDepth = zDepth; };
};


enum RENDERABLE_FLAGS
{
	INACTIVE = 1,
};

struct Renderable
{
	Renderable();
	virtual ~Renderable();
	virtual uint32_t GetNumCommands() const = 0;
	virtual uint32_t FillRenderCommandsOpaque(RenderCommand** cmdList) const = 0;		 // returns num added to list
	virtual uint32_t FillRenderCommandsTransparent(RenderCommand** cmdList) const = 0; // returns num added to list
	virtual const RenderCommand* GetRenderCommand(uint32_t idx) const = 0;
	virtual const AABB& GetBoundingBox() const = 0;
	virtual uint32_t GetFlags() const = 0;
};

struct PBRRenderable : public Renderable
{
	PBRRenderable(const Model* model, AnimationInstanceData* optionalAnim, const glm::mat4& transform);
	virtual ~PBRRenderable();
	RenderCommand* cmds;
	uint32_t numCmds;
	uint32_t flags;
	AABB bound;

	void Initialize(const Model* model, AnimationInstanceData* optionalAnim, const glm::mat4& transform);
	void Update(const Model* model, AnimationInstanceData* optionalAnim, const glm::mat4& transform);

	virtual uint32_t GetNumCommands() const override;
	virtual uint32_t FillRenderCommandsOpaque(RenderCommand** cmdList) const override;
	virtual uint32_t FillRenderCommandsTransparent(RenderCommand** cmdList) const override;
	virtual const RenderCommand* GetRenderCommand(uint32_t idx) const override;
	virtual const AABB& GetBoundingBox() const override;
	virtual uint32_t GetFlags() const override;
};

