#pragma once

#include "matrix.h"
#include "entitydef.h"
#include <stdbool.h>

#define QT_MAX_ELEMENTS 4

struct bh_bounding_box {
    struct vec2 top_left;
    struct vec2 bottom_right;
};

bool is_point_in_box(struct bh_bounding_box box, struct vec2 point);
bool do_boxes_intersect(struct bh_bounding_box box, struct bh_bounding_box other);

struct bh_qtree_entity {
    struct vec2 point;
    struct bh_sprite_entity entity;
};

struct bh_qtree {
    struct bh_qtree_entity elements[QT_MAX_ELEMENTS];
    size_t element_count;

    struct bh_bounding_box bb;

    struct bh_qtree* top_left;
    struct bh_qtree* top_right;
    struct bh_qtree* bottom_left;
    struct bh_qtree* bottom_right;
};

bool qtree_insert(struct bh_qtree* qtree, struct bh_qtree_entity point);
struct bh_qtree_query qtree_query(
    struct bh_qtree* qtree, struct bh_bounding_box box
);
void qtree_free(struct bh_qtree* qtree);

struct bh_qtree_query {
    struct bh_qtree_entity **entities;
    size_t count;
    size_t capacity;
};

void query_free(struct bh_qtree_query query);
