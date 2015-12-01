/* Print some BTA NewACS data (or write  to file)
 *
 * copyright: Vladimir Shergin <vsher@sao.ru>
 *
 * Usage:
 *         bta_print [time_step] [file_name]
 * Where:
 *         time_step - writing period in sec., >=1.0
 *                      <1.0 - once and exit (default)
 *         file_name - name of file to write to,
 *                      "-" - stdout (default)
 */
/*
 * bta_print.c
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

/**
 * Modified by E.E. as part of common BTA ACS management system
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/times.h>
#include <ctype.h>

#include <crypt.h>

#include "angle_functions.h"
#include "bta_shdata.h"
#include "bta_print.h"
#include "usefull_macros.h"

typedef struct{
	const char *name;
	const info_level lvl;
}levelstr;

const levelstr const infolevels_str[] = {
	{"coordinates", BASIC_COORDS},
	{"coords", BASIC_COORDS},
	{"extcrds", EXTENDED_COORDS},
	{"morecrds", EXTENDED_COORDS},
	{"meteo", METEO_INFO},
	{"time", TIME_INFO},
	{"acs", ACS_INFO},
	{"system", ACS_INFO},
	{NULL, NO_INFO}
};

typedef struct{
	const char *name;
	const int pos_idx;
}parstr;

// ./btatest -l|awk "{print  \"\tPAR_\"\$1\",\" }"
typedef enum{
	PAR_M_time,
	PAR_S_time,
	PAR_JDate,
	PAR_Tel_Mode,
	PAR_Tel_Focus,
	PAR_Tel_Taget,
	PAR_P2_Mode,
	PAR_PCS_Coeffs,
	PAR_code_KOST,
	PAR_Az_Reverce,
	PAR_Az_EndSw,
	PAR_Zen_EndSw,
	PAR_P2_EndSw,
	PAR_Worm_A,
	PAR_Worm_Z,
	PAR_Lock_Flags,
	PAR_Oil_Pres,
	PAR_Oil_Temp,
	PAR_Oil_Cool_Temp,
	PAR_CurAlpha,
	PAR_CurDelta,
	PAR_SrcAlpha,
	PAR_SrcDelta,
	PAR_InpAlpha,
	PAR_InpDelta,
	PAR_TelAlpha,
	PAR_TelDelta,
	PAR_CurAzim,
	PAR_CurZenD,
	PAR_InpAzim,
	PAR_InpZenD,
	PAR_CurPA,
	PAR_SrcPA,
	PAR_InpPA,
	PAR_TelPA,
	PAR_ValFoc,
	PAR_ValAzim,
	PAR_ValZenD,
	PAR_ValP2,
	PAR_ValDome,
	PAR_DiffAzim,
	PAR_DiffZenD,
	PAR_DiffP2,
	PAR_DiffDome,
	PAR_VelAzim,
	PAR_VelZenD,
	PAR_VelP2,
	PAR_VelPA,
	PAR_VelDome,
	PAR_CorrPCS,
	PAR_Refraction,
	PAR_CorrAlpha,
	PAR_CorrDelta,
	PAR_CorrAzim,
	PAR_CorrZenD,
	PAR_Foc_State,
	PAR_ValTout,
	PAR_ValTind,
	PAR_ValTmir,
	PAR_ValPres,
	PAR_ValWind,
	PAR_Blast10,
	PAR_Blast15,
	PAR_Precipitation,
	PAR_ValHumd,
	PAR_bta_pars_end
}bta_pars;

uint8_t parameters_to_show[PAR_bta_pars_end] = {0};

// ./btatest -l|awk "{print  \"\t\{\\\"\"\$1\"\\\", PAR_\"\$1\"\},\" }"
const parstr const parameters_str[] = {
	{"M_time", PAR_M_time},
	{"S_time", PAR_S_time},
	{"JDate", PAR_JDate},
	{"Tel_Mode", PAR_Tel_Mode},
	{"Tel_Focus", PAR_Tel_Focus},
	{"Tel_Taget", PAR_Tel_Taget},
	{"P2_Mode", PAR_P2_Mode},
	{"PCS_Coeffs", PAR_PCS_Coeffs},
	{"code_KOST", PAR_code_KOST},
	{"Az_Reverce", PAR_Az_Reverce},
	{"Az_EndSw", PAR_Az_EndSw},
	{"Zen_EndSw", PAR_Zen_EndSw},
	{"P2_EndSw", PAR_P2_EndSw},
	{"Worm_A", PAR_Worm_A},
	{"Worm_Z", PAR_Worm_Z},
	{"Lock_Flags", PAR_Lock_Flags},
	{"Oil_Pres", PAR_Oil_Pres},
	{"Oil_Temp", PAR_Oil_Temp},
	{"Oil_Cool_Temp", PAR_Oil_Cool_Temp},
	{"CurAlpha", PAR_CurAlpha},
	{"CurDelta", PAR_CurDelta},
	{"SrcAlpha", PAR_SrcAlpha},
	{"SrcDelta", PAR_SrcDelta},
	{"InpAlpha", PAR_InpAlpha},
	{"InpDelta", PAR_InpDelta},
	{"TelAlpha", PAR_TelAlpha},
	{"TelDelta", PAR_TelDelta},
	{"CurAzim", PAR_CurAzim},
	{"CurZenD", PAR_CurZenD},
	{"InpAzim", PAR_InpAzim},
	{"InpZenD", PAR_InpZenD},
	{"CurPA", PAR_CurPA},
	{"SrcPA", PAR_SrcPA},
	{"InpPA", PAR_InpPA},
	{"TelPA", PAR_TelPA},
	{"ValFoc", PAR_ValFoc},
	{"ValAzim", PAR_ValAzim},
	{"ValZenD", PAR_ValZenD},
	{"ValP2", PAR_ValP2},
	{"ValDome", PAR_ValDome},
	{"DiffAzim", PAR_DiffAzim},
	{"DiffZenD", PAR_DiffZenD},
	{"DiffP2", PAR_DiffP2},
	{"DiffDome", PAR_DiffDome},
	{"VelAzim", PAR_VelAzim},
	{"VelZenD", PAR_VelZenD},
	{"VelP2", PAR_VelP2},
	{"VelPA", PAR_VelPA},
	{"VelDome", PAR_VelDome},
	{"CorrPCS", PAR_CorrPCS},
	{"Refraction", PAR_Refraction},
	{"CorrAlpha", PAR_CorrAlpha},
	{"CorrDelta", PAR_CorrDelta},
	{"CorrAzim", PAR_CorrAzim},
	{"CorrZenD", PAR_CorrZenD},
	{"Foc_State", PAR_Foc_State},
	{"ValTout", PAR_ValTout},
	{"ValTind", PAR_ValTind},
	{"ValTmir", PAR_ValTmir},
	{"ValPres", PAR_ValPres},
	{"ValWind", PAR_ValWind},
	{"Blast10", PAR_Blast10},
	{"Blast15", PAR_Blast15},
	{"Precipitation", PAR_Precipitation},
	{"ValHumd", PAR_ValHumd},
	{NULL, 0}
};


#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

#define R2D  (180./M_PI)     // rad. to degr.
#define D2R  (M_PI/180.)     // degr. to rad.
#define R2S  (648000./M_PI)  // rad. to sec
#define S2R  (M_PI/648000.)  // sec. to rad.
#define S360 (1296000.)    // sec in 360degr


// By google maps: 43.646683 (43 38 48.0588), 41.440681 (41 26 26.4516)
// (real coordinates should be measured relative to mass center, not geoid)
const double longitude = 149189.175;   // SAO longitude 41 26 29.175 (-2:45:45.945)
const double Fi = 157152.7;            // SAO latitude 43 39 12.7
const double cos_fi = 0.7235272793;    // Cos of SAO latitude
const double sin_fi = 0.6902957888;    // Sin  ---  ""  -----

void calc_AZ(double alpha, double delta, double stime, double *az, double *zd){
	double sin_t,cos_t, sin_d,cos_d,  cos_z;
	double t, d, z, a, x, y;

	t = (stime - alpha) * 15.;
	if (t < 0.)
	t += S360;      // +360degr
	t *= S2R;          // -> rad
	d = delta * S2R;
	sincos(t, &sin_t, &cos_t);
	sincos(d, &sin_d, &cos_d);

	cos_z = cos_fi * cos_d * cos_t + sin_fi * sin_d;
	z = acos(cos_z);

	y = cos_d * sin_t;
	x = cos_d * sin_fi * cos_t - cos_fi * sin_d;
	a = atan2(y, x);

	*zd = z * R2S;
	*az = a * R2S;
}

double calc_PA(double alpha, double delta, double stime){
	double sin_t,cos_t, sin_d,cos_d;
	double t, d, p, sp, cp;

	t = (stime - alpha) * 15.;
	if (t < 0.)
		t += S360;      // +360degr
	t *= S2R;          // -> rad
	d = delta * S2R;
	sin_t = sin(t);
	cos_t = cos(t);
	sin_d = sin(d);
	cos_d = cos(d);

	sp = sin_t * cos_fi;
	cp = sin_fi * cos_d - sin_d * cos_fi * cos_t;
	p = atan2(sp, cp);
	if (p < 0.0)
		p += 2.0*M_PI;

	return(p * R2S);
}

void calc_AD(double az, double zd, double stime, double *alpha, double *delta){
	double sin_d, sin_a, cos_a, sin_z, cos_z;
	double t, d, z, a, x, y;
	a = az * S2R;
	z = zd * S2R;
	sin_a = sin(a);
	cos_a = cos(a);
	sin_z = sin(z);
	cos_z = cos(z);

	y = sin_z * sin_a;
	x = cos_a * sin_fi * sin_z + cos_fi * cos_z;
	t = atan2(y, x);
	if (t < 0.0)
		t += 2.0*M_PI;

	sin_d = sin_fi * cos_z - cos_fi * cos_a * sin_z;
	d = asin(sin_d);

	*delta = d * R2S;
	*alpha = (stime - t * R2S / 15.);
	if (*alpha < 0.0)
		*alpha += S360/15.;      // +24h
}

void my_sleep(double dt){
	int nfd;
	struct timeval tv;
	tv.tv_sec = (int)dt;
	tv.tv_usec = (int)((dt - tv.tv_sec)*1000000.);
slipping:
	nfd = select(0, (fd_set *)NULL,(fd_set *)NULL,(fd_set *)NULL, &tv);
	if(nfd < 0) {
		if(errno == EINTR)
		/*On  Linux,  timeout  is  modified to reflect the amount of
		time not slept; most other implementations DO NOT do this!*/
			goto slipping;
		fprintf(stderr,"Error in mydelay(){ select() }. %s\n",strerror(errno));
	}
}

