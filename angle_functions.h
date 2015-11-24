/*
 * angle_functions.h
 *
 * Copyright 2015 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */
#pragma once
#ifndef __ANGLE_FUNCTIONS_H__
#define __ANGLE_FUNCTIONS_H__

#include <stdbool.h>

#define BUFSZ 255

#ifndef TRUE
	#define TRUE true
#endif

#ifndef FALSE
	#define FALSE false
#endif

char *time_asc(double t);
char *angle_asc(double a);
char *angle_fmt(double a, char *format);
double sec_to_degr(double sec);
char *get_degrees(double *ret, char *str);
bool calc_AP(double r, double d, double *appRA, double *appDecl);
bool myatod(double *num, char **str);
bool myatoi(int32_t *num, char **str);

#endif // __ANGLE_FUNCTIONS_H__
