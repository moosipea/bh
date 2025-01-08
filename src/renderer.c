#include "renderer.h"
#include "matrix.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define SPNG_STATIC
#include <spng.h>

#include "../res/built_assets.h"
#include "error_macro.h"

static bool CompileShader(GLuint shader, const GLchar* src) {
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    GLint status, log_length;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    if (status != GL_TRUE) {
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
        if (log_length > 0) {
            GLchar* log_buffer = calloc(log_length, sizeof(GLchar));
            glGetShaderInfoLog(shader, log_length, NULL, log_buffer);
            error("Shader compilation error: %s\n", log_buffer);
            free(log_buffer);
            error("Dumping shader source:\n%s\n", src);
        }
        return false;
    }

    return true;
}

static bool LinkProgram(GLuint program) {
    glLinkProgram(program);

    GLint status, log_length;
    glGetProgramiv(program, GL_LINK_STATUS, &status);

    if (status != GL_TRUE) {
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
        if (log_length > 0) {
            GLchar* log_buffer = calloc(log_length, sizeof(GLchar));
            glGetProgramInfoLog(program, log_length, NULL, log_buffer);
            error("Shader linking error: %s\n", log_buffer);
            free(log_buffer);
        }
        return false;
    }
    return true;
}

GLuint BH_InitProgram(const GLchar* vertex_src, const GLchar* fragment_src) {
    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    GLuint program = glCreateProgram();

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);

    if (!CompileShader(vertex_shader, vertex_src)) {
        error("Vertex shader compilation failed");
        return 0;
    }

    if (!CompileShader(fragment_shader, fragment_src)) {
        error("Fragment shader compilation failed");
        return 0;
    }

    if (!LinkProgram(program)) {
        error("Shader linking failed");
        return 0;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return program;
}

void BH_DeinitProgram(GLuint program) { glDeleteProgram(program); }

struct BH_MeshHandle BH_UploadMesh(const GLfloat* vertices, size_t count) {
    struct BH_MeshHandle res = { 0 };

    glGenVertexArrays(1, &res.vao_handle);
    glBindVertexArray(res.vao_handle);

    glGenBuffers(1, &res.vbo_handle);
    glBindBuffer(GL_ARRAY_BUFFER, res.vbo_handle);
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(GLfloat), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), 0);
    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat))
    );

    glBindVertexArray(0);

    return res;
}

static void* LoadPNG(void* png_data, size_t size, size_t* width, size_t* height) {
    spng_ctx* ctx = spng_ctx_new(0);
    if (ctx == NULL) {
        error("Failed to initialise spng context");
        return NULL;
    }

    int err;
    struct spng_ihdr ihdr;

    err = spng_set_png_buffer(ctx, png_data, size);
    if (err) {
        error("spng_set_png_buffer: %s", spng_strerror(err));
        return NULL;
    }

    err = spng_get_ihdr(ctx, &ihdr);
    if (err) {
        error("Failed to read PNG header: %s", spng_strerror(err));
        return NULL;
    }

    size_t decoded_size;
    err = spng_decoded_image_size(ctx, SPNG_FMT_RGBA8, &decoded_size);
    if (err) {
        error("Failed to read image size: %s", spng_strerror(err));
        return NULL;
    }

    void* decoded_image = malloc(decoded_size);
    if (decoded_image == NULL) {
        error("Failed to allocate memory");
        return NULL;
    }

    err = spng_decode_image(ctx, decoded_image, decoded_size, SPNG_FMT_RGBA8, 0);
    if (err) {
        error("Failed to decode image: %s", spng_strerror(err));
        return NULL;
    }

    *width = ihdr.width;
    *height = ihdr.height;

    spng_ctx_free(ctx);

    return decoded_image;
}

static GLuint UploadTexture(void* image, size_t width, size_t height) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);

    return texture;
}

static GLuint CreateTexture(void* png_data, size_t size) {
    void* image_data;
    size_t width, height;

    if (!(image_data = LoadPNG(png_data, size, &width, &height))) {
        error("Couldn't load image");
        return 0;
    }

    GLuint texture = UploadTexture(image_data, width, height);

    free(image_data);

    return texture;
}

