/* ----------------------------------------------------------------------------- 
 * list.c
 *
 *     Implements a simple list object.
 * 
 * Author(s) : David Beazley (beazley@cs.uchicago.edu)
 *
 * Copyright (C) 1999-2000.  The University of Chicago
 * See the file LICENSE for information on usage and redistribution.	
 * ----------------------------------------------------------------------------- */

static char cvsroot[] = "$Header$";

#include "dohint.h"

typedef struct List {
    int          maxitems;        /* Max size  */
    int          nitems;          /* Num items */
    int          iter;            /* Iterator  */
    DOH         *file;
    int          line;
    DOH        **items;
} List;

extern DohObjInfo DohListType;

/* Doubles amount of memory in a list */
static 
void more(List *l) {
    l->items = (void **) DohRealloc(l->items, l->maxitems*2*sizeof(void *));
    assert(l->items);
    l->maxitems *= 2;
}

/* -----------------------------------------------------------------------------
 * CopyList()
 *
 * Make a shallow copy of a list.
 * ----------------------------------------------------------------------------- */
static DOH *
CopyList(DOH *lo) {
    List *l,*nl;
    int i;
    l = (List *) ObjData(lo);
    nl = (List *) DohMalloc(sizeof(List));
    nl->nitems = l->nitems;
    nl->maxitems = l->maxitems;
    nl->items = (void **) DohMalloc(l->maxitems*sizeof(void *));
    nl->iter = 0;
    for (i = 0; i < l->nitems; i++) {
	nl->items[i] = l->items[i];
	Incref(nl->items[i]);
    }
    nl->file = l->file;
    if (nl->file) Incref(nl->file);
    nl->line = l->line;
    return DohObjMalloc(&DohListType, nl);
}

/* -----------------------------------------------------------------------------
 * DelList()
 *
 * Delete a list.
 * ----------------------------------------------------------------------------- */

static void
DelList(DOH *lo) {
    List *l = (List *) ObjData(lo);
    int i;
    for (i = 0; i < l->nitems; i++)
	Delete(l->items[i]);
    DohFree(l->items);
    DohFree(l);
}

/* -----------------------------------------------------------------------------
 * List_clear()
 *
 * Remove all of the list entries, but keep the list object intact.
 * ----------------------------------------------------------------------------- */

static void
List_clear(DOH *lo) {
    List *l = (List *) ObjData(lo);
    int i;
    for (i = 0; i < l->nitems; i++) {
	Delete(l->items[i]);
    }
    l->nitems = 0;
}

/* -----------------------------------------------------------------------------
 * List_insert()
 *
 * Insert an item into the list. If the item is not a DOH object, it is assumed
 * to be a 'char *' and is used to construct an equivalent string object.
 * ----------------------------------------------------------------------------- */

static int 
List_insert(DOH *lo, int pos, DOH *item) {
  List *l = (List *) ObjData(lo);
  int i;
  
  if (!item) return -1;
  if (!DohCheck(item)) {
    item = NewString(item);
    Decref(item);
  }
  if (pos == DOH_END) pos = l->nitems;
  if (pos < 0) pos = 0;
  if (pos > l->nitems) pos = l->nitems;
  if (l->nitems == l->maxitems) more(l);
  for (i = l->nitems; i > pos; i--) {
    l->items[i] = l->items[i-1];
  }
  l->items[pos] = item;
  Incref(item);
  l->nitems++;
  return 0;
}

/* -----------------------------------------------------------------------------
 * List_remove()
 *
 * Remove an item from a list.
 * ----------------------------------------------------------------------------- */
    
static int
List_remove(DOH *lo, int pos) {
    List *l = (List *) ObjData(lo);
    int   i;
    if (pos == DOH_END) pos = l->nitems-1;
    if (pos == DOH_BEGIN) pos = 0;
    assert(!((pos < 0) || (pos >= l->nitems)));
    Delete(l->items[pos]);
    for (i = pos; i < l->nitems-1; i++) {
	l->items[i] = l->items[i+1];
    }
    l->nitems--;
    return 0;
}

/* -----------------------------------------------------------------------------
 * List_len()
 *
 * Return the number of elements in the list
 * ----------------------------------------------------------------------------- */

static int 
List_len(DOH *lo) {
  List *l = (List *) ObjData(lo);
  return l->nitems;
}

/* -----------------------------------------------------------------------------
 * List_get()
 *
 * Get the nth item from the list.
 * ----------------------------------------------------------------------------- */

static DOH *
List_get(DOH *lo, int n) {
    List *l = (List *) ObjData(lo);
    if (n == DOH_END) n = l->nitems-1;
    if (n == DOH_BEGIN) n = 0;
    assert(!((n < 0) || (n >= l->nitems)));
    return l->items[n];
}

