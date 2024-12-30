#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

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

    FT_Library ft;

    void* user_state;
};

bool init_ctx(struct bh_ctx* ctx, void* user_state, user_cb user_init);
void ctx_run(struct bh_ctx* ctx);

bool get_key(int glfw_key);

void spawn_entity(struct bh_de_ll* entities, struct bh_sprite_entity entity);
void entities_free(struct bh_entity_ll* entities);

bool entities_collide(struct bh_sprite_entity* entity, struct bh_sprite_entity* other);
