#include "qtree.h"
#include "matrix.h"
#include <stdlib.h>

bool point_in_box(struct bounding_box box, struct vec2 point) {
    return point.x >= box.top_left.x && point.x < box.bottom_right.x &&
           point.y >= box.top_left.y && point.y < box.bottom_right.y;
}

bool boxes_intersect(struct bounding_box box, struct bounding_box other);

static inline struct bh_qtree*
create_subdivision(struct vec2 top_left, struct vec2 bottom_right) {
    struct bh_qtree* qtree = calloc(1, sizeof(struct bh_qtree));

    qtree->bb = (struct bounding_box){
        .top_left     = top_left,
        .bottom_right = bottom_right,
    };

    return qtree;
}

static inline void qtree_subdivide(struct bh_qtree* qtree) {
    const float centre_x =
        (qtree->bb.top_left.x + qtree->bb.bottom_right.x) / 2.0f;
    const float centre_y =
        (qtree->bb.top_left.y + qtree->bb.bottom_right.y) / 2.0f;

    qtree->top_left = create_subdivision(
        qtree->bb.top_left, (struct vec2){centre_x, centre_y}
    );
    qtree->top_right = create_subdivision(
        (struct vec2){centre_x, qtree->bb.top_left.y},
        (struct vec2){qtree->bb.bottom_right.x, centre_y}
    );
    qtree->bottom_left = create_subdivision(
        (struct vec2){qtree->bb.top_left.x, centre_y},
        (struct vec2){centre_x, qtree->bb.bottom_right.y}
    );
    qtree->bottom_right = create_subdivision(
        (struct vec2){centre_x, centre_y}, qtree->bb.bottom_right
    );
}

static inline bool is_leaf(struct bh_qtree *qtree) {
    return qtree->top_left == NULL;
}

// See: https://en.wikipedia.org/wiki/Quadtree
bool qtree_insert(struct bh_qtree* qtree, struct qtree_entity entity) {
    if (!point_in_box(qtree->bb, entity.point)) {
        return false;
    }

    if (qtree->element_count < QT_MAX_ELEMENTS && is_leaf(qtree)) {
        qtree->elements[qtree->element_count++] = entity;
        return true;
    }

    if (is_leaf(qtree)) {
        qtree_subdivide(qtree);
        for (size_t i = 0; i < qtree->element_count; i++) {
            qtree_insert(qtree, qtree->elements[i]);
        }
    }

    if (qtree_insert(qtree->top_left, entity))
        return true;
    if (qtree_insert(qtree->top_right, entity))
        return true;
    if (qtree_insert(qtree->bottom_left, entity))
        return true;
    if (qtree_insert(qtree->bottom_right, entity))
        return true;

    // Should be unreachable
    return false;
}

void qtree_free(struct bh_qtree* qtree) {
    if (qtree->top_left) {
        qtree_free(qtree->top_left->top_left);
        free(qtree->top_left);
    }
    if (qtree->top_right) {
        qtree_free(qtree->top_right->top_right);
        free(qtree->top_right);
    }
    if (qtree->bottom_left) {
        qtree_free(qtree->bottom_left->bottom_left);
        free(qtree->bottom_left);
    }
    if (qtree->bottom_right) {
        qtree_free(qtree->bottom_right->bottom_right);
        free(qtree->bottom_right);
    }
}
