/*
 * bta_control.c
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
#define _GNU_SOURCE 666 // for strcasestr
#include <assert.h>
#include <signal.h>
#include <strings.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

#include "bta_shdata.h"
#include "cmdlnopts.h"
#include "usefull_macros.h"
#include "ch4run.h"
#include "bta_control.h"
#include "angle_functions.h"
#include "bta_print.h"


#ifndef PIDFILE
#define PIDFILE  "/tmp/bta_control.pid"
#endif

// constants for choosing move/goto (move for near objects)
const double Amove = 1800.;   // +-30'
const double Zmove = 3600.;   // +-60'
// arcseconds to radians
#define AS2R  (M_PI/180./3600.)

glob_pars *GP = NULL;
#define PRINT(...) do{if(!GP->quiet) printf(__VA_ARGS__);}while(0)

// ACS command wrapper
#ifdef EMULATION
#define ACS_CMD(a)   do{green(#a); printf("\n");}while(0)
#else
//#define ACS_CMD(a)   do{red(#a); printf("\n");}while(0)
// Uncomment only in final release
#define ACS_CMD(a)   do{DBG(#a "\n"); a; }while(0)
#endif


#ifndef EMULATION
typedef struct{
	uint32_t keylev;
	uint32_t codelev;
} passhash;
#endif

void signals(int sig){
	if(sig)
		WARNX(_("Get signal %d, quit.\n"), sig);
	else
		sig = -1;
	unlink(PIDFILE);
	restore_console();
	exit(sig);
}


volatile int tmout = 0;
pthread_t athread;
void *tmout_thread(void *buf){
	int selfd = -1, *sec = (int*)buf;
	struct timeval tv;
	tv.tv_sec  = *sec;
	tv.tv_usec = 0;
	errno = 0;
	while(selfd < 0){
		selfd = select(0, NULL, NULL, NULL, &tv);
		if(selfd < 0 && errno != EINTR){
			WARN(_("Error while select()"));
			tmout = 1;
			return NULL;
		}
	}
	tmout = 1;
	return NULL;
}

/**
 * run thread with pause [delay] (in seconds), at its end set variable tmout
 */
void set_timeout(int delay){
	static int run = 0;
	static int *arg = NULL;
	if(!arg) arg = MALLOC(int, 1);
	if(run && (pthread_kill(athread, 0) != ESRCH)){ // another timeout process detected - kill it
		pthread_cancel(athread);
		pthread_join(athread, NULL);
	}
	tmout = 0;
	run = 1;
	*arg = delay;
	if(pthread_create(&athread, NULL, tmout_thread, (void*)arg)){
		WARN(_("Can't create timeout thread!"));
		tmout = 1;
		return;
	}
	;
}
char indi[] = "|/-\\";
char *iptr = indi;
#define WAIT_EVENT(evt, max_delay)  do{int __ = 0; set_timeout(max_delay); \
		PRINT(" "); while(!tmout && !(evt)){ \
		usleep(100000); if(!*(++iptr)) iptr = indi; if(++__%10==0) PRINT("\b. "); \
		PRINT("\b%c", *iptr);}; PRINT("\n");}while(0)

