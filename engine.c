#include "engine.h"
#include "GLFW/glfw3.h"
#include "entitydef.h"
#include "matrix.h"
#include "qtree.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SPNG_STATIC
#include <spng.h>

#include "error_macro.h"

#define RENDER_DEBUG_INFO

static bool compile_shader(bh_shader shader, const GLchar* src) {
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

static inline bool link_program(bh_program program) {
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

bh_program create_program(const GLchar* vertex_src, const GLchar* fragment_src) {
    bh_shader vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    bh_shader fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    bh_program program = glCreateProgram();

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

void delete_program(bh_program* program) {
    glDeleteProgram(*program);
    *program = 0;
}

struct bh_mesh_handle upload_mesh(const GLfloat* vertices, size_t count) {
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

static inline void* load_png(void* png_data, size_t size, size_t* width, size_t* height) {
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

static inline bh_texture upload_texture(void* image, size_t width, size_t height) {
    bh_texture texture;
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

static inline bh_texture create_texture(void* png_data, size_t size) {
    void* image_data;
    size_t width, height;

    if (!(image_data = load_png(png_data, size, &width, &height))) {
        error("Couldn't load image");
        return 0;
    }

    bh_texture texture = upload_texture(image_data, width, height);

    free(image_data);

    return texture;
}

GLuint64 textures_load(struct bh_textures* textures, void* png_data, size_t size) {
    if (textures->count >= BH_MAX_TEXTURES) {
        error("Couldn't load texture, textures->count exceeds BH_MAX_TEXTURES");
        return 0;
    }

    bh_texture texture = create_texture(png_data, size);
    if (!texture) {
        error("Couldn't create texture");
        return 0;
    }

    /* Create bindless texture handle */
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

void textures_delete(struct bh_textures textures) {
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

struct bh_sprite_batch batch_init(void) {
    struct bh_sprite_batch res = { 0 };

    /* For use with GL_TRIANGLE_FAN */
    /* Positions and UV coords */
    const GLfloat vertices[] = { -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,  -1.0f, 0.0f, 1.0f, 0.0f,
                                 1.0f,  1.0f,  0.0f, 1.0f, 1.0f, -1.0f, 1.0f,  0.0f, 0.0f, 1.0f };

    res.mesh = upload_mesh(vertices, sizeof(vertices) / sizeof(vertices[0]));
    res.instances_ssbo = create_ssbo(res.instance_transforms, sizeof(res.instance_transforms));
    res.textures_ssbo = create_ssbo(res.instance_textures, sizeof(res.instance_textures));

    return res;
}

void batch_delete(struct bh_sprite_batch batch) {
    glDeleteBuffers(1, &batch.textures_ssbo);
    glDeleteBuffers(1, &batch.instances_ssbo);
}

void batch_render(struct bh_sprite_batch* batch, struct bh_sprite sprite, bh_program program) {
    /* Insert sprite data into batch array */
    memcpy(&batch->instance_transforms[batch->count], &sprite.transform, sizeof(m4));
    batch->instance_textures[batch->count] = sprite.texture_handle;
    batch->count++;

    /* Draw the batch when it is full */
    if (batch->count >= BH_BATCH_SIZE) {
        batch_finish(batch, program);
    }
}

static inline void batch_drawcall(struct bh_sprite_batch* batch, bh_program program) {
    glUseProgram(program);
    glBindVertexArray(batch->mesh.vao_handle);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, batch->count);

    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(0);
    glUseProgram(0);
}

void batch_finish(struct bh_sprite_batch* batch, bh_program program) {
    /* Sync SSBO contents */
    glNamedBufferSubData(
        batch->instances_ssbo, 0, batch->count * sizeof(m4), batch->instance_transforms
    );
    glNamedBufferSubData(
        batch->textures_ssbo, 0, batch->count * sizeof(GLuint64), batch->instance_textures
    );

    /* Make sure SSBOs are bound */
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, batch->instances_ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, batch->textures_ssbo);

    /* Draw call */
    batch_drawcall(batch, program);

    /* Reset batch state */
    batch->count = 0;
}

void spawn_entity(struct bh_de_ll* entities, struct bh_sprite_entity entity) {
    if (entities->entities == NULL) {
        entities->entities = calloc(1, sizeof(struct bh_entity_ll));
    }

    if (entities->last == NULL) {
        entities->entities->entity = entity;
        entities->last = entities->entities;
    } else {
        entities->last->next = calloc(1, sizeof(struct bh_entity_ll));
        entities->last->next->entity = entity;
        entities->last = entities->last->next;
    }
}

void entities_free(struct bh_entity_ll* entities) {
    if (entities->next != NULL) {
        entities_free(entities->next);
    }
    if (entities->entity.state) {
        free(entities->entity.state);
    }
    free(entities);
}

static inline void update_entity_transform(struct bh_sprite_entity* entity) {
    m4 model_matrix;
    m4 translation;

    m4_scale(model_matrix, entity->scale.x, entity->scale.y, 1.0f);
    m4_translation(translation, entity->position.x, entity->position.y, 0.0f);
    m4_multiply(model_matrix, translation);

    memcpy(entity->sprite.transform, model_matrix, sizeof(m4));
}

#ifdef RENDER_DEBUG_INFO
static void render_bounding_box(
    struct bh_sprite_batch* batch, struct bh_bounding_box bb, struct vec2 offset, GLuint64 texture,
    bh_program program
) {
    struct bh_sprite hitbox_sprite = {
        .texture_handle = texture,
    };

    struct bh_bounding_box globalised_bb = bb_make_global(offset, bb);
    struct vec2 hitbox_centre = box_centre(globalised_bb);
    struct vec2 hitbox_dimensions = box_dimensions(globalised_bb);

    m4_scale(hitbox_sprite.transform, hitbox_dimensions.x / 2.0f, hitbox_dimensions.y / 2.0f, 1.0f);

    m4 translation;
    m4_translation(translation, hitbox_centre.x, hitbox_centre.y, 0.0f);
    m4_multiply(hitbox_sprite.transform, translation);

    batch_render(batch, hitbox_sprite, program);
}
#endif

#ifdef RENDER_DEBUG_INFO
static void render_qtree(
    struct bh_qtree* qtree, GLuint64 texture, struct bh_sprite_batch* batch, bh_program program
) {
    if (qtree == NULL) {
        return;
    }

    if (qtree_is_leaf(qtree)) {
        render_bounding_box(batch, qtree->bb, (struct vec2){ 0.0f, 0.0f }, texture, program);
    } else {
        render_qtree(qtree->top_left, texture, batch, program);
        render_qtree(qtree->top_right, texture, batch, program);
        render_qtree(qtree->bottom_left, texture, batch, program);
        render_qtree(qtree->bottom_right, texture, batch, program);
    }
}
#endif

void tick_all_entities(
    struct bh_ctx* state, struct bh_entity_ll* entities, struct bh_qtree* qtree,
    struct bh_sprite_batch* batch, bh_program program
) {
    struct bh_qtree next_qtree = { .bb = qtree->bb };

    struct bh_entity_ll* node = entities;

    while (node != NULL) {
        struct bh_sprite_entity* entity = &node->entity;

        qtree_insert(
            &next_qtree, (struct bh_qtree_entity){ .entity = entity, .point = entity->position }
        );

        entity->callback(state, entity);
        update_entity_transform(entity);
        batch_render(batch, entity->sprite, program);
#ifdef RENDER_DEBUG_INFO
        render_bounding_box(batch, entity->bb, entity->position, state->debug_texture, program);
#endif

        node = node->next;
    }

#ifdef RENDER_DEBUG_INFO
    render_qtree(qtree, state->green_debug_texture, batch, program);
#endif

    batch_finish(batch, program);

    /* Update qtree */
    qtree_free(qtree);
    *qtree = next_qtree;
}

bool entities_collide(struct bh_sprite_entity* entity, struct bh_sprite_entity* other) {
    return do_boxes_intersect(
        bb_make_global(entity->position, entity->bb), bb_make_global(other->position, other->bb)
    );
}

static bool GLOBAL_KEYS_HELD[GLFW_KEY_LAST + 1];

static void glfw_key_cb(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)window;
    (void)scancode;
    (void)mods;
    if (action == GLFW_PRESS) {
        GLOBAL_KEYS_HELD[key] = true;
    } else if (action == GLFW_RELEASE) {
        GLOBAL_KEYS_HELD[key] = false;
    }
}

bool get_key(int glfw_key) {
    if (glfw_key < 0 || glfw_key > GLFW_KEY_LAST) {
        error("Invalid key: %d", glfw_key);
        return false;
    }
    return GLOBAL_KEYS_HELD[glfw_key];
}

static inline bool init_glfw(struct bh_ctx* ctx) {
    if (!glfwInit()) {
        error("GLFW initialization failed");
        return false;
    }

    ctx->window = glfwCreateWindow(ctx->width, ctx->height, "Hello, world!", NULL, NULL);
    if (!ctx->window) {
        error("Window creation failed");
        return false;
    }

    glfwSetKeyCallback(ctx->window, glfw_key_cb);

    glfwMakeContextCurrent(ctx->window);
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

static inline bool init_gl(struct bh_ctx* ctx) {
    if (!init_glfw(ctx)) {
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

static inline bool init_shaders(struct bh_ctx* ctx) {
    if ((ctx->program =
             create_program((const GLchar*)ASSET_vertex, (const GLchar*)ASSET_fragment))) {
        glUseProgram(ctx->program);
        return true;
    }
    return false;
}

static void update_projection_matrix(struct bh_ctx* ctx) {
    m4_scale(ctx->projection_matrix, 1.0f, 1.0f, 1.0f);

    m4 projection;
    m4_ortho(projection, 1.0f, ctx->width, 1.0f, ctx->height, 0.001f, 1000.0f);
    m4_multiply(ctx->projection_matrix, projection);

    glUniformMatrix4fv(
        glGetUniformLocation(ctx->program, "projection_matrix"), 1, GL_FALSE,
        (const GLfloat*)ctx->projection_matrix
    );
}

bool init_ctx(struct bh_ctx* ctx, void* user_state, user_cb user_init) {
    ctx->width = 640;
    ctx->height = 480;

    if (!init_gl(ctx))
        return false;
    if (!init_shaders(ctx))
        return false;

    ctx->batch = batch_init();
    ctx->entity_qtree.bb = (struct bh_bounding_box){
        .top_left = { -1.0f,  1.0f },
        .bottom_right = {  1.0f, -1.0f },
    };

#ifdef RENDER_DEBUG_INFO
    ctx->debug_texture = textures_load(&ctx->textures, (void*)ASSET_debug, sizeof(ASSET_debug) - 1);
    if (!ctx->debug_texture)
        return false;

    ctx->green_debug_texture =
        textures_load(&ctx->textures, (void*)ASSET_green_debug, sizeof(ASSET_debug) - 1);
    if (!ctx->green_debug_texture)
        return false;
#endif

    update_projection_matrix(ctx);

    ctx->user_state = user_state;
    if (!user_init(ctx, ctx->user_state))
        return false;

    return true;
}

static void begin_frame(struct bh_ctx* ctx) {
    glfwSetTime(0.0);
    glfwPollEvents();

    int width, height;
    glfwGetFramebufferSize(ctx->window, &width, &height);

    if (width != ctx->width || height != ctx->height) {
        ctx->width = width;
        ctx->height = height;
        glViewport(0, 0, ctx->width, ctx->height);
        update_projection_matrix(ctx);
    }

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
}

static void end_frame(struct bh_ctx* ctx) {
    glfwSwapBuffers(ctx->window);
    ctx->dt = (float)glfwGetTime();
}

static void delete_ctx(struct bh_ctx* ctx) {
    qtree_free(&ctx->entity_qtree);
    entities_free(ctx->entities.entities);
    textures_delete(ctx->textures);
    batch_delete(ctx->batch);
    delete_program(&ctx->program);
    glfwDestroyWindow(ctx->window);
    glfwTerminate();
}

void ctx_run(struct bh_ctx* ctx) {
    while (!glfwWindowShouldClose(ctx->window)) {
        begin_frame(ctx);
        tick_all_entities(
            ctx, ctx->entities.entities, &ctx->entity_qtree, &ctx->batch, ctx->program
        );
        end_frame(ctx);
    }
    delete_ctx(ctx);
}
