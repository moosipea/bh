#define GLFW_INCLUDE_NONE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "error_macro.h"
#include "engine.h"

#define BATCH_SIZE 256

struct bh_context {
    GLFWwindow *window;
    bh_program program;
    struct bh_mesh_handle square_mesh;
    struct {
        m4 transforms[BATCH_SIZE]; 
        size_t count;
    } batch;
};

static char *read_file(const char *path) {
    FILE *file = fopen(path, "r");
    
    if (file == NULL) {
        error("Couldn't open file `%s`", path);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char *buffer = malloc(size + 1);
    fread(buffer, 1, size, file);
    buffer[size] = '\0';

    fclose(file);

    return buffer;
}

static inline struct bh_mesh_handle upload_square_mesh(void) {
    /* For use with GL_TRIANGLE_FAN */
    const GLfloat vertices[] = {
        -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f
    };
    return upload_mesh(vertices, sizeof(vertices) / sizeof(vertices[0]));
}

static inline bool init_gl(struct bh_context *context) {
    if (!glfwInit()) {
        error("GLFW initialization failed");
        return false;
    }

    context->window = glfwCreateWindow(640, 480, "Hello, world!", NULL, NULL);
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

    return true;
}

static inline bool init_shaders(struct bh_context *context) {
    bool success = true;
    
    char *vertex_src = NULL;
    char *fragment_src = NULL;

    if ((vertex_src = read_file("vertex.glsl")) == NULL) {
        success = false;
        goto end;
    }

    if ((fragment_src = read_file("fragment.glsl")) == NULL) {
        success = false;
        goto end;
    }

    if ((context->program = create_program(vertex_src, fragment_src))) {
        glUseProgram(context->program);
    } else {
        success = false;
        goto end;
    }

end:
    free(vertex_src);
    free(fragment_src);
    return success;
}

static inline float uniform_rand(void) {
    return 2.0f * ((float)rand() / (float)RAND_MAX) - 1.0f;
}

void generate_random_batch(struct bh_context *context) {
    const size_t n = BATCH_SIZE;
    for (size_t i = 0; i < BATCH_SIZE && i < n; i++) {
        m4_scale(context->batch.transforms[i], 0.025f, 0.025f, 0.025f);

        m4 translation;
        m4_translation(translation, uniform_rand(), uniform_rand(), uniform_rand());
        m4_multiply(context->batch.transforms[i], translation);

        context->batch.count++;
    }
    
    GLuint uniform = glGetUniformLocation(context->program, "transforms");
    glUniformMatrix4fv(uniform, context->batch.count, GL_FALSE, (const GLfloat*)context->batch.transforms);
}

static inline bool init_context(struct bh_context *context) {
    if (!init_gl(context)) {
        return false;
    }

    if (!init_shaders(context)) {
        return false;
    }

    context->square_mesh = upload_square_mesh();
    generate_random_batch(context);

    return true;
}

static inline void pre_frame() {
    glfwPollEvents();
    glClearColor(0.1, 0.2, 0.3, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
}

static inline void post_frame(struct bh_context *context) {
    glfwSwapBuffers(context->window);
}


static inline void main_loop(struct bh_context *context) {
    while (!glfwWindowShouldClose(context->window)) {
        pre_frame();

        glBindVertexArray(context->square_mesh.vao_handle);
        glEnableVertexAttribArray(0);
        glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, context->batch.count);

        post_frame(context);
    }
}

static inline void delete_context(struct bh_context context) {
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