#ifndef EMULATION
void get_passhash(passhash *p){
	int fd = -1, i, c, nlev = 0;
	char *filename = GP->passfile;
	if(filename){ // user give filename with [stored?] hash
		struct stat statbuf;
		if((fd = open(filename, O_RDWR | O_CREAT,  S_IRUSR | S_IWUSR)) < 0)
			ERR(_("Can't open %s for reading"), filename);
		if(fstat (fd, &statbuf) < 0)
			ERR(_("Can't stat %s"), filename);
		if(!S_ISREG(statbuf.st_mode))
			ERR(_("%s is not regular file"), filename);
		if(statbuf.st_mode != (S_IRUSR | S_IWUSR)){ // wrong mode
			if(chmod(filename, S_IRUSR | S_IWUSR))
				ERR(_("Can't change permissions of %s to 0600"), filename);
		}
		if(8 == read(fd, p, 8)){  // check password, if it is good, return
			for(i = 5; i > 0; --i){
				if(p->codelev == code_Lev(i)) break;
			}
			if(i){
				set_acckey(p->keylev);
				close(fd);
				return;
			}
		}
	}
	// ask user to enter password
	setup_con(); // hide echo
	for(i = 3; i > 0; --i){ // try 3 times
		char pass[256]; int k = 0;
		printf("Enter password, you have %d tr%s\n", i, (i > 1) ? "ies":"y");
		while ((c = mygetchar()) != '\n' && c != EOF && k < 255){
			if(c == '\b' || c == 127){ // use DEL and BACKSPACE to erase previous symbol
				if(k) --k;
				printf("\b \b");
			}else{
				pass[k++] = c;
				printf("*");
			}
			fflush(stdout);
		}
		pass[k] = 0;
		printf("\n");
		if((nlev = find_lev_passwd(pass, &p->keylev, &p->codelev)))
			break;
		printf(_("No, not this\n"));
	}
	restore_console();
	if(nlev == 0)
		ERRX(_("Tries excess!"));
	set_acckey(p->keylev);
	DBG("OK, level %d", nlev);
	if(fd > 0){
		PRINT(_("Store\n"));
		if(0 != lseek(fd, 0, SEEK_SET)){
			WARN(_("Can't seek to start of %s"), filename);
		}else if(8 != write(fd, p, 8))
			WARN(_("Can't store password hash in %s"), filename);
		close(fd);
	}
}
#endif // EMULATION

/***************************************************************
 * All functions for changing telescope parameters are boolean *
 * returning TRUE in case of succsess or FALSE if failed       *
 ***************************************************************/

/**
 * move P2 to the given angle relative to current position +- P2_ANGLE_THRES
 */
void cmd_P2moveto(double p2shift){
	double p2vel = 45.*60., p2dt, p2mintime = P2_MINTIME, p2secs = fabs(p2shift) * 3600.;
	if(fabs(p2shift) < P2_ANGLE_THRES) return;
	p2dt = p2secs / p2vel;
	if(p2dt < p2mintime){
		p2vel = p2secs / p2mintime;
		if(p2vel < 1.) p2vel = 1.;
		p2dt = p2secs / p2vel;
	}
	// if speed is too fast, make dt less
	if(p2vel > P2_FAST_SPEED && p2dt > P2_FAST_T_CORR + P2_MINTIME){
		p2dt -= P2_FAST_T_CORR;
	}
	if(p2shift < 0) p2vel = -p2vel;
	DBG("p2vel=%g, p2dt = %g, p2_val=%s", p2vel, p2dt, angle_asc(val_P));
	ACS_CMD(MoveP2To(p2vel, p2dt));
#ifndef EMULATION
	PRINT(_("Wait for starting"));
	WAIT_EVENT(((fabs(vel_P) > 1.) && (P2_State != P2_Off)), WAITING_TMOUT);
	if((fabs(vel_P) < 1.) || (P2_State == P2_Off)){
		DBG("vel: %g, state: %d", vel_P, P2_State);
		WARNX(_("P2 didn't start!"));
		return;
	}
	PRINT(_("Moving P2 "));
	// wait until P2 stops, set to guiding or timeout ends
	WAIT_EVENT(((fabs(vel_P) < 1.) && (P2_State == P2_Off)), p2dt + 1.);
	DBG("P2 state: %d, vel_P: %g, p2_val=%s", P2_State, vel_P, angle_asc(val_P));
	if(tmout && P2_State != P2_Off){
		WARNX(_("Timeout reached, stop P2"));
		ACS_CMD(MoveP2(0));
	}
#endif
}

/**
 * move P2 to given angle or at given delta
 * @param angle    angle to move (in degrees) with suffix "rel" for relative moving
 */
