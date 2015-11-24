/*
 * angle_functions.c - different functions for angles/times processing in different format
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
#define _GNU_SOURCE 666  // strcasecmp
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <string.h>
//#include <slalib.h>  // SLA library files in  bad format
#include <slamac.h>  // SLA macros
#include "cmdlnopts.h"
#include "bta_shdata.h"
#include "angle_functions.h"
#include "usefull_macros.h"

static char buf[BUFSZ+1];

extern void sla_caldj(int*, int*, int*, double*, int*);
extern void sla_amp(double*, double*, double*, double*, double*, double*);
extern void sla_map(double*, double*, double*, double*, double*,double*, double*, double*, double*, double*);
void slacaldj(int y, int m, int d, double *djm, int *j){
	int iy = y, im = m, id = d;
	sla_caldj(&iy, &im, &id, djm, j);
}
void slaamp(double ra, double da, double date, double eq, double *rm, double *dm ){
	double r = ra, d = da, mjd = date, equi = eq;
	sla_amp(&r, &d, &mjd, &equi, rm, dm);
}
void slamap(double rm, double dm, double pr, double pd,
             double px, double rv, double eq, double date,
             double *ra, double *da){
	double r = rm, d = dm, p1 = pr, p2 = pd, ppx = px, prv = rv, equi = eq, dd = date;
	sla_map(&r, &d, &p1, &p2, &ppx, &prv, &equi, &dd, ra, da);
}

/**
 *  convert angle in seconds into degrees
 * @return angle in range [0..360]
 */
double sec_to_degr(double sec){
	double sign = 1.;
	if(sec < 0.){
		sign = -1.;
		sec = -sec;
	}
	int d = ((int)sec / 3600.);
	sec -= ((double)d) * 3600.;
	d %= 360;
	double angle = sign * (((double)d) + sec / 3600.);
	if(angle < 0.) angle += 360.;
	return (angle);
}

/**
 * Carefull atoi (int32_t)
 * @param num (o)  - returning value (or NULL if you wish only check number) - allocated by user
 * @param str (io) - pointer to string with number must not be NULL (return remain string or NULL if all processed)
 * @return TRUE if conversion sone without errors, FALSE otherwise
 * ALSO return FALSE (but set num to readed integer) and not modify str if find "." after integer
 */
bool myatoi(int32_t *num, char **str){
	long long tmp;
	char *endptr;
	assert(str);
	assert(num);
	assert(*str);
	*num = 0;
	tmp = strtoll(*str, &endptr, 0);
	if(endptr == *str || errno == ERANGE)
		return FALSE;
	if(tmp < INT_MIN || tmp > INT_MAX){
		WARNX(_("Integer out of range"));
		return FALSE;
	}
	*num = (int32_t)tmp;
	if(endptr && *endptr == '.') return FALSE; // double number
	*str = endptr;
	return TRUE;
}

// the same as myatoi but for double
bool myatod(double *num, char **str){
	double tmp;
	char *endptr;
	assert(str);
	assert(num);
	assert(*str);
	tmp = strtod(*str, &endptr);
	if(endptr == *str || errno == ERANGE)
		return FALSE;
	*num = tmp;
	*str = endptr;
	return TRUE;
}

/**
 * try to convert first numbers in string into integer or double
 * @arg i (o)  - readed integer
 * @arg d (o)  - readed double
 * @arg s (io) - string to convert (modified)
 * @return 0 in case of error; 1 if number is integer & 2 if number is double
 */
int getIntDbl(int32_t *i, double *d, char **s){
	//DBG("str: %s", *s);
	int32_t i0; double d0;
	if(!myatoi(&i0, s)){ // bad number or double
		if(!myatod(&d0, s))
			return 0;
		*d = d0;
		//DBG("got dbl: %g", d0);
		return 2;
	}
	*i = i0;
	//DBG("got int: %d", i0);
	return 1;
}

/**
 * Convert string "[+-][DD][MM'][SS.S'']into degrees
 * available formats:
 * dd[.d]       - degrees
 * mm[.m]'      - arc minutes
 * ss[.s]''     - arc seconds
 * dd:mm[.m]    - dd degrees & mm minutes
 * dd:mm:ss[.s] - dd degrees & mm minutes & ss seconds
 * also delimeter can be space, comma or semicolon
 *
 * @param ang (o) - angle in radians or exit with help message
 * @param str (i) - string with angle
 * @return NULL if false or str remainder ('\0' if all string processed) if OK
 */
char *get_degrees(double *ret, char *str){
	if(!ret || !str){
		WARNX(_("Wrong get_radians() argument"));
		return NULL;
	}
	const char delimeters[] = ": ,;";
	char *str_ori = str;
	double sign = 1., degr = 0., min = 0., sec = 0., d;
	int32_t i;
	int res;
	while(*str){ // check sign & omit leading shit
		char c = *str;
		if(c > '0'-1 && c < '9'+1) break;
		++str;
		if(c == '+') break;
		if(c == '-'){ sign = -1.; break;}
	}
	// now check string
	res = getIntDbl(&i, &d, &str);
#define assignresult(dst) do{dst = (res == 2) ? fabs(d) : (double)abs(i);}while(0)
	if(!res || !str) goto badfmt;
	if(*str == 0 || res == 2){ // argument - only one number or double
		assignresult(degr);
		goto allOK;
	}
	// we have something else
	if(str[0] == '\''){ // one number in format "xx'" or "xx''"
		if(str[1] == 0){ // minutes
			assignresult(min);
			goto allOK;
		}else if(str[1] == '\'' && str[2] == 0){ // seconds
			assignresult(sec);
			goto allOK;
		}else goto badfmt;
	}
	if(strchr(delimeters, (int)*str)){ // dd:mm[:ss]?
		assignresult(degr);
		if(res == 2) goto allOK; // we get double - next number isn't our
		++str;
		res = getIntDbl(&i, &d, &str);
		if(!res) goto badfmt;
		assignresult(min);
		if(res == 2) goto allOK;
	}else goto badfmt;
	if(str && *str){ // there's something remain - ss.ss?
		if(!strchr(delimeters, (int)*str)) goto badfmt;
		++str;
		res = getIntDbl(&i, &d, &str);
		if(!res) goto badfmt;
		assignresult(sec);
	}
#undef assignresult
allOK:
	*ret = sign*(degr + min/60. + sec/3600.);
	DBG("Got %g:%g:%g = %g", sign*degr, min, sec, *ret);
	return str;
badfmt:
	WARNX(_("Bad angle format: %s"), str_ori);
	return NULL;
}

