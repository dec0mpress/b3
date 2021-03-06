/******************************************************************************
  This file is part of b3.

  Copyright 2020-2021 Richard Paul Baeck <richard.baeck@mailbox.org>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*******************************************************************************/

/**
 * @author Richard Bäck
 * @date 2021-01-03
 * @brief File contains the implementation of the utility functions
 */

#include "utils.h"

#include <stdlib.h>
#include <string.h>
#include <w32bindkeys/logger.h>

static wbk_logger_t logger = { "utils" };

char *
b3_add_c_to_s(char *modified_str, char new_c)
{
  int length;

  if (modified_str == NULL) {
    length = 2;
    modified_str = malloc(sizeof(char) * length);
  } else {
    length = strlen(modified_str) + 1 + 1;
    modified_str = realloc(modified_str, sizeof(char) * length);
  }

  modified_str[length - 2] = new_c;
  modified_str[length - 1] = '\0';

  return modified_str;
}

char *
b3_add_s_to_s(char *modified_str, const char *new_s)
{
  int offset;
  int length;

  offset = 0;
  length = 0;


  if (modified_str != NULL) {
    offset = strlen(modified_str);
    length += offset;
  }

  length += strlen(new_s);
  length += 1; /** \0 */

  if (modified_str) {
    modified_str = realloc(modified_str, sizeof(char) * length);
  } else {
    modified_str = malloc(sizeof(char) * length);
  }

  strcpy(modified_str + offset, new_s);

  return modified_str;
}

int
b3_compile_pattern(const char *pattern, pcre **re_compiled, pcre_extra **re_extra)
{
  int error;
  const char *pcre_err_str;
  int pcre_err_offset;

  error = 0;

  *re_compiled = NULL;
  *re_extra = NULL;

  if (!error) {
    *re_compiled = pcre_compile(pattern, 0, &pcre_err_str, &pcre_err_offset, NULL);

    if(*re_compiled == NULL) {
      wbk_logger_log(&logger, SEVERE, "Could not compile '%s': %s\n", pattern, pcre_err_str);
      error = 1;
    }
  }

  if (!error) {
    *re_extra = pcre_study(*re_compiled, 0, &pcre_err_str);

    if (pcre_err_str) {
      wbk_logger_log(&logger, SEVERE, "Could not study '%s': %s\n", pattern, pcre_err_str);
      error = 2;
    }
  }

  if (error) {
    if (*re_compiled) {
      pcre_free(*re_compiled);
      *re_compiled = NULL;
    }

    if (*re_extra) {
#ifdef PCRE_CONFIG_JIT
      pcre_free_study(*re_extra);
#else
      pcre_free(*re_extra);
#endif
      *re_extra = NULL;
    }
  }

  return error;
}