bool moveP2(char *arg){
	if(!arg) return FALSE;
	int p2rel = 0;
	char *eptr = NULL;
	int badarg = 0;
	if((eptr = strcasestr(arg, "rel"))){
		if(eptr == arg){
			badarg = 1;
			goto bdrg;
		}else{
			if(eptr[-1] < '0' || eptr[-1] > '9') eptr[-1] = 0; // omit last non-number
			else *eptr = 0;
			eptr = NULL;
			p2rel = 1;
		}
	}
	double p2angle;
	if(!get_degrees(&p2angle, arg)) badarg = 1;
	else{ // now check if there a good angle
		if(p2angle < -360. || p2angle > 360.) badarg = 1;
		else if(eptr){
			if(strcasecmp(eptr, "rel") == 0)
				p2rel = 1;
			else // wrong degrees format
				badarg = 1;
		}
	}
bdrg:
	if(badarg){
		WARNX(_("Key p2move should be in format angle[rel],\n\tangle - from -360 to +360"
			"\n\twrite \"rel\" after angle for relative moving"));
			return FALSE;
	}
	// now get information about current angle & check target angle
	double p2val = sec_to_degr(val_P);
	DBG("p2 now is at %g", p2val);
	if(p2rel) p2angle += p2val;
	if(p2angle < 0.) p2angle += 360.;
	else if(p2angle > 360.) p2angle -= 360.;
	if(p2angle > P2_LOW_ES && p2angle < P2_HIGH_ES){ // prohibited angle
		WARNX(_("Target angle (%g) is in prohibited zone (between %g & %g degrees)"),
			p2angle, P2_LOW_ES, P2_HIGH_ES);
		return FALSE;
	}
	if(P2_State != P2_Off && P2_State != P2_On){
		WARNX(_("P2 is already moving!"));
		if(!GP->force) return FALSE;
		WARNX(_("Force stop"));
		ACS_CMD(MoveP2(0)); // stop P2
#ifndef EMULATION
		if(P2_State != P2_Off){
			PRINT(_("Wait for P2 stop "));
			WAIT_EVENT(P2_State == P2_Off, WAITING_TMOUT);
			if(tmout && P2_State != P2_Off){
				WARNX(_("Timeout reached, can't stop P2"));
				return 0;
			}
		}
#endif
	}
	int p2oldmode = P2_Mode;
	ACS_CMD(SetPMode(P2_Off));
	DBG("Move P2 to %gdegr", p2angle);
	if(fabs(p2angle - p2val) < P2_ANGLE_THRES){
		WARNX(_("Zero moving (< %g)"), P2_ANGLE_THRES);
		return TRUE;
	}
	int i;
	for(i = 0; i < 5; ++i){
		if(i) PRINT(_("Try %d. "), i+1);
		cmd_P2moveto(p2angle - p2val);
		p2val = sec_to_degr(val_P);
		if(fabs(p2angle - p2val) < P2_ANGLE_THRES) break;
	}
	if(fabs(p2angle - p2val) > P2_ANGLE_THRES){
		WARNX(_("Error moving P2: have %gdegr, need %gdegr"), p2val, p2angle);
		return FALSE;
	}
	PRINT(_("All OK, current P2 value: %s\n"), angle_asc(val_P));
	ACS_CMD(SetPMode(p2oldmode));
	return TRUE;
}

/**
 * set P2 mode: stop or track
 */
bool setP2mode(char *arg){
	int _U_ mode;
	if(!arg) goto badarg;
	if(strcasecmp(arg, "stop") == 0) mode = P2_Off;
	else if(strcasecmp(arg, "track") == 0) mode = P2_On;
	else goto badarg;
	DBG("Set P2 mode to %s", (mode == P2_Off) ? "stop" : "track");
	ACS_CMD(SetPMode(mode));
#ifndef EMULATION
	if(P2_State != mode){
		PRINT(_("Wait for given mode "));
		WAIT_EVENT(P2_State == mode, WAITING_TMOUT);
		if(tmout && P2_State != mode){
			WARNX(_("Timeout reached, can't set P2 mode"));
			return FALSE;
		}
	}
#endif
	return TRUE;
badarg:
	WARNX(_("Parameter should be \"stop\" or \"track\""));
	return FALSE;
}