static GLuint64 AppendTextureHandle(struct BH_Textures* textures, GLuint texture) {
    if (!texture) {
        error("texture == 0");
        return 0;
    }
    if (textures->count >= BH_MAX_TEXTURES) {
        error("Couldn't load texture, textures->count exceeds BH_MAX_TEXTURES");
        return 0;
    }

    GLuint64 texture_handle = glGetTextureHandleARB(texture);

    if (!texture_handle) {
        error("glGetTextureHandleARB returned NULL");
        return 0;
    }

    glMakeTextureHandleResidentARB(texture_handle);

    textures->texture_ids[textures->count] = texture;
    textures->texture_handles[textures->count] = texture_handle;
    textures->count++;

    return texture_handle;
}

GLuint64 BH_LoadTexture(struct BH_Textures* textures, void* png_data, size_t size) {
    GLuint texture = CreateTexture(png_data, size);
    if (!texture) {
        error("Couldn't create texture");
        return 0;
    }

    GLuint64 texture_handle = AppendTextureHandle(textures, texture);
    if (!texture_handle) {
        return 0;
    }

    return texture_handle;
}

void BH_DeinitTextures(struct BH_Textures textures) {
    for (size_t i = 0; i < textures.count; i++) {
        glMakeTextureHandleNonResidentARB(textures.texture_handles[i]);
    }
    glDeleteTextures(textures.count, (const GLuint*)&textures.texture_ids);
}

static GLuint CreateSSBO(const void* buffer, size_t size) {
    GLuint id;

    glCreateBuffers(1, &id);
    glNamedBufferStorage(id, size, buffer, GL_DYNAMIC_STORAGE_BIT);

    return id;
}

/* For use with GL_TRIANGLE_FAN */
/* Positions and UV coords */
// clang-format off
static const GLfloat QUAD_VERTICES[] = {
    -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
     1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
     1.0f,  1.0f, 0.0f, 1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f, 0.0f, 0.0f
};
// clang-format on

struct BH_SpriteBatch BH_InitBatch(void) {
    struct BH_SpriteBatch res = { 0 };

    res.mesh = BH_UploadMesh(QUAD_VERTICES, sizeof(QUAD_VERTICES) / sizeof(QUAD_VERTICES[0]));
    res.instances_ssbo = CreateSSBO(res.instance_data, sizeof(res.instance_data));
    res.textures_ssbo = CreateSSBO(res.instance_textures, sizeof(res.instance_textures));

    return res;
}

void BH_DeinitBatch(struct BH_SpriteBatch batch) {
    glDeleteBuffers(1, &batch.textures_ssbo);
    glDeleteBuffers(1, &batch.instances_ssbo);
}

void BH_RenderBatch(struct BH_Renderer* renderer, struct BH_Sprite sprite) {
    struct BH_SpriteBatch* batch = &renderer->batch;
    /* Insert sprite data into batch array */
    memcpy(batch->instance_data[batch->count].transform, sprite.transform, sizeof(m4));
    batch->instance_data[batch->count].flags = sprite.flags;
    batch->instance_data[batch->count].colour = sprite.colour;
    batch->instance_textures[batch->count] = sprite.texture_handle;
    batch->count++;

    /* Draw the batch when it is full */
    if (batch->count >= BH_BATCH_SIZE) {
        BH_FinishBatch(renderer);
    }
}

static void BatchDrawcall(struct BH_Renderer* renderer) {
    glUseProgram(renderer->main_program);
    glBindVertexArray(renderer->batch.mesh.vao_handle);

    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, renderer->batch.count);
}

void BH_FinishBatch(struct BH_Renderer* renderer) {
    struct BH_SpriteBatch* batch = &renderer->batch;
    /* Sync SSBO contents */
    glNamedBufferSubData(
        batch->instances_ssbo, 0, batch->count * sizeof(struct BH_InstanceData),
        batch->instance_data
    );
    glNamedBufferSubData(
        batch->textures_ssbo, 0, batch->count * sizeof(GLuint64), batch->instance_textures
    );

    /* Make sure SSBOs are bound */
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, batch->instances_ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, batch->textures_ssbo);

    /* Draw call */
    BatchDrawcall(renderer);

    /* Reset batch state */
    batch->count = 0;
}

