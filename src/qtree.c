#include "qtree.h"
#include "error_macro.h"
#include "matrix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool BH_IsPointInBox(struct bh_bounding_box box, struct vec2 point) {
    return point.x >= box.top_left.x && point.x <= box.bottom_right.x &&
           point.y >= box.top_left.y && point.y <= box.bottom_right.y;
}

bool BH_DoBoxesIntersect(struct bh_bounding_box box, struct bh_bounding_box other) {
    return box.top_left.x <= other.bottom_right.x && box.bottom_right.x >= other.top_left.x &&
           box.top_left.y <= other.bottom_right.y && box.bottom_right.y >= other.top_left.y;
}

struct vec2 BH_BoxCentre(struct bh_bounding_box bb) {
    return (struct vec2){ (bb.top_left.x + bb.bottom_right.x) / 2.0f,
                          (bb.top_left.y + bb.bottom_right.y) / 2.0f };
}

struct vec2 BH_BoxDimensions(struct bh_bounding_box bb) {
    return (struct vec2){ bb.bottom_right.x - bb.top_left.x, bb.bottom_right.y - bb.top_left.y };
}

struct bh_bounding_box BH_BoxToWorld(struct vec2 centre, struct bh_bounding_box dimensions) {
    return (struct bh_bounding_box){
        .top_left = vec2_add(dimensions.top_left, centre),
        .bottom_right = vec2_add(dimensions.bottom_right, centre),
    };
}

static struct bh_qtree* Subdivide(struct vec2 top_left, struct vec2 bottom_right) {
    struct bh_qtree* qtree = calloc(1, sizeof(struct bh_qtree));

    qtree->bb = (struct bh_bounding_box){
        .top_left = top_left,
        .bottom_right = bottom_right,
    };

    return qtree;
}

static void qtree_subdivide(struct bh_qtree* qtree) {
    const struct vec2 centre = BH_BoxCentre(qtree->bb);

    qtree->top_left = Subdivide(qtree->bb.top_left, centre);
    qtree->top_right = Subdivide(
        (struct vec2){ centre.x, qtree->bb.top_left.y },
        (struct vec2){ qtree->bb.bottom_right.x, centre.y }
    );
    qtree->bottom_left = Subdivide(
        (struct vec2){ qtree->bb.top_left.x, centre.y },
        (struct vec2){ centre.x, qtree->bb.bottom_right.y }
    );
    qtree->bottom_right = Subdivide(centre, qtree->bb.bottom_right);

    for (size_t i = 0; i < qtree->element_count; i++) {
        BH_InsertQTree(qtree, qtree->elements[i]);
    }
}

bool BH_IsQTreeLeaf(struct bh_qtree* qtree) {
    /* TODO: assert here */
    if (qtree == NULL) {
        error("qtree == NULL");
    }
    return qtree->top_left == NULL;
}

// See: https://en.wikipedia.org/wiki/Quadtree
bool BH_InsertQTree(struct bh_qtree* qtree, struct bh_qtree_entity entity) {
    if (!BH_IsPointInBox(qtree->bb, entity.point)) {
        return false;
    }

    if (qtree->element_count < QT_MAX_ELEMENTS && BH_IsQTreeLeaf(qtree)) {
        qtree->elements[qtree->element_count++] = entity;
        return true;
    }

    if (BH_IsQTreeLeaf(qtree)) {
        qtree_subdivide(qtree);
    }

    if (BH_InsertQTree(qtree->top_left, entity))
        return true;
    if (BH_InsertQTree(qtree->top_right, entity))
        return true;
    if (BH_InsertQTree(qtree->bottom_left, entity))
        return true;
    if (BH_InsertQTree(qtree->bottom_right, entity))
        return true;

    // Should be unreachable
    return false;
}

#define QUERY_START_CAPACITY 32
static struct bh_qtree_query InitQuery(void) {
    return (struct bh_qtree_query){
        .entities = calloc(QUERY_START_CAPACITY, sizeof(struct bh_qtree_entity*)),
        .count = 0,
        .capacity = QUERY_START_CAPACITY,
    };
}

#define QUERY_GROW_FACTOR 2
static void QueryAppend(struct bh_qtree_query* query, struct bh_qtree_entity* entity) {
    if (query->count >= query->capacity) {
        query->capacity *= QUERY_GROW_FACTOR;
        query->entities =
            realloc(query->entities, query->capacity * sizeof(struct bh_qtree_entity*));
    }
    query->entities[query->count++] = entity;
}

static void
QueryRecursively(struct bh_qtree* qtree, struct bh_bounding_box box, struct bh_qtree_query* query) {
    if (qtree == NULL || query == NULL) {
        return;
    }

    if (!BH_DoBoxesIntersect(box, qtree->bb)) {
        return;
    }

    if (BH_IsQTreeLeaf(qtree)) {
        for (size_t i = 0; i < qtree->element_count; i++) {
            QueryAppend(query, &qtree->elements[i]);
        }
    } else {
        QueryRecursively(qtree->top_left, box, query);
        QueryRecursively(qtree->top_right, box, query);
        QueryRecursively(qtree->bottom_left, box, query);
        QueryRecursively(qtree->bottom_right, box, query);
    }
}

struct bh_qtree_query BH_QueryQTree(struct bh_qtree* qtree, struct bh_bounding_box box) {
    struct bh_qtree_query query = InitQuery();
    QueryRecursively(qtree, box, &query);
    return query;
}

void BH_DeinitQTree(struct bh_qtree* qtree) {
    if (qtree->top_left) {
        BH_DeinitQTree(qtree->top_left);
        free(qtree->top_left);
    }
    if (qtree->top_right) {
        BH_DeinitQTree(qtree->top_right);
        free(qtree->top_right);
    }
    if (qtree->bottom_left) {
        BH_DeinitQTree(qtree->bottom_left);
        free(qtree->bottom_left);
    }
    if (qtree->bottom_right) {
        BH_DeinitQTree(qtree->bottom_right);
        free(qtree->bottom_right);
    }
}

void BH_DeinitQuery(struct bh_qtree_query query) { free(query.entities); }
