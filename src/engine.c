#include "engine.h"
#include "GLFW/glfw3.h"
#include "entitydef.h"
#include "matrix.h"
#include "qtree.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../res/built_assets.h"
#include "error_macro.h"
#include "renderer.h"

#define RENDER_DEBUG_INFO

void BH_SpawnEntity(struct bh_de_ll* entities, struct bh_sprite_entity entity) {
    if (entities->entities == NULL) {
        entities->entities = calloc(1, sizeof(struct bh_entity_ll));
    }

    if (entities->last == NULL) {
        entities->entities->entity = entity;
        entities->last = entities->entities;
    } else {
        entities->last->next = calloc(1, sizeof(struct bh_entity_ll));
        entities->last->next->entity = entity;
        entities->last = entities->last->next;
    }
}

void BH_DeinitEntities(struct bh_entity_ll* entities) {
    if (entities->next != NULL) {
        BH_DeinitEntities(entities->next);
    }
    if (entities->entity.state) {
        free(entities->entity.state);
    }
    free(entities);
}

static void update_entity_transform(struct bh_sprite_entity* entity) {
    m4 model_matrix;
    m4 translation;

    m4_scale(model_matrix, entity->scale.x, entity->scale.y, 1.0f);
    m4_translation(translation, entity->position.x, entity->position.y, 0.0f);
    m4_multiply(model_matrix, translation);

    memcpy(entity->sprite.transform, model_matrix, sizeof(m4));
}

#ifdef RENDER_DEBUG_INFO
static void render_bounding_box(
    struct bh_renderer* renderer, struct bh_bounding_box bb, struct vec2 offset, GLuint64 texture
) {
    struct bh_sprite hitbox_sprite = {
        .texture_handle = texture,
    };

    struct bh_bounding_box globalised_bb = BH_BoxToWorld(offset, bb);
    struct vec2 hitbox_centre = BH_BoxCentre(globalised_bb);
    struct vec2 hitbox_dimensions = BH_BoxDimensions(globalised_bb);

    m4_scale(hitbox_sprite.transform, hitbox_dimensions.x / 2.0f, hitbox_dimensions.y / 2.0f, 1.0f);

    m4 translation;
    m4_translation(translation, hitbox_centre.x, hitbox_centre.y, 0.0f);
    m4_multiply(hitbox_sprite.transform, translation);

    BH_RenderBatch(renderer, hitbox_sprite);
}
#endif

#ifdef RENDER_DEBUG_INFO
static void render_qtree(struct bh_renderer* renderer, struct bh_qtree* qtree, GLuint64 texture) {
    if (qtree == NULL) {
        return;
    }

    if (BH_IsQTreeLeaf(qtree)) {
        render_bounding_box(renderer, qtree->bb, (struct vec2){ 0.0f, 0.0f }, texture);
    } else {
        render_qtree(renderer, qtree->top_left, texture);
        render_qtree(renderer, qtree->top_right, texture);
        render_qtree(renderer, qtree->bottom_left, texture);
        render_qtree(renderer, qtree->bottom_right, texture);
    }
}
#endif

static void render_text(struct bh_renderer* renderer, const char* text) {
    float x0 = 100.0f;
    float y0 = 100.0f;
    float scale = 1.0f;

    for (size_t i = 0; text[i] != '\0'; i++) {
        unsigned char ch = text[i];
        struct bh_glyph glyph = renderer->font.glyphs[ch];

        float x = x0 + glyph.bearing_x * scale;
        float y = y0 - (glyph.bearing_y - 0.5f * glyph.height) * scale;

        float w = glyph.width * scale;
        float h = glyph.height * scale;

        if (glyph.texture != 0) {
            struct bh_sprite sprite;
            sprite.texture_handle = glyph.texture;
            sprite.flags = BH_SPRITE_TEXT;

            m4 translation;
            m4_translation(translation, x, y, 0.0f);
            m4_scale(sprite.transform, w / 2.0, h / 2.0, 1.0f);
            m4_multiply(sprite.transform, translation);

            BH_RenderBatch(renderer, sprite);
        }

        x0 += (glyph.advance >> 6) * scale;
    }
}