static bool InitGLFW(struct BH_Renderer* renderer) {
    if (!glfwInit()) {
        error("GLFW initialization failed");
        return false;
    }

    renderer->window =
        glfwCreateWindow(renderer->width, renderer->height, "Hello, world!", NULL, NULL);
    if (!renderer->window) {
        error("Window creation failed");
        return false;
    }

    glfwMakeContextCurrent(renderer->window);
    glfwSwapInterval(1);

    return true;
}

#ifndef NDEBUG
static void GLAPIENTRY GLErrorCB(
    GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message,
    const void* user_param
) {
    (void)source;
    (void)length;
    (void)user_param;
    error("GL ERROR (type=0x%x, severity=0x%x, id=0x%x): %s", type, severity, id, message);
}
#endif

static bool InitGL(struct BH_Renderer* renderer) {
    if (!InitGLFW(renderer)) {
        return false;
    }

    int glad_version = gladLoadGL(glfwGetProcAddress);
    if (!glad_version) {
        error("Failed to initialize OpenGL ctx");
        return false;
    }

#ifndef NDEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(GLErrorCB, NULL);
#endif

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return true;
}

static bool InitShaders(struct BH_Renderer* renderer) {
    renderer->main_program =
        BH_InitProgram((const GLchar*)ASSET_vertex, (const GLchar*)ASSET_fragment);
    if (renderer->main_program) {
        glUseProgram(renderer->main_program);
        return true;
    }
    return false;
}

static GLuint UploadGlyphTexture(FT_Bitmap bitmap) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RED, bitmap.width, bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE,
        bitmap.buffer
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    return texture;
}

static void PreloadGlyphs(struct BH_Font* font, FT_Face face) {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (unsigned char ch = 0; ch < MAX_CHARACTER; ch++) {
        if (FT_Load_Char(face, ch, FT_LOAD_RENDER)) {
            continue;
        }

        GLuint64 handle = 0;

        if (face->glyph->bitmap.width != 0) {
            GLuint texture = UploadGlyphTexture(face->glyph->bitmap);
            handle = AppendTextureHandle(&font->textures, texture);
        }

        font->glyphs[ch] = (struct BH_Glyph){ .texture = handle,
                                              .width = face->glyph->bitmap.width,
                                              .height = face->glyph->bitmap.rows,
                                              .bearing_x = face->glyph->bitmap_left,
                                              .bearing_y = face->glyph->bitmap_top,
                                              .advance = face->glyph->advance.x };
    }
}

static bool
InitFont(FT_Library ft, struct BH_Font* font, size_t font_size, void* data, size_t size) {
    FT_Face face;

    if (FT_New_Memory_Face(ft, data, size, 0, &face)) {
        error("Couldn't load font");
        return false;
    }

    if (FT_Set_Pixel_Sizes(face, 0, font_size)) {
        error("Couldn't set font size");
        return false;
    }

    PreloadGlyphs(font, face);

    FT_Done_Face(face);

    return true;
}

static void DeinitFont(struct BH_Font font) { BH_DeinitTextures(font.textures); }

static bool InitFreeType(struct BH_Renderer* renderer) {
    if (FT_Init_FreeType(&renderer->ft)) {
        error("FreeType initialisation failed");
        return false;
    }
    return true;
}

static void DeinitFreeType(FT_Library ft) { FT_Done_FreeType(ft); }

static void UpdateProjectionMatrix(struct BH_Renderer* renderer) {
    printf("UpdateProjectionMatrix(%d, %d)\n", renderer->width, renderer->height);
    m4_ortho(
        renderer->projection_matrix, 1.0f, renderer->width, 1.0f, renderer->height, 0.001f, 1000.0f
    );

    glUniformMatrix4fv(
        glGetUniformLocation(renderer->main_program, "projection_matrix"), 1, GL_FALSE,
        (const GLfloat*)renderer->projection_matrix
    );
}

