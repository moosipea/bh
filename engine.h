#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <stdbool.h>

#include "entitydef.h"
#include "matrix.h"
#include "qtree.h"
#include "res/built_assets.h"

#define BH_MAX_TEXTURES 256
#define BH_BATCH_SIZE 1024

typedef GLuint bh_program;
typedef GLuint bh_shader;
typedef GLuint bh_texture;
typedef GLuint bh_vbo;
typedef GLuint bh_vao;

bh_program create_program(const GLchar* vertex_src, const GLchar* fragment_src);
void delete_program(bh_program* program);

struct bh_mesh_handle {
    bh_vao vao_handle;
    bh_vbo vbo_handle;
};

struct bh_mesh_handle upload_mesh(const GLfloat* vertices, size_t count);

struct bh_textures {
    bh_texture texture_ids[BH_MAX_TEXTURES];
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

struct bh_entity_ll {
    struct bh_sprite_entity entity;
    struct bh_entity_ll* next;
};

struct bh_de_ll {
    struct bh_entity_ll* entities;
    struct bh_entity_ll* last;
};

typedef bool (*user_cb)(struct bh_ctx* ctx, void* user_state);

struct bh_ctx {
    int width, height;
    GLFWwindow* window;
    bh_program program;
    m4 projection_matrix;
    float dt;

    struct bh_sprite_batch batch;
    struct bh_textures textures;

    struct bh_de_ll entities;
    struct bh_qtree entity_qtree;

    GLuint64 debug_texture;
    GLuint64 green_debug_texture;

    FT_Library ft;

    void* user_state;
};

bool init_ctx(struct bh_ctx* ctx, void* user_state, user_cb user_init);
void ctx_run(struct bh_ctx* ctx);

bool get_key(int glfw_key);

struct bh_sprite_batch batch_init(void);
void batch_render(struct bh_sprite_batch* batch, struct bh_sprite sprite, bh_program program);
void batch_finish(struct bh_sprite_batch* batch, bh_program program);
void batch_delete(struct bh_sprite_batch batch);

void spawn_entity(struct bh_de_ll* entities, struct bh_sprite_entity entity);
void entities_free(struct bh_entity_ll* entities);

void tick_all_entities(
    struct bh_ctx* state, struct bh_entity_ll* entities, struct bh_qtree* qtree,
    struct bh_sprite_batch* batch, bh_program program
);

bool entities_collide(struct bh_sprite_entity* entity, struct bh_sprite_entity* other);
