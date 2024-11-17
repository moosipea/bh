#include "engine.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "error_macro.h"

static bool compile_shader(bh_shader shader, const GLchar *src) {
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    GLint status, log_length;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    if (status != GL_TRUE) {
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
        if (log_length > 0) {
            GLchar *log_buffer = calloc(log_length, sizeof(GLchar));
            glGetShaderInfoLog(shader, log_length, NULL, log_buffer);
            error("Shader compilation error: %s\n", log_buffer);
            free(log_buffer);
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
            GLchar *log_buffer = calloc(log_length, sizeof(GLchar));
            glGetProgramInfoLog(program, log_length, NULL, log_buffer);
            error("Shader linking error: %s\n", log_buffer);
            free(log_buffer);
        }
        return false;
    }
    return true;
}

bh_program create_program(const GLchar *vertex_src, const GLchar *fragment_src) {
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

void delete_program(bh_program *program) {
    glDeleteProgram(*program);
    *program = 0;
}

struct bh_mesh_handle upload_mesh(const GLfloat *vertices, size_t count) {
    struct bh_mesh_handle res = {0};

    glGenVertexArrays(1, &res.vao_handle);
    glBindVertexArray(res.vao_handle);

    glGenBuffers(1, &res.vbo_handle);
    glBindBuffer(GL_ARRAY_BUFFER, res.vbo_handle);
    glBufferData(GL_ARRAY_BUFFER, count*sizeof(GLfloat), vertices, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    
    glDisableVertexAttribArray(0); 
    glBindVertexArray(0);

    return res;
}
