#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>

#include "entitydef.h"
#include "matrix.h"

#define BH_MAX_TEXTURES 256
#define BH_BATCH_SIZE 1024

GLuint create_program(const GLchar* vertex_src, const GLchar* fragment_src);
void delete_program(GLuint program);

struct bh_mesh_handle {
    GLuint vao_handle;
    GLuint vbo_handle;
};

struct bh_mesh_handle upload_mesh(const GLfloat* vertices, size_t count);

struct bh_textures {
    GLuint texture_ids[BH_MAX_TEXTURES];
    GLuint64 texture_handles[BH_MAX_TEXTURES];
    size_t count;
};

GLuint64 textures_load(struct bh_textures* textures, void* png_data, size_t size);
void textures_delete(struct bh_textures textures);

struct bh_sprite_batch {
    struct bh_mesh_handle mesh;
    m4 instance_transforms[BH_BATCH_SIZE];
    GLuint64 instance_textures[BH_BATCH_SIZE];
    size_t count;
    GLuint instances_ssbo;
    GLuint textures_ssbo;
};

struct bh_renderer {
    GLFWwindow* window;
    int width, height;
    GLuint program;
    m4 projection_matrix;

    struct bh_textures textures;
    struct bh_sprite_batch batch;
};

struct bh_sprite_batch batch_init(void);
void batch_render(struct bh_renderer* batch, struct bh_sprite sprite);
void batch_finish(struct bh_renderer* batch);
void batch_delete(struct bh_sprite_batch batch);

bool renderer_init(struct bh_renderer* renderer);
void renderer_begin_frame(struct bh_renderer* renderer);
void renderer_end_frame(struct bh_renderer* renderer);
void delete_renderer(struct bh_renderer* renderer);
