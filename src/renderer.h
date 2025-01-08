#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "entitydef.h"
#include "matrix.h"

#define BH_MAX_TEXTURES 512
#define BH_BATCH_SIZE 1024

GLuint BH_InitProgram(const GLchar* vertex_src, const GLchar* fragment_src);
void BH_DeinitProgram(GLuint program);

struct BH_MeshHandle {
    GLuint vao_handle;
    GLuint vbo_handle;
};

struct BH_MeshHandle BH_UploadMesh(const GLfloat* vertices, size_t count);

struct BH_Textures {
    GLuint texture_ids[BH_MAX_TEXTURES];
    GLuint64 texture_handles[BH_MAX_TEXTURES];
    size_t count;
};

GLuint64 BH_LoadTexture(struct BH_Textures* textures, void* png_data, size_t size);
void BH_DeinitTextures(struct BH_Textures textures);

enum BH_SpriteFlag { BH_SPRITE_TEXT = 1 << 0, BH_SPRITE_HAS_COLOUR = 1 << 1 };

/* Must be aligned to 16 bytes! */
struct BH_InstanceData {
    m4 transform;            /* 64 bytes */
    struct BH_Colour colour; /* 16 bytes */
    GLuint flags;            /* 4 byte */
    uint32_t padding[3];     /* 12 bytes */
};

struct BH_SpriteBatch {
    struct BH_MeshHandle mesh;
    struct BH_InstanceData instance_data[BH_BATCH_SIZE];
    GLuint64 instance_textures[BH_BATCH_SIZE];
    size_t count;
    GLuint instances_ssbo;
    GLuint textures_ssbo;
};

#define MAX_CHARACTER 128

struct BH_Glyph {
    GLuint64 texture;
    int width, height;
    int bearing_x, bearing_y;
    int advance;
};

struct BH_Font {
    struct BH_Glyph glyphs[MAX_CHARACTER];
    struct BH_Textures textures;
};

struct BH_Renderer {
    GLFWwindow* window;
    int width, height;

    GLuint main_program;
    m4 projection_matrix;

    struct {
        GLuint fbo;
        GLuint color_buffer;
        GLuint rbo;
        GLuint post_program;
    } framebuffer;

    struct BH_Textures textures;
    struct BH_SpriteBatch batch;

    FT_Library ft;
    struct BH_Font font;
};

struct BH_SpriteBatch BH_InitBatch(void);
void BH_RenderBatch(struct BH_Renderer* batch, struct BH_Sprite sprite);
void BH_FinishBatch(struct BH_Renderer* batch);
void BH_DeinitBatch(struct BH_SpriteBatch batch);

void BH_RenderText(
    struct BH_Renderer* renderer, float x0, float y0, float scale, struct BH_Colour colour,
    const char* text
);

bool BH_InitRenderer(struct BH_Renderer* renderer);
void BH_RendererBeginFrame(struct BH_Renderer* renderer);
void BH_RendererEndFrame(struct BH_Renderer* renderer);
void BH_DeinitRenderer(struct BH_Renderer* renderer);