void cmd_Fmoveto(double f){
	const double FOC_HVEL = 0.63, FOC_LVEL = 0.13;
	int _U_ fspeed;
	if(f < 1. || f > 199.) return;
	double fshift = f - val_F, fvel, fdt;
	if(fabs(fshift) > 1.){
		fvel = FOC_HVEL;
		fspeed = (fshift > 0.) ? Foc_Hplus : Foc_Hminus;
	}else if(fabs(fshift) > FOCUS_THRES){
		fvel = FOC_LVEL;
		fspeed = (fshift > 0.) ? Foc_Lplus : Foc_Lminus;
	} else{
		WARNX(_("Can't move for such small distance (%gmm)"), fshift);
		return;
	}
	fdt = fabs(fshift) / fvel;
	ACS_CMD(MoveFocus(fspeed, fdt));
#ifndef EMULATION
	DBG("dt: %g, fvel: %g, fstate: %d, F:%g", fdt, vel_F, Foc_State, val_F);
	PRINT(_("Wait for starting"));
	WAIT_EVENT((Foc_State != Foc_Off || fabs(vel_F) > 0.01), WAITING_TMOUT);
	PRINT(_("Moving Focus "));
	WAIT_EVENT((fabs(vel_F) < 0.01 && Foc_State == Foc_Off), fdt + 1.);
	DBG("fvel: %g, fstate: %d, F:%g", vel_F, Foc_State, val_F);
	PRINT(_("Wait for stop"));
	WAIT_EVENT((Foc_State == Foc_Off && fabs(vel_F) < 0.01), WAITING_TMOUT);
	DBG("fvel: %g, fstate: %d, F:%g", vel_F, Foc_State, val_F);
	if(tmout && Foc_State != Foc_Off){
		WARNX(_("Timeout reached, stop focus"));
		ACS_CMD(MoveFocus(Foc_Off, 0.));
	}
#endif
}

/**
 * move focus to given position
 */
bool moveFocus(double val){
	if(val < 1. || val > 199.){
		WARNX(_("Focus value should be between 1mm & 199mm"));
		return FALSE;
	}
	if(Foc_State != Foc_Off){
		WARNX(_("Focus is already moving!"));
		if(!GP->force) return FALSE;
		WARNX(_("Force stop"));
		ACS_CMD(MoveFocus(Foc_Off, 0.));
	}
#ifndef EMULATION
	if(Foc_State != Foc_Off){
		PRINT(_("Wait for focus stop "));
		WAIT_EVENT(Foc_State == Foc_Off, WAITING_TMOUT);
		if(tmout && Foc_State != Foc_Off){
			WARNX(_("Timeout reached, can't stop focus motor"));
			return FALSE;
		}
	}
#endif
	DBG("Move focus to %g", val);
	if(fabs(val - val_F) < FOCUS_THRES){
		WARNX(_("Zero moving (< %g)"), FOCUS_THRES);
		return TRUE;
	}
	int i;
	for(i = 0; i < 3; ++i){
		if(i) PRINT(_("Try %d. "), i+1);
		cmd_Fmoveto(val);
		if(fabs(val - val_F) < FOCUS_THRES) break;
	}
	if(fabs(val - val_F) > FOCUS_THRES){
		WARNX(_("Error moving focus: have %gmm, need %gmm"), val_F, val);
		return FALSE;
	}
	return TRUE;
}

/**
 * set new equatorial/horizontal coordinates
 * @param coordinates: both RA&Decl/A&Z with any delimeter
 *   format RA:    hh[delimeter]mm[delimeter]ss.s - suitable for get_degrees() but in hours
 *   format DECL:  suitable for get_degrees() but with prefix +/- if goes first
 *   format A/Z:   suitable for get_degrees(), AZIMUTH GOES FIRST!
 * @param isEQ: TRUE if equatorial coordinates, FALSE if horizontal
 */
