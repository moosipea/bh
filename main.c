#define GLFW_INCLUDE_NONE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "error_macro.h"
#include "engine.h"
#include "res/built_assets.h"

#define TEST_SPRITES 64

struct bh_context {
    int width;
    int height;
    GLFWwindow *window;
    bh_program program;
    m4 projection_matrix;

    struct bh_sprite_batch batch;
    struct bh_textures textures;
    struct bh_sprite_ll *entities;

    struct bh_sprite_ll *player_entity;

    GLuint64 bulb_texture;

    int keys_held[GLFW_KEY_LAST];
} g_context;

static void glfw_key_cb(GLFWwindow* window, int key, int scancode, int action, int mods) {
    
}

static inline bool init_glfw(struct bh_context *context) {
    if (!glfwInit()) {
        error("GLFW initialization failed");
        return false;
    }

    context->window = glfwCreateWindow(context->width, context->height, "Hello, world!", NULL, NULL);
    if (!context->window) {
        error("Window creation failed");
        return false;
    }



    glfwMakeContextCurrent(context->window);
    glfwSwapInterval(1);
       
    return true;
}

static inline bool init_gl(struct bh_context *context) {
    if (!init_glfw(context)) {
        return false;
    }

    int glad_version = gladLoadGL(glfwGetProcAddress);
    if (!glad_version) {
        error("Failed to initialize OpenGL context");
        return false;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return true;
}

static inline bool init_shaders(struct bh_context *context) {
    if ((context->program = create_program((const GLchar*)ASSET_vertex, (const GLchar*)ASSET_fragment))) {
        glUseProgram(context->program);
        return true;
    }
    return false;
}

static inline float uniform_rand(void) {
    return 2.0f * ((float)rand() / (float)RAND_MAX) - 1.0f;
}

static void test_entity_system(void *context, struct bh_sprite_entity *entity) {
    (void) context;
    entity->y -= 1.0f / 60.0f;
    if (entity->y <= -1.0f) {
        entity->y = 1.0f;
    }
}

static inline void spawn_test_entities(struct bh_context *context) {
    for (size_t i = 0; i < TEST_SPRITES; i++) {

        struct bh_sprite sprite = {0};
        sprite.texture_handle = context->bulb_texture;
        m4_identity(sprite.transform);

        struct bh_sprite_entity entity = {
            .sprite = sprite,
            .x = uniform_rand(),
            .y = uniform_rand(),
            .z = uniform_rand(),
            .scale_x = 0.05f,
            .scale_y = 0.05f,
            .scale_z = 0.05f,
            .callback = test_entity_system,
        };

        spawn_entity(&context->entities, entity);
    }
}

static void update_player_system(void *context, struct bh_sprite_entity *entity) {
    (void) context;
    (void) entity;
}

static inline void spawn_player_entity(struct bh_context *context) {
    struct bh_sprite sprite = {0};
    sprite.texture_handle = textures_load(&context->textures, (void*)ASSET_player, sizeof(ASSET_player) - 1);

    struct bh_sprite_entity entity = {
        .sprite = sprite,
        .x = 0.0f,
        .y = 0.0f,
        .z = 0.0f,
        .scale_x = 0.15f,
        .scale_y = 0.15f,
        .scale_z = 0.15f,
        .callback = update_player_system,
    };

    spawn_entity(&context->entities, entity);
}

static void update_projection_matrix(struct bh_context *context) {
    m4_scale(context->projection_matrix, 1.0f, 1.0f, 1.0f);

    m4 projection;
    m4_ortho(projection, 1.0f, context->width, 1.0f, context->height, 0.001f, 1000.0f);
    m4_multiply(context->projection_matrix, projection);

    GLint projection_matrix_uniform = glGetUniformLocation(context->program, "projection_matrix");
    glUniformMatrix4fv(projection_matrix_uniform, 1, GL_FALSE, (const GLfloat*) context->projection_matrix);
}

static inline bool init_context(struct bh_context *context) {
    context->width = 640;
    context->height = 480;

    if (!init_gl(context)) {
        return false;
    }

    if (!init_shaders(context)) {
        return false;
    }

    context->batch = batch_init();

    struct bh_textures textures = {0};
    context->textures = textures;

    context->bulb_texture = textures_load(&context->textures, (void*)ASSET_star, sizeof(ASSET_star) - 1);
    if (!context->bulb_texture) {
        return false;
    }

    spawn_test_entities(context);
    spawn_player_entity(context);

    update_projection_matrix(context);

    return true;
}

static inline void pre_frame(struct bh_context *context) {
    glfwPollEvents();
    
    int width, height;
    glfwGetFramebufferSize(context->window, &width, &height);

    if (width != context->width || height != context->height) {
        context->width = width;
        context->height = height;
        glViewport(0, 0, context->width, context->height);
        update_projection_matrix(context);
    }

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
}

static inline void post_frame(struct bh_context *context) {
    glfwSwapBuffers(context->window);
}

static inline void main_loop(struct bh_context *context) {
    while (!glfwWindowShouldClose(context->window)) {
        pre_frame(context);

        /* Tick */
        tick_all_entities(context, context->entities);

        /* Draw */
        render_all_entities(&context->batch, context->entities, context->program);

        post_frame(context);
    }
}

static inline void delete_context(struct bh_context context) {
    kill_all_entities(context.entities);
    textures_delete(context.textures);
    batch_delete(context.batch);
    delete_program(&context.program);
    glfwDestroyWindow(context.window);
    glfwTerminate();
}

int main(void) {
    struct bh_context context = {0};

    if (!init_context(&context)) {
        error("Context initialisation failed");
        exit(1);
    }

    main_loop(&context);
    delete_context(context);
    return 0;
}