/**
 * print requested information
 * @param lvl      - requested information level
 * @param par_list - list of parameters (any separator) if lvl == REQUESTED_LIST
 * @return 0 in case of some error
 */
int bta_print(info_level lvl, char *par_list){
	int i, verb = 1, sel = 0;
	char *value = NULL;
	DBG("lvl: 0x%X, list: %s", lvl, par_list);
	if(lvl == NO_INFO && par_list) lvl = REQUESTED_LIST;
	else if(lvl == REQUESTED_LIST && !par_list) return 0;
	if(lvl == REQUESTED_LIST){
		parstr *ptr = (parstr*)parameters_str;
		while(ptr->name){
			if(strstr(par_list, ptr->name))
				parameters_to_show[ptr->pos_idx] = 1;
			++ptr;
		}
		lvl = ALL_INFO;
		sel = 1;
	}
	if(lvl == NO_INFO){ // show all parameters but without values
		lvl = ALL_INFO;
		verb = 0;
	}
#define FMSG(par,hlp, ...)  do{if(!sel || parameters_to_show[PAR_ ## par]){printf("\n" #par);if(verb){printf("=\""); \
		printf(__VA_ARGS__);printf("\"");}else printf(" (" hlp ")");}}while(0)
#define SMSG(par,hlp, str)  do{if(!sel || parameters_to_show[PAR_ ## par]){printf("\n" #par); if(verb)printf("=\"%s\"",str);\
		else printf(" (" hlp ")");}}while(0)