bool setCoords(char *coords, bool isEQ){
	if(!coords) return FALSE;
	char *ra = NULL, *dec = NULL, *ptr = coords;
	double r, d;
	if(isEQ){
		// find RA & DEC parameters in string
		// 1. find first number or +/-
		while(*ptr){
			char p = *ptr;
			if(p > '0'-1 && p < '9'+1){
				ra = ptr;
				break;
			}
			else if(p == '+' || p == '-'){
				dec = ptr;
				break;
			}
			++ptr;
		}
	}else ra = ptr;
	if(!*ptr || (!ra && !dec /*WTF?*/)) goto badcrds;
	if(ra){ // first was RA/AZ
		dec = get_degrees(&r, ra);
		if(!dec || !*dec || !(ptr = get_degrees(&d, dec))) goto badcrds;
		if(*ptr) goto badcrds; // something after last number
	}else{ // first was Decl
		ra  = get_degrees(&d, dec);
		if(!ra || !*ra || !(ptr = get_degrees(&r, ra))) goto badcrds;
		if(*ptr) goto badcrds;
	}
	if(isEQ){ // RA/Decl
		if(r < 0. || r > 24. || d > 90. || d < -90.) goto badcrds;
		double appRA, appDecl;
		// calculate apparent place according to other cmdline arguments
		if(!calc_AP(r, d, &appRA, &appDecl)) goto badcrds;
		DBG("Set RA/Decl to %g, %g", appRA/3600, appDecl/3600);
		r = InpAlpha, d = InpDelta; // save old coordinates
		ACS_CMD(SetRADec(appRA, appDecl));
#ifndef EMULATION
		DBG("InpAlpha = %g, InpDelta = %g, was: A = %g, D = %g",
			InpAlpha, InpDelta, r, d);
		// now check whether old coordinates was changed
		if(fabs(InpAlpha - r) > INPUT_COORDS_THRES || fabs(InpDelta - d) > INPUT_COORDS_THRES){
			PRINT(_("Wait for command result"));
			WAIT_EVENT((fabs(InpAlpha - r) < INPUT_COORDS_THRES &&
					fabs(InpDelta - d) < INPUT_COORDS_THRES), WAITING_TMOUT);
			if(tmout){
				WARNX(_("Can't send data to system!"));
				return FALSE;
			}
		}
#endif
	}else{ // A/Z: r==A, d==Z
		if(r < -360. || r > 360. || d < 0. || d > 90.) goto badcrds;
		// convert A/Z into arcsec
		r *= 3600;
		d *= 3600;
		DBG("Set A/Z to %g, %g", r/3600, d/3600);
		ACS_CMD(SetAzimZ(r, d));
#ifndef EMULATION
		if(fabs(InpAzim - r) > INPUT_COORDS_THRES || fabs(InpZdist - d) > INPUT_COORDS_THRES){
			PRINT(_("Wait for command result"));
			WAIT_EVENT((fabs(InpAzim - r) < INPUT_COORDS_THRES &&
					fabs(InpZdist - d) < INPUT_COORDS_THRES), WAITING_TMOUT);
			if(tmout){
				WARNX(_("Can't send data to system!"));
				return FALSE;
			}
		}
#endif
	}
	return TRUE;
badcrds:
	if(isEQ)
		WARNX(_("Wrong coordinates: \"%s\"; should be \"hh mm ss.ss +/-dd mm ss.ss\" (any order)"), coords);
	else
		WARNX(_("Wrong coordinates: \"%s\"; should be \"[+/-]dd mm ss.ss dd mm ss.ss\" (Az first)"), coords);
	return FALSE;
}

/**
 * reverce Azimuth traveling
 */
bool azreverce(){
	bool ret = TRUE;
	int mode = Az_Mode;
	DBG("mode: %d", mode);
	if(mode == Rev_Off) mode = Rev_On;
	else mode = Rev_Off;
	ACS_CMD(SetAzRevers(mode));
	PRINT(_("Turn %s azimuth reverce "), (mode == Rev_Off) ? "off" : "on");
#ifndef EMULATION
	WAIT_EVENT((Az_Mode == mode), WAITING_TMOUT);
	if(Az_Mode != mode) ret = FALSE;
#endif
	return ret;
}

bool testauto(){
	if(Tel_Mode != Automatic){
		WARNX(_("Not automatic mode!"));
		return FALSE;
	}
	return TRUE;
}

bool stop_telescope(){
	if(!testauto()) return FALSE;
	if(Sys_Mode == SysStop){
		WARNX(_("Already stoped"));
		return TRUE;
	}
	ACS_CMD(StopTeleskope());
	WAIT_EVENT((Sys_Mode == SysStop), WAITING_TMOUT);
	if(Tel_Mode != SysStop){
		WARNX(_("Can't stop telescope"));
		return FALSE;
	}
	return TRUE;
}

