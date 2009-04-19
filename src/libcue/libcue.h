/*
 * cue.h -- cue function declarations
 *
 * Copyright (C) 2004, 2005, 2006, 2007 Svend Sorensen
 * For license terms, see the file COPYING in this distribution.
 */

#include "cd.h"

Cd* cue_parse_file(FILE*);
Cd* cue_parse_string(const char*);
