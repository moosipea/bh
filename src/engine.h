#pragma once

#include <stdbool.h>

#include "entitydef.h"
#include "qtree.h"
#include "renderer.h"

struct bh_entity_ll {
    struct bh_sprite_entity entity;
    struct bh_entity_ll* next;
};

struct bh_de_ll {
    struct bh_entity_ll* entities;
    struct bh_entity_ll* last;
};

typedef bool (*user_cb)(struct bh_ctx* ctx, void* user_state);

struct bh_ctx {
    struct bh_renderer renderer;
    float dt;

    struct bh_de_ll entities;
    struct bh_qtree entity_qtree;

    GLuint64 debug_texture;
    GLuint64 green_debug_texture;

    void* user_state;
};

bool BH_InitContext(struct bh_ctx* ctx, void* user_state, user_cb user_init);
void BH_RunContext(struct bh_ctx* ctx);

bool BH_GetKey(int glfw_key);

void BH_SpawnEntity(struct bh_de_ll* entities, struct bh_sprite_entity entity);
void BH_DeinitEntities(struct bh_entity_ll* entities);

bool BH_DoEntitiesCollide(struct bh_sprite_entity* entity, struct bh_sprite_entity* other);
