/***************************************************************************//**

  @file         lex.c

  @author       Stephen Brennan

  @date         Created Monday,  6 July 2015

  @brief        A very generic lexer

  @copyright    Copyright (c) 2015, Stephen Brennan.  Released under the Revised
                BSD License.  See LICENSE.txt for details.

*******************************************************************************/

#include <assert.h>
#include <stdio.h>
#include "libstephen/al.h"
#include "libstephen/cb.h"
#include "libstephen/str.h"
#include "libstephen/log.h"
#include "libstephen/regex.h"
#include "lex.h"

smb_logger lex_log = {
  .format = SMB_DEFAULT_LOGFORMAT,
  .num = 0
};

void lex_init(smb_lex *obj)
{
  // Initialization logic
  al_init(&obj->patterns);
  al_init(&obj->tokens);
}

smb_lex *lex_create(void)
{
  smb_lex *obj = smb_new(smb_lex, 1);
  lex_init(obj);
  return obj;
}

void lex_destroy(smb_lex *obj, bool free_strings)
{
  // Cleanup logic
  smb_iter it;
  fsm *f;
  wchar_t *s;
  smb_status status = SMB_SUCCESS;

  it = al_get_iter(&obj->patterns);
  while (it.has_next(&it)) {
    f = it.next(&it, &status).data_ptr;
    assert(status == SMB_SUCCESS);
    fsm_delete(f, true);
  }
  al_destroy(&obj->patterns);

  it = al_get_iter(&obj->tokens);
  while (it.has_next(&it)) {
    s = it.next(&it, &status).data_ptr;
    assert(status == SMB_SUCCESS);
    if (free_strings) {
      smb_free(s);
    }
  }
  al_destroy(&obj->tokens);
}

void lex_delete(smb_lex *obj, bool free_strings) {
  lex_destroy(obj, free_strings);
  smb_free(obj);
}

void lex_add_token(smb_lex *obj, wchar_t *regex, DATA token)
{
  fsm *f = regex_parse(regex);
  al_append(&obj->patterns, (DATA){.data_ptr=f});
  al_append(&obj->tokens, token);
}

void lex_add_pattern(smb_lex *obj, wchar_t *regex, wchar_t *token)
{
  wchar_t *s = smb_new(wchar_t, wcslen(token) + 1);
  wcscpy(s, token);
  lex_add_token(obj, regex, PTR(token));
}

static void lex_load_line(smb_lex *obj, wchar_t *line, smb_status *status)
{
  wchar_t *token;
  int i = 0;
  while (line[i] != L'\0' && line[i] != L'\t') {
    i++;
  }

  if (line[i] != L'\t') {
    *status = SMB_INDEX_ERROR;  //TODO: more reasonable error message
    return;
  }

  // End the string so we can parse the regular expression.
  line[i] = L'\0';
  token = line + i + 1;
  lex_add_pattern(obj, line, token);
}

void lex_load(smb_lex *obj, const wchar_t *str, smb_status *status)
{
  smb_ll *lines;
  smb_iter iter;
  wchar_t *buf = smb_new(wchar_t, wcslen(str) + 1);
  wchar_t *line;
  wcscpy(buf, str);
  lines = split_linesw(buf);
  iter = ll_get_iter(lines);

  while (iter.has_next(&iter)) {
    line = iter.next(&iter, status).data_ptr;
    assert(*status == SMB_SUCCESS);
    fflush(stdout);
    if (line[0] != L'#') {
      lex_load_line(obj, line, status);
      if (*status != SMB_SUCCESS) {
        goto cleanup;
      }
    }
  }
 cleanup:
  smb_free(buf);
  ll_delete(lines);
}

smb_lex_sim *lex_start(smb_lex *obj)
{
  int i;
  fsm_sim *fs;
  smb_status status;
  smb_lex_sim *sim = smb_new(smb_lex_sim, 1);
  al_init(&sim->simulations);

  // Create simulations for each pattern.
  for (i = 0; i < al_length(&obj->patterns); i++) {
    fs = fsm_sim_nondet_begin(al_get(&obj->patterns, i, &status).data_ptr);
    assert(status == SMB_SUCCESS);
    al_append(&sim->simulations, (DATA){.data_ptr=fs});
  }

  sim->last_matched_pattern = -1;
  sim->last_matched_index = -1;
  sim->last_index = -1;
  sim->finished = false;
  return sim;
}