/**
 * Calculate apparent place for given coordinates
 * @param r,d (i)            - RA/Decl for given epoch (if --epoch given) or for 2000.0
 *                             (RA in hours, Decl in degrees)
 * @param appRA, appDecl (o) - calculated apparent place
 */
bool calc_AP(double r, double d, double *appRA, double *appDecl){
	double mjd = 51544.5; // mjd2000
	// convert to radians
	r *= DH2R;
	d *= DD2R;
	double ra2000 = r, decl2000 = d; // coordinates for 2000.0
	const double jd0 = 2400000.5; // JD for MJD==0
	// epoch given - calculate it
	if(GP->epoch){
		DBG("Epoch: %s", GP->epoch);
		char *str = GP->epoch;
		if(strcasecmp(str, "now") == 0 || strcmp(str, "1") == 0){ // now
			mjd = JDate - jd0;
		}else{ // check given data
			double year, add = 0.;
			int y, m, d, err;
			if(!myatod(&year, &str)) goto baddate;
			if(fabs(year - ((int)year)) < 0.001){
				y = (int)year; m = 1; d = 1;
			}else{
				time_t t = (year - 1970.) * 31557600.0; // date in UNIX time format
				struct tm tms;
				if(!gmtime_r(&t, &tms)) goto baddate;
				y = 1900 + tms.tm_year;
				m = tms.tm_mon + 1;
				d = tms.tm_mday;
				add = ((double)tms.tm_hour + (double)tms.tm_min/60.0 + tms.tm_sec/3600.0) / 24.;
			}
			slacaldj(y, m, d, &mjd, &err);
			if(err){
				WARNX(_("slacaldj(): Wrong %s!"), (err == 1) ? "year" :
					(err == 2? "month" : "day"));
				return FALSE;
			}
			mjd += add;
		}
		DBG("slaamp(%g, %g, %g, 2000.0, ra, dec)", r,d,mjd);
		slaamp(r, d, mjd, 2000.0, &ra2000, &decl2000);
		DBG("2000: %g, %g", ra2000*DR2H, decl2000*DR2D);
	}
	// proper motion on  R.A./Decl (mas/year)
	double pmra = GP->pmra/1000.*DAS2R, pmdecl = GP->pmdecl/1000.*DAS2R;
	mjd = JDate - jd0;
	slamap(ra2000, decl2000, pmra, pmdecl, 0., 0., 2000.0, mjd, &r, &d);
	DBG("APP: %g, %g", r*DR2H, d*DR2D);
	r *= DR2S;
	d *= DR2AS;
	if(appRA) *appRA = r;
	if(appDecl) *appDecl = d;
	return TRUE;
baddate:
	WARNX(_("Epoch should be \"now\" (or omit) or Julian year (don't use this arg for 2000.0)"));
	return FALSE;
}

char *time_asc(double t){
	int h, min;
	double sec;
	h   = (int)(t/3600.);
	min = (int)((t - (double)h*3600.)/60.);
	sec = t - (double)h*3600. - (double)min*60.;
	h %= 24;
	if(sec>59.99) sec=59.99;
	snprintf(buf, BUFSZ, "%02d:%02d:%05.2f", h,min,sec);
	return buf;
}

char *angle_asc(double a){
	char s;
	int d, min;
	double sec;
	if (a >= 0.)
		s = '+';
	else {
		s = '-'; a = -a;
	}
	d   = (int)(a/3600.);
	min = (int)((a - (double)d*3600.)/60.);
	sec = a - (double)d*3600. - (double)min*60.;
	d %= 360;
	if(sec>59.9) sec=59.9;
	snprintf (buf, BUFSZ, "%c%02d:%02d:%04.1f", s,d,min,sec);
	return buf;
}

char *angle_fmt(double a, char *format){
	char s, *p;
	int d, min, n;
	double sec, msec;
	char *newformat = MALLOC(char, strlen(format) + 3);
	sprintf(newformat, "%s", format);
	if (a >= 0.)
		s = '+';
	else {
		s = '-'; a = -a;
	}
	d   = (int)(a/3600.);
	min = (int)((a - (double)d*3600.)/60.);
	sec = a - (double)d*3600. - (double)min*60.;
	d %= 360;
	if ((p = strchr(format,'.')) == NULL)
		msec=59.;
	else if (*(p+2) == 'f' ) {
		n = *(p+1) - '0';
		msec = 60. - pow(10.,(double)(-n));
	} else
		msec=60.;
	if(sec>msec) sec=msec;
	if (strstr(format,"%c"))
		snprintf(buf, BUFSZ, newformat, s,d,min,sec);
	else
		snprintf(buf, BUFSZ, newformat, d,min,sec);
	free(newformat);
	return buf;
}
