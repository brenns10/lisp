/***************************************************************************//**

  @file         functions.c

  @author       Stephen Brennan

  @date         Created Thursday, 22 October 2015

  @brief        Builtin functions of the lisp interpreter.

  @copyright    Copyright (c) 2015, Stephen Brennan.  Released under the Revised
                BSD License.  See LICENSE.txt for details.

*******************************************************************************/

#include <wchar.h>
#include <stdarg.h>
#include <string.h>

#include "libstephen/ht.h"
#include "lisp.h"

bool lisp_truthy(lisp_value *expr)
{
  return (expr->type == &tp_int) && (((lisp_int*)expr)->value != 0);
}

static lisp_type *get_type(char code) {
  switch (code) {
  case 'd':
    return &tp_int;
  case 'l':
    return &tp_list;
  case 'a':
    return &tp_atom;
  case 'i':
    return &tp_identifier;
  case 'b':
    return &tp_builtin;
  case 'c':
    return &tp_funccall;
  default:
    return NULL;
  }
}

static void get_args(char *fname, lisp_list *args, char *format, ...)
{
  va_list va;
  int nargs;
  int nexp;
  lisp_value **v;
  lisp_type *expected_type;
  va_start(va, format);

  nargs = lisp_list_length(args);
  nexp = strlen(format);
  if (nargs != nexp) {
    fprintf(stderr, "%s: wrong number of args (expected %d, got %d)\n",
            fname, nexp, nargs);
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < nargs; i++) {
    v = va_arg(va, lisp_value**);
    expected_type = get_type(format[i]);
    if (expected_type != NULL && expected_type != args->value->type) {
      fprintf(stderr, "%s: argument %d: expected type %s, got type %s\n",
              fname, i, expected_type->tp_name, args->value->type->tp_name);
      exit(EXIT_FAILURE);
    }
    *v = args->value;
    args = args->next;
  }

  va_end(va);
}

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
static lisp_value *lisp_add(lisp_list *params, lisp_scope *scope)
{
  (void)scope; //unused
  lisp_int *rv = (lisp_int*)tp_int.tp_alloc();
  while (params->value) {
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
static lisp_value *lisp_length(lisp_list *params, lisp_scope *scope)
{
  (void)scope; //unused
  lisp_list *l;
  lisp_int *i;

  get_args("length", params, "l", &l);

  i = (lisp_int*)tp_int.tp_alloc();
  i->value = lisp_list_length(l);
  return (lisp_value*)i;
}

/**
   @brief Subtract some number of values form the first one.
 */
static lisp_value *lisp_subtract(lisp_list *params, lisp_scope *scope)
{
  (void)scope; //unused
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

static lisp_value *lisp_car(lisp_list *params, lisp_scope *scope)
{
  (void)scope; //unused
  lisp_list *l;
  get_args("car", params, "l", &l);

  if (l->value == NULL) {
    fprintf(stderr, "lisp_car(): car of empty list\n");
    exit(EXIT_FAILURE);
  }

  lisp_incref(l->value);
  return l->value;
}

static lisp_value *lisp_cdr(lisp_list *params, lisp_scope *scope)
{
  (void)scope; //unused
  lisp_list *l;
  get_args("cdr", params, "l", &l);

  if (l == NULL) {
    return NULL;
  } else {
    lisp_incref((lisp_value*)l->next);
    return (lisp_value*)l->next;
  }
}

static lisp_value *lisp_cons(lisp_list *params, lisp_scope *scope)
{
  (void)scope; //unused
  lisp_value *v;
  lisp_list *old_list;
  lisp_list *new_list;

  get_args("cons", params, "?l", &v, &old_list);

  new_list = (lisp_list*)tp_list.tp_alloc();
  new_list->value = v;
  new_list->next = old_list;
  lisp_incref(v);
  lisp_incref((lisp_value*)old_list);
  return (lisp_value*)new_list;
}

static lisp_value *lisp_exit(lisp_list *params, lisp_scope *scope)
{
  (void)scope; //unused
  (void)params; // unused
  lisp_value *rv;
  lisp_interactive_exit = true;
  if (params->value != NULL) {
    rv = params->value;
    lisp_incref(rv);
  } else {
    rv = (lisp_value*)tp_int.tp_alloc();
  }
  return rv;
}

static lisp_value *lisp_if(lisp_list *params, lisp_scope *scope)
{
  lisp_value *condition, *if_true, *if_false;
  get_args("if", params, "???", &condition, &if_true, &if_false);

  condition = lisp_evaluate(condition, scope);
  if (lisp_truthy(condition)) {
    return lisp_evaluate(if_true, scope);
  } else {
    return lisp_evaluate(if_false, scope);
  }
}

static lisp_value *lisp_lambda(lisp_list *params, lisp_scope *scope)
{
  (void)scope; //unused (for now)
  lisp_value *arglist;
  lisp_value *expression;
  lisp_list *list;
  lisp_function *function = (lisp_function*)tp_function.tp_alloc();
  get_args("lambda", params, "??", &arglist, &expression);

  // The argument list will show up as a func call when there are any arguments,
  // but it will be an empty list if there aren't.
  if (arglist->type == &tp_list) {
    lisp_incref(arglist);
    function->arglist = (lisp_list*) arglist;
  } else if (arglist->type == &tp_funccall) {
    list = (lisp_list*)tp_list.tp_alloc();
    list->value = ((lisp_funccall*)arglist)->function;
    list->next = ((lisp_funccall*)arglist)->arguments;
    lisp_incref(list->value);
    lisp_incref((lisp_value*)list->next);
    function->arglist = list;
  }
  lisp_incref(expression);
  function->code = expression;
  return (lisp_value*)function;
}

static lisp_value *lisp_define(lisp_list *params, lisp_scope *scope)
{
  lisp_identifier *name;
  lisp_value *value;
  get_args("define", params, "i?", &name, &value);
  value = lisp_evaluate(value, scope);
  lisp_incref(value); // one reference belongs to the table
  lisp_incref((lisp_value*)name); // one reference belongs to the table (but never leaves...)
  ht_insert(&scope->table, PTR(name->value), PTR(value));
  return value;
}

static lisp_value *lisp_numeq(lisp_list *params, lisp_scope *scope)
{
  (void)scope; // unused
  lisp_int *a, *b, *retval;
  get_args("=", params, "dd", &a, &b);
  retval = (lisp_int *)tp_int.tp_alloc();
  retval->value = (a->value == b->value);
  return (lisp_value *)retval;
}

static lisp_value *lisp_numlt(lisp_list *params, lisp_scope *scope)
{
  (void)scope; // unused
  lisp_int *a, *b, *retval;
  get_args("=", params, "dd", &a, &b);
  retval = (lisp_int *)tp_int.tp_alloc();
  retval->value = (a->value < b->value);
  return (lisp_value *)retval;
}

static lisp_value *lisp_numgt(lisp_list *params, lisp_scope *scope)
{
  (void)scope; // unused
  lisp_int *a, *b, *retval;
  get_args("=", params, "dd", &a, &b);
  retval = (lisp_int *)tp_int.tp_alloc();
  retval->value = (a->value > b->value);
  return (lisp_value *)retval;
}

static lisp_value *lisp_numle(lisp_list *params, lisp_scope *scope)
{
  (void)scope; // unused
  lisp_int *a, *b, *retval;
  get_args("=", params, "dd", &a, &b);
  retval = (lisp_int *)tp_int.tp_alloc();
  retval->value = (a->value <= b->value);
  return (lisp_value *)retval;
}

static lisp_value *lisp_numge(lisp_list *params, lisp_scope *scope)
{
  (void)scope; // unused
  lisp_int *a, *b, *retval;
  get_args("=", params, "dd", &a, &b);
  retval = (lisp_int *)tp_int.tp_alloc();
  retval->value = (a->value >= b->value);
  return (lisp_value *)retval;
}

static lisp_value *lisp_null_p(lisp_list *params, lisp_scope *scope)
{
  (void)scope; // unused
  lisp_value *v;
  lisp_list *l;
  lisp_int *retval;
  get_args("null?", params, "?", &v);
  retval = (lisp_int*) tp_int.tp_alloc();

  if (v->type == &tp_list) {
    l = (lisp_list *) v;
    retval->value = (l->value == NULL);
  } else {
    retval->value = 0;
  }
  return (lisp_value *)retval;
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

  bi = (lisp_builtin*)tp_builtin.tp_alloc();
  bi->function = &lisp_cdr;
  ht_insert(&scope->table, PTR(L"cdr"), PTR(bi));

  bi = (lisp_builtin*)tp_builtin.tp_alloc();
  bi->function = &lisp_cons;
  ht_insert(&scope->table, PTR(L"cons"), PTR(bi));

  bi = (lisp_builtin*)tp_builtin.tp_alloc();
  bi->function = &lisp_exit;
  ht_insert(&scope->table, PTR(L"exit"), PTR(bi));

  bi = (lisp_builtin*)tp_builtin.tp_alloc();
  bi->function = &lisp_numeq;
  ht_insert(&scope->table, PTR(L"="), PTR(bi));

  bi = (lisp_builtin*)tp_builtin.tp_alloc();
  bi->function = &lisp_numlt;
  ht_insert(&scope->table, PTR(L"<"), PTR(bi));

  bi = (lisp_builtin*)tp_builtin.tp_alloc();
  bi->function = &lisp_numgt;
  ht_insert(&scope->table, PTR(L">"), PTR(bi));

  bi = (lisp_builtin*)tp_builtin.tp_alloc();
  bi->function = &lisp_numle;
  ht_insert(&scope->table, PTR(L"<="), PTR(bi));

  bi = (lisp_builtin*)tp_builtin.tp_alloc();
  bi->function = &lisp_numge;
  ht_insert(&scope->table, PTR(L">="), PTR(bi));

  bi = (lisp_builtin*)tp_builtin.tp_alloc();
  bi->function = &lisp_null_p;
  ht_insert(&scope->table, PTR(L"null?"), PTR(bi));

  bi = (lisp_builtin*)tp_builtin.tp_alloc();
  bi->function = &lisp_if;
  bi->eval = false;
  ht_insert(&scope->table, PTR(L"if"), PTR(bi));

  bi = (lisp_builtin*)tp_builtin.tp_alloc();
  bi->function = &lisp_lambda;
  bi->eval = false;
  ht_insert(&scope->table, PTR(L"lambda"), PTR(bi));

  bi = (lisp_builtin*)tp_builtin.tp_alloc();
  bi->function = &lisp_define;
  bi->eval = false;
  ht_insert(&scope->table, PTR(L"define"), PTR(bi));

  return scope;
}
