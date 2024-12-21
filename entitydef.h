#pragma once

#include "matrix.h"

struct bh_ctx;
struct bh_sprite_entity;
typedef void (*bh_sprite_entity_cb)(
    struct bh_ctx* state, struct bh_sprite_entity* entity
);

struct bh_sprite {
    GLuint64 texture_handle;
    m4 transform;
};

struct bh_sprite_entity {
    struct bh_sprite sprite;
    struct vec2 position;
    struct vec2 scale;
    float rotation;
    bh_sprite_entity_cb callback;
};
