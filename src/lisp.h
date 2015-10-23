/***************************************************************************//**

  @file         lisp.h

  @author       Stephen Brennan

  @date         Created Thursday, 22 October 2015

  @brief        Public declarations for the lisp interpreter.

  @copyright    Copyright (c) 2015, Stephen Brennan.  Released under the Revised
                BSD License.  See LICENSE.txt for details.

*******************************************************************************/

#ifndef CKY_LISP_H
#define CKY_LISP_H

#include <stdio.h>

#include "libstephen/ht.h"
#include "libstephen/ll.h"

/*
  Basic types in lisp.
 */
#define TP_INT  0
#define TP_ATOM 1
#define TP_LIST 2
#define TP_BUILTIN 3
#define TP_FUNCTION 4
#define TP_FUNCCALL 5
#define TP_IDENTIFIER 6

struct lisp_value;
typedef struct lisp_value lisp_value;

/**
   @brief Type objects define how values of some type should behave.
 */
typedef struct {

  /**
     @brief The name of this type.
   */
  const char *tp_name;
  /**
     @brief Memory allocator for this type.
   */
  lisp_value* (*tp_alloc)(void);
  /**
     @brief Memory deallocator for this type.
   */
  void (*tp_dealloc)(lisp_value*);
  /**
     @brief Output function.
   */
  void (*tp_print)(lisp_value*, FILE *, int);

} lisp_type;

/**
  @brief The base lisp value type.
 */
struct lisp_value {

  /**
     @brief The type of the value.
   */
  lisp_type *type;

  /**
     @brief Number of references to this value.
   */
  unsigned int refcount;

};

/*******************************************************************************
                          "Child" types of lisp_value
*******************************************************************************/
typedef struct {
  lisp_value lv;
  long int value;
} lisp_int;
lisp_type tp_int;

typedef struct {
  lisp_value lv;
  wchar_t *value;
} lisp_atom;
lisp_type tp_atom;

typedef struct {
  lisp_value lv;
  wchar_t *value;
} lisp_identifier;
lisp_type tp_identifier;

typedef struct lisp_list {
  lisp_value lv;
  lisp_value *value;
  struct lisp_list *next;
} lisp_list;
lisp_type tp_list;

typedef struct {
  lisp_value lv;
  lisp_value *function;
  lisp_list *arguments;
} lisp_funccall;
lisp_type tp_funccall;

typedef struct {
  lisp_value lv;
  lisp_value * (*function) (lisp_list *);
} lisp_builtin;
lisp_type tp_builtin;

/*******************************************************************************
                    Some useful utility functions on lists.
*******************************************************************************/
int lisp_list_length(lisp_list *l);

/**
   @brief A struct to represent one level of scope.
 */
typedef struct lisp_scope {

  /**
     @brief Hash table containing variables!
   */
  smb_ht table;

  /**
     @brief Pointer to the previous level of scope.
   */
  struct lisp_scope *up;

} lisp_scope;

/**
   @brief Tokenize a string.

   The input string is not modified.  All returned tokens are newly allocated,
   so the input string does not need to be "kept around" while the tokens are
   used.  On the flip side, you must free all the tokens when you are done with
   them.

   @param str The string to tokenize.
   @returns A `smb_ll` containing `lisp_token` structs.
 */
smb_ll *lisp_lex(wchar_t *str);

/**
   @brief Tokenize a file incrementally.
 */
smb_iter lisp_lex_file(FILE *f);

/**
   @brief Parse a token stream and return the first expression.
   @param it Pointer to the iterator over the stream.
   @returns NEW REFERENCE to code
 */
lisp_value *lisp_parse(smb_iter *it);

/**
   @brief Evaluate an expression within a scope.
   @param expr Reference to expression.
   @param scope The scope to run in.
   @returns NEW REFERENCE to the return value
 */
lisp_value *lisp_evaluate(lisp_value *expr, lisp_scope *scope);

/**
   @brief Run a piece of lisp code.
   @param str Code to run.
   @returns The value of the last expression.
 */
lisp_value *lisp_run(wchar_t *str);

/**
   @brief Increment the reference count of an object.
   @param lv Object to incref (nullable)
 */
void lisp_incref(lisp_value *lv);

/**
   @brief Decrement the reference count of an object.
   @param lv Object to decref (nullable)

   When an object goes down to 0 references, it is deallocated.
 */
void lisp_decref(lisp_value *lv);

/**
   @brief Return a "scope" containing global definitions.
 */
lisp_scope *lisp_create_globals(void);
lisp_scope *lisp_scope_create(void);
void *lisp_scope_delete(lisp_scope *scope);
#endif // CKY_LISP_H
