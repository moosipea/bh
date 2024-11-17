#include "matrix.h"
#include <math.h>
#include <string.h>

/* If we're making a 2d game we don't really need 4x4 matrices, but
 * they're still more flexible. Could be used for some nice animations,
 * for example. */

void m4_identity(m4 matrix) {
    const m4 identity = {
        { 1, 0, 0, 0 },
        { 0, 1, 0, 0 },
        { 0, 0, 1, 0 },
        { 0, 0, 0, 1 },
    };
    memcpy(matrix, identity, sizeof(identity));
}

void m4_translation(m4 matrix, float x, float y, float z) {
    m4_identity(matrix);
    matrix[3][0] = x;
    matrix[3][1] = y;
    matrix[3][2] = z;
}

void m4_scale(m4 matrix, float x, float y, float z) {
    m4_identity(matrix);
    matrix[0][0] = x;
    matrix[1][1] = y;
    matrix[2][2] = z;
}

void m4_rotation(m4 matrix, float x, float y, float z);

void m4_multiply(m4 dest, m4 mat) {
    m4 res;
    for (size_t i = 0; i < 4; i++) {
        for (size_t j = 0; j < 4; j++) {
            res[i][j] = dest[i][0] * mat[0][j]
                      + dest[i][1] * mat[1][j]
                      + dest[i][2] * mat[2][j]
                      + dest[i][3] * mat[3][j];
        }
    }
    memcpy(dest, res, sizeof(res));
}
