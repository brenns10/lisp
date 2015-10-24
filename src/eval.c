/***************************************************************************//**

  @file         eval.c

  @author       Stephen Brennan

  @date         Created Wednesday, 21 October 2015

  @brief        Lisp evaluation routines. maybe.

  @copyright    Copyright (c) 2015, Stephen Brennan.  Released under the Revised
                BSD License.  See LICENSE.txt for details.

*******************************************************************************/

#include <assert.h>

#include "libstephen/str.h"
#include "libstephen/ht.h"
#include "lex.h"
#include "lisp.h"

bool lisp_interactive_exit = false;

// forward-declaration
lisp_value *lisp_evaluate(lisp_value *expression, lisp_scope *scope);
static lisp_list *lisp_evaluate_list(lisp_list *list, lisp_scope *scope);

static void add_to_scope(lisp_list *names, lisp_list *values, lisp_scope *scope)
{
  lisp_identifier *id;
  while (names->value != NULL && values->value != NULL) {
    id = (lisp_identifier*) names->value;
    lisp_incref(values->value);
    ht_insert(&scope->table, PTR(id->value), PTR(values->value));
    names = names->next; values = values->next;
  }
}

static lisp_value *lisp_evaluate_funccall(lisp_value *expression,
                                          lisp_scope *scope)
{
  lisp_builtin *bi;
  lisp_function *f;
  lisp_funccall *call;
  lisp_value *rv, *func;
  lisp_list *args;
  lisp_scope *new_scope;

  call = (lisp_funccall*) expression;
  func = lisp_evaluate((lisp_value*)call->function, scope);

  if (func->type == &tp_builtin) {
    // Calling builtin functions involves calling their function pointer.
    bi = (lisp_builtin*) func;

    // Builtins do things more powerful than normal functions, and thus they can
    // request that their arguments not be evaluated.  This is important for
    // implementing things like if, cond, etc.
    if (bi->eval) {
      // Evaluate arguments beforehand.
      args = lisp_evaluate_list(call->arguments, scope);
      rv = bi->function(args, scope);
      lisp_decref((lisp_value*)args);
    } else {
      // Don't evaluate arguments.
      args = call->arguments;
      rv = bi->function(args, scope);
    }
  } else if(func->type == &tp_function) {
    f = (lisp_function*) func;
    args = lisp_evaluate_list(call->arguments, scope);
    new_scope = lisp_scope_create();
    new_scope->up = scope;
    add_to_scope(f->arglist, args, new_scope);
    rv = lisp_evaluate((lisp_value*)f->code, new_scope);
    lisp_scope_delete(new_scope);
    lisp_decref((lisp_value*)args);
  } else {
    printf("error in evaluation\n");
  }
  lisp_decref(func);
  return rv;
}

/**
   @brief Return a list containing each item in a list, evaluated.
   @param list List of items to evaluate.
   @param scope Scope to evaluate each list item within.
   @returns A list of the evaluated items!
 */
static lisp_list *lisp_evaluate_list(lisp_list *list, lisp_scope *scope)
{
  if (list->value == NULL) {
    return (lisp_list*)tp_list.tp_alloc();
  }
  lisp_list *l = (lisp_list*) tp_list.tp_alloc();
  l->value = lisp_evaluate(list->value, scope);
  l->next = lisp_evaluate_list(list->next, scope);
  return l;
}

/**
   @brief Return the value of a piece of lisp code!
   @param expression The code to evaluate.
   @param scope The scope to evaluate the code within.
 */
lisp_value *lisp_evaluate(lisp_value *expression, lisp_scope *scope)
{
  smb_status st = SMB_SUCCESS;
  lisp_value *rv;
  lisp_identifier *id;

  if (expression->type == &tp_int ||
      expression->type == &tp_atom ||
      expression->type == &tp_list ||
      expression->type == &tp_builtin ||
      expression->type == &tp_function) {
    lisp_incref(expression);
    rv = expression;
  } else if (expression->type == &tp_funccall) {
    rv = lisp_evaluate_funccall(expression, scope);
  } else {
    id = (lisp_identifier*)expression;
    while (scope) {
      rv = ht_get(&scope->table, PTR(id->value), &st).data_ptr;
      if (st == SMB_SUCCESS) {
        lisp_incref(rv); // we are returning a new reference not owned by scope
        return rv;
      }
      st = SMB_SUCCESS;
      scope = scope->up;
    }
    fprintf(stderr, "lisp: definition of identifier \"%ls\" not found\n",
            id->value);
    exit(EXIT_FAILURE);
  }
  return rv;
}

lisp_value *lisp_run(wchar_t *str)
{
  // given string, return list of tokens (with strings if necessary)
  smb_ll *tokens = lisp_lex(str);
  // then, parse it to a (list of) lisp_value
  smb_iter it = ll_get_iter(tokens);
  lisp_value *code = lisp_parse(&it);
  // then, evaluate that
  lisp_scope *scope = lisp_create_globals();
  lisp_value *res = lisp_evaluate(code, scope);
  res->type->tp_print(res, stdout, 0);
  lisp_decref(code);
  lisp_scope_delete(scope);
  ll_delete(tokens);
  return res;
}

void lisp_interact(void)
{
  // Create an iterator of lisp tokens taken from stdin.
  smb_iter token_iter = lisp_lex_file(stdin);
  lisp_scope *scope = lisp_create_globals();
  lisp_interactive_exit = false;

  // While there are still tokens remaining...
  while (token_iter.has_next(&token_iter) && !lisp_interactive_exit) {
    printf("> ");
    fflush(stdout);

    lisp_value *code = lisp_parse(&token_iter);
    lisp_value *res = lisp_evaluate(code, scope);
    res->type->tp_print(res, stdout, 0);

    lisp_decref(code);
    lisp_decref(res);
  }

  token_iter.destroy(&token_iter);
  lisp_scope_delete(scope);
}
