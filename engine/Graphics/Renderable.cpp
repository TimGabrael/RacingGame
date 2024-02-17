#include "Renderable.h"
#include "Scene.h"
#include "GameState.h"

Renderable::Renderable()
{
	SC_AddRenderable(GetGameState()->scene, this);
}
Renderable::~Renderable()
{
	SC_RemoveRenderable(GetGameState()->scene, this);
}

PBRRenderable::PBRRenderable(const Model* model, AnimationInstanceData* optionalAnim, const glm::mat4& transform) : Renderable()
{
	cmds = nullptr;
	numCmds = 0;
	flags = 0;
	Initialize(model, optionalAnim, transform);
}
PBRRenderable::~PBRRenderable()
{
	if (cmds) delete[] (PBRRenderCommand*)cmds;
	cmds = nullptr;
	numCmds = 0;
}


void PBRRenderable::Initialize(const Model* model, AnimationInstanceData* optionalAnim, const glm::mat4& transform)
{
	if (model->numJoints == 0) return;

	uint32_t numCommands = 0;
	for (uint32_t j = 0; j < model->numNodes; j++)
	{
		for (uint32_t k = 0; k < model->nodes[j]->mesh->numPrimitives; k++)
		{
			if (!(model->nodes[j]->mesh->primitives[k].flags & PRIMITIVE_FLAG_UNSUPPORTED))
			{
				numCommands++;
			}
		}
	}
	if (numCommands == 0) return;
	numCmds = numCommands;
	cmds = new PBRRenderCommand[numCommands];
	Update(model, optionalAnim, transform);

	// FILL ALL RENDER FLAGS EXCEPT TRANSPARENT
	for (uint32_t i = 0; i < numCmds; i++)
	{
		PBRRenderCommand* cmd = (PBRRenderCommand*)((uintptr_t)cmds + sizeof(PBRRenderCommand) * i);
		cmd->renderFlags = cmd->renderFlags | PBRRenderCommand::PBR_RENDER_FLAGS::PBR_RENDER_GEOMETRY | PBRRenderCommand::PBR_RENDER_FLAGS::PBR_RENDER_SHADOW |
			PBRRenderCommand::PBR_RENDER_FLAGS::PBR_RENDER_SSR | PBRRenderCommand::PBR_RENDER_FLAGS::PBR_RENDER_OPAQUE;
	}
}
void PBRRenderable::Update(const Model* model, AnimationInstanceData* optionalAnim, const glm::mat4& transform)
{
	bound = { glm::vec3(FLT_MAX), glm::vec3(-FLT_MAX) };
	uint32_t curCommandIdx = 0;
	for (uint32_t j = 0; j < model->numNodes; j++)
	{
		for (uint32_t k = 0; k < model->nodes[j]->mesh->numPrimitives; k++)
		{

			if (!(model->nodes[j]->mesh->primitives[k].flags & PRIMITIVE_FLAG_UNSUPPORTED))
			{
				PBRRenderCommand* cmd = (PBRRenderCommand*)((uintptr_t)cmds + sizeof(PBRRenderCommand) * curCommandIdx);

				Primitive* prim = &model->nodes[j]->mesh->primitives[k];

				cmd->vao = model->nodes[j]->model->vao;
				cmd->indexBuffer = model->nodes[j]->model->indexBuffer;
				cmd->primFlags = prim->flags;
				cmd->material = prim->material;
				cmd->numIdx = prim->numInd;
				cmd->startIdx = prim->startIdx;

				if (model->animations && optionalAnim)
				{
					if (static_cast<uint32_t>(model->nodes[j]->skinIndex) < optionalAnim->numSkins)
					{
						cmd->transform = transform * model->baseTransform * optionalAnim->data[model->nodes[j]->skinIndex].baseTransform;
						cmd->animUniform = optionalAnim->data[model->nodes[j]->skinIndex].skinUniform;
					}
				}
				else
				{
					cmd->transform = transform * model->baseTransform * model->nodes[j]->defMatrix;
				}
				if (prim->material && prim->material->mode == Material::ALPHA_MODE_BLEND)
				{
					cmd->renderFlags |= PBRRenderCommand::PBR_RENDER_FLAGS::PBR_RENDER_TRANSPARENT;
				}
				else
				{
					cmd->renderFlags &= ~PBRRenderCommand::PBR_RENDER_FLAGS::PBR_RENDER_TRANSPARENT;
				}

				glm::vec3 min = model->nodes[j]->mesh->bound.min;
				glm::vec3 max = model->nodes[j]->mesh->bound.max;
				glm::vec4 positions[8] = {
					cmd->transform * glm::vec4(min.x, min.y, min.z, 1.0f),
					cmd->transform * glm::vec4(max.x, min.y, min.z, 1.0f),
					cmd->transform * glm::vec4(min.x, max.y, min.z, 1.0f),
					cmd->transform * glm::vec4(max.x, max.y, min.z, 1.0f),
					cmd->transform * glm::vec4(min.x, min.y, max.z, 1.0f),
					cmd->transform * glm::vec4(max.x, min.y, max.z, 1.0f),
					cmd->transform * glm::vec4(min.x, max.y, max.z, 1.0f),
					cmd->transform * glm::vec4(max.x, max.y, max.z, 1.0f),
				};
				min = glm::vec3(FLT_MAX);
				max = glm::vec3(-FLT_MAX);
				for (int posIdx = 0; posIdx < 8; posIdx++)
				{
					positions[posIdx] /= positions[posIdx].w;
					min = glm::min(min, glm::vec3(positions[posIdx].x, positions[posIdx].y, positions[posIdx].z));
					max = glm::max(max, glm::vec3(positions[posIdx].x, positions[posIdx].y, positions[posIdx].z));
				}
				cmd->bound = { min, max };

				bound.min = glm::min(bound.min, min);
				bound.max = glm::max(bound.max, max);

				curCommandIdx++;
			}
		}
	}
}

uint32_t PBRRenderable::GetNumCommands() const
{
	return numCmds;
}
uint32_t PBRRenderable::FillRenderCommandsOpaque(RenderCommand** cmdList) const
{
	uint32_t addedCount = 0;
	for (uint32_t i = 0; i < this->numCmds; i++)
	{
		if (!cmds->IsTransparent())
		{
			cmdList[i] = (RenderCommand*)((uintptr_t)cmds + cmds->GetSize() * i);
			addedCount++;
		}
	}
	return addedCount;
}
uint32_t PBRRenderable::FillRenderCommandsTransparent(RenderCommand** cmdList) const
{
	uint32_t addedCount = 0;
	for (uint32_t i = 0; i < this->numCmds; i++)
	{
		if (cmds->IsTransparent())
		{
			cmdList[i] = (RenderCommand*)((uintptr_t)cmds + cmds->GetSize() * i);
			addedCount++;
		}
	}
	return addedCount;
}
const RenderCommand* PBRRenderable::GetRenderCommand(uint32_t idx) const
{
	if (idx < numCmds) return (RenderCommand*)((uintptr_t)cmds + cmds->GetSize() * idx);
	return nullptr;
}

const AABB& PBRRenderable::GetBoundingBox() const
{
	return bound;
}
uint32_t PBRRenderable::GetFlags() const
{
	return flags;
}