/* TODO: handle window resizing */
static bool InitFramebuffer(struct BH_Renderer* renderer) {
    glGenFramebuffers(1, &renderer->framebuffer.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->framebuffer.fbo);

    /* Color attachment */
    glGenTextures(1, &renderer->framebuffer.color_buffer);
    glBindTexture(GL_TEXTURE_2D, renderer->framebuffer.color_buffer);

    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGB, renderer->width, renderer->height, 0, GL_RGB, GL_UNSIGNED_BYTE,
        NULL
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderer->framebuffer.color_buffer, 0
    );

    /* RBO (depth and stencil attachment) */
    glGenRenderbuffers(1, &renderer->framebuffer.rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, renderer->framebuffer.rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, renderer->width, renderer->height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderer->framebuffer.rbo
    );

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        error("Framebuffer is not complete");
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    renderer->framebuffer.post_program =
        BH_InitProgram((const GLchar*)ASSET_vertex_post, (const GLchar*)ASSET_fragment_post);
    if (!renderer->framebuffer.post_program) {
        return false;
    }

    return true;
}

bool BH_InitRenderer(struct BH_Renderer* renderer) {
    assert(sizeof(struct BH_InstanceData) % 16 == 0);

    renderer->width = 640;
    renderer->height = 480;

    if (!InitGL(renderer))
        return false;
    if (!InitShaders(renderer))
        return false;
    if (!InitFramebuffer(renderer))
        return false;
    if (!InitFreeType(renderer))
        return false;
    if (!InitFont(renderer->ft, &renderer->font, 28, (void*)ASSET_font, sizeof(ASSET_font) - 1))
        return false;

    renderer->batch = BH_InitBatch();
    UpdateProjectionMatrix(renderer);

    return true;
}

void BH_RenderText(
    struct BH_Renderer* renderer, float x0, float y0, float scale, struct BH_Colour colour,
    const char* text
) {
    for (size_t i = 0; text[i] != '\0'; i++) {
        unsigned char ch = text[i];
        struct BH_Glyph glyph = renderer->font.glyphs[ch];

        float x = x0 + (glyph.bearing_x + 0.5f * glyph.width) * scale;

        /* I would like to have the origin in the top left corner
         * but I am too stupid. For now, the origin shall lie in the
         * bottom left corner instead! */
        float y = y0 + (glyph.height * 0.5f - glyph.bearing_y) * scale;

        float w = glyph.width * scale;
        float h = glyph.height * scale;

        if (glyph.texture != 0) {
            struct BH_Sprite sprite;
            sprite.texture_handle = glyph.texture;
            sprite.colour = colour;
            sprite.flags = BH_SPRITE_TEXT | BH_SPRITE_HAS_COLOUR;

            m4 translation;
            m4_translation(translation, x, y, 0.0f);
            m4_scale(sprite.transform, w / 2.0, h / 2.0, 1.0f);
            m4_multiply(sprite.transform, translation);

            BH_RenderBatch(renderer, sprite);
        }

        x0 += (glyph.advance >> 6) * scale;
    }
}

void BH_RendererBeginFrame(struct BH_Renderer* renderer) {
    int width, height;
    glfwGetFramebufferSize(renderer->window, &width, &height);

    if (width != renderer->width || height != renderer->height) {
        renderer->width = width;
        renderer->height = height;
        glViewport(0, 0, renderer->width, renderer->height);
        UpdateProjectionMatrix(renderer);
    }

    /* Setup for rendering to FBO */
    glBindFramebuffer(GL_FRAMEBUFFER, renderer->framebuffer.fbo);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
}

void BH_RendererEndFrame(struct BH_Renderer* renderer) {
    /* Now render to the screen */
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.3f, 0.2f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    /* Draw contents of framebuffer to the screen */
    glUseProgram(renderer->framebuffer.post_program);
    glBindVertexArray(renderer->batch.mesh.vao_handle
    ); /* Reuse mesh from the batch, as it is just a quad */
    glBindTexture(GL_TEXTURE_2D, renderer->framebuffer.color_buffer);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glfwSwapBuffers(renderer->window);
}

void BH_DeinitRenderer(struct BH_Renderer* renderer) {
    DeinitFont(renderer->font);
    DeinitFreeType(renderer->ft);

    glDeleteFramebuffers(1, &renderer->framebuffer.fbo);
    BH_DeinitTextures(renderer->textures);
    BH_DeinitBatch(renderer->batch);
    BH_DeinitProgram(renderer->main_program);

    glfwDestroyWindow(renderer->window);
    glfwTerminate();
}
