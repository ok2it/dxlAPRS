/*
 * dxlAPRS toolchain
 *
 * Copyright (C) Christian Rabler <oe5dxl@oevsv.at>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */


#define X2C_int32
#define X2C_index32
#ifndef aprspos_H_
#include "aprspos.h"
#endif
#define aprspos_C_
#ifndef osi_H_
#include "osi.h"
#endif
#include <math.h>



/* get aprs position by OE5DXL */
#define aprspos_arccos acos

#define aprspos_arctan atan

#define aprspos_power pow

/*
PROCEDURE myround(x:REAL):INTEGER;
BEGIN
  IF x>=0.0 THEN x:=x+0.5 ELSE x:=x-0.5 END;
  RETURN VAL(INTEGER, x)
END myround;
*/

extern float aprspos_rad0(float w)
{
   return w*1.7453292519444E-2f;
} /* end rad() */


extern char aprspos_posvalid(struct aprspos_POSITION pos)
{
   return pos.lat!=0.0f || pos.long0!=0.0f;
} /* end posvalid() */

/*
PROCEDURE distance(home,dist:POSITION):REAL;
VAR h:LONGREAL;
BEGIN
  h:=ABS(dist.long-home.long);
  IF (h=0.0) & (dist.lat=home.lat) THEN RETURN 0.0
  ELSE
    IF h>PI2 THEN h:=PI2-h END;
    h:=LongMath.sin(dist.lat)*LongMath.sin(home.lat) + LongMath.cos(dist.lat)
                *LongMath.cos(home.lat)*LongMath.cos(h);
    IF ABS(h)>=1.0 THEN RETURN 0.0 END;
    RETURN 6370.0*LongMath.arccos(h)
  END;
END distance;
*/

extern float aprspos_distance(struct aprspos_POSITION home,
                struct aprspos_POSITION dist)
{
   float y;
   float x;
   x = (float)fabs(dist.long0-home.long0);
   y = (float)fabs(dist.lat-home.lat);
   if (x==0.0f && y==0.0f) return 0.0f;
   else if (x+y<0.04f) {
      /* near */
      x = (float)((double)x*cos((double)((home.lat+dist.lat)*0.5f)));
      return (float)(6370.0*sqrt((double)(x*x+y*y)));
   }
   else {
      /* far */
      if (x>6.283185307f) x = 6.283185307f-x;
      x = (float)(sin((double)dist.lat)*sin((double)home.lat)+cos((double)
                dist.lat)*cos((double)home.lat)*cos((double)x));
      if ((float)fabs(x)>=1.0f) return 0.0f;
      return (float)(6370.0*acos((double)x));
   }
   return 0;
} /* end distance() */


extern float aprspos_azimuth(struct aprspos_POSITION home,
                struct aprspos_POSITION dist)
{
   float ldiff;
   float h;
   ldiff = dist.long0-home.long0;
   if ((ldiff==0.0f || cos((double)home.lat)==0.0) || cos((double)dist.lat)
                ==0.0) {
      if (home.lat<=dist.lat) return 0.0f;
      else return 180.0f;
   }
   else {
      h = (float)(5.729577951472E+1*atan(cos((double)home.lat)
                *(X2C_DIVL(tan((double)home.lat)*cos((double)ldiff)
                -tan((double)dist.lat),sin((double)ldiff)))));
      if (ldiff<0.0f) return h+270.0f;
      else return h+90.0f;
   }
   return 0;
} /* end azimuth() */


static char dig(float * s, char c, float mul)
{
   if (c==' ') return 1;
   /* ambiguity */
   if ((unsigned char)c<'0' || (unsigned char)c>'9') return 0;
   *s = *s+(float)((unsigned long)(unsigned char)c-48UL)*mul;
   return 1;
} /* end dig() */


static char r91(float * s, char c, float mul)
{
   if ((unsigned char)c<'!' || (unsigned char)c>'|') return 0;
   *s = *s+(float)((unsigned long)(unsigned char)c-33UL)*mul;
   return 1;
} /* end r91() */


