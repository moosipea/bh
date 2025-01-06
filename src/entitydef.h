#pragma once

#include "matrix.h"
#include <stdbool.h>

struct BH_Context;
struct BH_SpriteEntity;
typedef void (*BH_SpriteEntityCB)(struct BH_Context* state, struct BH_SpriteEntity* entity);

struct BH_Sprite {
    m4 transform;
    GLuint64 texture_handle;
    GLuint flags;
};

struct BH_BB {
    struct vec2 top_left;
    struct vec2 bottom_right;
};

enum BH_EntityType {
    BH_BULLET = 0,
    BH_PLAYER,
};

struct BH_SpriteEntity {
    struct BH_Sprite sprite;
    struct vec2 position;
    struct vec2 scale;
    float rotation;
    struct BH_BB bb;

    enum BH_EntityType type;
    BH_SpriteEntityCB callback;
    void* state;
};