/**
 * move telecope to object by entered coordinates
 */
bool gotopos(bool isradec){
	if(!testauto()) return FALSE;
	if(Sys_Mode != SysStop && !stop_telescope()) return FALSE;
	if(isradec){
		if((fabs(val_A - InpAzim) < Amove && fabs(val_Z - InpZdist) < Zmove)){ // move back to last coords
				ACS_CMD(MoveToObject());
		}else{
			ACS_CMD(GoToObject());
		}
		ACS_CMD(SetSysTarg(TagObject));
	}else{
		ACS_CMD(GoToAzimZ());
		ACS_CMD(SetSysTarg(TagPosition));
	}
	DBG("start");
	ACS_CMD(StartTeleskope());
	usleep(500000);
#ifndef EMULATION
	PRINT("Go");
	WAIT_EVENT((Sys_Mode != SysStop && Sys_Mode != SysWait), WAITING_TMOUT);
	if(tmout){
		WARNX(_("Can't move telescope"));
		ACS_CMD(StopTeleskope());
		return FALSE;
	}
	PRINT("Wait for tracking");
	//  Wait with timeout 15min
	WAIT_EVENT((Sys_Mode == SysTrkOk), 900.);
	if(tmout){
		WARNX(_("Eror during telescope pointing"));
		return FALSE;
   }
#endif
	return TRUE;
}

/**
 * set PCS state (TRUE == on)
 */
bool PCS_state(bool on){
	int _U_ newstate = PC_Off;
	if(on){
		if(Pos_Corr == PC_On) return TRUE;
		ACS_CMD(SwitchPosCorr(PC_On));
		newstate = PC_On;
	}else{
		if(Pos_Corr == PC_Off) return TRUE;
		ACS_CMD(SwitchPosCorr(PC_Off));
	}
#ifndef EMULATION
		WAIT_EVENT((Pos_Corr == newstate), WAITING_TMOUT);
		if(tmout){
			WARNX(_("Can't set new PCS state"));
			return FALSE;
		}
#endif
	return TRUE;
}

/**
 * make small position correction for angles dx, dy (in arcseconds)
 * format: "dx,dy" with any 1-char delimeter
 * if isAZ == TRUE, dx is dA, dy is dZ
 * else dx is dRA, dy is dDecl
 */
bool run_correction(char *dxdy, bool isAZ){
	double dx, dy;
	char *eptr = dxdy;
	if(!myatod(&dx, &eptr) || !*eptr || !*(++eptr)) goto badang;
	if(!myatod(&dy, &eptr)) goto badang;
	DBG("dx: %g, dy: %g", dx, dy);
	if(!testauto() || dx > CORR_MAX_ANGLE || dy > CORR_MAX_ANGLE) return FALSE;
	int32_t oldmode = Sys_Mode;
	if(isAZ){
		dx /= sin(val_Z * AS2R); // transform dA to "telescope coordinates"
#ifndef EMULATION
		//double targA = val_A+dx, targZ = val_Z+dy;
#endif
		ACS_CMD(DoAZcorr(dx, dy));
#ifndef EMULATION
		PRINT(_("Wait for A/Z correction ends"));
		//WAIT_EVENT((fabs(val_A - targA) < CORR_THRES && fabs(val_Z - targZ) < CORR_THRES), 10.);
#endif
	}else{
#ifndef EMULATION
		//double targA = val_Alp+dx, targD = val_Del+dy;
#endif
		ACS_CMD(DoADcorr(dx, dy));
#ifndef EMULATION
		//WAIT_EVENT((fabs(val_Alp - targA) < CORR_THRES && fabs(val_Del - targD) < CORR_THRES), 10.);
#endif
	}
#ifndef EMULATION
	PRINT(_("Wait for correction starts"));
	WAIT_EVENT(Sys_Mode != oldmode, 10.);
	if(tmout) goto atmout;
	PRINT(_("Wait for correction ends"));
	WAIT_EVENT(Sys_Mode == oldmode, 150.);
atmout:
	if(tmout){
		WARNX(_("Can't do correction (or angle is too large)"));
		return FALSE;
	}
#endif
	return TRUE;
badang:
	WARNX(_("Bad format, need \"dx,dy\" in arcseconds"));
	return FALSE;
}

