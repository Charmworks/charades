/**********************************************************************
 * Additional Contributions and Acknowledgements
 *   Kalyan Perumalla - Ga Tech
 *
 *   This implementation is an adaption of the implementation done
 *   by Kalyan for Ga Tech Time Warp
 **********************************************************************/

#ifndef _PENDING_QUEUE_
#define _PENDING_QUEUE_

#include "pending_queue.h"
#include "typedefs.h"
#include "ross_event.h"
#include "event.h"
#include "charm_api.h"
#include "ross_util.h"
#include <malloc.h>

#include <float.h>

typedef Event *ELEMENT_TYPE;
typedef double KEY_TYPE;

#define KEY(i) (elems[i]->ts)
#define LEFT(i)   (2*i+1)
#define RIGHT(i)  (2*i+2)
#define PARENT(i) (((i-1)/2))

#define SWAP(x,y,t) { \
    t = elems[x]; \
    elems[x] = elems[y]; \
    elems[y] = t; \
    elems[x]->heap_index = x; \
    elems[y]->heap_index = y; \
    }

class PendingHeap : public PendingQueue {
  private:
    size_t  nelems;
    size_t  curr_max;
    Event** elems;

    void sift_down(int i) {
      if (nelems <= 1) return;

      size_t parent, left, right, temp_idx = i;
      Event* temp;

      /* Stops when neither child is "strictly less than" parent */
      do {
        parent = temp_idx;
        left = LEFT(parent);
        right = RIGHT(parent);
        if (left  < nelems && KEY(left)  < KEY(temp_idx)) temp_idx = left;
        if (right < nelems && KEY(right) < KEY(temp_idx)) temp_idx = right;
        if (parent != temp_idx) SWAP(parent, temp_idx, temp);
      } while (parent != temp_idx);
    }

    void percolate_up( int i ) {
      if (nelems <= 1) return;

      int child, parent, temp_idx = i;
      Event* temp;

      /* Stops when parent is "less than or equal to" child */
      do {
        child = temp_idx;
        parent = PARENT(child);
        if (parent >= 0 && KEY(parent) > KEY(temp_idx)) temp_idx = parent;
        if (child != temp_idx) SWAP(child, temp_idx, temp);
      } while (child != temp_idx);
    }

  public:
    PendingHeap() {
      int init_size = 50000;
      nelems = 0;
      curr_max = (2*init_size);
      int err = posix_memalign((void**)&elems, 64, sizeof(Event**)*curr_max);
      memset(elems, 0, sizeof(Event**) * curr_max);
    }

    PendingHeap(int init_size) {
      nelems = 0;
      curr_max = (2*init_size);
      int err = posix_memalign((void**)&elems, 64, sizeof(Event**)*curr_max);
      memset(elems, 0, sizeof(Event**) * curr_max);
    }

    ~PendingHeap() {
      for (int i = 0; i < nelems; i++) {
        tw_event_free(elems[i]);
      }
      delete[] elems;
    }

    virtual void pup(PUP::er& p) {
      p | nelems;
      p | curr_max;
      if (p.isUnpacking()) {
        int err = posix_memalign((void**)&elems, 64, sizeof(Event**)*curr_max);
        memset(elems, 0, sizeof(Event**) * curr_max);
      }
      for (int i = 0; i < nelems; i++) {
        if (p.isSizing()) {
          elems[i]->seq_num = i;
        } else if (p.isUnpacking()) {
          elems[i] = charm_allocate_event();
        }
        pup_pending_event(p, elems[i]);
      }
    }

    Event** get_temp_event_buffer() {
      return elems;
    }

    void delete_temp_event_buffer() {
    }

    tw_stime min() const {
      return (nelems <= 0) ? DBL_MAX : elems[0]->ts;
    }

    size_t size() const {
      return nelems;
    }

    void push(Event* e) {
      if (nelems >= curr_max) {
        size_t old_max = curr_max;
        curr_max += 50000;
        Event** old = elems;
        int err = posix_memalign((void**)&elems, 64, sizeof(Event**)*curr_max);
        memcpy(elems, old, sizeof(Event**) * old_max);
        memset(&elems[old_max], 0, sizeof(Event**) * 50000);
        free(old);
      }

      e->state.owner = TW_chare_q;
      e->heap_index = nelems;

      elems[nelems++] = e;
      percolate_up(nelems-1);
    }

    Event* pop() {
      if (nelems <= 0) {
        return NULL;
      } else {
        Event* e = elems[0];
        e->state.owner = 0;

        nelems--;
        elems[0] = elems[nelems];
        elems[0]->heap_index = 0;
        elems[nelems] = NULL;
        sift_down(0);

        return e;
      }
    }

    void erase(Event* victim) {
      if (nelems == 0) {
        tw_error(TW_LOC, "Can't erase an event from an empty heap\n");
      }
      int i = victim->heap_index;

      if(i < 0 || i >= nelems || elems[i]->heap_index != i) {
        tw_error(TW_LOC, "ERROR: Can't erase event from pending heap\n");
      } else {
        nelems--;

        if (i == nelems) {
          elems[nelems] = NULL;
          return;
        } else if (elems > 0) {
          elems[i] = elems[nelems];
          elems[i]->heap_index = i;
          elems[nelems] = NULL;

          if (elems[i]->ts <= victim->ts) {
            percolate_up(i);
          } else {
            sift_down(i);
          }
        }
      }
    }
};
#endif

