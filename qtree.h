#ifndef BH_QTREE_H
#define BH_QTREE_H

#include <stdbool.h>
#include "engine.h"

struct bh_qtree {
    struct bh_sprite_ll *items;
    struct {
        struct bh_qtree *top_left;
        struct bh_qtree *top_right;
        struct bh_qtree *bot_left;
        struct bh_qtree *bot_right;
    } branch;
};

#endif
