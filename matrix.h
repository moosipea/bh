#ifndef BH_MATRIX_H
#define BH_MATRIX_H

#include <glad/gl.h>

typedef GLfloat m4[4][4];

void m4_identity(m4 matrix);
void m4_translation(m4 matrix, float x, float y, float z);
void m4_scale(m4 matrix, float x, float y, float z);
void m4_rotation(m4 matrix, float x, float y, float z);
void m4_ortho(
    m4 matrix, float left, float right, float top, float bottom, float near,
    float far
);

void m4_multiply(m4 dest, m4 mat);

#endif // !BH_MATRIX_H
