#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "engine.h"
#include "error_macro.h"
#include "res/built_assets.h"

#define TEST_SPRITES 64

static struct bh_ctx {
    int width, height;
    GLFWwindow* window;
    bh_program program;
    m4 projection_matrix;
    float dt;

    struct bh_sprite_batch batch;
    struct bh_textures textures;
    struct bh_sprite_ll* entities;

    struct bh_sprite_ll* player_entity;

    GLuint64 bulb_texture;

    bool keys_held[GLFW_KEY_LAST + 1];
} g_ctx = {0};

static void
glfw_key_cb(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)window;
    (void)scancode;
    (void)mods;
    if (action == GLFW_PRESS) {
        g_ctx.keys_held[key] = true;
    } else if (action == GLFW_RELEASE) {
        g_ctx.keys_held[key] = false;
    }
}

static inline bool get_key(struct bh_ctx* ctx, int glfw_key) {
    if (glfw_key < 0 || glfw_key > GLFW_KEY_LAST) {
        error("Invalid key");
        return false;
    }
    return ctx->keys_held[glfw_key];
}

static inline bool init_glfw(struct bh_ctx* ctx) {
    if (!glfwInit()) {
        error("GLFW initialization failed");
        return false;
    }

    ctx->window =
        glfwCreateWindow(ctx->width, ctx->height, "Hello, world!", NULL, NULL);
    if (!ctx->window) {
        error("Window creation failed");
        return false;
    }

    glfwSetKeyCallback(ctx->window, glfw_key_cb);

    glfwMakeContextCurrent(ctx->window);
    glfwSwapInterval(1);

    return true;
}

static void GLAPIENTRY gl_error_cb(
    GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
    const GLchar* message, const void* user_param
) {
    (void)source;
    (void)id;
    (void)length;
    (void)user_param;
    error("GL ERROR (type=0x%x, severity=0x%x): %s", type, severity, message);
}

static inline bool init_gl(struct bh_ctx* ctx) {
    if (!init_glfw(ctx)) {
        return false;
    }

    int glad_version = gladLoadGL(glfwGetProcAddress);
    if (!glad_version) {
        error("Failed to initialize OpenGL ctx");
        return false;
    }

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(gl_error_cb, NULL);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return true;
}

static inline bool init_shaders(struct bh_ctx* ctx) {
    if ((ctx->program = create_program(
             (const GLchar*)ASSET_vertex, (const GLchar*)ASSET_fragment
         ))) {
        glUseProgram(ctx->program);
        return true;
    }
    return false;
}

static inline float uniform_rand(void) {
    return 2.0f * ((float)rand() / (float)RAND_MAX) - 1.0f;
}

static void test_entity_system(void* ctx, struct bh_sprite_entity* entity) {
    (void)ctx;
    entity->y -= 1.0f / 60.0f;
    if (entity->y <= -1.0f) {
        entity->y = 1.0f;
    }
}

static inline void spawn_test_entities(struct bh_ctx* ctx) {
    for (size_t i = 0; i < TEST_SPRITES; i++) {

        struct bh_sprite sprite = {0};
        sprite.texture_handle   = ctx->bulb_texture;
        m4_identity(sprite.transform);

        struct bh_sprite_entity entity = {
            .sprite   = sprite,
            .x        = uniform_rand(),
            .y        = uniform_rand(),
            .z        = uniform_rand(),
            .scale_x  = 0.05f,
            .scale_y  = 0.05f,
            .scale_z  = 0.05f,
            .callback = test_entity_system,
        };

        spawn_entity(&ctx->entities, entity);
    }
}

static void update_player_system(void* context, struct bh_sprite_entity* entity) {
    struct bh_ctx *ctx = context;

    if (get_key(ctx, GLFW_KEY_A)) {
        entity->x -= 1.0f * ctx->dt;
    }
    if (get_key(ctx, GLFW_KEY_D)) {
        entity->x += 1.0f * ctx->dt;
    }
}

static inline void spawn_player_entity(struct bh_ctx* ctx) {
    struct bh_sprite sprite = {0};
    sprite.texture_handle   = textures_load(
        &ctx->textures, (void*)ASSET_player, sizeof(ASSET_player) - 1
    );

    struct bh_sprite_entity entity = {
        .sprite   = sprite,
        .x        = 0.0f,
        .y        = 0.0f,
        .z        = 0.0f,
        .scale_x  = 0.15f,
        .scale_y  = 0.15f,
        .scale_z  = 0.15f,
        .callback = update_player_system,
    };

    spawn_entity(&ctx->entities, entity);
}

static void update_projection_matrix(struct bh_ctx* ctx) {
    m4_scale(ctx->projection_matrix, 1.0f, 1.0f, 1.0f);

    m4 projection;
    m4_ortho(projection, 1.0f, ctx->width, 1.0f, ctx->height, 0.001f, 1000.0f);
    m4_multiply(ctx->projection_matrix, projection);

    GLint projection_matrix_uniform =
        glGetUniformLocation(ctx->program, "projection_matrix");
    glUniformMatrix4fv(
        projection_matrix_uniform, 1, GL_FALSE,
        (const GLfloat*)ctx->projection_matrix
    );
}

static inline bool init_ctx(struct bh_ctx* ctx) {
    ctx->width  = 640;
    ctx->height = 480;

    if (!init_gl(ctx)) {
        return false;
    }

    if (!init_shaders(ctx)) {
        return false;
    }

    ctx->batch = batch_init();

    struct bh_textures textures = {0};
    ctx->textures               = textures;

    ctx->bulb_texture = textures_load(
        &ctx->textures, (void*)ASSET_star, sizeof(ASSET_star) - 1
    );
    if (!ctx->bulb_texture) {
        return false;
    }

    spawn_test_entities(ctx);
    spawn_player_entity(ctx);

    update_projection_matrix(ctx);

    return true;
}

static inline void pre_frame(struct bh_ctx* ctx) {
    glfwSetTime(0.0);
    glfwPollEvents();

    int width, height;
    glfwGetFramebufferSize(ctx->window, &width, &height);

    if (width != ctx->width || height != ctx->height) {
        ctx->width  = width;
        ctx->height = height;
        glViewport(0, 0, ctx->width, ctx->height);
        update_projection_matrix(ctx);
    }

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
}

static inline void post_frame(struct bh_ctx* ctx) {
    glfwSwapBuffers(ctx->window);
    ctx->dt = (float)glfwGetTime();
}

static inline void main_loop(struct bh_ctx* ctx) {
    while (!glfwWindowShouldClose(ctx->window)) {
        pre_frame(ctx);

        /* Tick */
        tick_all_entities(ctx, ctx->entities);

        /* Draw */
        render_all_entities(&ctx->batch, ctx->entities, ctx->program);

        post_frame(ctx);
    }
}

static inline void delete_ctx(struct bh_ctx ctx) {
    kill_all_entities(ctx.entities);
    textures_delete(ctx.textures);
    batch_delete(ctx.batch);
    delete_program(&ctx.program);
    glfwDestroyWindow(ctx.window);
    glfwTerminate();
}

int main(void) {
    if (!init_ctx(&g_ctx)) {
        error("Context initialisation failed");
        exit(1);
    }

    main_loop(&g_ctx);
    delete_ctx(g_ctx);
    return 0;
}
