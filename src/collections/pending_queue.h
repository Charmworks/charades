/**********************************************************************
 * Additional Contributions and Acknowledgements
 *   Kalyan Perumalla - Ga Tech
 *
 *   This implementation is an adaption of the implementation done
 *   by Kalyan for Ga Tech Time Warp
 **********************************************************************/

#ifndef _PENDING_QUEUE_
#define _PENDING_QUEUE_

#include "typedefs.h"

#include "event.h"

typedef Event *ELEMENT_TYPE;
typedef double KEY_TYPE;
#define KEY(e) (e->ts)

#define SWAP(x,y,t) { \
    t = elems[x]; \
    elems[x] = elems[y]; \
    elems[y] = t; \
    elems[x]->heap_index = x; \
    elems[y]->heap_index = y; \
    }

class PendingQueue
{
  size_t nelems;
  size_t curr_max;
  ELEMENT_TYPE *elems; /* Array [0..curr_max] of ELEMENT_TYPE */

  public:

  PendingQueue() {
    PendingQueue(50000);
  }

  PendingQueue(int init_size) {
    nelems = 0;
    curr_max = (2*init_size);
    elems = NULL;
    elems = (ELEMENT_TYPE*)realloc(elems, sizeof(ELEMENT_TYPE) * curr_max);
    memset(elems, 0, sizeof(ELEMENT_TYPE) * curr_max);
  }

  ELEMENT_TYPE top() {
    return (nelems <= 0) ? NULL : elems[0];
  }

  size_t size() {
    return nelems;
  }

  void sift_down( int i ) {
    size_t n = nelems, k = i, j, c1, c2;
    ELEMENT_TYPE temp;

    if( n <= 1 ) return;

    /* Stops when neither child is "strictly less than" parent */
    do{
      j = k;
      c1 = c2 = 2*k+1;
      c2++;
      if( c1 < n && KEY(elems[c1]) < KEY(elems[k]) ) k = c1;
      if( c2 < n && KEY(elems[c2]) < KEY(elems[k]) ) k = c2;
      SWAP( j, k, temp );
    }while( j != k );
  }

  void percolate_up( int i )
  {
    int n = nelems, k = i, j, p;
    ELEMENT_TYPE temp;

    if( n <= 1 ) return;

    /* Stops when parent is "less than or equal to" child */
    do
    {
      j = k;
      if( (p = (k+1)/2) )
      {
        --p;
        if( KEY(elems[k]) < KEY(elems[p]) ) k = p;
      }
      SWAP( j, k, temp );
    }while( j != k );
  }

  void push( ELEMENT_TYPE e )
  {
    if( nelems >= curr_max )
    {
      size_t i = 50000;
      size_t u = curr_max;
      curr_max += i;
      elems = (ELEMENT_TYPE*)realloc(elems, sizeof(ELEMENT_TYPE) * curr_max);
      memset(&elems[u], 0, sizeof(ELEMENT_TYPE) * i);
    }

    e->heap_index = nelems;
    elems[nelems++] = e;
    percolate_up( nelems-1 );

    e->state.owner = TW_pe_pq;
  }

  ELEMENT_TYPE pop()
  {
    if( nelems <= 0 )
      return NULL;
    else
    {
      ELEMENT_TYPE e = elems[0];
      elems[0] = elems[--nelems];
      elems[0]->heap_index = 0;
      sift_down( 0 );
      e->state.owner = 0;
      return e;
    }
  }

  void erase( tw_event * victim)
  {
    int i = victim->heap_index;

    if( !(0 <= i && i < nelems) || (elems[i]->heap_index != i) )
    {
      fprintf( stderr, "Fatal: Bad node in FEL!\n" ); exit(2);
    }
    else
    {
      nelems--;
      victim->state.owner = 0;

      if( elems > 0 )
      {
        ELEMENT_TYPE successor = elems[nelems];
        elems[i] = successor;
        successor->heap_index = i;
        if( KEY(successor) <= KEY(victim) ) percolate_up( i );
        else sift_down( i );
      }
    }
  }

};
#endif

