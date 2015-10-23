/***************************************************************************//**

  @file         main.c

  @author       Stephen Brennan

  @date         Created Friday, 23 October 2015

  @brief        Lisp main program.

  @copyright    Copyright (c) 2015, Stephen Brennan.  Released under the Revised
                BSD License.  See LICENSE.txt for details.

*******************************************************************************/

#include "lisp.h"

int main(int argc, char *argv[])
{
  (void)argc; // unused
  (void)argv; // unused
  lisp_interact();
  return 0;
}