static char mic(float * s, char c, float mul)
{
   unsigned long n;
   if ((unsigned char)c>='0' && (unsigned char)c<='9') {
      n = (unsigned long)(unsigned char)c-48UL;
   }
   else if ((unsigned char)c>='A' && (unsigned char)c<='J') {
      n = (unsigned long)(unsigned char)c-65UL;
   }
   else if ((c=='K' || c=='L') || c=='Z') n = 0UL;
   else if ((unsigned char)c>='P' && (unsigned char)c<='Y') {
      n = (unsigned long)(unsigned char)c-80UL;
   }
   else return 0;
   *s = *s+(float)n*mul;
   return 1;
} /* end mic() */


static char bit5(char c)
{
   return ((unsigned long)(unsigned char)c/32UL&1) || c=='L';
} /* end bit5() */
/* L is wildcard */


static void komma(char buf[], unsigned long buf_len, unsigned long * p)
{
   while ((*p<buf_len-1 && buf[*p]) && buf[*p]!=',') ++*p;
   ++*p;
} /* end komma() */


static char ns(char c)
{
   c = X2C_CAP(c);
   return c=='N' || c=='S';
} /* end ns() */


static char ew(char c)
{
   c = X2C_CAP(c);
   return c=='E' || c=='W';
} /* end ew() */


static char dig3(char buf[], unsigned long buf_len, unsigned long * s,
                unsigned long * p)
{
   unsigned long i;
   char c;
   *s = 0UL;
   if ((buf[*p]==' ' && buf[*p+1UL]==' ')
                && buf[*p+2UL]==' ' || (buf[*p]=='.' && buf[*p+1UL]=='.')
                && buf[*p+2UL]=='.') {
      *p += 3UL;
      return 1;
   }
   for (i = 0UL; i<=2UL; i++) {
      c = buf[*p];
      ++*p;
      if ((unsigned char)c<'0' || (unsigned char)c>'9') return 0;
      *s = ( *s*10UL+(unsigned long)(unsigned char)c)-48UL;
   } /* end for */
   return 1;
} /* end dig3() */


static char dig6(char buf[], unsigned long buf_len, float * s,
                unsigned long p)
{
   unsigned long i;
   char c;
   char sig;
   *s = 0.0f;
   if (buf[p]=='-') {
      sig = 1;
      i = 1UL;
   }
   else {
      sig = 0;
      i = 0UL;
   }
   do {
      c = buf[p+i];
      if ((unsigned char)c<'0' || (unsigned char)c>'9') return 0;
      *s =  *s*10.0f+(float)((unsigned long)(unsigned char)c-48UL);
      ++i;
   } while (i<6UL);
   if (sig) *s = -*s;
   return 1;
} /* end dig6() */


static char Mapsym(char c)
{
   if ((unsigned char)c>='a' && (unsigned char)c<='j') {
      c = (char)((unsigned long)(unsigned char)c-49UL);
   }
   return c;
} /* end Mapsym() */

#define aprspos_DAO10 2.9088820865741E-7
/* additional digit in lat/log */

#define aprspos_DAO91 3.1997702952315E-8
/* additional 0..90 * 1.1 digits */

#define aprspos_RND 1.454441043287E-6