bool lex_step(smb_lex *obj, smb_lex_sim *sim, wchar_t input)
{
  smb_status status = SMB_SUCCESS;
  fsm_sim *fs;
  int i, state;
  bool all_rejected = true;
  sim->last_index++;

  // For each simulation...
  for (i = 0; i < al_length(&obj->patterns); i++) {
    // Drive forward the simulation, and get its state.
    fs = al_get(&sim->simulations, i, &status).data_ptr;
    assert(status == SMB_SUCCESS);
    fsm_sim_nondet_step(fs, input);
    // It doesn't actually matter what the next input is, just that it's not a
    // null byte :/
    state = fsm_sim_nondet_state(fs, L'a');

    // If the simulation is in an accepting state...
    if (state == FSM_SIM_ACCEPTING || state == FSM_SIM_ACCEPTED) {
      LDEBUG(&lex_log, "lex_step() - L'%lc' - fsm %d - accepting", input, i);
      // And no other simulation has been accepting at this index...
      if (sim->last_matched_index < sim->last_index) {
        // Record this pattern and simulation.
        sim->last_matched_pattern = i;
        sim->last_matched_index = sim->last_index;
      }
      all_rejected = false;
    } else if (state == FSM_SIM_NOT_ACCEPTING) {
      LDEBUG(&lex_log, "lex_step() - L'%lc' - fsm %d - not accepting", input, i);
      all_rejected = false;
    } else {
      LDEBUG(&lex_log, "lex_step() - L'%lc' - fsm %d - rejected", input, i);
    }
  }

  // The simulation is done when all are rejected.
  sim->finished = all_rejected;
  return sim->finished;
}

DATA lex_get_token(smb_lex *obj, smb_lex_sim *sim)
{
  smb_status status;
  DATA token;
  if (!sim->finished || sim->last_matched_pattern < 0) {
    return (DATA){.data_ptr=NULL};
  } else {
    token = al_get(&obj->tokens, sim->last_matched_pattern, &status);
    assert(status == SMB_SUCCESS);
    return token;
  }
}

int lex_get_length(smb_lex *obj, smb_lex_sim *sim)
{
  (void)obj; //unused
  if (!sim->finished) {
    return -1;
  } else {
    return sim->last_matched_index + 1;
  }
}

void lex_sim_delete(smb_lex_sim *sim)
{
  smb_status status = SMB_SUCCESS;
  int i;
  // Finally, destroy the simulations for each pattern.
  for (i = 0; i < al_length(&sim->simulations); i++) {
    fsm_sim_delete(al_get(&sim->simulations, i, &status).data_ptr, true);
    assert(status == SMB_SUCCESS);
  }
  al_destroy(&sim->simulations);
  smb_free(sim);
}

void lex_yylex(smb_lex *obj, wchar_t *input, DATA *token, int *length,
               smb_status *status)
{
  (void)status; //unused
  smb_lex_sim *sim = lex_start(obj);

  while (!sim->finished) {
    lex_step(obj, sim, *input++);
  }

  *token = lex_get_token(obj, sim);
  *length = lex_get_length(obj, sim);
  lex_sim_delete(sim);
}

wchar_t *lex_fyylex(smb_lex *obj, FILE *input, DATA *token, int *length,
                    smb_status *status)
{
  (void)status; //unused
  smb_lex_sim *sim = lex_start(obj);
  wcbuf wcb;
  wchar_t curr;
  wcb_init(&wcb, 128);

  while (!sim->finished) {
    curr = fgetwc(input);
    wcb_append(&wcb, curr);
    lex_step(obj, sim, curr);
  }

  // The last character is never part of the token, so get rid of it.
  wcb.buf[wcb.length-1] = L'\0';
  ungetwc(curr, input);

  *token = lex_get_token(obj, sim);
  *length = lex_get_length(obj, sim);
  lex_sim_delete(sim);
  return wcb.buf;
}
