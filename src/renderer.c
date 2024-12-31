#include "renderer.h"
#include "freetype/freetype.h"
#include "freetype/ftimage.h"
#include "matrix.h"

#include <stdbool.h>
#include <string.h>

#define SPNG_STATIC
#include <spng.h>

#include "../res/built_assets.h"
#include "error_macro.h"

static bool compile_shader(GLuint shader, const GLchar* src) {
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

static bool link_program(GLuint program) {
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

    if (!compile_shader(vertex_shader, vertex_src)) {
        error("Vertex shader compilation failed");
        return 0;
    }

    if (!compile_shader(fragment_shader, fragment_src)) {
        error("Fragment shader compilation failed");
        return 0;
    }

    if (!link_program(program)) {
        error("Shader linking failed");
        return 0;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    return program;
}

void BH_DeinitProgram(GLuint program) { glDeleteProgram(program); }

struct bh_mesh_handle BH_UploadMesh(const GLfloat* vertices, size_t count) {
    struct bh_mesh_handle res = { 0 };

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

    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
    glBindVertexArray(0);

    return res;
}

static void* load_png(void* png_data, size_t size, size_t* width, size_t* height) {
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

static GLuint upload_texture(void* image, size_t width, size_t height) {
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

static GLuint create_texture(void* png_data, size_t size) {
    void* image_data;
    size_t width, height;

    if (!(image_data = load_png(png_data, size, &width, &height))) {
        error("Couldn't load image");
        return 0;
    }

    GLuint texture = upload_texture(image_data, width, height);

    free(image_data);

    return texture;
}

static GLuint64 append_new_texture_handle(struct bh_textures* textures, GLuint texture) {
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

GLuint64 BH_LoadTexture(struct bh_textures* textures, void* png_data, size_t size) {
    GLuint texture = create_texture(png_data, size);
    if (!texture) {
        error("Couldn't create texture");
        return 0;
    }

    GLuint64 texture_handle = append_new_texture_handle(textures, texture);
    if (!texture_handle) {
        return 0;
    }

    return texture_handle;
}

void BH_DeinitTextures(struct bh_textures textures) {
    for (size_t i = 0; i < textures.count; i++) {
        glMakeTextureHandleNonResidentARB(textures.texture_handles[i]);
    }
    glDeleteTextures(textures.count, (const GLuint*)&textures.texture_ids);
}

static GLuint create_ssbo(const void* buffer, size_t size) {
    GLuint id;

    glCreateBuffers(1, &id);
    glNamedBufferStorage(id, size, buffer, GL_DYNAMIC_STORAGE_BIT);

    return id;
}

struct bh_sprite_batch BH_InitBatch(void) {
    struct bh_sprite_batch res = { 0 };

    /* For use with GL_TRIANGLE_FAN */
    /* Positions and UV coords */
    const GLfloat vertices[] = { -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  -1.0f, 0.0f, 1.0f, 0.0f,
                                 1.0f,  1.0f,  0.0f, 1.0f, 1.0f, -1.0f, 1.0f,  0.0f, 0.0f, 1.0f };

    res.mesh = BH_UploadMesh(vertices, sizeof(vertices) / sizeof(vertices[0]));
    res.instances_ssbo = create_ssbo(res.instance_data, sizeof(res.instance_data));
    res.textures_ssbo = create_ssbo(res.instance_textures, sizeof(res.instance_textures));

    return res;
}

void BH_DeinitBatch(struct bh_sprite_batch batch) {
    glDeleteBuffers(1, &batch.textures_ssbo);
    glDeleteBuffers(1, &batch.instances_ssbo);
}

void BH_RenderBatch(struct bh_renderer* renderer, struct bh_sprite sprite) {
    struct bh_sprite_batch* batch = &renderer->batch;
    /* Insert sprite data into batch array */
    memcpy(batch->instance_data[batch->count].transform, sprite.transform, sizeof(m4));
    batch->instance_data[batch->count].flags = sprite.flags;
    batch->instance_textures[batch->count] = sprite.texture_handle;
    batch->count++;

    /* Draw the batch when it is full */
    if (batch->count >= BH_BATCH_SIZE) {
        BH_FinishBatch(renderer);
    }
}

static void batch_drawcall(struct bh_renderer* renderer) {
    glUseProgram(renderer->program);
    glBindVertexArray(renderer->batch.mesh.vao_handle);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, renderer->batch.count);

    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
    glUseProgram(0);
}

void BH_FinishBatch(struct bh_renderer* renderer) {
    struct bh_sprite_batch* batch = &renderer->batch;
    /* Sync SSBO contents */
    glNamedBufferSubData(
        batch->instances_ssbo, 0, batch->count * sizeof(struct bh_instance_data),
        batch->instance_data
    );
    glNamedBufferSubData(
        batch->textures_ssbo, 0, batch->count * sizeof(GLuint64), batch->instance_textures
    );

    /* Make sure SSBOs are bound */
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, batch->instances_ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, batch->textures_ssbo);

    /* Draw call */
    batch_drawcall(renderer);

    /* Reset batch state */
    batch->count = 0;
}

static bool init_glfw(struct bh_renderer* renderer) {
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
static void GLAPIENTRY gl_error_cb(
    GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message,
    const void* user_param
) {
    (void)source;
    (void)id;
    (void)length;
    (void)user_param;
    error("GL ERROR (type=0x%x, severity=0x%x): %s", type, severity, message);
}
#endif

static bool init_gl(struct bh_renderer* renderer) {
    if (!init_glfw(renderer)) {
        return false;
    }

    int glad_version = gladLoadGL(glfwGetProcAddress);
    if (!glad_version) {
        error("Failed to initialize OpenGL ctx");
        return false;
    }

#ifndef NDEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(gl_error_cb, NULL);
#endif

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return true;
}

static bool init_shaders(struct bh_renderer* renderer) {
    renderer->program = BH_InitProgram((const GLchar*)ASSET_vertex, (const GLchar*)ASSET_fragment);
    if (renderer->program) {
        glUseProgram(renderer->program);
        return true;
    }
    return false;
}

static GLuint upload_glyph_texture(FT_Bitmap bitmap) {
    GLint prior_alignment;
    glGetIntegerv(GL_PACK_ALIGNMENT, &prior_alignment);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

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

    glPixelStorei(GL_PACK_ALIGNMENT, prior_alignment);

    return texture;
}

static void preload_glyphs(struct bh_font* font, FT_Face face) {
    for (unsigned char ch = 0; ch < MAX_CHARACTER; ch++) {
        if (FT_Load_Char(face, ch, FT_LOAD_RENDER)) {
            continue;
        }

        GLuint texture = upload_glyph_texture(face->glyph->bitmap);
        GLuint64 handle = append_new_texture_handle(&font->textures, texture);

        font->glyphs[ch] = (struct bh_glyph){ .texture = handle,
                                              .width = face->glyph->bitmap.width,
                                              .height = face->glyph->bitmap.rows,
                                              .bearing_x = face->glyph->bitmap_left,
                                              .bearing_y = face->glyph->bitmap_top,
                                              .advance = face->glyph->advance.x };
    }
}

static bool
init_font(FT_Library ft, struct bh_font* font, size_t font_size, void* data, size_t size) {
    FT_Face face;

    if (FT_New_Memory_Face(ft, data, size, 0, &face)) {
        error("Couldn't load font");
        return false;
    }

    if (FT_Set_Pixel_Sizes(face, 0, font_size)) {
        error("Couldn't set font size");
        return false;
    }

    preload_glyphs(font, face);

    FT_Done_Face(face);

    return true;
}

static void deinit_font(struct bh_font font) { BH_DeinitTextures(font.textures); }

static bool init_freetype(struct bh_renderer* renderer) {
    if (FT_Init_FreeType(&renderer->ft)) {
        error("FreeType initialisation failed");
        return false;
    }
    return true;
}

static void deinit_freetype(FT_Library ft) { FT_Done_FreeType(ft); }

static void update_projection_matrix(struct bh_renderer* renderer) {
    m4_scale(renderer->projection_matrix, 1.0f, 1.0f, 1.0f);

    m4 projection;
    m4_ortho(projection, 1.0f, renderer->width, 1.0f, renderer->height, 0.001f, 1000.0f);
    m4_multiply(renderer->projection_matrix, projection);

    glUniformMatrix4fv(
        glGetUniformLocation(renderer->program, "projection_matrix"), 1, GL_FALSE,
        (const GLfloat*)renderer->projection_matrix
    );
}

bool BH_InitRenderer(struct bh_renderer* renderer) {
    renderer->width = 640;
    renderer->height = 480;

    if (!init_gl(renderer))
        return false;
    if (!init_shaders(renderer))
        return false;
    if (!init_freetype(renderer))
        return false;
    if (!init_font(renderer->ft, &renderer->font, 16, (void*)ASSET_font, sizeof(ASSET_font) - 1))
        return false;

    renderer->batch = BH_InitBatch();
    update_projection_matrix(renderer);

    return true;
}

void BH_RendererBeginFrame(struct bh_renderer* renderer) {
    int width, height;
    glfwGetFramebufferSize(renderer->window, &width, &height);

    if (width != renderer->width || height != renderer->height) {
        renderer->width = width;
        renderer->height = height;
        glViewport(0, 0, renderer->width, renderer->height);
        update_projection_matrix(renderer);
    }

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
}

void BH_RendererEndFrame(struct bh_renderer* renderer) { glfwSwapBuffers(renderer->window); }

void BH_DeinitRenderer(struct bh_renderer* renderer) {
    deinit_font(renderer->font);
    deinit_freetype(renderer->ft);

    BH_DeinitTextures(renderer->textures);
    BH_DeinitBatch(renderer->batch);
    BH_DeinitProgram(renderer->program);

    glfwDestroyWindow(renderer->window);
    glfwTerminate();
}
