/*
 * bta_control.h - main definitions
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
#ifndef __BTA_CONTROL_H__
#define __BTA_CONTROL_H__

// timeout for system messages process - 3 seconds
#define WAITING_TMOUT   (3.)
// end-switches position (in degr); prohibited angles are in range [P2_LOW_ES .. P2_HIGH_ES]
#define P2_LOW_ES       (21.0)
#define P2_HIGH_ES      (90.0)
// minimal P2 moving time
#define P2_MINTIME      (4.5)
// fast speed - 10arcmin per second
#define P2_FAST_SPEED   (600.)
// this value will be substituted from calculated rotation time in case of moving on large speed
#define P2_FAST_T_CORR  (1.5)
// angle threshold (for p2 move) in degrees
#define P2_ANGLE_THRES  (0.01)
#define FOCUS_THRES     (0.03)
// max angles for correction of telescope (5' = 300'')
#define CORR_MAX_ANGLE  (300)
// correction threshold (arcsec)
//#define CORR_THRES      (0.1)
// input coordinates threshold (arcsec)
#define INPUT_COORDS_THRES (1.)

#endif // __BTA_CONTROL_H__
