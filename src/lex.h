/***************************************************************************//**

  @file         lex.h

  @author       Stephen Brennan

  @date         Created Monday,  6 July 2015

  @brief        Lexer

  @copyright    Copyright (c) 2015, Stephen Brennan.  Released under the Revised
                BSD License.  See LICENSE.txt for details.

*******************************************************************************/

#ifndef SMB_LEX_H
#define SMB_LEX_H

#include <stdbool.h>
#include "libstephen/al.h"

typedef struct {

  smb_al patterns;
  smb_al tokens;

} smb_lex;

typedef struct {

  smb_al simulations;
  int last_matched_pattern;
  int last_matched_index;
  int last_index;
  bool finished;

} smb_lex_sim;

// Data structure functions.
void lex_init(smb_lex *obj);
smb_lex *lex_create();
void lex_destroy(smb_lex *obj, bool free_strings);
void lex_delete(smb_lex *obj, bool free_strings);

// Adding patterns
void lex_add_token(smb_lex *obj, wchar_t *regex, DATA token);

// Loading from a file.
void lex_load(smb_lex *obj, const wchar_t *str, smb_status *status);

// Helper functions for the tokenizer.
smb_lex_sim *lex_start(smb_lex *obj);
bool lex_step(smb_lex *obj, smb_lex_sim *sim, wchar_t input);
DATA lex_get_token(smb_lex *obj, smb_lex_sim *sim);
int lex_get_length(smb_lex *obj, smb_lex_sim *sim);
void lex_sim_delete(smb_lex_sim *sim);

// Two tokenizer functions:
void lex_yylex(smb_lex *obj, wchar_t *input, DATA *token, int *length,
               smb_status *st);
wchar_t *lex_fyylex(smb_lex *obj, FILE *f, DATA *token, int *length,
                    smb_status *s);

#endif//SMB_LEX_H
