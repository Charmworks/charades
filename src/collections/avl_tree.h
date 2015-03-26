#include "typedefs.h"
#include "ross_event.h"

/* Copied and modified from http://pine.cs.yale.edu/pinewiki/C/AvlTree google cache */

/* implementation of an AVL tree with explicit heights */

struct avlNode {
  struct avlNode *child[2];    /* left and right */
  Event *key;
  struct avlNode *next;        /* for ROSS weird linked-list memory */
  int height;
  /* Enabling the padding should be tested */
  //int padding[7];
};

/* empty avl tree is just a null pointer */

#define AVL_EMPTY (0)

/* free a tree */
void avlDestroy(AvlTree t);

/* return the height of a tree */
int avlGetHeight(AvlTree t);

/* return nonzero if key is present in tree */
int avlSearch(AvlTree t, Event *key);

/* insert a new element into a tree */
/* note *t is actual tree */
void avlInsert(AvlTree *t, Event *key);

/* run sanity checks on tree (for debugging) */
/* assert will fail if heights are wrong */
void avlSanityCheck(AvlTree t);

/* print all keys of the tree in order */
void avlPrintKeys(AvlTree t);

/* delete and return minimum value in a tree */
Event * avlDeleteMin(AvlTree *t);

Event * avlDelete(AvlTree *t, Event *key);

Event * avlInsertOrDelete(AvlTree *t, Event *key);

AvlTree avl_alloc(void);

void avl_free(AvlTree t);
