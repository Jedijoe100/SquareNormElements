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

/*******************************************************************/
/*                                                                 */
/*           DECLARATIONS SPECIFIC TO THE PORTABLE VERSION         */
/*                                                                 */
/*******************************************************************/

/*******************************************************************/
/*                                                                 */
/*                    OPERATIONS BY VALUE                          */
/*            f is a pointer to the function called.               */
/*            result is gaffect-ed to last parameter               */
/*                                                                 */
/*******************************************************************/

#define gop1z(f, x, y) \
  STMT_START { GEN __x = (x), __y = (y);\
               pari_sp __av = avma;\
               gaffect(f(__x), __y); avma=__av; } STMT_END
#define gop2z(f, x, y, z) \
  STMT_START { GEN __x = (x), __y = (y), __z = (z);\
               pari_sp __av = avma;\
               gaffect(f(__x,__y), __z); avma=__av; } STMT_END
#define gops2gsz(f, x, s, z) \
  STMT_START { GEN __x = (x), __z = (z);\
               long __s = (s);\
               pari_sp __av = avma;\
               gaffect(f(__x,__s), __z); avma=__av; } STMT_END
#define gops2sgz(f, s, y, z) \
  STMT_START { GEN __y = (y), __z = (z);\
               long __s = (s);\
               pari_sp __av = avma;\
               gaffect(f(__s,__y), __z); avma=__av; } STMT_END
#define gops2ssz(f, x, y, z) \
  STMT_START { GEN __z = (z);\
               long __x = (x), __y = (y);\
               pari_sp __av = avma;\
              gaffect(f(__x,__y), __z); avma=__av; } STMT_END

/* mp.c */

#define cmpis(x,y)     (-cmpsi((y),(x)))
#define cmprs(x,y)     (-cmpsr((y),(x)))
#define cmpri(x,y)     (-cmpir((y),(x)))
#define subis(x,y)     (addsi(-(y),(x)))
#define subrs(x,y)     (addsr(-(y),(x)))

#define divii(a,b)     (dvmdii((a),(b),NULL))
#define resii(a,b)     (dvmdii((a),(b),ONLY_REM))

#define mpshift(x,s)   ((typ(x)==t_INT)?shifti((x),(s)):shiftr((x),(s)))
#define affsz(s,x)     ((typ(x)==t_INT)?affsi((s),(x)):affsr((s),(x)))
#define mptruncz(x,y)  gop1z(mptrunc,(x),(y))
#define mpentz(x,y)    gop1z(mpent,(x),(y))
#define mpaddz(x,y,z)  gop2z(mpadd,(x),(y),(z))
#define addsiz(s,y,z)  gops2sgz(addsi,(s),(y),(z))
#define addsrz(s,y,z)  gops2sgz(addsr,(s),(y),(z))
#define addiiz(x,y,z)  gop2z(addii,(x),(y),(z))
#define addirz(x,y,z)  gop2z(addir,(x),(y),(z))
#define addriz(x,y,z)  gop2z(addir,(y),(x),(z))
#define addrrz(x,y,z)  gop2z(addrr,(x),(y),(z))
#define mpsubz(x,y,z)  gop2z(mpsub,(x),(y),(z))
#define subss(x,y)     (addss(-(y),(x)))
#define subssz(x,y,z)  (addssz((x),-(y),(z)))
#define subsiz(s,y,z)  gops2sgz(subsi,(s),(y),(z))
#define subsrz(s,y,z)  gops2sgz(subsr,(s),(y),(z))
#define subisz(y,s,z)  gops2sgz(addsi,-(s),(y),(z))
#define subrsz(y,s,z)  gops2sgz(addsr,-(s),(y),(z))
#define subiiz(x,y,z)  gop2z(subii,(x),(y),(z))
#define subirz(x,y,z)  gop2z(subir,(x),(y),(z))
#define subriz(x,y,z)  gop2z(subri,(x),(y),(z))
#define subrrz(x,y,z)  gop2z(subrr,(x),(y),(z))
#define mpmulz(x,y,z)  gop2z(mpmul,(x),(y),(z))
#define mulsiz(s,y,z)  gops2sgz(mulsi,(s),(y),(z))
#define mulsrz(s,y,z)  gops2sgz(mulsr,(s),(y),)(z)
#define muliiz(x,y,z)  gop2z(mulii,(x),(y),(z))
#define mulirz(x,y,z)  gop2z(mulir,(x),(y),(z))
#define mulriz(x,y,z)  gop2z(mulir,(y),(x),(z))
#define mulrrz(x,y,z)  gop2z(mulrr,(x),(y),(z))
#define mpdvmdz(x,y,z,t) (dvmdiiz((x),(y),(z),(t))
#define modssz(s,y,z)  gops2ssz(modss,(s),(y),(z))
#define modsiz(s,y,z)  gops2sgz(modsi,(s),(y),(z))
#define modisz(y,s,z)  gops2gsz(modis,(y),(s),(z))
#define ressiz(s,y,z)  gops2sgz(ressi,(s),(y),(z))
#define resisz(y,s,z)  gops2gsz(resis,(y),(s),(z))
#define resssz(s,y,z)  gops2ssz(resss,(s),(y),(z))
#define diviiz(x,y,z)  gop2z(divii,(x),(y),(z))
#define divirz(x,y,z)  gop2z(divir,(x),(y),(z))
#define divisz(x,y,z)  gop2gsz(divis,(x),(y),(z))
#define divriz(x,y,z)  gop2z(divri,(x),(y),(z))
#define divsrz(s,y,z)  gops2sgz(divsr,(s),(y),(z))
#define divrsz(y,s,z)  gops2gsz(divrs,(y),(s),(z))
