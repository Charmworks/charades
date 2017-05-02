/*
 * queue-splay.c :- splay tree for priority queue
 * 
 * THIS IMPLEMENTATION IS ADAPTED FROM THE DASSF 
 * C++ IMPLEMENTATION.
 * THEIR COPYRIGHT IS ATTACHED BELOW
 */

/*
 * Copyright (c) 1998-2002 Dartmouth College
 *
 * Permission is hereby granted, free of charge, to any individual or 
 * institution obtaining a copy of this software and associated 
 * documentation files (the "Software"), to use, copy, modify, and 
 * distribute without restriction, provided that this copyright and 
 * permission notice is maintained, intact, in all copies and supporting 
 * documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL DARTMOUTH COLLEGE BE LIABLE FOR ANY CLAIM, DAMAGES 
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _PENDING_SPLAY_H_
#define _PENDING_SPLAY_H_

#include "pending_queue.h"

#include "event.h"
#include "ross_util.h"

#include <float.h>

// Macros for tree manipulation
#define UP(t)   ((t)->up)
#define UPUP(t)   ((t)->up->up)
#define LEFT(t)   ((t)->next)
#define RIGHT(t)  ((t)->prev)
#define KEY(t)          ((t)->ts)

#define ROTATE_R(n,p,g) \
  if((LEFT(p) = RIGHT(n))) UP(RIGHT(n)) = p;  RIGHT(n) = p; \
  UP(n) = g;  UP(p) = n;

#define ROTATE_L(n,p,g) \
  if((RIGHT(p) = LEFT(n))) UP(LEFT(n)) = p;  LEFT(n) = p; \
  UP(n) = g;  UP(p) = n;

class PendingSplay : public PendingQueue {
  private:
    Event* root;
    Event* least;
    unsigned nitems;
    unsigned max_size;

    Event** temp_event_buffer;

    void splay(Event* node) {
      register tw_event *n = node, *g, *p;
      register tw_event *x, *z;

      for (; (p = UP(n));) {
        if (n == LEFT(p)) {
          if (!((g = UPUP(n)))) {
            ROTATE_R(n, p, g);
          } else if (p == LEFT(g)) {
            if ((z = UP(g))) {
              if (g == LEFT(z)) {
                LEFT(z) = n;
              } else {
                RIGHT(z) = n;
              }
            }
            UP(n) = z;
            if ((x = LEFT(p) = RIGHT(n))) {
              UP(x) = p;
            }
            RIGHT(n) = p;
            UP(p) = n;
            if ((x = LEFT(g) = RIGHT(p))) {
              UP(x) = g;
            }
            RIGHT(p) = g;
            UP(g) = p;
          } else {
            if ((z = UP(g))) {
              if (g == LEFT(z)) {
                LEFT(z) = n;
              } else {
                RIGHT(z) = n;
              }
            }
            UP(n) = z;
            if ((x = LEFT(p) = RIGHT(n))) {
              RIGHT(n) = UP(x) = p;
            } else {
              RIGHT(n) = p;
            }
            if ((x = RIGHT(g) = LEFT(n))) {
              LEFT(n) = UP(x) = g;
            } else {
              LEFT(n) = g;
            }
            UP(g) = UP(p) = n;
          }
        } else {
          if (!((g = UPUP(n)))) {
            ROTATE_L(n, p, g);
          } else if (p == RIGHT(g)) {
            if ((z = UP(g))) {
              if (g == RIGHT(z)) {
                RIGHT(z) = n;
              } else {
                LEFT(z) = n;
              }
            }
            UP(n) = z;
            if ((x = RIGHT(p) = LEFT(n))) {
              UP(x) = p;
            }
            LEFT(n) = p;
            UP(p) = n;
            if ((x = RIGHT(g) = LEFT(p))) {
              UP(x) = g;
            }
            LEFT(p) = g;
            UP(g) = p;
          } else {
            if ((z = UP(g))) {
              if (g == RIGHT(z)) {
                RIGHT(z) = n;
              } else {
                LEFT(z) = n;
              }
            }
            UP(n) = z;
            if ((x = RIGHT(p) = LEFT(n))) {
              LEFT(n) = UP(x) = p;
            } else {
              LEFT(n) = p;
            }
            if ((x = LEFT(g) = RIGHT(n))) {
              RIGHT(n) = UP(x) = g;
            } else {
              RIGHT(n) = g;
            }
            UP(g) = UP(p) = n;
          }
        }
      }
    }

    // NOTE: This does an in-order travesal which will result in a linear tree
    // on the other side. Should use a better traversal order.
    void flatten_tree(Event* e, int& idx) {
      if (LEFT(e)) {
        flatten_tree(LEFT(e), idx);
      }
      if (e) {
        e->index = idx;
        temp_event_buffer[idx++] = e;
      }
      if (RIGHT(e)) {
        flatten_tree(RIGHT(e), idx);
      }
    }

  public:
    PendingSplay() {
      root = least = NULL;
      nitems = max_size = 0;
    }

    ~PendingSplay() {
      while (nitems) {
        Event* e = pop();
        tw_event_free(e,false);
      }
    }

    virtual void pup(PUP::er &p) {
      p | nitems;
      p | max_size;

      int temp_items = nitems;
      // If sizing, the temp_event_buffer is freed after packing.
      if (p.isSizing()) {
        int idx = 0;
        if (temp_items) {
          temp_event_buffer = new Event*[temp_items];
          flatten_tree(root, idx);
        }
      }
      // If unpacking, the temp_event_buffer is freed by the LP after
      // reconstructing all of the causality chains.
      if (p.isUnpacking()) {
        temp_event_buffer = new Event*[temp_items];
        nitems = 0;
      }

      Event* e;
      for (int i = 0; i < temp_items; i++) {
        if (p.isUnpacking()) {
          e = charm_allocate_event();
        } else {
          e = temp_event_buffer[i];
        }
        pup_pending_event(p, e);
        if (p.isUnpacking()) {
          temp_event_buffer[e->index] = e;
          push(e);
        }
      }

      if (p.isPacking()) {
        delete[] temp_event_buffer;
      }
    } 

    Event** get_temp_event_buffer() const {
      return temp_event_buffer;
    }

    void delete_temp_event_buffer() {
      delete[] temp_event_buffer;
    }
    
    void push(Event* e) {
      tw_event* n = root;

      e->state.owner = TW_chare_q;
      nitems++;

      if (nitems > max_size) {
        max_size = nitems;
      }

      RIGHT(e) = LEFT(e) = NULL;
      if (n) {
        for (;;) {
          if (KEY(n) <= KEY(e)) {
            if (RIGHT(n)) {
              n = RIGHT(n);
            } else {
              RIGHT(n) = e;
              UP(e) = n;
              break;
            }
          } else {
            if (LEFT(n)) {
              n = LEFT(n);
            } else {
              if (least == n) {
                least = e;
              }
              LEFT(n) = e;
              UP(e) = n;
              break;
            }
          }
        }
        splay(e);
        root = e;
      } else {
        root = least = e;
        UP(e) = LEFT(e) = RIGHT(e) = NULL;
      }
    }

    Event* pop() {
      tw_event       *r = least;
      tw_event       *tmp, *p;

      if (nitems == 0)
        return (tw_event *) NULL;


      nitems--;

      if ((p = UP(least))) {
        if ((tmp = RIGHT(least))) {
          LEFT(p) = tmp;
          UP(tmp) = p;
          for (; LEFT(tmp); tmp = LEFT(tmp));
          least = tmp;
        } else {
          least = UP(least);
          LEFT(least) = NULL;
        }
      } else {
        if ((root = RIGHT(least))) {
          UP(root) = NULL;
          for (tmp = root; LEFT(tmp); tmp = LEFT(tmp));
          least = tmp;
        } else {
          least = NULL;
        }
      }

      LEFT(r) = NULL;
      RIGHT(r) = NULL;
      UP(r) = NULL;

      r->state.owner = 0;
      return r;
    }

    void erase(Event* r) {
      TW_ASSERT(nitems > 0, "Attempting to delete from empty queue\n");
      TW_ASSERT(r->state.owner == TW_chare_q, "Bad event owner\n");
      tw_event       *n, *p;
      tw_event       *tmp;

      if (r == least) {
        pop();
        return;
      }

      r->state.owner = 0;
      nitems--;
      if ((n = LEFT(r))) {
        if ((tmp = RIGHT(r))) {
          UP(n) = NULL;
          for (; RIGHT(n); n = RIGHT(n));
          splay(n);
          RIGHT(n) = tmp;
          UP(tmp) = n;
        }
        UP(n) = UP(r);
      } else if ((n = RIGHT(r))) {
        UP(n) = UP(r);
      }

      if ((p = UP(r))) {
        if (r == LEFT(p)) {
          LEFT(p) = n;
        } else {
          RIGHT(p) = n;
        }
        if (n) {
          splay(p);
          root = p;
        }
      } else {
        root = n;
      }
      LEFT(r) = NULL;
      RIGHT(r) = NULL;
      UP(r) = NULL;
    }

    tw_event* top() const {
      return least;
    }

    tw_stime min() const {
      return (least ? least->ts : DBL_MAX);
    }
    size_t size() const {
      return nitems;
    }
};

#endif
