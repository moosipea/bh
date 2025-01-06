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

void BH_SpawnEntity(struct BH_DELL* entities, struct BH_SpriteEntity entity) {
    if (entities->entities == NULL) {
        entities->entities = calloc(1, sizeof(struct BH_EntityLL));
    }

    if (entities->last == NULL) {
        entities->entities->entity = entity;
        entities->last = entities->entities;
    } else {
        entities->last->next = calloc(1, sizeof(struct BH_EntityLL));
        entities->last->next->entity = entity;
        entities->last = entities->last->next;
    }
}

void BH_DeinitEntities(struct BH_EntityLL* entities) {
    if (entities->next != NULL) {
        BH_DeinitEntities(entities->next);
    }
    if (entities->entity.state) {
        free(entities->entity.state);
    }
    free(entities);
}

static void UpdateEntityTransform(struct BH_SpriteEntity* entity) {
    m4 model_matrix;
    m4 translation;

    m4_scale(model_matrix, entity->scale.x, entity->scale.y, 1.0f);
    m4_translation(translation, entity->position.x, entity->position.y, 0.0f);
    m4_multiply(model_matrix, translation);

    memcpy(entity->sprite.transform, model_matrix, sizeof(m4));
}

#ifdef RENDER_DEBUG_INFO
static void
RenderBB(struct BH_Renderer* renderer, struct BH_BB bb, struct vec2 offset, GLuint64 texture) {
    struct BH_Sprite hitbox_sprite = {
        .texture_handle = texture,
    };

    struct BH_BB globalised_bb = BH_BoxToWorld(offset, bb);
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
static void RenderQTree(struct BH_Renderer* renderer, struct BH_QTree* qtree, GLuint64 texture) {
    if (qtree == NULL) {
        return;
    }

    if (BH_IsQTreeLeaf(qtree)) {
        RenderBB(renderer, qtree->bb, (struct vec2){ 0.0f, 0.0f }, texture);
    } else {
        RenderQTree(renderer, qtree->top_left, texture);
        RenderQTree(renderer, qtree->top_right, texture);
        RenderQTree(renderer, qtree->bottom_left, texture);
        RenderQTree(renderer, qtree->bottom_right, texture);
    }
}
#endif

static void TickEntities(
    struct BH_Context* state, struct BH_EntityLL* entities, struct BH_QTree* qtree,
    struct BH_Renderer* renderer
) {
    struct BH_QTree next_qtree = { .bb = qtree->bb };

    struct BH_EntityLL* node = entities;

    while (node != NULL) {
        struct BH_SpriteEntity* entity = &node->entity;

        BH_InsertQTree(
            &next_qtree, (struct BH_QTreeEntity){ .entity = entity, .point = entity->position }
        );

        entity->callback(state, entity);
        UpdateEntityTransform(entity);
        BH_RenderBatch(renderer, entity->sprite);

#ifdef RENDER_DEBUG_INFO
        RenderBB(renderer, entity->bb, entity->position, state->debug_texture);
#endif

        node = node->next;
    }

#ifdef RENDER_DEBUG_INFO
    RenderQTree(renderer, qtree, state->green_debug_texture);
#endif

    BH_RenderText(renderer, 32.0f, 32.0f, 1.0f, "The quick brown fox jumps over the lazy dog.");

    BH_FinishBatch(renderer);

    /* Update qtree */
    BH_DeinitQTree(qtree);
    *qtree = next_qtree;
}

bool BH_DoEntitiesCollide(struct BH_SpriteEntity* entity, struct BH_SpriteEntity* other) {
    return BH_DoBoxesIntersect(
        BH_BoxToWorld(entity->position, entity->bb), BH_BoxToWorld(other->position, other->bb)
    );
}

static bool GLOBAL_KEYS_HELD[GLFW_KEY_LAST + 1];

static void GLFWKeyCB(GLFWwindow* window, int key, int scancode, int action, int mods) {
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

bool BH_InitContext(struct BH_Context* ctx, void* user_state, BH_UserCB user_init) {
    if (!BH_InitRenderer(&ctx->renderer))
        return false;
    glfwSetKeyCallback(ctx->renderer.window, GLFWKeyCB);

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

    ctx->entity_qtree.bb = (struct BH_BB){
        .top_left = {   0.0f,   0.0f },
        .bottom_right = { 640.0f, 480.0f },
    };

    ctx->user_state = user_state;
    if (!user_init(ctx, ctx->user_state))
        return false;

    return true;
}

static void BeginFrame(struct BH_Context* ctx) {
    glfwSetTime(0.0);
    glfwPollEvents();
    BH_RendererBeginFrame(&ctx->renderer);
}

static void EndFrame(struct BH_Context* ctx) {
    BH_RendererEndFrame(&ctx->renderer);
    ctx->dt = (float)glfwGetTime();
}

static void DeinitContext(struct BH_Context* ctx) {
    BH_DeinitQTree(&ctx->entity_qtree);
    BH_DeinitEntities(ctx->entities.entities);
    BH_DeinitRenderer(&ctx->renderer);
}

void BH_RunContext(struct BH_Context* ctx) {
    while (!glfwWindowShouldClose(ctx->renderer.window)) {
        BeginFrame(ctx);
        TickEntities(ctx, ctx->entities.entities, &ctx->entity_qtree, &ctx->renderer);
        EndFrame(ctx);
    }
    DeinitContext(ctx);
}
