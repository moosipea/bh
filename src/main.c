#include <glad/gl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../res/built_assets.h"
#include "engine.h"
#include "entitydef.h"
#include "error_macro.h"
#include "qtree.h"

#define TEST_SPRITES 16

static float uniform_rand(void) { return (float)rand() / (float)RAND_MAX; }

static void test_entity_system(struct BH_Context* ctx, struct BH_SpriteEntity* entity) {
    entity->position.y += 256.0f * ctx->dt;
    if (entity->position.y >= ctx->renderer.height) {
        entity->position.x = uniform_rand() * ctx->renderer.width;
        entity->position.y = 0.0f;
    }
}

static void spawn_test_entities(struct BH_Context* ctx) {
    GLuint64 star_texture =
        BH_LoadTexture(&ctx->renderer.textures, (void*)ASSET_star, sizeof(ASSET_star) - 1);
    for (size_t i = 0; i < TEST_SPRITES; i++) {

        struct BH_Sprite sprite = { 0 };
        sprite.texture_handle = star_texture;

        // clang-format off
        struct BH_SpriteEntity entity = {
            .sprite = sprite,
            .position = { ctx->renderer.width * uniform_rand(), ctx->renderer.height * uniform_rand() },
            .scale = { 32.0f, 32.0f },
            .depth = 2.0f,
            .bb = {
                { -12.0f, -12.0f },
                { 12.0f, 12.0f },
            },
            .callback = test_entity_system,
        };
        // clang-format on

        BH_SpawnEntity(&ctx->entities, entity);
    }
}

static struct BH_BB expand_bb(struct BH_BB bb, float by) {
    return (struct BH_BB){
        .top_left = {     bb.top_left.x - by,     bb.top_left.y + by },
        .bottom_right = { bb.bottom_right.x + by, bb.bottom_right.y - by }
    };
}

struct player_state {
    float immunity;
};

static void update_player_system(struct BH_Context* ctx, struct BH_SpriteEntity* player) {
    struct BH_BB bb = BH_BoxToWorld(player->position, expand_bb(player->bb, 0.15f));
    struct BH_QTreeQuery collision_query = BH_QueryQTree(&ctx->entity_qtree, bb);

    struct player_state* state = player->state;
    for (size_t i = 0; i < collision_query.count; i++) {
        struct BH_SpriteEntity* entity = collision_query.entities[i]->entity;

        if (entity->type == BH_PLAYER) {
            continue;
        }

        if (BH_DoEntitiesCollide(player, entity) && state->immunity <= 0.01f) {
            state->immunity = 0.33f;
            break;
        }
    }

    BH_DeinitQuery(collision_query);

    /* Update immunity timer */
    state->immunity -= ctx->dt;
    if (state->immunity < 0.0f) {
        state->immunity = 0.0f;
    }

    if (BH_GetKey(GLFW_KEY_W)) {
        player->position.y -= 128.0f * ctx->dt;
    }
    if (BH_GetKey(GLFW_KEY_S)) {
        player->position.y += 128.0f * ctx->dt;
    }
    if (BH_GetKey(GLFW_KEY_A)) {
        player->position.x -= 128.0f * ctx->dt;
    }
    if (BH_GetKey(GLFW_KEY_D)) {
        player->position.x += 128.0f * ctx->dt;
    }
}

static void spawn_player_entity(struct BH_Context* ctx) {
    struct BH_Sprite sprite = { 0 };
    sprite.texture_handle =
        BH_LoadTexture(&ctx->renderer.textures, (void*)ASSET_player, sizeof(ASSET_player) - 1);

    // clang-format off
    struct BH_SpriteEntity entity = {
        .sprite = sprite,
        .position = { ctx->renderer.width / 2.0f, ctx->renderer.height / 2.0f },
        .scale = { 64.0f, 64.0f },
        .depth = 1.0f,
        .bb = {
            { -32.0f, -32.0f },
            { 32.0f, 32.0f },
        },
        .type = BH_PLAYER,
        .callback = update_player_system,
    };
    // clang-format on

    struct player_state state = { .immunity = 0.0f };

    entity.state = calloc(1, sizeof(struct player_state));
    memcpy(entity.state, &state, sizeof(struct player_state));

    BH_SpawnEntity(&ctx->entities, entity);
}

bool user_init(struct BH_Context* ctx, void* state) {
    (void)state;

    spawn_test_entities(ctx);
    spawn_player_entity(ctx);

    return true;
}

int main(void) {
    struct BH_Context ctx = { 0 };

    if (!BH_InitContext(&ctx, NULL, user_init)) {
        error("Context initialisation failed");
        exit(1);
    }
    BH_RunContext(&ctx);

    return 0;
}