extern void aprspos_GetPos(struct aprspos_POSITION * pos,
                unsigned long * speed, unsigned long * course,
                long * altitude, char * symb, char * symbt, char buf[],
                unsigned long buf_len, unsigned long micedest,
                unsigned long payload, unsigned long * compos,
                char * postyp)
{
   unsigned long na;
   unsigned long nc;
   unsigned long n;
   unsigned long len;
   unsigned long i;
   char gpst;
   char c;
   char nornd;
   char ok0;
   float scl;
   float sc;
   len = payload;
   nornd = 0;
   /*
   IO.WrStr(" index="); IO.WrCard(len, 1); IO.WrLn;
   */
   while (len<buf_len-1 && (unsigned char)buf[len]>'\015') ++len;
   /*
     speed:=0;
     course:=0;
     symb:=0C;
     symbt:=0C;
   */
   *compos = 0UL;
   c = buf[payload];
   if ((payload+8UL<len && micedest>0UL) && (((c=='\034' || c=='\035')
                || c=='\'') || c=='`')) {
      /* mic-e */
      pos->lat = 0.0f;
      i = micedest;
      ok0 = 1;
      if (!mic(&pos->lat, buf[i], 1.7453292519444E-1f)) ok0 = 0;
      ++i;
      if (!mic(&pos->lat, buf[i], 1.7453292519444E-2f)) ok0 = 0;
      ++i;
      if (!mic(&pos->lat, buf[i], 2.9088820865741E-3f)) ok0 = 0;
      ++i;
      if (!mic(&pos->lat, buf[i], 2.9088820865741E-4f)) ok0 = 0;
      ++i;
      if (!mic(&pos->lat, buf[i], 2.9088820865741E-5f)) ok0 = 0;
      ++i;
      if (!mic(&pos->lat, buf[i], 2.9088820865741E-6f)) ok0 = 0;
      /*    IF ok & (pos.lat<>0.0) THEN pos.lat:=pos.lat+0.005/60.0*RAD END;
                   (* rounding *)  */
      if (bit5(buf[micedest+3UL])) pos->lat = -pos->lat;
      n = (unsigned long)(unsigned char)buf[payload+1UL]; /* long degrees */
      if (n>=28UL && n<=127UL) {
         n -= 28UL;
         if (!bit5(buf[micedest+4UL])) n += 100UL;
         if (n>=180UL && n<=189UL) n -= 80UL;
         else if (n>=190UL && n<=199UL) n -= 190UL;
      }
      else ok0 = 0;
      pos->long0 = (float)n*1.7453292519444E-2f;
      n = (unsigned long)(unsigned char)buf[payload+2UL]; /* long min */
      if (n>=28UL && n<=127UL) {
         n -= 28UL;
         if (n>=60UL) n -= 60UL;
      }
      else ok0 = 0;
      pos->long0 = pos->long0+(float)n*2.9088820865741E-4f;
      n = (unsigned long)(unsigned char)buf[payload+3UL]; /* long min/100 */
      if (n>=28UL && n<=127UL) {
         pos->long0 = pos->long0+(float)(n-28UL)*2.9088820865741E-6f;
      }
      else ok0 = 0;
      /*    IF ok & (pos.long<>0.0)
                THEN pos.long:=pos.long+0.005/60.0*RAD END;
                (* rounding *) */
      if (!bit5(buf[micedest+5UL])) pos->long0 = -pos->long0;
      *speed = 0UL;
      *course = 0UL;
      if (ok0) {
         n = (unsigned long)(unsigned char)buf[payload+4UL];
         if (n>=28UL && n<=127UL) *speed = (n-28UL)*10UL;
         else ok0 = 0;
         n = (unsigned long)(unsigned char)buf[payload+5UL];
         if (n>=28UL && n<=127UL) {
            n -= 28UL;
            *speed += n/10UL;
            *course = (n%10UL)*100UL;
         }
         else ok0 = 0;
         n = (unsigned long)(unsigned char)buf[payload+6UL];
         if (n>=28UL && n<=127UL) {
            n -= 28UL;
            *course += n;
            if (*speed>=800UL) *speed -= 800UL;
            if (*course>=400UL) *course -= 400UL;
            if (*course>=360UL) ok0 = 0;
         }
         else ok0 = 0;
         *symb = buf[payload+7UL];
         *symbt = Mapsym(buf[payload+8UL]);
      }
      *compos = payload+9UL;
      i = payload+9UL;
      if (buf[i+4UL]=='}') {
         /* kenwood ... */
         if (buf[i]==']') {
            ++i; /* TMD700 singularity */
         }
         if (buf[i]=='>') ++i;
      }
      if ((buf[i]=='\035' || buf[i]=='`') || buf[i]=='\'') ++i;
      if (buf[i+3UL]=='}') {
         /* altitude in meters +10000 */
         sc = 0.0f;
         if ((r91(&sc, buf[i], 8281.0f) && r91(&sc, buf[i+1UL],
                91.0f)) && r91(&sc, buf[i+2UL], 1.0f)) {
            *altitude = (long)(unsigned long)X2C_TRUNCC(sc,0UL,
                X2C_max_longcard)-10000L;
            *compos = i+4UL;
         }
         else ok0 = 0;
      }
      *postyp = 'm';
   }
   else if (c=='$') {
      /* gps 4806.9409,N,01134.6219,E */
      ok0 = 0;
      if (buf[payload+1UL]=='G' && buf[payload+2UL]=='P') {
         i = payload+7UL;
         gpst = buf[payload+5UL];
         if ((buf[payload+3UL]=='G' && buf[payload+4UL]=='G') && gpst=='A') {
            komma(buf, buf_len, &i);
            ok0 = 1;
         }
         else if ((buf[payload+3UL]=='R' && buf[payload+4UL]=='M')
                && gpst=='C') {
            komma(buf, buf_len, &i);
            komma(buf, buf_len, &i);
            ok0 = 1;
         }
         else if ((buf[payload+3UL]=='G' && buf[payload+4UL]=='L')
                && gpst=='L') ok0 = 1;
         pos->lat = 0.0f;
         if (!dig(&pos->lat, buf[i], 1.7453292519444E-1f)) ok0 = 0;
         ++i;
         if (!dig(&pos->lat, buf[i], 1.7453292519444E-2f)) ok0 = 0;
         ++i;
         if (!dig(&pos->lat, buf[i], 2.9088820865741E-3f)) ok0 = 0;
         ++i;
         if (!dig(&pos->lat, buf[i], 2.9088820865741E-4f)) ok0 = 0;
         i += 2UL;
         if (!dig(&pos->lat, buf[i], 2.9088820865741E-5f)) ok0 = 0;
         ++i;
         if (!dig(&pos->lat, buf[i], 2.9088820865741E-6f)) ok0 = 0;
         if (dig(&pos->lat, buf[i+1UL], 2.9088820865741E-7f)) {
            ++i;
            nornd = 1;
         }
         if (dig(&pos->lat, buf[i+1UL], 2.9088820865741E-8f)) {
            ++i;
            nornd = 1;
         }
         komma(buf, buf_len, &i);
         if (buf[i]=='S') pos->lat = -pos->lat;
         komma(buf, buf_len, &i);
         pos->long0 = 0.0f;
         if (!dig(&pos->long0, buf[i], 1.7453292519444f)) ok0 = 0;
         ++i;
         if (!dig(&pos->long0, buf[i], 1.7453292519444E-1f)) ok0 = 0;
         ++i;
         if (!dig(&pos->long0, buf[i], 1.7453292519444E-2f)) ok0 = 0;
         ++i;
         if (!dig(&pos->long0, buf[i], 2.9088820865741E-3f)) ok0 = 0;
         ++i;
         if (!dig(&pos->long0, buf[i], 2.9088820865741E-4f)) ok0 = 0;
         i += 2UL;
         if (!dig(&pos->long0, buf[i], 2.9088820865741E-5f)) ok0 = 0;
         ++i;
         if (!dig(&pos->long0, buf[i], 2.9088820865741E-6f)) ok0 = 0;
         if (dig(&pos->long0, buf[i+1UL], 2.9088820865741E-7f)) {
            ++i;
            nornd = 1;
         }
         if (dig(&pos->long0, buf[i+1UL], 2.9088820865741E-8f)) {
            ++i;
            nornd = 1;
         }
         komma(buf, buf_len, &i);
         if (buf[i]=='W') pos->long0 = -pos->long0;
         if (gpst=='C') {
            /* speed and course */
            komma(buf, buf_len, &i);
            sc = 0.0f;
            for (;;) {
               sc = sc*10.0f;
               if ((i>=len || sc>1.E+6f) || !dig(&sc, buf[i], 0.1f)) break;
               ++i;
            }
            *speed = (unsigned long)X2C_TRUNCC(X2C_DIVR(sc,1.852f)+0.5f,0UL,
                X2C_max_longcard);
            komma(buf, buf_len, &i);
            sc = 0.0f;
            for (;;) {
               sc = sc*10.0f;
               if ((i>=len || sc>360.0f) || !dig(&sc, buf[i], 0.1f)) break;
               ++i;
            }
            *course = (unsigned long)X2C_TRUNCC(sc,0UL,
                X2C_max_longcard)%360UL;
         }
         else if (gpst=='A') {
            /* altitude */
            komma(buf, buf_len, &i);
            komma(buf, buf_len, &i);
            komma(buf, buf_len, &i);
            komma(buf, buf_len, &i);
            sc = 0.0f;
            c = buf[i];
            if (c=='-') ++i;
            for (;;) {
               sc = sc*10.0f;
               if ((i>=len || sc>1.E+6f) || !dig(&sc, buf[i], 0.1f)) break;
               ++i;
            }
            komma(buf, buf_len, &i);
            if (c=='-') sc = -sc;
            if (buf[i]=='M') {
               *altitude = (long)X2C_TRUNCI(sc,X2C_min_longint,
                X2C_max_longint);
            }
         }
         while (i<len && buf[i]!='*') ++i;
         if (i+3UL<len) *compos = i+3UL;
         else *compos = len;
         *postyp = 'g';
      }
   }
   else {
      /* !4915.10N/01402.20E& */
      /* xnnnn.nnNxnnnnn.nnEx */
      ok0 = 0;
      pos->lat = 0.0f;
      pos->long0 = 0.0f;
      i = 0UL;
      if (buf[payload]=='!' || buf[payload]=='=') i = payload+1UL;
      else if ((buf[payload]=='/' || buf[payload]=='@')
                && ((buf[payload+7UL]=='z' || buf[payload+7UL]=='h')
                || buf[payload+7UL]=='/')) i = payload+8UL;
      else if (buf[payload]==';') i = payload+18UL;
      else if (buf[payload]==')') {
         /* skip item */
         i = payload+4UL;
         while ((i<len && buf[i]!='!') && buf[i]!='_') ++i;
         ++i;
      }
      if (i>0UL) {
         if ((unsigned char)buf[i]>='0' && (unsigned char)buf[i]<='9') {
            if ((((i+17UL<=len && buf[i+4UL]=='.') && buf[i+14UL]=='.')
                && ns(buf[i+7UL])) && ew(buf[i+17UL])) {
               /* not compressed */
               ok0 = 1;
               if (!dig(&pos->lat, buf[i], 1.7453292519444E-1f)) ok0 = 0;
               ++i;
               if (!dig(&pos->lat, buf[i], 1.7453292519444E-2f)) ok0 = 0;
               ++i;
               if (!dig(&pos->lat, buf[i], 2.9088820865741E-3f)) ok0 = 0;
               if ((unsigned char)buf[i]>='6') ok0 = 0;
               ++i;
               if (!dig(&pos->lat, buf[i], 2.9088820865741E-4f)) ok0 = 0;
               i += 2UL;
               if (!dig(&pos->lat, buf[i], 2.9088820865741E-5f)) ok0 = 0;
               ++i;
               if (!dig(&pos->lat, buf[i], 2.9088820865741E-6f)) ok0 = 0;
               ++i;
               if (X2C_CAP(buf[i])=='S') pos->lat = -pos->lat;
               else if (X2C_CAP(buf[i])!='N') {
                  ok0 = 0;
               }
               ++i;
               *symbt = buf[i];
               ++i;
               if (!dig(&pos->long0, buf[i], 1.7453292519444f)) ok0 = 0;
               ++i;
               if (!dig(&pos->long0, buf[i], 1.7453292519444E-1f)) ok0 = 0;
               ++i;
               if (!dig(&pos->long0, buf[i], 1.7453292519444E-2f)) ok0 = 0;
               ++i;
               if (!dig(&pos->long0, buf[i], 2.9088820865741E-3f)) ok0 = 0;
               if ((unsigned char)buf[i]>='6') ok0 = 0;
               ++i;
               if (!dig(&pos->long0, buf[i], 2.9088820865741E-4f)) ok0 = 0;
               i += 2UL;
               if (!dig(&pos->long0, buf[i], 2.9088820865741E-5f)) ok0 = 0;
               ++i;
               if (!dig(&pos->long0, buf[i], 2.9088820865741E-6f)) ok0 = 0;
               ++i;
               if (X2C_CAP(buf[i])=='W') pos->long0 = -pos->long0;
               else if (X2C_CAP(buf[i])!='E') ok0 = 0;
               ++i;
               *symb = buf[i];
               ++i;
               *compos = i;
               if ((i+7UL<=len && dig3(buf, buf_len, &nc,
                &i)) && buf[i]=='/') {
                  /* ccc/sss */
                  ++i;
                  if (dig3(buf, buf_len, &n, &i)) {
                     *speed = n;
                     if (nc>0UL && nc<=360UL) *course = nc%360UL;
                     *compos = i;
                  }
               }
               *postyp = 'g';
            }
         }
         else if (i+11UL+2UL*(unsigned long)(buf[i+10UL]!=' ')<=len) {
            /* compressed pos*/
            ok0 = 1;
            *symbt = Mapsym(buf[i]);
            ++i;
            if (!r91(&pos->lat, buf[i], 7.53571E+5f)) ok0 = 0;
            ++i;
            if (!r91(&pos->lat, buf[i], 8281.0f)) ok0 = 0;
            ++i;
            if (!r91(&pos->lat, buf[i], 91.0f)) ok0 = 0;
            ++i;
            if (!r91(&pos->lat, buf[i], 1.0f)) ok0 = 0;
            pos->lat = 1.57079632675f-pos->lat*4.5818065764596E-8f;
            ++i;
            if (!r91(&pos->long0, buf[i], 7.53571E+5f)) ok0 = 0;
            ++i;
            if (!r91(&pos->long0, buf[i], 8281.0f)) ok0 = 0;
            ++i;
            if (!r91(&pos->long0, buf[i], 91.0f)) ok0 = 0;
            ++i;
            if (!r91(&pos->long0, buf[i], 1.0f)) ok0 = 0;
            pos->long0 = pos->long0*9.1636131529192E-8f-3.1415926535f;
            *symb = buf[i+1UL];
            if (ok0) {
               if (buf[i+2UL]=='}') {
               }
               else if (((unsigned long)(unsigned char)buf[i+4UL]/8UL&3UL)
                ==2UL) {
                  /* radio range */
                  /* T byte says GGA so we have altitude */
                  na = (unsigned long)(unsigned char)buf[i+2UL];
                /* sc is altitude */
                  n = (unsigned long)(unsigned char)buf[i+3UL];
                  if (((na>=33UL && na<=127UL) && n>=33UL) && n<=127UL) {
                     *altitude = (long)(unsigned long)
                X2C_TRUNCC(0.3048*pow(1.002,
                (double)(float)(((na-33UL)*91UL+n)-33UL)),0UL,
                X2C_max_longcard); /* in feet */
                  }
               }
               else {
                  n = (unsigned long)(unsigned char)buf[i+2UL];
                /* speed course */
                  if (n>=33UL && n<=127UL) {
                     n -= 33UL;
                     if (n<=89UL) {
                        *course = n*4UL;
                        n = (unsigned long)(unsigned char)buf[i+3UL];
                        if (n>=33UL && n<=127UL) {
                           *speed = (unsigned long)X2C_TRUNCC(pow(1.08,
                (double)(float)(n-33UL)),0UL,X2C_max_longcard);
                        }
                     }
                  }
               }
               *compos = i+5UL;
               nornd = 1;
            }
            *postyp = 'c';
         }
      }
   }
   if (*compos>0UL) {
      i = *compos; /* look for altitude in comment /A= */
      for (;;) {
         if (i+9UL>len) break;
         /*      n:=i+3;   */
         /*      IF (buf[i]="/") & (buf[i+1]="A") & (buf[i+2]="=") & dig3(na,
                 n) & dig3(nc, n) THEN */
         if (((buf[i]=='/' && buf[i+1UL]=='A') && buf[i+2UL]=='=')
                && dig6(buf, buf_len, &sc, i+3UL)) {
            /*      sc:=FEET*FLOAT(na*1000 + nc);  */
            /*      IF sc<10000000.0 THEN altitude:=TRUNC(sc) END; */
            *altitude = (long)X2C_TRUNCI(sc*0.3048f,X2C_min_longint,
                X2C_max_longint);
            if (i==*compos) *compos += 9UL;
            break;
         }
         ++i;
      }
      if (pos->long0!=0.0f || pos->lat!=0.0f) {
         i = *compos; /* look for !DAO! precision extension */
         for (;;) {
            if (i+5UL>len) break;
            if (buf[i]=='!' && buf[i+4UL]=='!') {
               c = buf[i+1UL];
               sc = 0.0f;
               scl = 0.0f;
               if ((((unsigned char)c>='A' && (unsigned char)c<='Z')
                && dig(&sc, buf[i+2UL], 2.9088820865741E-7f)) && dig(&scl,
                buf[i+3UL], 2.9088820865741E-7f)) {
                  if (pos->lat<0.0f) pos->lat = pos->lat-sc;
                  else pos->lat = pos->lat+sc;
                  if (pos->long0<0.0f) pos->long0 = pos->long0-scl;
                  else pos->long0 = pos->long0+scl;
                  if (i==*compos) *compos += 5UL;
                  nornd = 1;
               }
               else if ((((unsigned char)c>='a' && (unsigned char)c<='z')
                && r91(&sc, buf[i+2UL], 3.1997702952315E-8f)) && r91(&scl,
                buf[i+3UL], 3.1997702952315E-8f)) {
                  if (pos->lat<0.0f) pos->lat = pos->lat-sc;
                  else pos->lat = pos->lat+sc;
                  if (pos->long0<0.0f) pos->long0 = pos->long0-scl;
                  else pos->long0 = pos->long0+scl;
                  if (i==*compos) *compos += 5UL;
                  nornd = 1;
               }
               if ((unsigned char)*postyp>0) *postyp = X2C_CAP(*postyp);
               break;
            }
            ++i;
         }
      }
   }
   if (ok0 && !nornd) {
      /* rounding */
      if (pos->lat<0.0f) pos->lat = pos->lat-1.454441043287E-6f;
      else if (pos->lat>0.0f) pos->lat = pos->lat+1.454441043287E-6f;
      if (pos->long0<0.0f) pos->long0 = pos->long0-1.454441043287E-6f;
      else if (pos->long0>0.0f) pos->long0 = pos->long0+1.454441043287E-6f;
   }
   ok0 = ((ok0 && (pos->long0!=0.0f || pos->lat!=0.0f)) && (float)
                fabs(pos->long0)<3.1415926535f) && (float)fabs(pos->lat)
                <1.57079632675f;
   if (!ok0) {
      pos->long0 = 0.0f;
      pos->lat = 0.0f;
   }
} /* end GetPos() */

