/* $Id$

Copyright (C) 2000  The PARI group.

This file is part of the PARI/GP package.

PARI/GP is free software; you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation. It is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY WHATSOEVER.

Check the License for details. You should have received a copy of it, along
with the package; see the file 'COPYING'. If not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. */

/* This file defines some "level 0" kernel functions for SPARC V8 */
/* These functions can be inline, with gcc                        */
/* If not gcc, they are defined externally with "level0.s"        */
/* This file is common to SuperSparc and MicroSparc               */
/*                                                                */
/* These following symbols can be inlined                         */
/* overflow hiremainder                                           */
/* addll addllx subll subllx shiftl shiftlr mulll addmul          */
/*                                                                */
/* These functions are always in level0.s                         */

extern ulong hiremainder, overflow;
extern int  bfffo(ulong x);

#ifndef ASMINLINE
#define LOCAL_OVERFLOW
#define LOCAL_HIREMAINDER
extern long addll(ulong a, ulong b);
extern long addllx(ulong a, ulong b);
extern long subll(ulong a, ulong b);
extern long subllx(ulong a, ulong b);
extern long shiftl(ulong x, ulong y);
extern long shiftlr(ulong x, ulong y);
extern long mulll(ulong x, ulong y);
extern long addmul(ulong x, ulong y);
extern long divll(ulong x, ulong y);

#else /* ASMINLINE */

#define LOCAL_HIREMAINDER  ulong hiremainder
#define LOCAL_OVERFLOW     ulong overflow

#define addll(a,b) \
({ ulong __value, __arg1 = (a), __arg2 = (b); \
   __asm__ ( "addcc %2,%3,%0; \
          addx  %%g0,%%g0,%1" \
	 : "=r" (__value), "=r" (overflow) \
	 : "r" (__arg1), "r" (__arg2) \
         : "cc"); \
__value; })								

#define addllx(a,b) \
({ ulong __value, __arg1 = (a), __arg2 = (b); \
   __asm__ ( "subcc %%g0,%1,%%g0; \
          addxcc %2,%3,%0; \
          addx  %%g0,%%g0,%1" \
	 : "=r" (__value), "=r" (overflow) \
	 : "r" (__arg1), "r" (__arg2), "1" (overflow) \
         : "cc"); \
__value; })								

#define subll(a,b) \
({ ulong __value, __arg1 = (a), __arg2 = (b); \
   __asm__ ( "subcc %2,%3,%0; \
          addx  %%g0,%%g0,%1" \
	 : "=r" (__value), "=r" (overflow) \
	 : "r" (__arg1), "r" (__arg2) \
         : "cc"); \
__value; })								

#define subllx(a,b) \
({ ulong __value, __arg1 = (a), __arg2 = (b); \
   __asm__ ( "subcc %%g0,%1,%%g0; \
          subxcc %2,%3,%0; \
          addx  %%g0,%%g0,%1" \
	 : "=r" (__value), "=r" (overflow) \
	 : "r" (__arg1), "r" (__arg2), "1" (overflow) \
         : "cc"); \
__value; })								

#if 0
#define shiftl(a,b) \
({ ulong __value, __arg1 = (a), __arg2 = (b); \
   __asm__ ( "neg %3,%%o4; \
          srl %2,%%o4,%1; \
          sll %2,%3,%0" \
	 : "=r" (__value), "=r" (hiremainder) \
	 : "r" (__arg1), "r" (__arg2) \
         : "%o4"); \
__value; })								

#define shiftlr(a,b) \
({ ulong __value, __arg1 = (a), __arg2 = (b); \
   __asm__ ( "neg %3,%%o4; \
          sll %2,%%o4,%1; \
          srl %2,%3,%0" \
	 : "=r" (__value), "=r" (hiremainder) \
	 : "r" (__arg1), "r" (__arg2) \
         : "%o4"); \
__value; }) 			
#endif

#define mulll(a,b) \
({ ulong __value, __arg1 = (a), __arg2 = (b); \
   __asm__ ( "umul %2,%3,%0; \
          rd  %%y,%1" \
	 : "=r" (__value), "=r" (hiremainder) \
	 : "r" (__arg1), "r" (__arg2));	\
__value;})								

#define addmul(a,b) \
({ ulong __value, __arg1 = (a), __arg2 = (b), __tmp; \
   __asm__ ( "umul %3,%4,%0; \
          rd  %%y,%2; \
          addcc %0,%1,%0; \
          addx %%g0,%2,%1" \
	 : "=&r" (__value), "=&r" (hiremainder), "=&r" (__tmp) \
	 : "r" (__arg1), "r" (__arg2), "1" (hiremainder) \
         : "cc");	\
__value;})								
#endif /* ASMINLINE */
