#define GLFW_INCLUDE_NONE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "error_macro.h"
#include "engine.h"
#include "res/built_assets.h"

#define TEST_SPRITES 8192

struct bh_context {
    int width;
    int height;
    GLFWwindow *window;
    bh_program program;
    m4 projection_matrix;

    struct bh_sprite_batch batch;
    struct bh_textures textures;
    struct bh_sprite_ll *entities;

    GLuint64 bulb_texture;
};

static inline bool init_gl(struct bh_context *context) {
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

static void test_entity_system(struct bh_sprite_entity *entity) {
    return;
}

static inline void spawn_test_entities(struct bh_context *context) {
    for (size_t i = 0; i < TEST_SPRITES; i++) {

        struct bh_sprite sprite = {0};
        sprite.texture_handle = context->bulb_texture;
        m4_scale(sprite.transform, 0.03f, 0.03f, 0.03f);

        struct bh_sprite_entity entity = {
            .sprite = sprite,
            .x = uniform_rand(),
            .y = uniform_rand(),
            .z = uniform_rand(),
            .callback = test_entity_system,
        };

        spawn_entity(&context->entities, entity);
    }
}

static void update_projection_matrix(struct bh_context *context) {
    m4_ortho(context->projection_matrix, 1.0f, context->width, 1.0f, context->height, 0.001f, 1000.0f);
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

    context->bulb_texture = textures_load(&context->textures, (void*)ASSET_bulb, sizeof(ASSET_bulb) - 1);
    if (!context->bulb_texture) {
        return false;
    }

    spawn_test_entities(context);

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
        printf("Resize: %dx%d\n", width, height);
    }

    glClearColor(0.1, 0.2, 0.3, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
}

static inline void post_frame(struct bh_context *context) {
    glfwSwapBuffers(context->window);
}

static inline void main_loop(struct bh_context *context) {
    while (!glfwWindowShouldClose(context->window)) {
        pre_frame(context);

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
