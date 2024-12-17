#ifndef BH_QTREE_H
#define BH_QTREE_H

#include "matrix.h"
#include <stdbool.h>
#include <vcruntime.h>

struct bounding_box {
    struct vec2 top_left;
    struct vec2 bottom_right;
};

bool point_in_box(struct bounding_box box, struct vec2 point);
bool boxes_intersect(struct bounding_box box, struct bounding_box other);

struct qtree_entity {
    struct vec2 point;
};

#define QT_MAX_ELEMENTS 4

struct bh_qtree {
    struct qtree_entity elements[QT_MAX_ELEMENTS];
    size_t element_count;

    struct bounding_box bb;

    struct bh_qtree* top_left;
    struct bh_qtree* top_right;
    struct bh_qtree* bottom_left;
    struct bh_qtree* bottom_right;
};

bool qtree_insert(struct bh_qtree* qtree, struct qtree_entity point);
size_t qtree_query(
    struct bh_qtree* qtree, struct bounding_box box, struct qtree_entity* dest,
    size_t count
);
void qtree_free(struct bh_qtree* qtree);

#endif