int main(int argc, char **argv){
	check4running(argv, PIDFILE, NULL);
	int retcode = 0;
	initial_setup();
	info_level showinfo = NO_INFO;
#ifndef EMULATION
	passhash pass = {0,0};
#endif
	int needblock = 0, needqueue = 0;
	GP = parce_args(argc, argv);
	assert(GP);
	signal(SIGTERM, signals); // kill (-15) - quit
	signal(SIGHUP, signals);  // hup - quit
	signal(SIGINT, signals);  // ctrl+C - quit
	signal(SIGQUIT, signals); // ctrl+\ - quit
	signal(SIGTSTP, SIG_IGN); // ignore ctrl+Z
	setbuf(stdout, NULL);
	if(GP->getinfo){
		needblock = 1;
		char *infostr = GP->getinfo;
		if(strcmp(infostr, "1") == 0){ // show ALL args
			showinfo = ALL_INFO;
		}else{
			showinfo = get_infolevel(infostr);
		}
	}
	if(showinfo == NO_INFO){
		if(GP->infoargs){
			showinfo = REQUESTED_LIST;
			needblock = 1;
		}else if(GP->getinfo) show_infolevels();
	}
	if(GP->p2move || GP->p2mode || GP->focmove > 0. || GP->eqcrds || GP->horcrds
		|| GP->azrev || GP->telstop || GP->gotoRaDec || GP->gotoAZ || GP->PCSoff
		|| GP->corrAZ || GP->corrRAD){
		needqueue = 1;
	}
	if(needqueue){
		needblock = 1;
	}
	if(needblock){
		if(!get_shm_block(&sdat, ClientSide))
			ERRX(_("Can't find shared memory block"));
	}
	if(needqueue) get_cmd_queue(&ucmd, ClientSide);
	if(needblock){
		if(!check_shm_block(&sdat))
			ERRX(_("There's no connection to BTA!"));
#ifndef EMULATION
		double last = M_time;
		PRINT(_("Test multicast connection\n"));
		WAIT_EVENT((fabs(M_time - last) > 0.02), 5);
		if(tmout)
			ERRX(_("Multicasts stale!"));
		if(needqueue) get_passhash(&pass);
#endif
	}
	if(showinfo != NO_INFO) bta_print(showinfo, GP->infoargs);
	else if(GP->listinfo) bta_print(NO_INFO, NULL); // show arguments available
#define RUN(arg)     do{if(!arg) retcode = 1;}while(0)
#define RUNBLK(arg)  do{if(!arg){retcode = 1; goto restoring;}}while(0)
	if(GP->telstop)      RUN(stop_telescope());
	if(GP->eqcrds)       RUNBLK(setCoords(GP->eqcrds, TRUE));
	else if(GP->horcrds) RUNBLK(setCoords(GP->horcrds, FALSE));
	if(GP->p2move)       RUN(moveP2(GP->p2move));
	if(GP->p2mode)       RUN(setP2mode(GP->p2mode));
	if(GP->focmove > 0.) RUN(moveFocus(GP->focmove));
	if(GP->azrev)        RUN(azreverce());
	if(GP->PCSoff)       RUNBLK(PCS_state(FALSE));
	else if(needqueue)   RUNBLK(PCS_state(TRUE));
	if(GP->gotoRaDec)    RUNBLK(gotopos(TRUE));
	else if(GP->gotoAZ)  RUNBLK(gotopos(FALSE));
	else if(GP->corrAZ)  RUN(run_correction(GP->corrAZ, TRUE));
	else if(GP->corrRAD) RUN(run_correction(GP->corrRAD, FALSE));
#undef RUN
#undef RUNBLK
restoring:
	unlink(PIDFILE);
	restore_console();
	return retcode;
}


/*
 * Добавить:
  * коррекция положения по A/Z или RA/Dec
 */
