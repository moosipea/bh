#ifndef BH_ENGINE_H
#define BH_ENGINE_H

#include <glad/gl.h>
#include <stdlib.h>
#include "matrix.h"

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
    bh_texture texture; 
    m4 transform;
};

#endif // !BH_ENGINE_H
