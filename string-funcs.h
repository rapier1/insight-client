/*
 * Copyright (c) 2013 The Board of Trustees of Carnegie Mellon University.
 *
 *  Author: Chris Rapier <rapier@psc.edu>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT License.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the MIT License for more details.
 *
 * You should have received a copy of the MIT License along with this library;
 * if not, see http://opensource.org/licenses/MIT.
 *
 */
/* string-funcs.h */

#ifndef STRING_FUNCS_H
#define STRING_FUNCS_H

int strpos (char *, char *);
char** str_split( char*, char, int*);
char* strip(char *);
char* noquotes(char *);
void join_strings(char **, char** , char* , int);

#endif
