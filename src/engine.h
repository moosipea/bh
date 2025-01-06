#pragma once

#include <stdbool.h>

#include "entitydef.h"
#include "qtree.h"
#include "renderer.h"

struct BH_EntityLL {
    struct BH_SpriteEntity entity;
    struct BH_EntityLL* next;
};

struct BH_DELL {
    struct BH_EntityLL* entities;
    struct BH_EntityLL* last;
};

typedef bool (*BH_UserCB)(struct BH_Context* ctx, void* user_state);

struct BH_Context {
    struct BH_Renderer renderer;
    float dt;

    struct BH_DELL entities;
    struct BH_QTree entity_qtree;

    GLuint64 debug_texture;
    GLuint64 green_debug_texture;

    void* user_state;
};

bool BH_InitContext(struct BH_Context* ctx, void* user_state, BH_UserCB user_init);
void BH_RunContext(struct BH_Context* ctx);

bool BH_GetKey(int glfw_key);

void BH_SpawnEntity(struct BH_DELL* entities, struct BH_SpriteEntity entity);
void BH_DeinitEntities(struct BH_EntityLL* entities);

bool BH_DoEntitiesCollide(struct BH_SpriteEntity* entity, struct BH_SpriteEntity* other);
