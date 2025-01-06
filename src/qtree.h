#pragma once

#include "entitydef.h"
#include "matrix.h"
#include <stdbool.h>

#define QT_MAX_ELEMENTS 4

bool BH_IsPointInBox(struct BH_BB box, struct vec2 point);
bool BH_DoBoxesIntersect(struct BH_BB box, struct BH_BB other);

struct BH_QTreeEntity {
    struct vec2 point;
    struct BH_SpriteEntity* entity;
};

struct BH_QTree {
    struct BH_QTreeEntity elements[QT_MAX_ELEMENTS];
    size_t element_count;

    struct BH_BB bb;

    struct BH_QTree* top_left;
    struct BH_QTree* top_right;
    struct BH_QTree* bottom_left;
    struct BH_QTree* bottom_right;
};

bool BH_InsertQTree(struct BH_QTree* qtree, struct BH_QTreeEntity point);
struct BH_QTreeQuery BH_QueryQTree(struct BH_QTree* qtree, struct BH_BB box);
void BH_DeinitQTree(struct BH_QTree* qtree);
bool BH_IsQTreeLeaf(struct BH_QTree* qtree);

struct BH_QTreeQuery {
    struct BH_QTreeEntity** entities;
    size_t count;
    size_t capacity;
};

void BH_DeinitQuery(struct BH_QTreeQuery query);

struct vec2 BH_BoxCentre(struct BH_BB bb);
struct vec2 BH_BoxDimensions(struct BH_BB bb);
struct BH_BB BH_BoxToWorld(struct vec2 centre, struct BH_BB dimensions);
