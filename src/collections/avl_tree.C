#include "avl_tree.h"

#include "globals.h"
#include "util.h"

/* Copied and modified from http://pine.cs.yale.edu/pinewiki/C/AvlTree google cache */
/* implementation of an AVL tree with explicit heights */

/* free a tree */
void avlDestroy(AvlTree t)
{
  if (t != AVL_EMPTY) {
    avlDestroy(t->child[0]);
    t->child[0] = AVL_EMPTY;
    avlDestroy(t->child[1]);
    t->child[1] = AVL_EMPTY;
    avl_free(t);
  }
}

/* return height of an AVL tree */
int avlGetHeight(AvlTree t)
{
  if (t != AVL_EMPTY) {
    return t->height;
  } else {
    return 0;
  }
}

/* return nonzero if key is present in tree */
int avlSearch(AvlTree t, Event *key)
{
  if (t == AVL_EMPTY) {
    return 0;
  }

  if (key->ts == t->key->ts) {
    // Timestamp is the same
    if (key->event_id == t->key->event_id) {
      // Event ID is the same
      if (key->src_lp == t->key->src_lp) {
        // src_lp is the same
        return 1;
      } else {
        // src_lp is different
        return avlSearch(t->child[key->src_lp > t->key->src_lp], key);
      }
    } else {
      // Event ID is different
      return avlSearch(t->child[key->event_id > t->key->event_id], key);
    }
  } else {
    // Timestamp is different
    return avlSearch(t->child[key->ts > t->key->ts], key);
  }
}

#define Max(x,y) ((x)>(y) ? (x) : (y))

/* assert height fields are correct throughout tree */
void avlSanityCheck(AvlTree root)
{
  int i;

  if (root != AVL_EMPTY) {
    for (i = 0; i < 2; i++) {
      avlSanityCheck(root->child[i]);
    }

    TW_ASSERT(root->height == 1 +
        Max(avlGetHeight(root->child[0]), avlGetHeight(root->child[1])),
        "AVL Sanity Check Failed\n");
  }
}

/* recompute height of a node */
static void avlFixHeight(AvlTree t)
{
  TW_ASSERT(t != AVL_EMPTY, "Empty tree in avlFixHeight()\n");
  t->height = 1 + Max(avlGetHeight(t->child[0]), avlGetHeight(t->child[1]));
}

/* rotate child[d] to root */
/* assumes child[d] exists */
/* Picture:
 *
 *     y            x
 *    / \   <==>   / \
 *   x   C        A   y
 *  / \              / \
 * A   B            B   C
 *
 */
static void avlRotate(AvlTree *root, int d)
{
  AvlTree oldRoot;
  AvlTree newRoot;
  AvlTree oldMiddle;

  oldRoot = *root;
  newRoot = oldRoot->child[d];
  oldMiddle = newRoot->child[!d];

  oldRoot->child[d] = oldMiddle;
  newRoot->child[!d] = oldRoot;
  *root = newRoot;

  /* update heights */
  avlFixHeight((*root)->child[!d]);   /* old root */
  avlFixHeight(*root);                /* new root */
}


/* rebalance at node if necessary */
/* also fixes height */
static void avlRebalance(AvlTree *t)
{
  int d;

  if (*t != AVL_EMPTY) {
    for (d = 0; d < 2; d++) {
      /* maybe child[d] is now too tall */
      if (avlGetHeight((*t)->child[d]) > avlGetHeight((*t)->child[!d]) + 1) {
        /* imbalanced! */
        /* how to fix it? */
        /* need to look for taller grandchild of child[d] */
        if (avlGetHeight((*t)->child[d]->child[d]) > avlGetHeight((*t)->child[d]->child[!d])) {
          /* same direction grandchild wins, do single rotation */
          avlRotate(t, d);
        } else {
          /* opposite direction grandchild moves up, do double rotation */
          avlRotate(&(*t)->child[d], !d);
          avlRotate(t, d);
        }

        return;   /* avlRotate called avlFixHeight */
      }
    }

    /* update height */
    avlFixHeight(*t);
  }
}

/* insert into tree */
/* this may replace root, which is why we pass
 * in a AvlTree * */
void avlInsert(AvlTree *t, Event *key)
{
  key->state.avl_tree = 1;
  /* insertion procedure */
  if (*t == AVL_EMPTY) {
    /* new t */
    *t = avl_alloc();
    (*t)->child[0] = AVL_EMPTY;
    (*t)->child[1] = AVL_EMPTY;

    (*t)->key = key;

    (*t)->height = 1;

    /* done */
    return;
  }

  if (key->ts == (*t)->key->ts) {
    // We have a timestamp tie, check the event ID
    if (key->event_id == (*t)->key->event_id) {
      // We have a event ID tie, check the src_lp
      if (key->src_lp == (*t)->key->src_lp) {
        // This shouldn't happen but we'll allow it
        CkPrintf("Warning: identical events in AVL tree!\n");
      }
      avlInsert(&(*t)->child[key->src_lp > (*t)->key->src_lp], key);
      avlRebalance(t);
    } else {
      // Event IDs are different
      avlInsert(&(*t)->child[key->event_id > (*t)->key->event_id], key);
      avlRebalance(t);
    }
    return;
  } else {
    // Timestamps are different
    avlInsert(&(*t)->child[key->ts > (*t)->key->ts], key);
    avlRebalance(t);
  }
}


