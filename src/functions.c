/***************************************************************************//**

  @file         functions.c

  @author       Stephen Brennan

  @date         Created Thursday, 22 October 2015

  @brief        Builtin functions of the lisp interpreter.

  @copyright    Copyright (c) 2015, Stephen Brennan.  Released under the Revised
                BSD License.  See LICENSE.txt for details.

*******************************************************************************/

#include <wchar.h>

#include "libstephen/ht.h"
#include "lisp.h"

unsigned int wchar_hash(DATA data)
{
  wchar_t *wc = data.data_ptr;
  unsigned int hash = 0;

  while (wc && *wc != L'\0') {
    hash = (hash << 5) - hash + *wc;
    wc++;
  }

  return hash;
}

int data_compare_wstring(DATA d1, DATA d2)
{
  wchar_t *ws1, *ws2;
  ws1 = d1.data_ptr;
  ws2 = d2.data_ptr;
  return wcscmp(ws1, ws2);
}

/**
   @brief Add any number of values.
 */
static lisp_value *lisp_add(lisp_list *params)
{
  lisp_int *rv = (lisp_int*)tp_int.tp_alloc();
  while (params) {
    if (params->value->type != &tp_int) {
      fprintf(stderr, "lisp_add(): type error\n");
      exit(EXIT_FAILURE);
    }
    rv->value += ((lisp_int*)params->value)->value;
    params = params->next;
  }
  return (lisp_value *)rv;
}

/**
   @brief Return the length of a list.
 */
static lisp_value *lisp_length(lisp_list *params)
{
  lisp_int *i;
  if (params == NULL) {
    fprintf(stderr, "lisp_length(): too few arguments\n");
    exit(EXIT_FAILURE);
  }
  if (params->value->type != &tp_list) {
    fprintf(stderr, "lisp_length(): incorrect type argument\n");
    exit(EXIT_FAILURE);
  }

  i = (lisp_int*)tp_int.tp_alloc();
  i->value = lisp_list_length((lisp_list*)params->value);
  return (lisp_value*)i;
}

/**
   @brief Subtract some number of values form the first one.
 */
static lisp_value *lisp_subtract(lisp_list *params)
{
  lisp_int *rv;
  int len = lisp_list_length(params);

  if (len == 0) {
    fprintf(stderr, "lisp_subtract(): too few arguments\n");
    exit(EXIT_FAILURE);
  } else if (len == 1) {
    // negate the argument
    if (params->value->type != &tp_int) {
      fprintf(stderr, "lisp_subtract(): wrong type argument\n");
      exit(EXIT_FAILURE);
    }
    rv = (lisp_int*)tp_int.tp_alloc();
    rv->value = -((lisp_int*)params->value)->value;
  } else {
    if (params->value->type != &tp_int) {
      fprintf(stderr, "lisp_subtract(): wrong type argument\n");
      exit(EXIT_FAILURE);
    }
    rv = (lisp_int*)tp_int.tp_alloc();
    rv->value = ((lisp_int*)params->value)->value;

    params = params->next;
    while (params) {
      if (params->value->type != &tp_int) {
        fprintf(stderr, "lisp_subtract(): wrong type argument\n");
        exit(EXIT_FAILURE);
      }
      rv->value -= ((lisp_int*)params->value)->value;
      params = params->next;
    }
  }

  return (lisp_value*)rv;
}

static lisp_value *lisp_car(lisp_list *params)
{
  lisp_list *l;
  lisp_value *val;
  int len = lisp_list_length(params);

  if (len <= 0) {
    fprintf(stderr, "lisp_car(): too few arguments\n");
    exit(EXIT_FAILURE);
  } else if (len == 1) {
    if (params->value == NULL) {
      fprintf(stderr, "lisp_car(): car of empty list\n");
      exit(EXIT_FAILURE);
    }
    if (params->value->type != &tp_list) {
      fprintf(stderr, "lisp_car(): must provide list\n");
      exit(EXIT_FAILURE);
    }
    l = (lisp_list *)params->value;
    val = l->value;
    lisp_incref(val);
  } else {
    fprintf(stderr, "lisp_car(): too many arguments\n");
    exit(EXIT_FAILURE);
  }

  return val;
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

void *lisp_scope_delete(lisp_scope *scope)
{
  ht_destroy_act(&scope->table, &lisp_scope_values_decref);
  smb_free(scope);
}

/**
   @brief Return a scope containing the top-level variables for our lisp.
 */
lisp_scope *lisp_create_globals(void)
{
  lisp_scope *scope = lisp_scope_create();
  lisp_builtin *bi;

  bi = (lisp_builtin*)tp_builtin.tp_alloc();
  bi->function = &lisp_add;
  ht_insert(&scope->table, PTR(L"+"), PTR(bi));

  bi = (lisp_builtin*)tp_builtin.tp_alloc();
  bi->function = &lisp_subtract;
  ht_insert(&scope->table, PTR(L"-"), PTR(bi));

  bi = (lisp_builtin*)tp_builtin.tp_alloc();
  bi->function = &lisp_length;
  ht_insert(&scope->table, PTR(L"length"), PTR(bi));

  bi = (lisp_builtin*)tp_builtin.tp_alloc();
  bi->function = &lisp_car;
  ht_insert(&scope->table, PTR(L"car"), PTR(bi));

  return scope;
}
