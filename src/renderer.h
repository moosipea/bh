#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "entitydef.h"
#include "matrix.h"

#define BH_MAX_TEXTURES 256
#define BH_BATCH_SIZE 1024

GLuint BH_InitProgram(const GLchar* vertex_src, const GLchar* fragment_src);
void BH_DeinitProgram(GLuint program);

struct bh_mesh_handle {
    GLuint vao_handle;
    GLuint vbo_handle;
};

struct bh_mesh_handle BH_UploadMesh(const GLfloat* vertices, size_t count);

struct bh_textures {
    GLuint texture_ids[BH_MAX_TEXTURES];
    GLuint64 texture_handles[BH_MAX_TEXTURES];
    size_t count;
};

GLuint64 BH_LoadTexture(struct bh_textures* textures, void* png_data, size_t size);
void BH_DeinitTextures(struct bh_textures textures);

enum bh_sprite_flag { BH_SPRITE_TEXT = 1 << 0 };

struct bh_instance_data {
    m4 transform;
    GLuint flags;
    uint32_t padding[3];
};

struct bh_sprite_batch {
    struct bh_mesh_handle mesh;
    struct bh_instance_data instance_data[BH_BATCH_SIZE];
    GLuint64 instance_textures[BH_BATCH_SIZE];
    size_t count;
    GLuint instances_ssbo;
    GLuint textures_ssbo;
};

#define MAX_CHARACTER 128

struct bh_glyph {
    GLuint64 texture;
    int width, height;
    int bearing_x, bearing_y;
    int advance;
};

struct bh_font {
    struct bh_glyph glyphs[MAX_CHARACTER];
    struct bh_textures textures;
};

struct bh_renderer {
    GLFWwindow* window;
    int width, height;
    GLuint program;
    m4 projection_matrix;

    struct bh_textures textures;
    struct bh_sprite_batch batch;

    FT_Library ft;
    struct bh_font font;
};

struct bh_sprite_batch BH_InitBatch(void);
void BH_RenderBatch(struct bh_renderer* batch, struct bh_sprite sprite);
void BH_FinishBatch(struct bh_renderer* batch);
void BH_DeinitBatch(struct bh_sprite_batch batch);

bool BH_InitRenderer(struct bh_renderer* renderer);
void BH_RendererBeginFrame(struct bh_renderer* renderer);
void BH_RendererEndFrame(struct bh_renderer* renderer);
void BH_DeinitRenderer(struct bh_renderer* renderer);
