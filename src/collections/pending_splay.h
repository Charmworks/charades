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

#include "pending_queue.h"

#include "ross_event.h"
#include "ross_util.h"

#include "event.h"

#include <float.h>

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

  public:
    PendingSplay() {
      root = least = NULL;
      nitems = max_size = 0;
    }

    virtual void pup(PUP::er &p) {
      p | nitems;
      p | max_size;


      int temp_items = nitems;
      if (p.isUnpacking()) {
        temp_event_buffer = new Event*[temp_items];
        nitems = 0;
      }

      // When packing: pop each event off the tree, record seq_num, and pup.
      // When unpacking: Allocate a new event (with RemoteEvent), pup, push
      // the event onto the tree, and add it to the correct place in the temp
      // buffer (making sure they are coming off in the same order).
      for (int i = 0; i < temp_items; i++) {
        Event* e;
        // TODO: This sizing clause is only temporary, need better soln.
        if (p.isSizing()) {
          e = top();
        } else if (p.isPacking()) {
          e = pop();
          e->seq_num = i;
        } else if (p.isUnpacking()) {
          e = tw_event_new(0,0,0);
        } 
        p | e;
        if (p.isUnpacking()) {
          if (i != e->seq_num) {
            tw_error(TW_LOC, "seq_num mismatch while unpacking splay.\n");
          }
          temp_event_buffer[e->seq_num] = e;
          push(e);
        }
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

	    nitems++;
	    if (nitems > max_size)
		    max_size = nitems;

	    e->state.owner = TW_pe_pq;

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
		    UP(e) = NULL;
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
	    tw_event       *n, *p;
	    tw_event       *tmp;

	    r->state.owner = 0;

	    if (r == least) {
		    pop();
		    return;
	    }

	    if (nitems == 0) {
		    tw_error(TW_LOC,
				    "tw_pq_delete_any: attempt to delete from empty queue \n");
	    }

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
