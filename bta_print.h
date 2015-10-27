/*
 * bta_print.h
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
#ifndef __BTA_PRINT_H__
#define __BTA_PRINT_H__

typedef enum{
	 NO_INFO          = 0      // don't show anything
	,BASIC_COORDS     = 1      // show basic coordinates
	,METEO_INFO       = 2      // meteo info
	,TIME_INFO        = 4      // all info about time
	,ACS_INFO         = 8      // extended ACS information
	,EXTENDED_COORDS  = 0x11   // extended coords (val, spd, corr) + basic
	,ALL_INFO         = 0xff   // all information available
	,REQUESTED_LIST   = 0x8000 // show only parameters given in list
} info_level;

int bta_print (info_level lvl, char *par_list);
void show_infolevels();
info_level get_infolevel(char* infostr);

#endif // __BTA_PRINT_H__