/* -----------------------------------------------------------------------------
 * List_set()
 *
 * Set the nth item in the list replacing any previous item. 
 * ----------------------------------------------------------------------------- */

static int
List_set(DOH *lo, int n, DOH *val) {
    List *l = (List *) ObjData(lo);
    if (!val) return -1;
    assert(!((n < 0) || (n >= l->nitems)));
    if (!DohCheck(val)) {
      val = NewString(val);
      Decref(val);
    }
    Delete(l->items[n]);
    l->items[n] = val;
    Incref(val);
    Delete(val);
    return 0;
}

/* -----------------------------------------------------------------------------
 * List_first()
 *
 * Return the first item in the list.
 * ----------------------------------------------------------------------------- */

static DOH *
List_first(DOH *lo) {
    List *l = (List *) ObjData(lo);
    l->iter = 0;
    if (l->iter >= l->nitems) return 0;
    return l->items[l->iter];
}

/* -----------------------------------------------------------------------------
 * List_next()
 * 
 * Return the next item in the list.
 * ----------------------------------------------------------------------------- */

static DOH *
List_next(DOH *lo) {
    List *l = (List *) ObjData(lo);
    l->iter++;
    if (l->iter >= l->nitems) return 0;
    return l->items[l->iter];
}

/* -----------------------------------------------------------------------------
 * List_str()
 *
 * Create a string representation of the list.
 * ----------------------------------------------------------------------------- */
static DOH *
List_str(DOH *lo) {
    DOH *s;
    int i;
    List *l = (List *) ObjData(lo);
    s = NewString("");
    if (ObjGetMark(lo)) {
      Printf(s,"List(%x)", lo);
      return s;
    }
    ObjSetMark(lo,1);
    Printf(s,"List[ ");
    for (i = 0; i < l->nitems; i++) {
      Printf(s, "%s", l->items[i]);
      if ((i+1) < l->nitems)
	Printf(s,", ");
    }
    Printf(s," ]\n");
    ObjSetMark(lo,0);
    return s;
}

/* -----------------------------------------------------------------------------
 * List_dump()
 *
 * Dump the items to an output stream.
 * ----------------------------------------------------------------------------- */

static int 
List_dump(DOH *lo, DOH *out) {
  int nsent = 0;
  int i,ret;
  List *l = (List *) ObjData(lo);
  for (i = 0; i < l->nitems; i++) {
    ret = Dump(l->items[i],out);
    if (ret < 0) return -1;
    nsent += ret;
  }
  return nsent;
}

static void 
List_setfile(DOH *lo, DOH *file) {
  DOH *fo;
  List *l = (List *) ObjData(lo);

  if (!DohCheck(file)) {
    fo = NewString(file);
    Decref(fo);
  } else fo = file;
  Incref(fo);
  Delete(l->file);
  l->file = fo;
}

static DOH *
List_getfile(DOH *lo) {
  List *l = (List *) ObjData(lo);
  return l->file;
}

static void 
List_setline(DOH *lo, int line) {
  List *l = (List *) ObjData(lo);
  l->line = line;
}

static int List_getline(DOH *lo) {
  List *l = (List *) ObjData(lo);
  return l->line;
}

static DohListMethods ListListMethods = {
  List_get,
  List_set,
  List_remove,
  List_insert,
  List_first,
  List_next,
};

DohObjInfo DohListType = {
    "List",          /* objname */
    DelList,         /* doh_del */
    CopyList,        /* doh_copy */
    List_clear,      /* doh_clear */
    List_str,        /* doh_str */
    0,               /* doh_data */
    List_dump,       /* doh_dump */
    List_len,        /* doh_len */
    0,               /* doh_hash    */
    0,               /* doh_cmp */
    List_setfile,               /* doh_setfile */
    List_getfile,               /* doh_getfile */
    List_setline,               /* doh_setline */
    List_getline,               /* doh_getline */
    0,               /* doh_mapping */
    &ListListMethods, /* doh_sequence */
    0,               /* doh_file */
    0,               /* doh_string */
    0,               /* doh_callable */
    0,               /* doh_position */         
};

/* -----------------------------------------------------------------------------
 * NewList()
 *
 * Create a new list.
 * ----------------------------------------------------------------------------- */

#define MAXLISTITEMS 8

DOH *
DohNewList() {
    List *l;
    int   i;
    l = (List *) DohMalloc(sizeof(List));
    l->nitems = 0;
    l->maxitems = MAXLISTITEMS;
    l->items = (void **) DohMalloc(l->maxitems*sizeof(void *));
    for (i = 0; i < MAXLISTITEMS; i++) {
	l->items[i] = 0;
    }
    l->iter = 0;
    l->file = 0;
    l->line = 0;
    return DohObjMalloc(&DohListType,l);
}

