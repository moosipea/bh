#include <glad/gl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine.h"
#include "entitydef.h"
#include "error_macro.h"
#include "qtree.h"
#include "res/built_assets.h"

#define TEST_SPRITES 16

static inline float uniform_rand(void) { return 2.0f * ((float)rand() / (float)RAND_MAX) - 1.0f; }

static void test_entity_system(struct bh_ctx* ctx, struct bh_sprite_entity* entity) {
    (void)ctx;
    entity->position.y -= 1.0f / 120.0f;
    if (entity->position.y <= -1.0f) {
        entity->position.x = uniform_rand();
        entity->position.y = 1.0f;
    }
}

static inline void spawn_test_entities(struct bh_ctx* ctx) {
    GLuint64 star_texture =
        textures_load(&ctx->renderer.textures, (void*)ASSET_star, sizeof(ASSET_star) - 1);
    for (size_t i = 0; i < TEST_SPRITES; i++) {

        struct bh_sprite sprite = { 0 };
        sprite.texture_handle = star_texture;
        m4_identity(sprite.transform);

        // clang-format off
        struct bh_sprite_entity entity = {
            .sprite = sprite,
            .position = { uniform_rand(), uniform_rand() },
            .scale = { 0.05f, 0.05f },
            .bb = {
                { -0.05f, 0.05f },
                { 0.05f, -0.05f },
            },
            .callback = test_entity_system,
        };
        // clang-format on

        spawn_entity(&ctx->entities, entity);
    }
}

static inline struct bh_bounding_box expand_bb(struct bh_bounding_box bb, float by) {
    return (struct bh_bounding_box){
        .top_left = {     bb.top_left.x - by,     bb.top_left.y + by },
        .bottom_right = { bb.bottom_right.x + by, bb.bottom_right.y - by }
    };
}

struct player_state {
    float immunity;
};

static void update_player_system(struct bh_ctx* ctx, struct bh_sprite_entity* player) {
    struct bh_qtree_query collision_query =
        qtree_query(&ctx->entity_qtree, expand_bb(player->bb, 0.15f));

    struct player_state* state = player->state;
    for (size_t i = 0; i < collision_query.count; i++) {
        struct bh_sprite_entity* entity = collision_query.entities[i]->entity;

        if (entity->type == BH_PLAYER) {
            continue;
        }

        if (entities_collide(player, entity) && state->immunity <= 0.01f) {
            printf("Collision!\n");
            state->immunity = 0.5f;
            break;
        }
    }

    query_free(collision_query);

    /* Update immunity timer */
    state->immunity -= ctx->dt;
    if (state->immunity < 0.0f) {
        state->immunity = 0.0f;
    }

    if (get_key(GLFW_KEY_W)) {
        player->position.y += 1.0f * ctx->dt;
    }
    if (get_key(GLFW_KEY_S)) {
        player->position.y -= 1.0f * ctx->dt;
    }
    if (get_key(GLFW_KEY_A)) {
        player->position.x -= 1.0f * ctx->dt;
    }
    if (get_key(GLFW_KEY_D)) {
        player->position.x += 1.0f * ctx->dt;
    }
}

static inline void spawn_player_entity(struct bh_ctx* ctx) {
    struct bh_sprite sprite = { 0 };
    sprite.texture_handle =
        textures_load(&ctx->renderer.textures, (void*)ASSET_player, sizeof(ASSET_player) - 1);

    // clang-format off
    struct bh_sprite_entity entity = {
        .sprite = sprite,
        .position = { 0.0f, 0.0f },
        .scale = { 0.15f, 0.15f },
        .bb = {
            { -0.075f, 0.075f },
            { 0.075f, -0.075f },
        },
        .type = BH_PLAYER,
        .callback = update_player_system,
    };
    // clang-format on

    struct player_state state = { .immunity = 0.0f };

    entity.state = calloc(1, sizeof(struct player_state));
    memcpy(entity.state, &state, sizeof(struct player_state));

    spawn_entity(&ctx->entities, entity);
}

bool user_init(struct bh_ctx* ctx, void* state) {
    (void)state;

    spawn_test_entities(ctx);
    spawn_player_entity(ctx);

    return true;
}

int main(void) {
    struct bh_ctx ctx = { 0 };

    if (!init_ctx(&ctx, NULL, user_init)) {
        error("Context initialisation failed");
        exit(1);
    }
    ctx_run(&ctx);

    return 0;
}
