/*
 * cmdlnopts.h - comand line options for parceargs
 *
 * Copyright 2013 Edward V. Emelianoff <eddy@sao.ru>
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
#ifndef __CMDLNOPTS_H__
#define __CMDLNOPTS_H__

#include "parceargs.h"

/*
 * here are some typedef's for global data
 */

typedef struct{
	char *passfile; // filename with password
	int force;      // force command running (stop p2 if moving etc)
	char *p2move;   // rotate p2 arguments: angle[rel]
	char *p2mode;   // set P2 mode (stop/track)
	double focmove; // move focus to given value
	char *eqcrds;   // set new equatorial coordinates
	char *horcrds;  // set new horizontal coordinates
	int azrev;      // reverse A/Z
	int telstop;    // stop telescope
	int gotoRaDec;  // goto last entered RA/Decl
	int gotoAZ;     // goto last entered A/Z
	char *epoch;    // epoch for given coordinates (vararg: year or "now" if present, "now" if absent)
	double pmra;    // proper motion by R.A. (mas/year)
	double pmdecl;  // proper motion by Decl (mas/year)
	int PCSoff;     // turn OFF PCS for current moving
	char *corrAZ;   // run correction by A/Z
	char *corrRAD;  // run correction by RA/Decl
	int quiet;      // ==1 - no messages to stdout
	char *getinfo;  // level of requested information (meteo, coords, etc)
	char *infoargs; // list of requested information (certain parameters)
	int listinfo;   // show list of information parameters available
}glob_pars;

glob_pars *parce_args(int argc, char **argv);
extern glob_pars *GP;

#endif // __CMDLNOPTS_H__
