#pragma once

#include "matrix.h"
#include <stdbool.h>

struct bh_ctx;
struct bh_sprite_entity;
typedef void (*bh_sprite_entity_cb)(struct bh_ctx* state, struct bh_sprite_entity* entity);

struct bh_sprite {
    m4 transform;
    GLuint64 texture_handle;
    GLuint flags;
};

struct bh_bounding_box {
    struct vec2 top_left;
    struct vec2 bottom_right;
};

enum bh_entity_type {
    BH_BULLET = 0,
    BH_PLAYER,
};

struct bh_sprite_entity {
    struct bh_sprite sprite;
    struct vec2 position;
    struct vec2 scale;
    float rotation;
    struct bh_bounding_box bb;

    enum bh_entity_type type;
    bh_sprite_entity_cb callback;
    void* state;
};
