/***************************************************************************//**

  @file         types.c

  @author       Stephen Brennan

  @date         Created Thursday, 22 October 2015

  @brief        Type objects for all standard lisp types.

  @copyright    Copyright (c) 2015, Stephen Brennan.  Released under the Revised
                BSD License.  See LICENSE.txt for details.

*******************************************************************************/

#include "libstephen/base.h"
#include "lisp.h"

/*******************************************************************************
                      Major Garbage Collection Functions!
*******************************************************************************/

void lisp_incref(lisp_value *lv)
{
  if (lv == NULL) return;
  lv->refcount += 1;
}

void lisp_decref(lisp_value *lv)
{
  if (lv == NULL) return;
  lv->refcount -= 1;
  if (lv->refcount == 0) {
    lv->type->tp_dealloc(lv);
  }
}

/*******************************************************************************
                                Private Helpers
*******************************************************************************/

static void print_n_spaces(FILE *f, int nspaces)
{
  while (nspaces--) {
    fputc(' ', f);
  }
}

static void generic_dealloc(lisp_value *lv)
{
  smb_free(lv);
}

/*******************************************************************************
                               tp_int / lisp_int
*******************************************************************************/

static lisp_value *lisp_int_alloc(void)
{
  lisp_int *rv = smb_new(lisp_int, 1);
  rv->lv.type = &tp_int;
  rv->lv.refcount = 1;
  rv->value = 0;
  return (lisp_value *)rv;
}

static void lisp_int_print(lisp_value *value, FILE *f, int indent)
{
  lisp_int *val = (lisp_int *)value;
  fprintf(f, "%ld\n", val->value);
}

lisp_type tp_int = {
  .tp_name = "int",
  .tp_alloc = &lisp_int_alloc,
  .tp_dealloc = &generic_dealloc,
  .tp_print = &lisp_int_print
};

/*******************************************************************************
                              tp_atom / lisp_atom
*******************************************************************************/

static lisp_value *lisp_atom_alloc(void)
{
  lisp_atom *rv = smb_new(lisp_atom, 1);
  rv->lv.type = &tp_atom;
  rv->lv.refcount = 1;
  rv->value = NULL;
  return (lisp_value *)rv;
}

static void lisp_atom_dealloc(lisp_value *value)
{
  lisp_atom *id = (lisp_atom *) value;
  smb_free(id->value);
  smb_free(id);
}

static void lisp_atom_print(lisp_value *value, FILE *f, int indent)
{
  lisp_atom *val = (lisp_atom *) value;
  fprintf(f, "'%ls\n", val->value);
}

lisp_type tp_atom = {
  .tp_name = "atom",
  .tp_alloc = &lisp_atom_alloc,
  .tp_dealloc = &lisp_atom_dealloc,
  .tp_print = &lisp_atom_print
};

/*******************************************************************************
                        tp_identifier / lisp_identifier
*******************************************************************************/

static lisp_value *lisp_identifier_alloc(void)
{
  lisp_identifier *rv = smb_new(lisp_identifier, 1);
  rv->lv.type = &tp_identifier;
  rv->lv.refcount = 1;
  rv->value = NULL;
  return (lisp_value *)rv;
}

static void lisp_identifier_dealloc(lisp_value *value)
{
  lisp_identifier *id = (lisp_identifier *) value;
  smb_free(id->value);
  smb_free(id);
}

static void lisp_identifier_print(lisp_value *value, FILE *f, int indent)
{
  lisp_identifier *val = (lisp_identifier *) value;
  fprintf(f, "%ls\n", val->value);
}

lisp_type tp_identifier = {
  .tp_name = "identifier",
  .tp_alloc = &lisp_identifier_alloc,
  .tp_dealloc = &lisp_identifier_dealloc,
  .tp_print = &lisp_identifier_print
};

/*******************************************************************************
                          tp_funccall / lisp_funccall
*******************************************************************************/

static lisp_value *lisp_funccall_alloc(void)
{
  lisp_funccall *rv = smb_new(lisp_funccall, 1);
  rv->lv.type = &tp_funccall;
  rv->lv.refcount = 1;
  rv->function = NULL;
  rv->arguments = NULL;
  return (lisp_value *)rv;
}

static void lisp_funccall_dealloc(lisp_value *value)
{
  lisp_funccall *call = (lisp_funccall *)value;
  lisp_decref(call->function);
  lisp_decref((lisp_value*)call->arguments);
  smb_free(value);
}

static void lisp_funccall_print(lisp_value *value, FILE *f, int indent)
{
  lisp_funccall *call = (lisp_funccall *) value;
  lisp_list *l;

  fprintf(f, "(");
  call->function->type->tp_print(call->function, f, indent + 2);
  fprintf(f, "\n");

  l = call->arguments;
  while (l != NULL) {
    print_n_spaces(f, indent + 1);
    l->value->type->tp_print(l->value, f, indent + 1);
    l = l->next;
  }

  print_n_spaces(f, indent);
  fprintf(f, ")\n");
}

lisp_type tp_funccall = {
  .tp_name = "funccall",
  .tp_alloc = &lisp_funccall_alloc,
  .tp_dealloc = &lisp_funccall_dealloc,
  .tp_print = &lisp_funccall_print
};

/*******************************************************************************
                              tp_list / lisp_list
*******************************************************************************/

static lisp_value *lisp_list_alloc(void)
{
  lisp_list *rv = smb_new(lisp_list, 1);
  rv->lv.type = &tp_list;
  rv->lv.refcount = 1;
  rv->value = NULL;
  rv->next = NULL;
  return (lisp_value *)rv;
}

static void lisp_list_dealloc(lisp_value *value)
{
  lisp_list *list = (lisp_list *)value;
  lisp_decref(list->value);
  lisp_decref((lisp_value*)list->next);
  smb_free(value);
}

static void lisp_list_print(lisp_value *value, FILE *f, int indent)
{
  lisp_list *l = (lisp_list *) value;

  fprintf(f, "(\n");

  while (l != NULL) {
    print_n_spaces(f, indent + 1);
    l->value->type->tp_print(l->value, f, indent + 1);
    fprintf(f, "\n");
    l = l->next;
  }

  print_n_spaces(f, indent);
  fprintf(f, ")\n");
}

lisp_type tp_list = {
  .tp_name = "list",
  .tp_alloc = &lisp_list_alloc,
  .tp_dealloc = &lisp_list_dealloc,
  .tp_print = &lisp_list_print
};

int lisp_list_length(lisp_list *l)
{
  int i = 0;
  while (l) {
    i++;
    l = l->next;
  }
  return i;
}

/*******************************************************************************
                           tp_builtin / lisp_builtin
*******************************************************************************/

static lisp_value *lisp_builtin_alloc(void)
{
  lisp_builtin *rv = smb_new(lisp_builtin, 1);
  rv->lv.type = &tp_builtin;
  rv->lv.refcount = 1;
  rv->function = NULL;
  return (lisp_value *)rv;
}

static void lisp_builtin_print(lisp_value *value, FILE *f, int indent)
{
  fprintf(f, "builtin-function\n");
}

lisp_type tp_builtin = {
  .tp_name = "builtin",
  .tp_alloc = &lisp_builtin_alloc,
  .tp_dealloc = &generic_dealloc,
  .tp_print = &lisp_builtin_print
};