static void tick_all_entities(
    struct bh_ctx* state, struct bh_entity_ll* entities, struct bh_qtree* qtree,
    struct bh_renderer* renderer
) {
    struct bh_qtree next_qtree = { .bb = qtree->bb };

    struct bh_entity_ll* node = entities;

    while (node != NULL) {
        struct bh_sprite_entity* entity = &node->entity;

        BH_InsertQTree(
            &next_qtree, (struct bh_qtree_entity){ .entity = entity, .point = entity->position }
        );

        entity->callback(state, entity);
        update_entity_transform(entity);
        BH_RenderBatch(renderer, entity->sprite);

#ifdef RENDER_DEBUG_INFO
        render_bounding_box(renderer, entity->bb, entity->position, state->debug_texture);
#endif

        node = node->next;
    }

#ifdef RENDER_DEBUG_INFO
    render_qtree(renderer, qtree, state->green_debug_texture);
#endif

    render_text(renderer, "The quick brown fox jumps over the lazy dog.");

    BH_FinishBatch(renderer);

    /* Update qtree */
    BH_DeinitQTree(qtree);
    *qtree = next_qtree;
}

bool BH_DoEntitiesCollide(struct bh_sprite_entity* entity, struct bh_sprite_entity* other) {
    return BH_DoBoxesIntersect(
        BH_BoxToWorld(entity->position, entity->bb), BH_BoxToWorld(other->position, other->bb)
    );
}

static bool GLOBAL_KEYS_HELD[GLFW_KEY_LAST + 1];

static void glfw_key_cb(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)window;
    (void)scancode;
    (void)mods;
    if (action == GLFW_PRESS) {
        GLOBAL_KEYS_HELD[key] = true;
    } else if (action == GLFW_RELEASE) {
        GLOBAL_KEYS_HELD[key] = false;
    }
}

bool BH_GetKey(int glfw_key) {
    if (glfw_key < 0 || glfw_key > GLFW_KEY_LAST) {
        error("Invalid key: %d", glfw_key);
        return false;
    }
    return GLOBAL_KEYS_HELD[glfw_key];
}

bool BH_InitContext(struct bh_ctx* ctx, void* user_state, user_cb user_init) {
    if (!BH_InitRenderer(&ctx->renderer))
        return false;
    glfwSetKeyCallback(ctx->renderer.window, glfw_key_cb);

#ifdef RENDER_DEBUG_INFO
    ctx->debug_texture =
        BH_LoadTexture(&ctx->renderer.textures, (void*)ASSET_debug, sizeof(ASSET_debug) - 1);
    if (!ctx->debug_texture)
        return false;

    ctx->green_debug_texture =
        BH_LoadTexture(&ctx->renderer.textures, (void*)ASSET_green_debug, sizeof(ASSET_debug) - 1);
    if (!ctx->green_debug_texture)
        return false;
#endif

    ctx->entity_qtree.bb = (struct bh_bounding_box){
        .top_left = {   0.0f,   0.0f },
        .bottom_right = { 640.0f, 480.0f },
    };

    ctx->user_state = user_state;
    if (!user_init(ctx, ctx->user_state))
        return false;

    return true;
}

static void begin_frame(struct bh_ctx* ctx) {
    glfwSetTime(0.0);
    glfwPollEvents();
    BH_RendererBeginFrame(&ctx->renderer);
}

static void end_frame(struct bh_ctx* ctx) {
    BH_RendererEndFrame(&ctx->renderer);
    ctx->dt = (float)glfwGetTime();
}

static void delete_ctx(struct bh_ctx* ctx) {
    BH_DeinitQTree(&ctx->entity_qtree);
    BH_DeinitEntities(ctx->entities.entities);
    BH_DeinitRenderer(&ctx->renderer);
}

void BH_RunContext(struct bh_ctx* ctx) {
    while (!glfwWindowShouldClose(ctx->renderer.window)) {
        begin_frame(ctx);
        tick_all_entities(ctx, ctx->entities.entities, &ctx->entity_qtree, &ctx->renderer);
        end_frame(ctx);
    }
    delete_ctx(ctx);
}
