/* * * * *
 *  AzTimer.hpp 
 *  Copyright (C) 2011, 2012 Rie Johnson
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * * * * */

#ifndef _AZ_TIMER_HPP_
#define _AZ_TIMER_HPP_

#include "AzUtil.hpp"

class AzTimer {
public:
  int chk;  /* next check point */
  int inc;  /* increment: negative means no checking */

  AzTimer() : chk(-1), inc(-1) {}
  ~AzTimer() {}

  inline void reset(int inp_inc) {
    chk = -1; 
    inc = inp_inc; 
    if (inc > 0) {
      chk = inc;   
    }
  }

  inline bool ringing(bool isRinging, int inp) { /* timer is ringing */
    if (isRinging) return true; 

    if (chk > 0 && inp >= chk) {
      while(chk <= inp) {
        chk += inc; /* next check point */
      }
      return true; 
    }
    return false;
  }

  inline bool reachedMax(int inp, 
                    const char *msg, 
                    const AzOut &out) const {
    bool yes_no = reachedMax(inp); 
    if (yes_no) {
      AzTimeLog::print(msg, " reached max", out); 
    }
    return yes_no; 
  }
  inline bool reachedMax(int inp) const {
    if (chk > 0 && inp >= chk) return true; 
    else                       return false; 
  }
}; 

#endif