/****************************** TIME_INFO *************************************/
	if(lvl & TIME_INFO){
		SMSG(M_time, "mean solar time", time_asc(M_time + DUT1));
#ifdef EE_time
		SMSG(S_time, "mean sidereal time", time_asc(S_time - EE_time));
		FMSG(JDate, "julian date", "%.6f", JDate);
#else
		SMSG(S_time, "mean sidereal time", time_asc(S_time));
#endif
	}

/******************************** ACS_INFO ************************************/
	if(lvl & ACS_INFO){
		if(verb){
			if(Tel_Hardware == Hard_Off) value = "Off";
			else if(Tel_Mode != Automatic) value = "Manual";
			else{
				switch(Sys_Mode){
					default:
					case SysStop    :  value = "Stopped";   break;
					case SysWait    :  value = "Waiting";   break;
					case SysPointAZ :
					case SysPointAD :  value = "Pointing";  break;
					case SysTrkStop :
					case SysTrkStart:
					case SysTrkMove :
					case SysTrkSeek :  value = "Seeking";   break;
					case SysTrkOk   :  value = "Tracking";  break;
					case SysTrkCorr :  value = "Correction";break;
					case SysTest    :  value = "Testing";   break;
				}
			}
		}
		SMSG(Tel_Mode, "telescope mode", value);
		if(verb){
			switch(Tel_Focus){
				default:
				case Prime    :  value = "Prime";     break;
				case Nasmyth1 :  value = "Nasmyth1";  break;
				case Nasmyth2 :  value = "Nasmyth2";  break;
			}
		}
		SMSG(Tel_Focus, "focus mode", value);
		if(verb){
			switch(Sys_Target) {
				default:
				case TagObject   :  value = "Object";   break;
				case TagPosition :  value = "A/Z-Pos."; break;
				case TagNest     :  value = "Nest";     break;
				case TagZenith   :  value = "Zenith";   break;
				case TagHorizon  :  value = "Horizon";  break;
			}
		}
		SMSG(Tel_Taget, "current or last telescope target", value);
		if(verb){
			if(Tel_Hardware == Hard_On)
				switch (P2_State) {
					default:
					case P2_Off   :  value = "Stop";    break;
					case P2_On    :  value = "Track";   break;
					case P2_Plus  :  value = "Move+";   break;
					case P2_Minus :  value = "Move-";   break;
				}
			else value = "Off";
		}
		SMSG(P2_Mode, "P2 rotator mode", value);
		if(!sel || parameters_to_show[PAR_PCS_Coeffs]){
			printf("\nPCS_Coeffs");
			if(verb){
				printf("=\"");
				if(Pos_Corr){
					for(i = 0; i < 8; ++i){
						printf("%.2f%s", PosCor_Coeff[i], (i == 7) ? "\"" : ",");
					}
				}else{
					printf("Off\"");
				}
			}else printf(" (Precision Correction System coefficients");
		}
		if(!sel || parameters_to_show[PAR_code_KOST]){
			printf("\ncode_KOST");
			if(verb){
				printf("=\"0x%04X", code_KOST);
				if(code_KOST){
					printf(": ");
					if(code_KOST & 0x8000) printf("A>0 ");
					if(code_KOST & 0x4000) printf("PowerOn ");
					if(code_KOST & 0x2000) printf("Guiding ");
					if(code_KOST & 0x1000) printf("P2On ");
					if(code_KOST & 0x01F0){
						printf("CorrSpd=");
						if(code_KOST & 0x0010) printf("0.2");
						else if(code_KOST & 0x0020) printf("0.4");
						else if(code_KOST & 0x0040) printf("1.0");
						else if(code_KOST & 0x0080) printf("2.0");
						else if(code_KOST & 0x0100) printf("5.0");
						printf("''/s ");
					}
					if(code_KOST & 0x000F){
						if(code_KOST & 0x0001) printf("Z+");
						else if(code_KOST & 0x0002) printf("Z-");
						else if(code_KOST & 0x0004) printf("A+");
						else if(code_KOST & 0x0008) printf("A-");
					}
				}
				printf("\"");
			}else printf(" (syscodes)");
		}
		if(verb){
			if(Az_Mode) value = "On";
			else value = "Off";
		}
		SMSG(Az_Reverce, "reverce of azimuth direction", value);
		if(!sel || parameters_to_show[PAR_Az_EndSw]){
			printf("\nAz_EndSw");
			if(verb){
				printf("=\"");
				if(switch_A){
					if(switch_A & Sw_minus_A)   printf("A<0 ");
					if(switch_A & Sw_plus240_A) printf("A=+240 ");
					if(switch_A & Sw_minus240_A)printf("A=-240 ");
					if(switch_A & Sw_minus45_A) printf("horizon");
				}else printf("Off");
				printf("\"");
			}else printf(" (Azimuth end-switches)");
		}
		if(!sel || parameters_to_show[PAR_Zen_EndSw]){
			printf("\nZen_EndSw");
			if(verb){
				printf("=\"");
				if(switch_Z){
					if(switch_Z & Sw_0_Z)  printf("Zenith ");
					if(switch_Z & Sw_5_Z)  printf("Z<=5 ");
					if(switch_Z & Sw_20_Z) printf("Z<=20 ");
					if(switch_Z & Sw_60_Z) printf("Z>=60 ");
					if(switch_Z & Sw_80_Z) printf("Z>=80 ");
					if(switch_Z & Sw_90_Z) printf("Z=90 ");
				}else printf("Off");
				printf("\"");
			}else printf(" (Zenith end-switches)");
		}
		if(!sel || parameters_to_show[PAR_P2_EndSw]){
			printf("\nP2_EndSw");
			if(verb){
				printf("=\"");
				if(switch_P){
					if(switch_P & Sw_22_P) printf("22degr ");
					if(switch_P & Sw_89_P) printf("89degr ");
					if(switch_P & Sw_Sm_P) printf("SMOKE");
				}else  printf("Off");
				printf("\"");
			}else printf(" (P2 end-switches)");
		}
		FMSG(Worm_A, "worm A position", "%gmkm", worm_A);
		FMSG(Worm_Z, "worm Z position", "%gmkm", worm_Z);
		if(verb && !sel){
			for(i = 0; i < MesgNum; ++i){
				switch (Sys_Mesg(i).type){
					case MesgInfor  : value = "information"; break;
					case MesgWarn   : value = "warning";     break;
					case MesgFault  : value = "FAULT";       break;
					case MesgLog    : value = "log";         break;
					default         : value = NULL;
				}
				if(!value) continue;
				printf("\nMessage[%d](num=%d, status=\"%s\")=\"%s\"", i,
					Sys_Mesg(i).seq_num, value, Sys_Mesg(i).text);
			}
		}
		if(!sel || parameters_to_show[PAR_Lock_Flags]){
			printf("\nLock_Flags");
			if(verb){
				printf("=\"");
				if(LockFlags){
					if(A_Locked) printf("A ");
					if(Z_Locked) printf("Z ");
					if(P_Locked) printf("P2 ");
					if(F_Locked) printf("F ");
					if(D_Locked) printf("D ");
				}else printf("Off");
				printf("\"");
			}else printf(" (locked motors)");
		}
		FMSG(Oil_Pres, "oil pressure in A,Z & tank (Pa)", "p(A)=%.1f, p(Z)=%.1f, p(tank)=%.1f", PressOilA, PressOilZ, PressOilTank);
		FMSG(Oil_Temp, "oil temperature (degrC)", "%.1f", OilTemper1);
		FMSG(Oil_Cool_Temp, "oil coolant themperature (degrC)", "%.1f", OilTemper2);
	}
