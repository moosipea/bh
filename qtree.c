#include "qtree.h"
#include "error_macro.h"
#include "matrix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool is_point_in_box(struct bh_bounding_box box, struct vec2 point) {
    return point.x >= box.top_left.x && point.x <= box.bottom_right.x &&
           point.y <= box.top_left.y && point.y >= box.bottom_right.y;
}

bool do_boxes_intersect(struct bh_bounding_box box, struct bh_bounding_box other) {
    return box.top_left.x <= other.bottom_right.x && box.bottom_right.x >= other.top_left.x &&
           box.top_left.y >= other.bottom_right.y && box.bottom_right.y <= other.top_left.y;
}

struct vec2 box_centre(struct bh_bounding_box bb) {
    return (struct vec2){ (bb.top_left.x + bb.bottom_right.x) / 2.0f,
                          (bb.top_left.y + bb.bottom_right.y) / 2.0f };
}

struct vec2 box_dimensions(struct bh_bounding_box bb) {
    return (struct vec2){ bb.bottom_right.x - bb.top_left.x, bb.bottom_right.y - bb.top_left.y };
}

struct bh_bounding_box bb_make_global(struct vec2 centre, struct bh_bounding_box dimensions) {
    return (struct bh_bounding_box){
        .top_left = vec2_add(dimensions.top_left, centre),
        .bottom_right = vec2_add(dimensions.bottom_right, centre),
    };
}

static inline struct bh_qtree* create_subdivision(struct vec2 top_left, struct vec2 bottom_right) {
    struct bh_qtree* qtree = calloc(1, sizeof(struct bh_qtree));

    qtree->bb = (struct bh_bounding_box){
        .top_left = top_left,
        .bottom_right = bottom_right,
    };

    return qtree;
}

static inline void qtree_subdivide(struct bh_qtree* qtree) {
    const struct vec2 centre = box_centre(qtree->bb);

    qtree->top_left = create_subdivision(qtree->bb.top_left, centre);
    qtree->top_right = create_subdivision(
        (struct vec2){ centre.x, qtree->bb.top_left.y },
        (struct vec2){ qtree->bb.bottom_right.x, centre.y }
    );
    qtree->bottom_left = create_subdivision(
        (struct vec2){ qtree->bb.top_left.x, centre.y },
        (struct vec2){ centre.x, qtree->bb.bottom_right.y }
    );
    qtree->bottom_right = create_subdivision(centre, qtree->bb.bottom_right);
}

bool qtree_is_leaf(struct bh_qtree* qtree) {
    if (qtree == NULL) {
        error("qtree == NULL");
    }
    return qtree->top_left == NULL;
}

// See: https://en.wikipedia.org/wiki/Quadtree
bool qtree_insert(struct bh_qtree* qtree, struct bh_qtree_entity entity) {
    if (!is_point_in_box(qtree->bb, entity.point)) {
        return false;
    }

    if (qtree->element_count < QT_MAX_ELEMENTS && qtree_is_leaf(qtree)) {
        qtree->elements[qtree->element_count++] = entity;
        return true;
    }

    if (qtree_is_leaf(qtree)) {
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

#define QUERY_START_CAPACITY 32
static struct bh_qtree_query query_init(void) {
    return (struct bh_qtree_query){
        .entities = calloc(QUERY_START_CAPACITY, sizeof(struct bh_qtree_entity*)),
        .count = 0,
        .capacity = QUERY_START_CAPACITY,
    };
}

#define QUERY_GROW_FACTOR 2
static void query_append(struct bh_qtree_query* query, struct bh_qtree_entity* entity) {
    if (query->count >= query->capacity) {
        query->capacity *= QUERY_GROW_FACTOR;
        query->entities =
            realloc(query->entities, query->capacity * sizeof(struct bh_qtree_entity*));
    }
    query->entities[query->count++] = entity;
}

static void qtree_query_recursively(
    struct bh_qtree* qtree, struct bh_bounding_box box, struct bh_qtree_query* query
) {
    if (qtree == NULL || query == NULL) {
        return;
    }

    if (!do_boxes_intersect(box, qtree->bb)) {
        return;
    }

    if (qtree_is_leaf(qtree)) {
        for (size_t i = 0; i < qtree->element_count; i++) {
            query_append(query, &qtree->elements[i]);
        }
    } else {
        qtree_query_recursively(qtree->top_left, box, query);
        qtree_query_recursively(qtree->top_right, box, query);
        qtree_query_recursively(qtree->bottom_left, box, query);
        qtree_query_recursively(qtree->bottom_right, box, query);
    }
}

struct bh_qtree_query qtree_query(struct bh_qtree* qtree, struct bh_bounding_box box) {
    struct bh_qtree_query query = query_init();
    qtree_query_recursively(qtree, box, &query);
    return query;
}

void qtree_free(struct bh_qtree* qtree) {
    if (qtree->top_left) {
        qtree_free(qtree->top_left);
        free(qtree->top_left);
    }
    if (qtree->top_right) {
        qtree_free(qtree->top_right);
        free(qtree->top_right);
    }
    if (qtree->bottom_left) {
        qtree_free(qtree->bottom_left);
        free(qtree->bottom_left);
    }
    if (qtree->bottom_right) {
        qtree_free(qtree->bottom_right);
        free(qtree->bottom_right);
    }
}

void query_free(struct bh_qtree_query query) { free(query.entities); }