#define aprspos_SYMT1 "BBBCBDBEBFBGBHBIBJBKBLBMBNBOBPP0P1P2P3P4P5P6P7P8P9MRMS\
MTMUMVMWMXPAPBPCPDPEPFPGPHPIPJPKPLPMPNPOPPPQPRPSPTPUPVPWPXPYPZHSHTHUHVHWHXLAL\
BLCLDLELFLGLHLILJLKLLLMLNLOLPLQLRLSLTLULVLWLXLYLZJ1J2J3J4"

#define aprspos_SYMT2 "OBOCODOEOFOGOHOIOJOKOLOMONOOOPA0A1A2A3A4A5A6A7A8A9NRNS\
NTNUNVNWNXAAABACADAEAFAGAHAIAJAKALAMANAOAPAQARASATAUAVAWAXAYAZDSDTDUDVDWDXSAS\
BSCSDSESFSGSHSISJSKSLSMSNSOSPSQSRSSSTSUSVSWSXSYSZQ1Q2Q3Q4"

#define aprspos_LEN 94


extern void aprspos_GetSym(char d[], unsigned long d_len, char * symb,
                char * symt)
/* symbol out of destination call */
{
   unsigned long n;
   if (((d[0UL]=='G' && d[1UL]=='P') && d[2UL]=='S')
                && (d[3UL]=='C' || d[3UL]=='E')) {
      if ((((unsigned char)d[4UL]>='0' && (unsigned char)d[4UL]<='9')
                && (unsigned char)d[5UL]>='0') && (unsigned char)d[5UL]<='9')
                 {
         n = (((unsigned long)(unsigned char)d[4UL]-48UL)
                *10UL+(unsigned long)(unsigned char)d[5UL])-48UL;
         if (n>=1UL && n<=94UL) {
            *symb = (char)(32UL+n);
            if (d[3UL]=='C') *symt = '/';
            else *symt = '\\';
         }
      }
   }
   else if (((d[0UL]=='G' && d[1UL]=='P')
                && d[2UL]=='S' || (d[0UL]=='S' && d[1UL]=='P')
                && d[2UL]=='C') || (d[0UL]=='S' && d[1UL]=='Y')
                && d[2UL]=='M') {
      if (d[3UL]==0) {
         *symb = '/';
         *symt = '/';
      }
      else {
         n = 0UL;
         for (;;) {
            if (n>=94UL) break;
            if (d[3UL]=="BBBCBDBEBFBGBHBIBJBKBLBMBNBOBPP0P1P2P3P4P5P6P7P8P9MR\
MSMTMUMVMWMXPAPBPCPDPEPFPGPHPIPJPKPLPMPNPOPPPQPRPSPTPUPVPWPXPYPZHSHTHUHVHWHXL\
ALBLCLDLELFLGLHLILJLKLLLMLNLOLPLQLRLSLTLULVLWLXLYLZJ1J2J3J4"[n*2UL] && d[4UL]
                =="BBBCBDBEBFBGBHBIBJBKBLBMBNBOBPP0P1P2P3P4P5P6P7P8P9MRMSMTMU\
MVMWMXPAPBPCPDPEPFPGPHPIPJPKPLPMPNPOPPPQPRPSPTPUPVPWPXPYPZHSHTHUHVHWHXLALBLCL\
DLELFLGLHLILJLKLLLMLNLOLPLQLRLSLTLULVLWLXLYLZJ1J2J3J4"[n*2UL+1UL]) {
               *symb = (char)(33UL+n);
               *symt = '/';
               break;
            }
            if (d[3UL]=="OBOCODOEOFOGOHOIOJOKOLOMONOOOPA0A1A2A3A4A5A6A7A8A9NR\
NSNTNUNVNWNXAAABACADAEAFAGAHAIAJAKALAMANAOAPAQARASATAUAVAWAXAYAZDSDTDUDVDWDXS\
ASBSCSDSESFSGSHSISJSKSLSMSNSOSPSQSRSSSTSUSVSWSXSYSZQ1Q2Q3Q4"[n*2UL] && d[4UL]
                =="OBOCODOEOFOGOHOIOJOKOLOMONOOOPA0A1A2A3A4A5A6A7A8A9NRNSNTNU\
NVNWNXAAABACADAEAFAGAHAIAJAKALAMANAOAPAQARASATAUAVAWAXAYAZDSDTDUDVDWDXSASBSCS\
DSESFSGSHSISJSKSLSMSNSOSPSQSRSSSTSUSVSWSXSYSZQ1Q2Q3Q4"[n*2UL+1UL]) {
               *symb = (char)(33UL+n);
               *symt = '\\';
               if ((unsigned char)d[5UL]>='0' && (unsigned char)
                d[5UL]<='9' || (unsigned char)d[5UL]>='A' && (unsigned char)
                d[5UL]<='Z') *symt = d[5UL];
               break;
            }
            ++n;
         }
      }
   }
} /* end GetSym() */


extern void aprspos_BEGIN(void)
{
   static int aprspos_init = 0;
   if (aprspos_init) return;
   aprspos_init = 1;
   osi_BEGIN();
}