/************************* BASIC_COORDS ***************************************/
	if(lvl & BASIC_COORDS){ // basic coordinates: cur, src, inp, tel
		// Target: object, show A/D
		if(Sys_Target == TagObject){
			SMSG(CurAlpha, "current", time_asc(CurAlpha));
			SMSG(CurDelta, "current", angle_asc(CurDelta));
			SMSG(SrcAlpha, "last source position", time_asc(SrcAlpha));
			SMSG(SrcDelta, "last source position", angle_asc(SrcDelta));
			SMSG(InpAlpha, "last input value", time_asc(InpAlpha));
			SMSG(InpDelta, "last input value", angle_asc(InpDelta));
			SMSG(TelAlpha, "real telescope", time_asc(val_Alp));
			SMSG(TelDelta, "real telescope", angle_asc(val_Del));
		}
		SMSG(CurAzim, "current", angle_fmt(tag_A,"%c%03d:%02d:%04.1f"));
		SMSG(CurZenD, "current", angle_fmt(tag_Z,"%02d:%02d:%04.1f"));
		SMSG(InpAzim, "input", angle_fmt(InpAzim,"%c%03d:%02d:%04.1f"));
		SMSG(InpZenD, "input", angle_fmt(InpZdist,"%02d:%02d:%04.1f"));
		SMSG(CurPA, "current", angle_fmt(tag_P,"%03d:%02d:%04.1f"));
		SMSG(SrcPA, "source", angle_fmt(calc_PA(SrcAlpha,SrcDelta,S_time),"%03d:%02d:%04.1f"));
		SMSG(InpPA, "input", angle_fmt(calc_PA(InpAlpha,InpDelta,S_time),"%03d:%02d:%04.1f"));
		SMSG(TelPA, "telescope", angle_fmt(calc_PA(val_Alp, val_Del, S_time),"%03d:%02d:%04.1f"));

		FMSG(ValFoc, "focus value", "%0.2f", val_F);
	}
	if(lvl & EXTENDED_COORDS){
		SMSG(ValAzim, "from encoder", angle_fmt(val_A,"%c%03d:%02d:%04.1f"));
		SMSG(ValZenD, "from encoder", angle_fmt(val_Z,"%02d:%02d:%04.1f"));
		SMSG(ValP2, "from encoder", angle_fmt(val_P,"%03d:%02d:%04.1f"));
		SMSG(ValDome, "from encoder", angle_fmt(val_D,"%c%03d:%02d:%04.1f"));

		SMSG(DiffAzim, "A difference", angle_fmt(Diff_A,"%c%03d:%02d:%04.1f"));
		SMSG(DiffZenD, "Z difference", angle_fmt(Diff_Z,"%c%02d:%02d:%04.1f"));
		SMSG(DiffP2, "P2 difference", angle_fmt(Diff_P,"%c%03d:%02d:%04.1f"));
		SMSG(DiffDome, "DomeAz difference", angle_fmt(val_A-val_D,"%c%03d:%02d:%04.1f"));

		SMSG(VelAzim, "A velocity", angle_fmt(vel_A,"%c%02d:%02d:%04.1f"));
		SMSG(VelZenD, "Z velocity", angle_fmt(vel_Z,"%c%02d:%02d:%04.1f"));
		SMSG(VelP2, "P2 velocity", angle_fmt(vel_P,"%c%02d:%02d:%04.1f"));
		SMSG(VelPA, "object PA velocity", angle_fmt(vel_objP,"%c%02d:%02d:%04.1f"));
		SMSG(VelDome, "DomeAz velocity", angle_fmt(vel_D,"%c%02d:%02d:%04.1f"));

		double corAlp = 0.,corDel = 0.,corA = 0.,corZ = 0.;
		double PCSA = 0., PCSZ = 0., refr = 0.;
		if(verb){
			if(Sys_Mode == SysTrkSeek || Sys_Mode == SysTrkOk || Sys_Mode == SysTrkCorr){
				double curA,curZ,srcA,srcZ;
				corAlp = CurAlpha-SrcAlpha;
				corDel = CurDelta-SrcDelta;
				if(corAlp >  23*3600.) corAlp -= 24*3600.;
				if(corAlp < -23*3600.) corAlp += 24*3600.;
				calc_AZ(SrcAlpha, SrcDelta, S_time, &srcA, &srcZ);
				calc_AZ(CurAlpha, CurDelta, S_time, &curA, &curZ);
				corA = curA - srcA;
				corZ = curZ - srcZ;
				PCSA = tel_cor_A; PCSZ = tel_cor_Z; refr = tel_ref_Z;
			}
		}
		FMSG(CorrPCS, "Point Correction System value",
			"A=%s, Z=%s", angle_fmt(PCSA, "%c%01d:%02d:%04.1f"),
			angle_fmt(PCSZ, "%c%01d:%02d:%04.1f"));
		SMSG(Refraction, "calculated refraction value", angle_fmt(refr, "%c%01d:%02d:%04.1f"));
		SMSG(CorrAlpha, "correction by RA", angle_fmt(corAlp,"%c%01d:%02d:%05.2f"));
		SMSG(CorrDelta, "correction by Decl", angle_fmt(corDel,"%c%01d:%02d:%04.1f"));
		SMSG(CorrAzim, "correction by A", angle_fmt(corA,"%c%01d:%02d:%04.1f"));
		SMSG(CorrZenD, "correction by Z", angle_fmt(corZ,"%c%01d:%02d:%04.1f"));
		if(verb){
			switch(Foc_State){
				case Foc_Hminus :
				case Foc_Hplus  : value = "fast move"; break;
				case Foc_Lminus :
				case Foc_Lplus  : value = "slow move"; break;
				default         : value = "stopped";
			}
		}
		SMSG(Foc_State, "focus motor state", value);
	}
