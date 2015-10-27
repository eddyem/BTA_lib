/*
 * cmdlnopts.c - the only function that parce cmdln args and returns glob parameters
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
#include "cmdlnopts.h"
#include "usefull_macros.h"
#include <assert.h>

/*
 * here are global parameters initialisation
 */
glob_pars G;  // internal global parameters structure
int help = 0; // whether to show help string

glob_pars Gdefault = {
	 .passfile       = NULL
	,.force          = 0
	,.p2move         = NULL
	,.p2mode         = NULL
	,.focmove        = -1.
	,.eqcrds         = NULL
	,.horcrds        = NULL
	,.azrev          = 0
	,.telstop        = 0
	,.gotoRaDec      = 0
	,.gotoAZ         = 0
	,.epoch          = NULL
	,.pmra           = 0.
	,.pmdecl         = 0.
	,.PCSoff         = 0
	,.corrAZ         = 0
	,.corrRAD        = 0
	,.quiet          = 0
	,.getinfo        = NULL
	,.infoargs       = NULL
	,.listinfo       = 0
};

/*
 * Define command line options by filling structure:
 *	name	has_arg	flag	val		type		argptr			help
*/
myoption cmdlnopts[] = {
	{"help",	0,	NULL,	'h',	arg_int,	APTR(&help),		N_("show this help")},
	{"passfile",1,	NULL,	'p',	arg_string,	APTR(&G.passfile),	N_("file with password hash (in/out)")},
	{"force",	0,	NULL,	'f',	arg_int,	APTR(&G.force),		N_("force command executions")},
	{"p2move",	1,	NULL,	'P',	arg_string,	APTR(&G.p2move),	N_("move P2 (arg: angle[rel])")},
	{"p2mode",  1,	NULL,	'M',	arg_string,	APTR(&G.p2mode),	N_("set P2 mode (arg: stop/track)")},
	{"focmove",	1,	NULL,	'F',	arg_double,	APTR(&G.focmove),	N_("move focus to given value")},
	{"eq-crds",	1,	NULL,	'e',	arg_string,	APTR(&G.eqcrds),	N_("set new equatorial coordinates")},
	{"hor-crds",1,	NULL,	'a',	arg_string,	APTR(&G.horcrds),	N_("set new horizontal coordinates")},
	{"az-reverce",0,NULL,	'R',	arg_int,	APTR(&G.azrev),		N_("switch Az reverce")},
	{"stop-tel",0,	NULL,	'S',	arg_int,	APTR(&G.telstop),	N_("stop telescope")},
	{"gotoradec",0,	NULL,	'G',	arg_int,	APTR(&G.gotoRaDec),	N_("go to last entered RA/Decl")},
	{"gotoaz",	0,	NULL,	'A',	arg_int,	APTR(&G.gotoAZ),	N_("go to last entered A/Z")},
	{"epoch",   2,	NULL,	'E',	arg_string,	APTR(&G.epoch),		N_("epoch for given RA/Decl (without argument is \"now\")")},
	{"pm-ra",	1,	NULL,	'x',	arg_double,	APTR(&G.pmra),		N_("proper motion by R.A.  (mas/year)")},
	{"pm-decl",	1,	NULL,	'y',	arg_double,	APTR(&G.pmdecl),	N_("proper motion by Decl. (mas/year)")},
	{"pcs-off",	0,	NULL,	'O',	arg_int,	APTR(&G.PCSoff),	N_("turn OFF pointing correction system")},
	{"az-corr",	1,	NULL,	1,		arg_string,	APTR(&G.corrAZ),	N_("run correction by A/Z (arg in arcsec: dA,dZ)")},
	{"rad-corr",1,	NULL,	1,		arg_string,	APTR(&G.corrRAD),	N_("run correction by RA/Decl (arg in arcsec: dRA,dDecl)")},
	{"quiet",	0,	NULL,	'q',	arg_int,	APTR(&G.quiet),		N_("almost no messages into stdout")},
	{"get-info",2,	NULL,	'I',	arg_string,	APTR(&G.getinfo),	N_("show information (default: all, \"help\" for list)")},
	{"info-args",1,	NULL,	'i',	arg_string,	APTR(&G.infoargs),	N_("show values of given ACS parameters")},
	{"list-info",0,	NULL,	'l',	arg_string,	APTR(&G.listinfo),	N_("list all ACS parameters available")},
	// ...
	end_option
};

/**
 * Parce command line options and return dynamically allocated structure
 * 		to global parameters
 * @param argc - copy of argc from main
 * @param argv - copy of argv from main
 * @return allocated structure with global parameters
 */
glob_pars *parce_args(int argc, char **argv){
	int i;
	void *ptr;
	ptr = memcpy(&G, &Gdefault, sizeof(G)); assert(ptr);
	// format of help: "Usage: progname [args]\n"
	change_helpstring("Usage: %s [args]\n\n\tWhere args are:\n");
	// parse arguments
	parceargs(&argc, &argv, cmdlnopts);
	if(help) showhelp(-1, cmdlnopts);
	if(argc > 0){
		printf("\nIgnore argument[s]:\n");
		for (i = 0; i < argc; i++)
			printf("\t%s\n", argv[i]);
	}
	return &G;
}