/* print all elements of the tree in order */
void avlPrintKeys(AvlTree t)
{
  if (t != AVL_EMPTY) {
    avlPrintKeys(t->child[0]);
    CkPrintf("%f\n", t->key->ts);
    avlPrintKeys(t->child[1]);
  }
}


/* delete and return minimum value in a tree */
Event * avlDeleteMin(AvlTree *t)
{
  TW_ASSERT(t != AVL_EMPTY, "Can't delete from an empty AVL tree\n");
  AvlTree oldroot;
  Event *event_with_lowest_ts = NULL;

  if ((*t)->child[0] == AVL_EMPTY) {
    /* root is min value */
    oldroot = *t;
    event_with_lowest_ts = oldroot->key;
    *t = oldroot->child[1];
    avl_free(oldroot);
  } else {
    /* min value is in left subtree */
    event_with_lowest_ts = avlDeleteMin(&(*t)->child[0]);
  }

  avlRebalance(t);
  event_with_lowest_ts->state.avl_tree = 0;
  return event_with_lowest_ts;
}

/* delete the given value */
Event * avlDelete(AvlTree *t, Event *key)
{
  TW_ASSERT(t != AVL_EMPTY, "Can't delete from an empty AVL tree\n");
  Event *target = NULL;
  AvlTree oldroot;

  if (key->ts == (*t)->key->ts) {
    // We have a timestamp tie, check the event ID
    if (key->event_id == (*t)->key->event_id) {
      // We have a event ID tie, check the src_lp
      if (key->src_lp == (*t)->key->src_lp) {
        // This is actually the one we want to delete
        target = (*t)->key;
        /* do we have a right child? */
        if ((*t)->child[1] != AVL_EMPTY) {
          /* give root min value in right subtree */
          (*t)->key = avlDeleteMin(&(*t)->child[1]);
          (*t)->key->state.avl_tree = 1;
        } else {
          /* splice out root */
          oldroot = (*t);
          *t = (*t)->child[0];
          avl_free(oldroot);
        }
      } else {
        // Timestamp and event IDs are the same, but different src_lp
        target = avlDelete(&(*t)->child[key->src_lp > (*t)->key->src_lp], key);
      }
    } else {
      // Timestamps are the same but event IDs differ
      target = avlDelete(&(*t)->child[key->event_id > (*t)->key->event_id], key);
    }
  } else {
    // Timestamps are different
    target = avlDelete(&(*t)->child[key->ts > (*t)->key->ts], key);
  }

  avlRebalance(t);
  target->state.avl_tree = 0;
  return target;
}

/* Attempt to insert an event, but if we find it already exists in the tree,
 * don't insert. Instead remove the existing event and return it. Returns NULL
 * if the insertion was successful. Makes calls to avlInsert and avlDelete.
 */
Event * avlInsertOrDelete(AvlTree *t, Event *key)
{
  // If we've hit the empty node, then key is not in the tree, so insert.
  if (*t == AVL_EMPTY) {
    avlInsert(t, key);
    return NULL;
  }

  Event* target;
  if (key->ts == (*t)->key->ts) {
    if (key->event_id == (*t)->key->event_id) {
      if (key->src_lp == (*t)->key->src_lp) {
        // We've hit the exact event, so insertion fails and delete it.
        return avlDelete(t, key);
      } else {
        // src_lp is different
        target = avlInsertOrDelete(&(*t)->child[key->src_lp > (*t)->key->src_lp], key);
      }
    } else {
      // Event ID is different
      target = avlInsertOrDelete(&(*t)->child[key->event_id > (*t)->key->event_id], key);
    }
  } else {
    // Timestamp is different
    target = avlInsertOrDelete(&(*t)->child[key->ts > (*t)->key->ts], key);
  }

  // We still need to recursively rebalance as we go back up the tree.
  avlRebalance(t);
  return target;
}

AvlTree avl_alloc(void)
{
  TW_ASSERT(PE_VALUE(avl_list_head) != NULL, "Out of AVL nodes\n");

  AvlTree head = PE_VALUE(avl_list_head);
  PE_VALUE(avl_list_head) = head->next;
  head->next = NULL;

  return head;
}

void avl_free(AvlTree t)
{
  (t)->child[0] = AVL_EMPTY;
  (t)->child[1] = AVL_EMPTY;
  (t)->key = NULL;
  (t)->height = 0;
  (t)->next = PE_VALUE(avl_list_head);
  PE_VALUE(avl_list_head) = t;
}
