/*
 * main.c
 *
 * Copyright 2016 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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
/*

#include <strings.h>
#include <string.h>
#include <pthread.h>



#include "angle_functions.h"

*/
#include <assert.h>
#include <signal.h>
#include <math.h>

#include "bta_control.h"
#include "ch4run.h"
#include "cmdlnopts.h"
#include "usefull_macros.h"
#include "bta_print.h"
#include "bta_shdata.h"

glob_pars *GP = NULL;

#ifndef PIDFILE
#define PIDFILE  "/tmp/bta_control.pid"
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

int main(int argc, char **argv){
    check4running(PIDFILE, NULL);
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
        }else if(GP->getinfo){
            show_infolevels();
            return 0;
        }
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
