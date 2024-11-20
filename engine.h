#ifndef BH_ENGINE_H
#define BH_ENGINE_H

#include <glad/gl.h>
#include <stdlib.h>
#include "matrix.h"

#define BH_MAX_TEXTURES 256
#define BH_BATCH_SIZE 1024

typedef GLuint bh_program;
typedef GLuint bh_shader;
typedef GLuint bh_texture;
typedef GLuint bh_vbo;
typedef GLuint bh_vao;


bh_program create_program(const GLchar *vertex_src, const GLchar *fragment_src);
void delete_program(bh_program *program);

struct bh_mesh_handle {
    bh_vao vao_handle;
    bh_vbo vbo_handle;
};

struct bh_mesh_handle upload_mesh(const GLfloat *vertices, size_t count);

struct bh_sprite {
    GLuint64 texture_handle;
    m4 transform;
};

struct bh_textures {
    bh_texture texture_ids[BH_MAX_TEXTURES];
    GLuint64 texture_handles[BH_MAX_TEXTURES];
    size_t count;
};

GLuint64 textures_load(struct bh_textures *textures, void *png_data, size_t size);
void textures_delete(struct bh_textures textures);

struct bh_sprite_batch {
    struct bh_mesh_handle mesh;
    m4 instance_transforms[BH_BATCH_SIZE];
    GLuint64 instance_textures[BH_BATCH_SIZE];
    size_t count;
    GLuint instances_ssbo;
    GLuint textures_ssbo;
};

struct bh_sprite_batch batch_init(void);
void batch_render(struct bh_sprite_batch *batch, struct bh_sprite sprite, bh_program program);
void batch_finish(struct bh_sprite_batch *batch, bh_program program);
void batch_delete(struct bh_sprite_batch batch);

#endif // !BH_ENGINE_H