/************************** METEO_INFO ****************************************/
	if(lvl & METEO_INFO){
		FMSG(ValTout, "outern temperature (DegrC)", "%+05.1f", val_T1);
		FMSG(ValTind, "indome temperature (DegrC)", "%+05.1f", val_T2);
		FMSG(ValTmir, "mirror temperature (DegrC)", "%+05.1f", val_T3);
		FMSG(ValPres, "atm. pressure (mmHg)", "%+05.1f", val_B);
		FMSG(ValWind, "wind speed (m/s)", "%04.1f", val_Wnd);
		double w10 = -1., w15 = -1., pre = -1.;
		if(verb){
			if(Wnd10_time > 0.1 && Wnd10_time <= M_time){
				w10 = (M_time-Wnd10_time)/60.;
				w15 = (M_time-Wnd15_time)/60.;
			}
			if(Precip_time > 0.1 && Precip_time <= M_time)
				pre = (M_time-Precip_time)/60.;
		}
		FMSG(Blast10, "wind blast >=10m/s (minutes ago)", "%.1f", w10);
		FMSG(Blast15, "wind blast >=15m/s (minutes ago)", "%.1f", w15);
		FMSG(Precipitation, "last precipitation (minutes ago)", "%.1f", pre);
		FMSG(ValHumd, "Humidity, %%", "%04.1f", val_Hmd);
	}
// FMSG("", "", "", ));
/******************************************************************************/
	printf("\n");
#undef SMSG
#undef FMSG
	return 1;
}

void show_infolevels(){
	printf("\n\t\tList of information levels: \n\tall");
	levelstr *sptr = (levelstr *)infolevels_str;
	info_level lastlvl = NO_INFO;
	int start = 1; // 1 - first message, 0 - first message in line, 2 - next message in line
	for(; ; ++sptr){
		info_level l = sptr->lvl;
		if(lastlvl == l){ // more variants
			if(start != 2){
				printf(" (");
				start = 2;
			}else
				printf(" ,");
		}else{
			if(start == 2) printf(")");
			printf("\n");
			start = 0;
			lastlvl = l;
		}
		if(!sptr->name) break;
		if(start != 2) printf("\t");
		printf(sptr->name);
	}
}

info_level get_infolevel(char* infostr){
	FNAME();
	info_level ret = NO_INFO;
	if(strcasestr(infostr, "all")) return ALL_INFO;
	levelstr *sptr = (levelstr *)infolevels_str;
	for(; sptr->name; ++sptr){
		if(strcasestr(infostr, sptr->name))
			ret |= sptr->lvl;
	}
	return ret;
}
