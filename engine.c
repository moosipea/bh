#include "engine.h"
#include "entitydef.h"
#include "qtree.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SPNG_STATIC
#include <spng.h>

#include "error_macro.h"

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

void tick_all_entities(
    struct bh_ctx* state, struct bh_entity_ll* entities, struct bh_qtree* qtree, struct bh_sprite_batch* batch,
    bh_program program
) {
    struct bh_qtree next_qtree = {
        .bb = qtree->bb
    };

    struct bh_entity_ll* node = entities;

    while (node != NULL) {
        struct bh_sprite_entity *entity = &node->entity;

        qtree_insert(&next_qtree, (struct bh_qtree_entity) {
            .entity = entity,
            .point = entity->position
        });

        entity->callback(state, entity);
        update_entity_transform(entity);
        batch_render(batch, entity->sprite, program);

        node = node->next; 
    }

    batch_finish(batch, program);

    /* Update qtree */
    qtree_free(qtree);
    *qtree = next_qtree;
}
