#pragma once

#include "entitydef.h"
#include "matrix.h"
#include <stdbool.h>

#define QT_MAX_ELEMENTS 4

bool BH_IsPointInBox(struct bh_bounding_box box, struct vec2 point);
bool BH_DoBoxesIntersect(struct bh_bounding_box box, struct bh_bounding_box other);

struct bh_qtree_entity {
    struct vec2 point;
    struct bh_sprite_entity* entity;
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

bool BH_InsertQTree(struct bh_qtree* qtree, struct bh_qtree_entity point);
struct bh_qtree_query BH_QueryQTree(struct bh_qtree* qtree, struct bh_bounding_box box);
void BH_DeinitQTree(struct bh_qtree* qtree);
bool BH_IsQTreeLeaf(struct bh_qtree* qtree);

struct bh_qtree_query {
    struct bh_qtree_entity** entities;
    size_t count;
    size_t capacity;
};

void BH_DeinitQuery(struct bh_qtree_query query);

struct vec2 BH_BoxCentre(struct bh_bounding_box bb);
struct vec2 BH_BoxDimensions(struct bh_bounding_box bb);
struct bh_bounding_box BH_BoxToWorld(struct vec2 centre, struct bh_bounding_box dimensions);
