/***************************************************************************//**

  @file         scope.c

  @author       Stephen Brennan

  @date         Created Friday, 30 October 2015

  @brief        Contains functions for manipulating scope objects.

  @copyright    Copyright (c) 2015, Stephen Brennan.  Released under the Revised
                BSD License.  See LICENSE.txt for details.

*******************************************************************************/

#include "wchar.h"

#include "lisp.h"

static unsigned int wchar_hash(DATA data)
{
  wchar_t *wc = data.data_ptr;
  unsigned int hash = 0;

  while (wc && *wc != L'\0') {
    hash = (hash << 5) - hash + *wc;
    wc++;
  }

  return hash;
}

static int data_compare_wstring(DATA d1, DATA d2)
{
  wchar_t *ws1, *ws2;
  ws1 = d1.data_ptr;
  ws2 = d2.data_ptr;
  return wcscmp(ws1, ws2);
}

lisp_scope *lisp_scope_create(void)
{
  lisp_scope *scope = smb_new(lisp_scope, 1);
  scope->up = NULL;
  ht_init(&scope->table, &wchar_hash, &data_compare_wstring);
  return scope;
}

static void lisp_scope_values_decref(DATA d)
{
  lisp_value *v = d.data_ptr;
  lisp_decref(v);
}

void lisp_scope_delete(lisp_scope *scope)
{
  ht_destroy_act(&scope->table, &lisp_scope_values_decref);
  smb_free(scope);
}
