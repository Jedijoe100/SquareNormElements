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

/*********************************************************************/
/**                                                                 **/
/**                     ARITHMETIC FUNCTIONS                        **/
/**                        (second part)                            **/
/**                                                                 **/
/*********************************************************************/
#include "pari.h"

extern GEN arith_proto(long f(GEN), GEN x, int do_error);
extern GEN arith_proto2(long f(GEN,GEN), GEN x, GEN n);
extern GEN arith_proto2gs(long f(GEN,long), GEN x, long y);
extern GEN garith_proto(GEN f(GEN), GEN x, int do_error);
extern GEN garith_proto2gs(GEN f(GEN,long), GEN x, long y);
extern GEN garith_proto3ggs(GEN f(GEN,GEN,long), GEN x, GEN y, long z);

#define sqru(i) (muluu((i),(i)))

/***********************************************************************/
/**                                                                   **/
/**                          PRIME NUMBERS                            **/
/**                                                                   **/
/***********************************************************************/

GEN
prime(long n)
{
  byteptr p = diffptr;
  ulong prime = 0;

  if (n <= 0) err(talker, "n-th prime meaningless if n = %ld",n);
  while (n--) NEXT_PRIME_VIADIFF_CHECK(prime,p);
  return utoi(prime);
}

long
pith(long n)
{
  byteptr p = diffptr;
  ulong prime = 0, res = 0;

  if (n <= 0) err(talker, "pith meaningless if n = %ld",n);
  maxprime_check((ulong)n);
  while (prime<=(ulong)n) { res++; NEXT_PRIME_VIADIFF(prime,p); }
  return res-1;
}

GEN
primes(long n)
{
  byteptr p = diffptr;
  ulong prime = 0;
  GEN y,z;

  if (n < 0) n = 0;
  z = y = cgetg(n+1,t_VEC);
  while (n--)
  {
    NEXT_PRIME_VIADIFF_CHECK(prime,p);
    *++z = lutoi(prime);
  }
  return y;
}

/* Building/Rebuilding the diffptr table. The actual work is done by the
 * following two subroutines;  the user entry point is the function
 * initprimes() below.  initprimes1() is the old algorithm, called when
 * maxnum (size) is moderate. */
static byteptr
initprimes1(ulong size, long *lenp, long *lastp)
{
  long k;
  byteptr q, r, s, fin, p = (byteptr) gpmalloc(size+2);

  memset(p, 0, size + 2); fin = p + size;
  for (r=q=p,k=1; r<=fin; )
  {
    do { r+=k; k+=2; r+=k; } while (*++q);
    for (s=r; s<=fin; s+=k) *s = 1;
  }
  r = p; *r++ = 2; *r++ = 1; /* 2 and 3 */
  for (s=q=r-1; ; s=q)
  {
    do q++; while (*q);
    if (q > fin) break;
    *r++ = (unsigned char) ((q-s) << 1);
  }
  *r++ = 0;
  *lenp = r - p;
  *lastp = ((s - p) << 1) + 1;
  return (byteptr) gprealloc(p,r-p);
}

/*  Timing in ms (Athlon/850; reports 512K of secondary cache; looks
    like there is 64K of quickier cache too).

      arena|    30m     100m    300m    1000m    2000m  <-- primelimit
      =================================================
      16K       1.1053  1.1407  1.2589  1.4368   1.6086 
      24K       1.0000  1.0625  1.1320  1.2443   1.3095 
      32K       1.0000  1.0469  1.0761  1.1336   1.1776 
      48K       1.0000  1.0000  1.0254  1.0445   1.0546 
      50K       1.0000  1.0000  1.0152  1.0345   1.0464 
      52K       1.0000  1.0000  1.0203  1.0273   1.0362 
      54K       1.0000  1.0000  1.0812  1.0216   1.0281 
      56K       1.0526  1.0000  1.0051  1.0144   1.0205 
      58K       1.0000  1.0000  1.0000  1.0086   1.0123 
      60K       0.9473  0.9844  1.0051  1.0014   1.0055 
      62K       1.0000  0.9844  0.9949  0.9971   0.9993 
      64K       1.0000  1.0000  1.0000  1.0000   1.0000 
      66K       1.2632  1.2187  1.2183  1.2055   1.1953 
      68K       1.4211  1.4844  1.4721  1.4425   1.4188 
      70K       1.7368  1.7188  1.7107  1.6767   1.6421 
      72K       1.9474  1.9531  1.9594  1.9023   1.8573 
      74K       2.2105  2.1875  2.1827  2.1207   2.0650 
      76K       2.4211  2.4219  2.4010  2.3305   2.2644 
      78K       2.5789  2.6250  2.6091  2.5330   2.4571 
      80K       2.8421  2.8125  2.8223  2.7213   2.6380 
      84K       3.1053  3.1875  3.1776  3.0819   2.9802 
      88K       3.5263  3.5312  3.5228  3.4124   3.2992 
      92K       3.7895  3.8438  3.8375  3.7213   3.5971 
      96K       4.0000  4.1093  4.1218  3.9986   3.9659 
      112K      4.3684  4.5781  4.5787  4.4583   4.6115 
      128K      4.7368  4.8750  4.9188  4.8075   4.8997 
      192K      5.5263  5.7188  5.8020  5.6911   5.7064 
      256K      6.0000  6.2187  6.3045  6.1954   6.1033 
      384K      6.7368  6.9531  7.0405  6.9181   6.7912 
      512K      7.3158  7.5156  7.6294  7.5000   7.4654 
      768K      9.1579  9.4531  9.6395  9.5014   9.1075 
      1024K    10.368  10.7497 10.9999 10.878   10.8201
      1536K    12.579  13.3124 13.7660 13.747   13.4739
      2048K    13.737  14.4839 15.0509 15.151   15.1282
      3076K    14.789  15.5780 16.2993 16.513   16.3365

    Now the same number relative to the model

    (1 + 0.36*sqrt(primelimit)/arena) * (arena <= 64 ? 1.05 : (arena-64)**0.38)

     [SLOW2_IN_ROOTS = 0.36, ALPHA = 0.38]

      arena|    30m     100m    300m    1000m    2000m  <-- primelimit
      =================================================
        16K    1.014    0.9835  0.9942  0.9889  1.004
        24K    0.9526   0.9758  0.9861  0.9942  0.981
        32K    0.971    0.9939  0.9884  0.9849  0.9806
        48K    0.9902   0.9825  0.996   0.9945  0.9885
        50K    0.9917   0.9853  0.9906  0.9926  0.9907
        52K    0.9932   0.9878  0.9999  0.9928  0.9903
        54K    0.9945   0.9902  1.064   0.9939  0.9913
        56K    1.048    0.9924  0.9925  0.993   0.9921
        58K    0.9969   0.9945  0.9909  0.9932  0.9918
        60K    0.9455   0.9809  0.9992  0.9915  0.9923
        62K    0.9991   0.9827  0.9921  0.9924  0.9929
        64K    1        1       1       1       1
        66K    1.02     0.9849  0.9857  0.9772  0.9704
        68K    0.8827   0.9232  0.9176  0.9025  0.8903
        70K    0.9255   0.9177  0.9162  0.9029  0.8881
        72K    0.9309   0.936   0.9429  0.9219  0.9052
        74K    0.9715   0.9644  0.967   0.9477  0.9292
        76K    0.9935   0.9975  0.9946  0.9751  0.9552
        78K    0.9987   1.021   1.021   1.003   0.9819
        80K    1.047    1.041   1.052   1.027   1.006
        84K    1.052    1.086   1.092   1.075   1.053
        88K    1.116    1.125   1.133   1.117   1.096
        92K    1.132    1.156   1.167   1.155   1.134
        96K    1.137    1.177   1.195   1.185   1.196
       112K    1.067    1.13    1.148   1.15    1.217
       128K    1.04     1.083   1.113   1.124   1.178
       192K    0.9368   0.985   1.025   1.051   1.095
       256K    0.8741   0.9224  0.9619  0.995   1.024
       384K    0.8103   0.8533  0.8917  0.9282  0.9568
       512K    0.7753   0.8135  0.8537  0.892   0.935
       768K    0.8184   0.8638  0.9121  0.9586  0.9705
      1024K    0.8241   0.8741  0.927   0.979   1.03
      1536K    0.8505   0.9212  0.9882  1.056   1.096
      2048K    0.8294   0.8954  0.9655  1.041   1.102

*/

#ifndef SLOW2_IN_ROOTS
  /* SLOW2_IN_ROOTS below 3: some slowdown starts to be noticable
   * when things fit into the cache on Sparc.
   * XXX The choice of 2.6 gives a slowdown of 1-2% on UltraSparcII,
   * but makes calculations for "maximum" of 436273009 (not applicable anymore)
   * fit into 256K cache (still common for some architectures).
   *
   * One may change it when small caches become uncommon, but the gain
   * is not going to be very noticable... */
#  ifdef i386           /* gcc defines this? */
#    define SLOW2_IN_ROOTS      0.36
#  else
#    define SLOW2_IN_ROOTS      2.6
#  endif
#endif
#ifndef CACHE_ARENA
#  ifdef i386           /* gcc defines this? */
   /* Due to smaller SLOW2_IN_ROOTS, smaller arena is OK; fit L1 cache */
#    define CACHE_ARENA (63 * 1024UL) /* No slowdown even with 64K L1 cache */
#  else
#    define CACHE_ARENA (200 * 1024UL) /* No slowdown even with 256K L2 cache */
#  endif
#endif

#define CACHE_ALPHA     (0.38)          /* Cache performance model parameter */
#define CACHE_CUTOFF    (0.018)         /* Cache performance not smooth here */

static double slow2_in_roots = SLOW2_IN_ROOTS;

typedef struct {
    ulong arena;
    double power;
    double cutoff;
} cache_model_t;

static cache_model_t cache_model = { CACHE_ARENA, CACHE_ALPHA, CACHE_CUTOFF };

/* Assume that some calculation requires a chunk of memory to be
   accessed often in more or less random fashion (as in sieving).
   Assume that the calculation can be done in steps by subdividing the
   chunk into smaller subchunks (arenas) and treathing them
   separately.  Assume that the overhead of subdivision is equivalent
   to the number of arenas.

   Find an optimal size of the arena taking into account the overhead
   of subdivision, and the overhead of arena not fitting into the
   cache.  Assume that arenas of size slow2_in_roots slows down the
   calculation 2x (comparing to very big arenas; when cache hits do
   not matter).  Since cache performance varies wildly with
   architecture, load, and wheather (especially with cache coloring
   enabled), use an idealized cache model based on benchmarks above.

   Assume that an independent region of FIXED_TO_CACHE bytes is accessed
   very often concurrently with the arena access.
 */
ulong
good_arena_size(ulong slow2_size, ulong total, ulong fixed_to_cache,
                cache_model_t *cache_model, long model_type)
{
  ulong asize, cache_arena = cache_model->arena;
  double Xmin, Xmax, A, B, C1, C2, D, V;
  double alpha = cache_model->power, cut_off = cache_model->cutoff;

  /* Estimated relative slowdown,
     with overhead = max((fixed_to_cache+arena)/cache_arena - 1, 0):

     1 + slow2_size/arena due to initialization overhead;

     max(1, 4.63 * overhead^0.38 ) due to footprint > cache size.

     [The latter is hard to substantiate theoretically, but this
     function describes benchmarks pretty close; it does not hurt that
     one can minimize it explicitly too ;-).  The switch between
     diffenent choices of max() happens whe overhead=0.018.]

     Thus the problem is minimizing (1 + slow2_size/arena)*overhead**0.29.
     This boils down to F=((X+A)/(X+B))X^alpha, X=overhead, 
     B = (1 - fixed_to_cache/cache_arena), A = B +
     slow2_size/cache_arena, alpha = 0.38, and X>=0.018, X>-B.  It
     turns out that the minimization problem is not very nasty.  (As
     usual with minimization problems which depend on parameters,
     different values of A,B lead to different algorithms.  However,
     the boundaries between the domains in (A,B) plane where different
     algorithms work are explicitly calculatable.)

     Thus we need to find the rightmost root of (X+A)*(X+B) -
     alpha(A-B)X to the right of 0.018 (if such exists and is below
     Xmax).  Then we manually check the remaining region [0, 0.018].

     Since we cannot trust the purely-experimental cache-hit slowdown
     function, as a sanity check always prefer fitting into the
     cache (or "almost fitting") if F-law predicts that the larger
     value of the arena provides less than 10% speedup.

   */

  if (model_type != 0)
      err(talker, "unsupported type of cache model"); /* Future expansion */

  /* The simplest case: we fit into cache */
  if (total + fixed_to_cache <= cache_arena)
      return total;
  /* The simple case: fitting into cache doesn't slow us down more than 10% */
  if (cache_arena - fixed_to_cache > 10 * slow2_size) {
      asize = cache_arena - fixed_to_cache;
      if (asize > total) asize = total; /* Automatically false... */
      return asize;
  }
  /* Slowdown of not fitting into cache is significant.  Try to optimize.
     Do not be afraid to spend some time on optimization - in trivial
     cases we do not reach this point; any gain we get should
     compensate the time spent on optimization.  */

  B = (1 - ((double)fixed_to_cache)/cache_arena);
  A = B + ((double)slow2_size)/cache_arena;
  C2 = A*B;
  C1 = (A + B - 1/alpha*(A - B))/2;
  D = C1*C1 - C2;
  if (D > 0)
      V = cut_off*cut_off + 2*C1*cut_off + C2; /* Value at CUT_OFF */
  else
      V = 0;                            /* Peacify the warning */
  Xmin = cut_off;
  Xmax = ((double)total - fixed_to_cache)/cache_arena; /* Two candidates */

  if ( D <= 0 || (V >= 0 && C1 + cut_off >= 0) ) /* slowdown increasing */
      Xmax = cut_off;                   /* Only one candidate */
  else if (V >= 0 &&                    /* slowdown concave down */
           ((Xmax + C1) <= 0 || (Xmax*Xmax + 2*C1*Xmax + C2) <= 0))
      /* DO NOTHING */;                 /* Keep both candidates */
  else if (V <= 0 && (Xmax*Xmax + 2*C1*Xmax + C2) <= 0) /* slowdown decreasing */
      Xmin = cut_off;                   /* Only one candidate */
  else /* Now we know: two root, the largest is in CUT_OFF..Xmax */
      Xmax = sqrt(D) - C1;
  if (Xmax != Xmin) {   /* Xmin == CUT_OFF; Check which one is better */
      double v1 = (cut_off + A)/(cut_off + B);
      double v2 = 2.33 * (Xmax + A)/(Xmax + B) * pow(Xmax, alpha);

      if (1.1 * v2 >= v1) /* Prefer fitting into the cache if slowdown < 10% */
          V = v1;
      else {
          Xmin = Xmax;
          V = v2;
      }
  } else if (B > 0)                     /* We need V */
      V = 2.33 * (Xmin + A)/(Xmin + B) * pow(Xmin, alpha);
  if (B > 0 && 1.1 * V > A/B)  /* Now Xmin is the minumum.  Compare with 0 */
      Xmin = 0;

  asize = (ulong)((1 + Xmin)*cache_arena - fixed_to_cache);
  if (asize > total) asize = total;     /* May happen due to approximations */
  return asize;
}

/* Use as in
    install(set_optimize,lLDG)          \\ Through some M too?
    set_optimize(2,1) \\ disable dependence on limit
    \\ 1: how much cache usable, 2: slowdown of setup, 3: alpha, 4: cutoff
    \\ 2,3,4 are in units of 0.001

    { time_primes_arena(ar,limit) =     \\ ar = arena size in K
        set_optimize(1,floor(ar*1024));
        default(primelimit, 200 000);   \\ 100000 results in *larger* malloc()!
        gettime;
        default(primelimit, floor(limit));
        if(ar >= 1, ar=floor(ar));
        print("arena "ar"K => "gettime"ms");
    }
*/
long
set_optimize(long what, GEN g)
{
  long ret = 0;

  switch (what) {
  case 1:
    ret = (long)cache_model.arena;
    break;
  case 2:
    ret = (long)(slow2_in_roots * 1000);
    break;
  case 3:
    ret = (long)(cache_model.power * 1000);
    break;
  case 4:
    ret = (long)(cache_model.cutoff * 1000);
    break;
  default:
    err(talker, "panic: set_optimize");
    break;
  }
  if (g != NULL) {
    ulong val = itou(g);

    switch (what) {
    case 1: cache_model.arena = val; break;
    case 2: slow2_in_roots     = (double)val / 1000.; break;
    case 3: cache_model.power  = (double)val / 1000.; break;
    case 4: cache_model.cutoff = (double)val / 1000.; break;
    }
  }
  return ret;
}

static void
sieve_chunk(byteptr known_primes, ulong s, byteptr data, ulong count)
{   /* start must be odd;
       prime differences (starting from (5-3)=2) start at known_primes[2],
       are terminated by a 0 byte;

       Checks cnt odd numbers starting at `start', setting bytes
       starting at data to 0 or 1 depending on whether the
       corresponding odd number is prime or not */
    ulong p;
    byteptr q;
    register byteptr write_to = data;   /* Better code with gcc 2.8.1 */
    register ulong   cnt      = count;  /* Better code with gcc 2.8.1 */
    register ulong   start    = s;      /* Better code with gcc 2.8.1 */
    register ulong   delta    = 1;      /* Better code with gcc 2.8.1 */

    memset(data, 0, cnt);
    start >>= 1;                        /* (start - 1)/2 */
    start += cnt;                       /* Corresponds to the end */
    cnt -= 1;
    /* data corresponds to start.  q runs over primediffs.  */
    /* Don't care about DIFFPTR_SKIP: false positives provide no problem */
    for (q = known_primes + 1, p = 3; delta; delta = *++q, p += delta) {
        /* first odd number which is >= start > p and divisible by p
           = last odd number which is <= start + 2p - 1 and 0 (mod p)
           = p + the last even number which is <= start + p - 1 and 0 (mod p)
           = p + the last even number which is <= start + p - 2 and 0 (mod p)
           = p + start + p - 2 - (start + p - 2) % 2p
           = start + 2(p - 1 - ((start-1)/2 + (p-1)/2) % p). */
      long off = cnt - ((start+(p>>1)) % p);

      while (off >= 0) {
          write_to[off] = 1;
          off -= p;
      }
    }
}

/* Do not inline sieve_chunk()!  It fits into registers in ix86 non-inlined */
extern void (*sieve_chunk_p)(byteptr known_primes, ulong s, byteptr data, ulong count);
void (*sieve_chunk_p)(byteptr known_primes, ulong s, byteptr data, ulong count)
    = sieve_chunk;

/* Here's the workhorse.  This is recursive, although normally the first
   recursive call will bottom out and invoke initprimes1() at once.
   (Not static;  might conceivably be useful to someone in library mode) */
byteptr
initprimes0(ulong maxnum, long *lenp, ulong *lastp)
{
  long size, alloced, psize;
  byteptr q, fin, p, p1, fin1, plast, curdiff;
  ulong last, remains, curlow, rootnum, asize;
  ulong prime_above = 3;
  byteptr p_prime_above;

  if (maxnum <= 1ul<<17)        /* Arbitrary. */
    return initprimes1(maxnum>>1, lenp, (long*)lastp); /* Break recursion */

  maxnum |= 1;                  /* make it odd. */

  /* Checked to be enough up to 40e6, attained at 155893 */
  /* Due to multibyte representation of large gaps, this estimate will
     be broken by large enough maxnum.  However, assuming exponential
     distribution of gaps with the average log(n), we are safe up to
     circa exp(-256/log(1/0.09)) = 1.5e46.  OK with LONG_BITS <= 128. ;-) */
  size = (long) (1.09 * maxnum/log((double)maxnum)) + 145;
  p1 = (byteptr) gpmalloc(size);
  rootnum = (ulong) sqrt((double)maxnum); /* cast it back to a long */
  rootnum |= 1;
  {
    byteptr p2 = initprimes0(rootnum, &psize, &last); /* recursive call */
    memcpy(p1, p2, psize); free(p2);
  }
  fin1 = p1 + psize - 1;
  remains = (maxnum - rootnum) >> 1; /* number of odd numbers to check */

  /* Actually, we access primes array of psize too; but we access it
     consequently, thus we do not include it in fixed_to_cache */
  asize = good_arena_size((ulong)(rootnum * slow2_in_roots), remains + 1, 0,
                          &cache_model, 0) - 1;
  /* enough room on the stack ? */
  alloced = (((byteptr)avma) <= ((byteptr)bot) + asize);
  if (alloced)
    p = (byteptr) gpmalloc(asize + 1);
  else
    p = (byteptr) bot;
  fin = p + asize;              /* the 0 sentinel goes at fin. */
  curlow = rootnum + 2; /* First candidate: know primes up to rootnum (odd). */
  curdiff = fin1;

  /* During each iteration p..fin-1 represents a range of odd
     numbers.  plast is a pointer which represents the last prime seen,
     it may point before p..fin-1. */
  plast = p - ((rootnum - last) >> 1) - 1;
  p_prime_above = p1 + 2;
  while (remains)       /* Cycle over arenas.  Performance is not crucial */
  {
    unsigned char was_delta;

    if (asize > remains) {
      asize = remains;
      fin = p + asize;
    }
    /* Fake the upper limit appropriate for the given arena */
    while (prime_above*prime_above <= curlow + (asize << 1) && *p_prime_above)
        prime_above += *p_prime_above++;
    was_delta = *p_prime_above;
    *p_prime_above = 0;                 /* Put a 0 sentinel for sieve_chunk */

    (*sieve_chunk_p)(p1, curlow, p, asize);

    *p_prime_above = was_delta;         /* Restore */
    p[asize] = 0;                       /* Put a 0 sentinel for ZZZ */
    /* now q runs over addresses corresponding to primes */
    for (q = p; ; plast = q++)
    {
      long d;

      while (*q) q++;                   /* use ZZZ 0-sentinel at end */
      if (q >= fin) break;
      d = (q - plast) << 1;
      while (d >= DIFFPTR_SKIP)
        *curdiff++ = DIFFPTR_SKIP, d -= DIFFPTR_SKIP;
      *curdiff++ = (unsigned char)d;
    }
    plast -= asize;
    remains -= asize;
    curlow += (asize<<1);
  } /* while (remains) */
  last = curlow - ((p - plast) << 1);
  *curdiff++ = 0;               /* sentinel */
  *lenp = curdiff - p1;
  *lastp = last;
  if (alloced) free(p);
  return (byteptr) gprealloc(p1, *lenp);
}
#if 0 /* not yet... GN */
/* The diffptr table will contain at least 6548 entries (up to and including
   the 6547th prime, 65557, and the terminal null byte), because the isprime/
   small-factor-extraction machinery wants to depend on everything up to 65539
   being in the table, and we might as well go to a multiple of 4 Bytes.--GN */

void
init_tinyprimes_tridiv(byteptr p);      /* in ifactor2.c */
#endif

static ulong _maxprime = 0;

ulong
maxprime(void) { return _maxprime; }

void
maxprime_check(ulong c)
{
  if (_maxprime < c) err(primer2, c);
}

byteptr
initprimes(ulong maxnum)
{
  long len;
  ulong last;
  byteptr p;
  /* The algorithm must see the next prime beyond maxnum, whence the +512. */
  ulong maxnum1 = ((maxnum<65302?65302:maxnum)+512ul);

  if ((maxnum>>1) > VERYBIGINT - 1024)
      err(talker, "Too large primelimit");
  p = initprimes0(maxnum1, &len, &last);
#if 0 /* not yet... GN */
  static int build_the_tables = 1;
  if (build_the_tables) { init_tinyprimes_tridiv(p); build_the_tables=0; }
#endif
  _maxprime = last; return p;
}

static void
cleanprimetab(void)
{
  long i,j, lp = lg(primetab);

  for (i=j=1; i < lp; i++)
    if (primetab[i]) primetab[j++] = primetab[i];
  setlg(primetab,j);
}

/* p is a single element or a row vector with "primes" to add to primetab.
 * If p shares a non-trivial factor with another element in primetab, take it
 * into account. */
GEN
addprimes(GEN p)
{
  pari_sp av;
  long i,k,tx,lp;
  GEN L;

  if (!p) return primetab;
  tx = typ(p);
  if (is_vec_t(tx))
  {
    for (i=1; i < lg(p); i++) (void)addprimes((GEN) p[i]);
    return primetab;
  }
  if (tx != t_INT) err(typeer,"addprime");
  if (is_pm1(p)) return primetab;
  av = avma; i = signe(p);
  if (i == 0) err(talker,"can't accept 0 in addprimes");
  if (i < 0) p = absi(p);

  lp = lg(primetab);
  L = cgetg(2*lp,t_VEC); k = 1;
  for (i=1; i < lp; i++)
  {
    GEN n = (GEN)primetab[i], d = mppgcd(n, p);
    if (! is_pm1(d))
    {
      if (!egalii(p,d)) L[k++] = (long)d;
      L[k++] = ldivii(n,d);
      gunclone(n); primetab[i] = 0;
    }
  }
  primetab = (GEN) gprealloc(primetab, (lp+1)*sizeof(long));
  primetab[i] = lclone(p); setlg(primetab, lp+1);
  if (k > 1) { cleanprimetab(); setlg(L,k); (void)addprimes(L); }
  avma = av; return primetab;
}

static GEN
removeprime(GEN prime)
{
  long i;

  if (typ(prime) != t_INT) err(typeer,"removeprime");
  for (i=lg(primetab) - 1; i; i--)
    if (absi_equal((GEN) primetab[i], prime))
    {
      gunclone((GEN)primetab[i]); primetab[i]=0;
      cleanprimetab(); break;
    }
  if (!i) err(talker,"prime %Z is not in primetable", prime);
  return primetab;
}

GEN
removeprimes(GEN prime)
{
  long i,tx;

  if (!prime) return primetab;
  tx = typ(prime);
  if (is_vec_t(tx))
  {
    if (prime == primetab)
    {
      for (i=1; i < lg(prime); i++) gunclone((GEN)prime[i]);
      setlg(prime, 1);
    }
    else
    {
      for (i=1; i < lg(prime); i++) (void)removeprime((GEN) prime[i]);
    }
    return primetab;
  }
  return removeprime(prime);
}

/***********************************************************************/
/**                                                                   **/
/**       COMPUTING THE MATRIX OF PRIME DIVISORS AND EXPONENTS        **/
/**                                                                   **/
/***********************************************************************/

/* Configure may/should define this to 1 if MPQS is not installed --GN */
#ifndef decomp_default_hint
#define decomp_default_hint 0
#endif

/* Some overkill removed from this  (15 spsp for an integer < 2^32 ?!).
   Should really revert to isprime() once the new primality tester is ready
   --GN */
#if 0
#define pseudoprime(p) millerrabin(p,3*lgefint(p))
#else /* remove further overkill :-)  --KB */
#define pseudoprime(p) BSW_psp(p)
#endif

/* where to stop trial dividing in factorization */

static ulong
tridiv_bound(GEN n, long all)
{
  ulong size = (ulong)expi(n) + 1;
  if (all > 1)                /* bounded factoring */
    return all;               /* use the given limit */
  if (all == 0)
    return (ulong)VERYBIGINT; /* smallfact() case */
  if (size <= 32)
    return 16384UL;
  else if (size <= 512)
    return (size-16) << 10;
  return 1UL<<19;             /* Rho will generally be faster above this */
}

/* function imported from ifactor1.c */
extern long ifac_decomp_break(GEN n, long (*ifac_break)(GEN,GEN,GEN,GEN),
                  GEN state, long hint);
static GEN
aux_end(GEN n, long nb)
{
  GEN p1,p2, z = (GEN)avma;
  long i;

  if (n) gunclone(n);
  p1=cgetg(nb+1,t_COL);
  p2=cgetg(nb+1,t_COL);
  for (i=nb; i; i--)
  {
    p2[i] = (long)z; z += lg(z);
    p1[i] = (long)z; z += lg(z);
  }
  z[1] = (long)p1;
  z[2] = (long)p2;
  if (nb) (void)sort_factor_gen(z,cmpii);
  return z;
}

GEN
auxdecomp1(GEN n, long (*ifac_break)(GEN n, GEN pairs, GEN here, GEN state),
                  GEN state, long all, long hint)
{
  pari_sp av;
  long pp[] = { evaltyp(t_INT)|_evallg(4), 0,0,0 };
  long nb = 0,i,k,lp;
  ulong p, lim1;
  byteptr d=diffptr+1;

  if (typ(n) != t_INT) err(arither1);
  i=signe(n); if (!i) err(arither3);
  (void)cgetg(3,t_MAT);
  if (i<0) { (void)stoi(-1); (void)stoi(1); nb++; }
  if (is_pm1(n)) return aux_end(NULL,nb);

  n = gclone(n); setsigne(n,1);
  i = vali(n);
  if (i)
  {
    (void)stoi(2); (void)stoi(i); nb++;
    av=avma; affii(shifti(n,-i), n); avma=av;
  }
  if (is_pm1(n)) return aux_end(n,nb);
  lim1 = tridiv_bound(n, all);

  /* trial division */
  p = 2;
  while (*d && p <= lim1)
  {
    NEXT_PRIME_VIADIFF(p,d);
    if (mpdivisis(n,p,n))
    {
      nb++; k=1; while (mpdivisis(n,p,n)) k++;
      (void)utoi(p); (void)stoi(k);
      if (is_pm1(n)) return aux_end(n,nb);
    }
  }

  /* pp = square of biggest p tried so far */
  av=avma; affii(muluu(p,p),pp); avma=av;
  if (cmpii(pp,n) > 0)
  {
    nb++;
    (void)icopy(n); (void)stoi(1); return aux_end(n,nb);
  }

  /* trial divide by the "special primes" (usually huge composites...) */
  lp = lg(primetab); k=0;
  for (i=1; i<lp; i++)
    if (mpdivis(n,(GEN) primetab[i],n))
    {
      nb++; k=1; while (mpdivis(n,(GEN) primetab[i],n)) k++;
      (void)icopy((GEN) primetab[i]); (void)stoi(k);
      if (is_pm1(n)) return aux_end(n,nb);
    }

  /* test primality (unless all != 1 (i.e smallfact))*/
  if ((k && cmpii(pp,n) > 0) || (all==1 && pseudoprime(n)))
  {
    nb++;
    (void)icopy(n); (void)stoi(1); return aux_end(n,nb);
  }

  /* now we have a large composite.  Use hint as is if all==1 */
  if (all!=1) hint = 15; /* turn off everything except pure powers */
  if (ifac_break && (*ifac_break)(n,NULL,NULL,state))
    /*Initialize ifac_break*/
  {
    if (DEBUGLEVEL >= 3)
      fprintferr("IFAC: (Partial fact.) Initial stop requested.\n");
  }
  else
    nb += ifac_decomp_break(n, ifac_break, state, hint);

  return aux_end(n, nb);
}

/* state[1]: current unfactored part.
 * state[2]: limit. */
static long
ifac_break_limit(GEN n, GEN pairs/*unused*/, GEN here, GEN state)
{
  pari_sp ltop = avma;
  int res;
  (void)pairs;
  if (!here) /* initial call */
   /* Small prime have been removed since start, n is the new unfactored part.
    * Result is affect()ed to state[1] to preserve stack. */
    affii(n, (GEN)state[1]);
  else
  {
    GEN q = powgi((GEN)here[0],(GEN)here[1]); /* primary factor found.*/
    if (DEBUGLEVEL>2) fprintferr("IFAC: Stop: Primary factor: %Z\n",q);
    /* divide unfactored part by q and assign the result to state[1] */
    diviiz((GEN)state[1],q, (GEN)state[1]);
  }
  if (DEBUGLEVEL>=3) fprintferr("IFAC: Stop: remaining %Z\n",state[1]);
  /* check the stopping criterion, then restore stack */
  res = cmpii((GEN)state[1],(GEN)state[2]) <= 0;
  avma = ltop; return res;
}

static GEN
auxdecomp0(GEN n, long all, long hint)
{
  return auxdecomp1(n, NULL, gzero, all, hint);
}

GEN
auxdecomp(GEN n, long all)
{
  return auxdecomp0(n,all,decomp_default_hint);
}

/* see before ifac_crack() in ifactor1.c for current semantics of `hint'
   (factorint's `flag') */
GEN
factorint(GEN n, long flag)
{
  return auxdecomp0(n,1,flag);
}

GEN
decomp(GEN n)
{
  return auxdecomp0(n,1,decomp_default_hint);
}

/* Factor until the unfactored part is smaller than limit. */
GEN
decomp_limit(GEN n, GEN limit)
{
  GEN state = cgetg(3,t_VEC);
 /* icopy is mainly done to allocate memory for affect().
  * Currently state[1] value is discarded in initial call to ifac_break_limit */
  state[1] = licopy(n);
  state[2] = lcopy(limit);
  return auxdecomp1(n, &ifac_break_limit, state, 1, decomp_default_hint);
}

GEN
smallfact(GEN n)
{
  return boundfact(n,0);
}

GEN
gboundfact(GEN n, long lim)
{
  return garith_proto2gs(boundfact,n,lim);
}

GEN
boundfact(GEN n, long lim)
{
  GEN p1,p2,p3,p4,p5,y;
  pari_sp av = avma,tetpil;

  if (lim<=1) lim=0;
  switch(typ(n))
  {
    case t_INT:
      return auxdecomp(n,lim);
    case t_FRACN:
      n=gred(n);
      if (typ(n) == t_INT)
        return gerepileupto(av, boundfact(n,lim));
      /* fall through */
    case t_FRAC:
      p1=auxdecomp((GEN)n[1],lim);
      p2=auxdecomp((GEN)n[2],lim);
      p4=concatsp((GEN)p1[1],(GEN)p2[1]);
      p5=concatsp((GEN)p1[2],gneg((GEN)p2[2]));
      p3=indexsort(p4);

      tetpil=avma; y=cgetg(3,t_MAT);
      y[1]=(long)extract(p4,p3);
      y[2]=(long)extract(p5,p3);
      return gerepile(av,tetpil,y);
  }
  err(arither1);
  return NULL; /* not reached */
}
/***********************************************************************/
/**                                                                   **/
/**                    SIMPLE FACTORISATIONS                          **/
/**                                                                   **/
/***********************************************************************/

/*Factorize n and output [fp,fe] where
 * fp and fe are vecsmall and n=prod{fp[i]^fe[i]}
 */

GEN
decomp_small(long n)
{
  pari_sp ltop=avma;
  GEN F = factor(stoi(n));
  long i, l=lg(F[1]);
  GEN f=cgetg(3,t_VEC);
  GEN fp=cgetg(l,t_VECSMALL);
  GEN fe=cgetg(l,t_VECSMALL);
  f[1] = (long) fp;
  f[2] = (long) fe;
  for(i = 1; i < l; i++)
  {
    fp[i]=itos(gcoeff(F,i,1));
    fe[i]=itos(gcoeff(F,i,2));
  }
  return gerepileupto(ltop,f);
}

/*Return the primary factors of a small integer as a vecsmall*/
GEN
decomp_primary_small(long n)
{
  pari_sp ltop=avma;
  GEN F = factor(stoi(n));
  GEN fc = cgetg(lg(F[1]), t_VECSMALL);
  pari_sp av=avma;
  long i;
  for (i = 1; i < lg(fc); i++)
    fc[i] = itos(powgi(gmael(F,1,i), gmael(F,2,i)));
  avma=av;
  return gerepileupto(ltop,fc);
}


/***********************************************************************/
/**                                                                   **/
/**                    BASIC ARITHMETIC FUNCTIONS                     **/
/**                                                                   **/
/***********************************************************************/

/* functions imported from ifactor1.c */
long
ifac_moebius(GEN n, long hint);

long
ifac_issquarefree(GEN n, long hint);

long
ifac_omega(GEN n, long hint);

long
ifac_bigomega(GEN n, long hint);

GEN
ifac_totient(GEN n, long hint);

GEN
ifac_numdiv(GEN n, long hint);

GEN
ifac_sumdiv(GEN n, long hint);

GEN
ifac_sumdivk(GEN n, long k, long hint);

GEN
gmu(GEN n)
{
  return arith_proto(mu,n,1);
}

long
mu(GEN n)
{
  byteptr d = diffptr+1;        /* point at 3 - 2 */
  pari_sp av = avma;
  ulong p, lim1;
  long s, v;

  if (typ(n) != t_INT) err(arither1);
  if (!signe(n)) err(arither3);
  if (is_pm1(n)) return 1;
  v = vali(n);
  if (v>1) return 0;
  s = v ? -1 : 1;
  n=absi(shifti(n,-v)); p = 2;
  if (is_pm1(n)) return s;
  lim1 = tridiv_bound(n,1);

  while (*d && p < lim1)
  {
    NEXT_PRIME_VIADIFF(p,d);
    if (mpdivisis(n,p,n))
    {
      if (smodis(n,p) == 0) { avma=av; return 0; }
      s = -s; if (is_pm1(n)) { avma=av; return s; }
    }
  }
  /* we normally get here with p==nextprime(PRE_TUNE), which will already
     have been tested against n, so p^2 >= n  (instead of p^2 > n)  is
     enough to guarantee n is prime */
  if (cmpii(sqru(p),n) >= 0 || pseudoprime(n)) { avma=av; return -s; }
  /* large composite without small factors */
  v = ifac_moebius(n, decomp_default_hint);
  avma=av;
  return (s<0 ? -v : v);        /* correct also if v==0 */
}

GEN
gissquarefree(GEN x)
{
  return arith_proto(issquarefree,x,0);
}

long
issquarefree(GEN x)
{
  ulong p, lim1;
  pari_sp av = avma;
  long tx;
  GEN d;

  if (gcmp0(x)) return 0;
  tx = typ(x);
  if (tx == t_INT)
  {
    long v;
    byteptr d=diffptr+1;
    if (is_pm1(x)) return 1;
    if((v = vali(x)) > 1) return 0;
    x = absi(shifti(x,-v));
    if (is_pm1(x)) return 1;
    lim1 = tridiv_bound(x,1);

    p = 2;
    while (*d && p < lim1)
    {
      NEXT_PRIME_VIADIFF(p,d);
      if (mpdivisis(x,p,x))
      {
        if (smodis(x,p) == 0) { avma = av; return 0; }
        if (is_pm1(x)) { avma = av; return 1; }
      }
    }
    if (cmpii(sqru(p),x) >= 0 || pseudoprime(x)) { avma = av; return 1; }
    v = ifac_issquarefree(x, decomp_default_hint);
    avma = av; return v;
  }
  if (tx != t_POL) err(typeer,"issquarefree");
  d = ggcd(x, derivpol(x));
  avma = av; return (lg(d) == 3);
}

GEN
gomega(GEN n)
{
  return arith_proto(omega,n,1);
}

long
omega(GEN n)
{
  byteptr d=diffptr+1;
  pari_sp av = avma;
  long nb,v;
  ulong p, lim1;

  if (typ(n) != t_INT) err(arither1);
  if (!signe(n)) err(arither3);
  if (is_pm1(n)) return 0;
  v=vali(n);
  nb = v ? 1 : 0;
  n = absi(shifti(n,-v));
  if (is_pm1(n)) return nb;

  lim1 = tridiv_bound(n,1);
  p = 2;
  while (*d && p < lim1)
  {
    NEXT_PRIME_VIADIFF(p,d);
    if (mpdivisis(n,p,n))
    {
      nb++; while (mpdivisis(n,p,n)); /* empty */
      if (is_pm1(n)) { avma = av; return nb; }
    }
  }
  if (cmpii(sqru(p),n) >= 0 || pseudoprime(n)) { avma = av; return nb+1; }
  /* large composite without small factors */
  nb += ifac_omega(n, decomp_default_hint);
  avma=av; return nb;
}

GEN
gbigomega(GEN n)
{
  return arith_proto(bigomega,n,1);
}

long
bigomega(GEN n)
{
  byteptr d=diffptr+1;
  ulong p, lim1;
  pari_sp av = avma;
  long nb,v;

  if (typ(n) != t_INT) err(arither1);
  if (!signe(n)) err(arither3);
  if (is_pm1(n)) return 0;
  nb=v=vali(n);
  n=absi(shifti(n,-v));
  if (is_pm1(n)) { avma = av; return nb; }

  lim1 = tridiv_bound(n,1);
  p = 2;
  while (*d && p < lim1)
  {
    NEXT_PRIME_VIADIFF(p,d);
    if (mpdivisis(n,p,n))
    {
      do nb++; while (mpdivisis(n,p,n));
      if (is_pm1(n)) { avma = av; return nb; }
    }
  }
  if (cmpii(sqru(p),n) >= 0 || pseudoprime(n)) { avma = av; return nb+1; }
  nb += ifac_bigomega(n, decomp_default_hint);
  avma=av; return nb;
}

GEN
gphi(GEN n)
{
  return garith_proto(phi,n,1);
}

GEN
phi(GEN n)
{
  byteptr d = diffptr+1;
  GEN m;
  ulong p, lim1;
  pari_sp av = avma;
  long v;

  if (typ(n) != t_INT) err(arither1);
  if (!signe(n)) err(arither3);
  if (is_pm1(n)) return gun;
  v = vali(n);
  n = absi(shifti(n,-v));
  m = (v > 1 ? shifti(gun,v-1) : stoi(1));
  if (is_pm1(n)) { return gerepileupto(av,m); }

  p = 2;
  lim1 = tridiv_bound(n,1);
  while (*d && p < lim1)
  {
    NEXT_PRIME_VIADIFF(p,d);
    if (mpdivisis(n,p,n))
    {
      m = mulis(m, p-1);
      while (mpdivisis(n,p,n)) m = mulis(m,p);
      if (is_pm1(n)) { return gerepileupto(av,m); }
    }
  }
  if (cmpii(sqru(p),n) >= 0 || pseudoprime(n))
  {
    m = mulii(m,addsi(-1,n));
    return gerepileupto(av,m);
  }
  m = mulii(m, ifac_totient(n, decomp_default_hint));
  return gerepileupto(av,m);
}

GEN
gnumbdiv(GEN n)
{
  return garith_proto(numbdiv,n,1);
}

GEN
numbdiv(GEN n)
{
  byteptr d=diffptr+1;
  GEN m;
  long l, v;
  ulong p, lim1;
  pari_sp av = avma;

  if (typ(n) != t_INT) err(arither1);
  if (!signe(n)) err(arither3);
  if (is_pm1(n)) return gun;
  v = vali(n);
  n = absi(shifti(n,-v));
  m = stoi(v+1);
  if (is_pm1(n)) return gerepileupto(av,m);

  lim1 = tridiv_bound(n,1);
  p = 2;
  while (*d && p < lim1)
  {
    NEXT_PRIME_VIADIFF(p,d);
    l=1; while (mpdivisis(n,p,n)) l++;
    m = mulsi(l,m); if (is_pm1(n)) { return gerepileupto(av,m); }
  }
  if(cmpii(sqru(p),n) >= 0 || pseudoprime(n))
  {
    m = shifti(m,1);
    return gerepileupto(av,m);
  }
  m = mulii(m, ifac_numdiv(n, decomp_default_hint));
  return gerepileupto(av,m);
}

GEN
gsumdiv(GEN n)
{
  return garith_proto(sumdiv,n,1);
}

GEN
sumdiv(GEN n)
{
  byteptr d=diffptr+1;
  GEN m,m1;
  ulong p, lim1;
  pari_sp av=avma;
  long v;

  if (typ(n) != t_INT) err(arither1);
  if (!signe(n)) err(arither3);
  if (is_pm1(n)) return gun;
  v = vali(n);
  n = absi(shifti(n,-v));
  m = (v ? addsi(-1,shifti(gun,v+1)) : stoi(1));
  if (is_pm1(n)) { return gerepileupto(av,m); }

  lim1 = tridiv_bound(n,1);
  p = 2;
  while (*d && p < lim1)
  {
    NEXT_PRIME_VIADIFF(p,d);
    if (mpdivisis(n,p,n))
    {
      m1 = utoi(p+1);
      while (mpdivisis(n,p,n)) m1 = addsi(1, mului(p,m1));
      m = mulii(m1,m); if (is_pm1(n)) { return gerepileupto(av,m); }
    }
  }
  if(cmpii(sqru(p),n) >= 0 || pseudoprime(n))
  {
    m = mulii(m,addsi(1,n));
    return gerepileupto(av,m);
  }
  m = mulii(m, ifac_sumdiv(n, decomp_default_hint));
  return gerepileupto(av,m);
}

GEN
gsumdivk(GEN n, long k)
{
  return garith_proto2gs(sumdivk,n,k);
}

GEN
sumdivk(GEN n, long k)
{
  byteptr d=diffptr+1;
  GEN n1,m,m1,pk;
  ulong p, lim1;
  pari_sp av = avma;
  long k1,v;

  if (!k) return numbdiv(n);
  if (k==1) return sumdiv(n);
  if (typ(n) != t_INT) err(arither1);
  if (!signe(n)) err(arither3);
  if (is_pm1(n)) return gun;
  k1 = k; n1 = n;
  if (k==-1) { m=sumdiv(n); k = 1; goto fin; }
  if (k<0)  k = -k;
  v=vali(n);
  n=absi(shifti(n,-v));
  m = stoi(1);
  while (v--)  m = addsi(1,shifti(m,k));
  if (is_pm1(n)) goto fin;

  lim1 = tridiv_bound(n,1);
  p = 2;
  while (*d && p < lim1)
  {
    NEXT_PRIME_VIADIFF(p,d);
    if (mpdivisis(n,p,n))
    {
      pk = gpowgs(utoi(p),k); m1 = addsi(1,pk);
      while (mpdivisis(n,p,n)) m1 = addsi(1, mulii(pk,m1));
      m = mulii(m1,m); if (is_pm1(n)) goto fin;
    }
  }
  if(cmpii(sqru(p),n) >= 0 || pseudoprime(n))
  {
    pk = gpuigs(n,k);
    m = mulii(m,addsi(1,pk));
    goto fin;
  }
  m = mulii(m, ifac_sumdivk(n, k, decomp_default_hint));
 fin:
  if (k1 < 0) m = gdiv(m, gpowgs(n1,k));
  return gerepileupto(av,m);
}

/***********************************************************************/
/**                                                                   **/
/**                MISCELLANEOUS ARITHMETIC FUNCTIONS                 **/
/**         (all of these ultimately depend on auxdecomp())           **/
/**                                                                   **/
/***********************************************************************/

GEN
divisors(GEN n)
{
  pari_sp tetpil, av = avma;
  long i, j, l, nbdiv;
  GEN *d, *t, *t1, *t2, *t3, P, E, e;

  if (typ(n) != t_MAT || lg(n) != 3) n = auxdecomp(n,1);

  P = (GEN)n[1]; l = lg(P);
  E = (GEN)n[2];
  if (l>1 && signe(P[1]) < 0) { E++; P++; l--; } /* skip -1 */
  e = cgetg(l, t_VECSMALL);
  nbdiv = 1;
  for (i=1; i<l; i++)
  {
    e[i] = itos((GEN)E[i]);
    nbdiv = itos_or_0( mulss(nbdiv, 1+e[i]) );
  }
  if (!nbdiv || nbdiv & ~LGBITS)
    err(talker, "too many divisors (more than %ld)", LGBITS - 1);
  d = t = (GEN*) cgetg(nbdiv+1,t_VEC);
  *++d = gun;
  for (i=1; i<l; i++)
    for (t1=t,j=e[i]; j; j--,t1=t2)
      for (t2=d, t3=t1; t3<t2; ) *++d = mulii(*++t3, (GEN)P[i]);
  tetpil = avma; return gerepile(av,tetpil,sort((GEN)t));
}

static GEN
do_core(GEN n, int all)
{
  pari_sp av = avma;
  long i;
  GEN fa,p1,p2,c = gun;

  fa = auxdecomp(n,all);
  p1 = (GEN)fa[1];
  p2 = (GEN)fa[2];
  for (i=1; i<lg(p1); i++)
    if (mod2((GEN)p2[i])) c = mulii(c,(GEN)p1[i]);
  return gerepileuptoint(av, c);
}

static GEN
do_core2(GEN n, int all)
{
  pari_sp av = avma;
  long i;
  GEN fa,p1,p2,e,c=gun,f=gun,y;

  fa = auxdecomp(n,all);
  p1 = (GEN)fa[1];
  p2 = (GEN)fa[2];
  for (i=1; i<lg(p1); i++)
  {
    e = (GEN)p2[i];
    if (mod2(e))   c = mulii(c, (GEN)p1[i]);
    if (!gcmp1(e)) f = mulii(f, powgi((GEN)p1[i], shifti(e,-1)));
  }
  y = cgetg(3,t_VEC);
  y[1] = (long)c;
  y[2] = (long)f; return gerepilecopy(av, y);
}

GEN core(GEN n)        { return do_core(n,1); }
GEN corepartial(GEN n) { return do_core(n,0); }
GEN core2(GEN n)       { return do_core2(n,1); }
GEN core2partial(GEN n){ return do_core2(n,0); }

GEN
core0(GEN n,long flag) { return flag? core2(n): core(n); }

static long
_mod4(GEN c) { long r = mod4(c); if (signe(c) < 0) r = 4-r; return r; }

GEN
coredisc(GEN n)
{
  pari_sp av = avma;
  GEN c = core(n);
  long r = _mod4(c);
  if (r==1 || r==4) return c;
  return gerepileuptoint(av, shifti(c,2));
}

GEN
coredisc2(GEN n)
{
  pari_sp av = avma;
  GEN y = core2(n);
  GEN c = (GEN)y[1], f = (GEN)y[2];
  long r = _mod4(c);
  if (r==1 || r==4) return y;
  y = cgetg(3,t_VEC);
  y[1] = lshifti(c,2);
  y[2] = lmul2n(f,-1); return gerepileupto(av, y);
}

GEN
coredisc0(GEN n,long flag) { return flag? coredisc2(n): coredisc(n); }

/***********************************************************************/
/**                                                                   **/
/**                      BINARY QUADRATIC FORMS                       **/
/**                                                                   **/
/***********************************************************************/

GEN
qf_disc(GEN x, GEN y, GEN z)
{
  if (!y) { y=(GEN)x[2]; z=(GEN)x[3]; x=(GEN)x[1]; }
  return subii(sqri(y), shifti(mulii(x,z),2));
}

static GEN
qf_create(GEN x, GEN y, GEN z, long s)
{
  GEN t;
  if (typ(x)!=t_INT || typ(y)!=t_INT || typ(z)!=t_INT) err(typeer,"Qfb");
  if (!s)
  {
    pari_sp av=avma; s = signe(qf_disc(x,y,z)); avma=av;
    if (!s) err(talker,"zero discriminant in Qfb");
  }
  if (s < 0)
  {
    t = cgetg(4,t_QFI);
    if (signe(x) < 0) err(impl,"negative definite t_QFI");
  }
  else
    t = cgetg(5,t_QFR);
  t[1]=(long)icopy(x); t[2]=(long)icopy(y); t[3]=(long)icopy(z);
  return t;
}

GEN
qfi(GEN x, GEN y, GEN z)
{
  return qf_create(x,y,z,-1);
}

GEN
qfr(GEN x, GEN y, GEN z, GEN d)
{
  GEN t = qf_create(x,y,z,1);
  if (typ(d) != t_REAL)
    err(talker,"Shanks distance should be a t_REAL (in qfr)");
  t[4]=lrcopy(d); return t;
}

GEN
Qfb0(GEN x, GEN y, GEN z, GEN d, long prec)
{
  GEN t = qf_create(x,y,z,0);
  if (lg(t)==4) return t;
  if (!d) d = gzero;
  if (typ(d) == t_REAL)
    t[4] = lrcopy(d);
  else
    { t[4]=lgetr(prec); gaffect(d,(GEN)t[4]); }
  return t;
}

/* composition */
static void
sq_gen(GEN z, GEN x)
{
  GEN d1,x2,y2,v1,v2,c3,m,p1,r;

  d1 = bezout((GEN)x[2],(GEN)x[1],&x2,&y2);
  if (gcmp1(d1)) v1 = v2 = (GEN)x[1];
  else
  {
    v1 = divii((GEN)x[1],d1);
    v2 = mulii(v1,mppgcd(d1,(GEN)x[3]));
  }
  m = mulii((GEN)x[3],x2);
  setsigne(m,-signe(m));
  r = modii(m,v2); p1 = mulii(v1,r);
  c3 = addii(mulii((GEN)x[3],d1), mulii(r,addii((GEN)x[2],p1)));
  z[1] = lmulii(v1,v2);
  z[2] = laddii((GEN)x[2], shifti(p1,1));
  z[3] = ldivii(c3,v2);
}

void
comp_gen(GEN z,GEN x,GEN y)
{
  GEN s,n,d,d1,x1,x2,y1,y2,v1,v2,c3,m,p1,r;

  if (x == y) { sq_gen(z,x); return; }
  s = shifti(addii((GEN)x[2],(GEN)y[2]),-1);
  n = subii((GEN)y[2],s);
  d = bezout((GEN)y[1],(GEN)x[1],&y1,&x1);
  d1 = bezout(s,d,&x2,&y2);
  if (gcmp1(d1))
  {
    v1 = (GEN)x[1];
    v2 = (GEN)y[1];
  }
  else
  {
    v1 = divii((GEN)x[1],d1);
    v2 = divii((GEN)y[1],d1);
    v1 = mulii(v1, mppgcd(d1,mppgcd((GEN)x[3],mppgcd((GEN)y[3],n))));
  }
  m = addii(mulii(mulii(y1,y2),n), mulii((GEN)y[3],x2));
  setsigne(m,-signe(m));
  r = modii(m,v1); p1 = mulii(v2,r);
  c3 = addii(mulii((GEN)y[3],d1), mulii(r,addii((GEN)y[2],p1)));
  z[1] = lmulii(v1,v2);
  z[2] = laddii((GEN)y[2], shifti(p1,1));
  z[3] = ldivii(c3,v1);
}

static GEN
compimag0(GEN x, GEN y, int raw)
{
  pari_sp av = avma;
  long tx = typ(x);
  GEN z;

  if (typ(y) != tx || tx!=t_QFI) err(typeer,"composition");
  if (cmpii((GEN)x[1],(GEN)y[1]) > 0) { z=x; x=y; y=z; }
  z=cgetg(4,t_QFI); comp_gen(z,x,y);
  if (raw) return gerepilecopy(av,z);
  return gerepileupto(av, redimag(z));
}

static GEN
compreal0(GEN x, GEN y, int raw)
{
  pari_sp av = avma;
  long tx = typ(x);
  GEN z;

  if (typ(y) != tx || tx!=t_QFR) err(typeer,"composition");
  z=cgetg(5,t_QFR); comp_gen(z,x,y);
  z[4]=laddrr((GEN)x[4],(GEN)y[4]);
  if (raw) return gerepilecopy(av,z);
  return gerepileupto(av,redreal(z));
}

GEN
compreal(GEN x, GEN y) { return compreal0(x,y,0); }

GEN
comprealraw(GEN x, GEN y) { return compreal0(x,y,1); }

GEN
compimag(GEN x, GEN y) { return compimag0(x,y,0); }

GEN
compimagraw(GEN x, GEN y) { return compimag0(x,y,1); }

GEN
compraw(GEN x, GEN y)
{
  return (typ(x)==t_QFI)? compimagraw(x,y): comprealraw(x,y);
}

GEN
sqcompimag0(GEN x, long raw)
{
  pari_sp av = avma;
  GEN z = cgetg(4,t_QFI);

  if (typ(x)!=t_QFI) err(typeer,"composition");
  sq_gen(z,x);
  if (raw) return gerepilecopy(av,z);
  return gerepileupto(av,redimag(z));
}

static GEN
sqcompreal0(GEN x, long raw)
{
  pari_sp av = avma;
  GEN z = cgetg(5,t_QFR);

  if (typ(x)!=t_QFR) err(typeer,"composition");
  sq_gen(z,x); z[4]=lshiftr((GEN)x[4],1);
  if (raw) return gerepilecopy(av,z);
  return gerepileupto(av,redreal(z));
}

GEN
sqcompreal(GEN x) { return sqcompreal0(x,0); }

GEN
sqcomprealraw(GEN x) { return sqcompreal0(x,1); }

GEN
sqcompimag(GEN x) { return sqcompimag0(x,0); }

GEN
sqcompimagraw(GEN x) { return sqcompimag0(x,1); }

static GEN
real_unit_form_by_disc(GEN D, long prec)
{
  GEN y = cgetg(5,t_QFR), isqrtD;
  pari_sp av = avma;

  if (typ(D) != t_INT || signe(D) <= 0) err(typeer,"real_unit_form_by_disc");
  switch(mod4(D))
  {
    case 2:
    case 3: err(funder2,"real_unit_form_by_disc");
  }
  y[1]=un; isqrtD = racine(D);
  /* we know that D and isqrtD are non-zero */
  if (mod2(D) != mod2(isqrtD))
    isqrtD = gerepileuptoint(av, addsi(-1,isqrtD));
  y[2] = (long)isqrtD; av = avma;
  y[3] = lpileuptoint(av, shifti(subii(sqri(isqrtD),D),-2));
  y[4] = (long)realzero(prec); return y;
}

GEN
real_unit_form(GEN x)
{
  pari_sp av = avma;
  long prec;
  GEN D;
  if (typ(x) != t_QFR) err(typeer,"real_unit_form");
  prec = precision((GEN)x[4]);
  if (!prec) err(talker,"not a t_REAL in 4th component of a t_QFR");
  D = qf_disc(x,NULL,NULL);
  return gerepileupto(av, real_unit_form_by_disc(D,prec));
}

static GEN
imag_unit_form_by_disc(GEN D)
{
  GEN y = cgetg(4,t_QFI);
  long isodd;

  if (typ(D) != t_INT || signe(D) >= 0) err(typeer,"imag_unit_form_by_disc");
  switch(mod4(D))
  {
    case 0: isodd = 0; break;
    case 3: isodd = 1; break;
    default: err(funder2,"imag_unit_form_by_disc");
             return NULL; /* not reached */
  }
  y[1] = un;
  y[2] = isodd? un: zero;
  /* upon return, y[3] = (1-D) / 4 or -D / 4, whichever is an integer */
  y[3] = lshifti(D,-2);
  if (isodd)
  {
    pari_sp av = avma;
    y[3] = lpileuptoint(av, addis((GEN)y[3],-1));
  }
  /* at this point y[3] < 0 */
  setsigne(y[3], 1); return y;
}

GEN
imag_unit_form(GEN x)
{
  GEN p1,p2, y = cgetg(4,t_QFI);
  pari_sp av;
  if (typ(x) != t_QFI) err(typeer,"imag_unit_form");
  y[1] = un;
  y[2] = mpodd((GEN)x[2])? un: zero;
  av = avma; p1 = mulii((GEN)x[1],(GEN)x[3]);
  p2 = shifti(sqri((GEN)x[2]),-2);
  y[3] = lpileuptoint(av, subii(p1,p2));
  return y;
}

static GEN
invraw(GEN x)
{
  GEN y = gcopy(x);
  setsigne(y[2], -signe(y[2]));
  if (typ(y) == t_QFR) setsigne(y[4], -signe(y[4]));
  return y;
}

GEN
powrealraw(GEN x, long n)
{
  pari_sp av = avma;
  long m;
  GEN y;

  if (typ(x) != t_QFR)
    err(talker,"not a real quadratic form in powrealraw");
  if (!n) return real_unit_form(x);
  if (n== 1) return gcopy(x);
  if (n==-1) return invraw(x);

  y = NULL; m=labs(n);
  for (; m>1; m>>=1)
  {
    if (m&1) y = y? comprealraw(y,x): x;
    x=sqcomprealraw(x);
  }
  y = y? comprealraw(y,x): x;
  if (n<0) y = invraw(y);
  return gerepileupto(av,y);
}

GEN
powimagraw(GEN x, long n)
{
  pari_sp av = avma;
  long m;
  GEN y;

  if (typ(x) != t_QFI)
    err(talker,"not an imaginary quadratic form in powimag");
  if (!n) return imag_unit_form(x);
  if (n== 1) return gcopy(x);
  if (n==-1) return invraw(x);

  y = NULL; m=labs(n);
  for (; m>1; m>>=1)
  {
    if (m&1) y = y? compimagraw(y,x): x;
    x=sqcompimagraw(x);
  }
  y = y? compimagraw(y,x): x;
  if (n<0) y = invraw(y);
  return gerepileupto(av,y);
}

GEN
powraw(GEN x, long n)
{
  return (typ(x)==t_QFI)? powimagraw(x,n): powrealraw(x,n);
}

/* composition: Shanks' NUCOMP & NUDUPL */
/* l = floor((|d|/4)^(1/4)) */
GEN
nucomp(GEN x, GEN y, GEN l)
{
  pari_sp av=avma,tetpil;
  long cz;
  GEN a,a1,a2,b,b2,d,d1,e,g,n,p1,p2,p3,q1,q2,q3,q4,s,t2,t3,u,u1,v,v1,v2,v3,z;

  if (x==y) return nudupl(x,l);
  if (typ(x) != t_QFI || typ(y) != t_QFI)
    err(talker,"not an imaginary quadratic form in nucomp");

  if (cmpii((GEN)x[1],(GEN)y[1]) < 0) { s=x; x=y; y=s; }
  s=shifti(addii((GEN)x[2],(GEN)y[2]),-1); n=subii((GEN)y[2],s);
  a1=(GEN)x[1]; a2=(GEN)y[1]; d=bezout(a2,a1,&u,&v);
  if (gcmp1(d)) { a=negi(gmul(u,n)); d1=d; }
  else
    if (divise(s,d))
    {
      a=negi(gmul(u,n)); d1=d; a1=divii(a1,d1);
      a2=divii(a2,d1); s=divii(s,d1);
    }
    else
    {
      d1=bezout(s,d,&u1,&v1);
      if (!gcmp1(d1))
      {
        a1=divii(a1,d1); a2=divii(a2,d1);
        s=divii(s,d1); d=divii(d,d1);
      }
      p1=resii((GEN)x[3],d); p2=resii((GEN)y[3],d);
      p3=modii(negi(mulii(u1,addii(mulii(u,p1),mulii(v,p2)))),d);
      a=subii(mulii(p3,divii(a1,d)),mulii(u,divii(n,d)));
    }
  a=modii(a,a1); p1=subii(a1,a); if (cmpii(a,p1)>0) a=negi(p1);
  v=gzero; d=a1; v2=gun; v3=a;
  for (cz=0; absi_cmp(v3,l) > 0; cz++)
  {
    p1=dvmdii(d,v3,&t3); t2=subii(v,mulii(p1,v2));
    v=v2; d=v3; v2=t2; v3=t3;
  }
  z=cgetg(4,t_QFI);
  if (!cz)
  {
    q1=mulii(a2,v3); q2=addii(q1,n);
    g=divii(addii(mulii(v3,s),(GEN)y[3]),d);
    z[1]=lmulii(d,a2);
    z[2]=laddii(shifti(q1,1),(GEN)y[2]);
    z[3]=laddii(mulii(v3,divii(q2,d)), mulii(g,d1));
  }
  else
  {
    if (cz&1) { v3=negi(v3); v2=negi(v2); }
    b=divii(addii(mulii(a2,d),mulii(n,v)),a1);
    q1=mulii(b,v3); q2=addii(q1,n);
    e=divii(addii(mulii(s,d),mulii((GEN)y[3],v)),a1);
    q3=mulii(e,v2); q4=subii(q3,s);
    g=divii(q4,v); b2=addii(q3,q4);
    if (!gcmp1(d1)) { v2=mulii(d1,v2); v=mulii(d1,v); b2=mulii(d1,b2); }
    z[1]=laddii(mulii(d,b),mulii(e,v));
    z[2]=laddii(b2,addii(q1,q2));
    z[3]=laddii(mulii(v3,divii(q2,d)), mulii(g,v2));
  }
  tetpil=avma; return gerepile(av,tetpil,redimag(z));
}

GEN
nudupl(GEN x, GEN l)
{
  pari_sp av=avma,tetpil;
  long cz;
  GEN u,v,d,d1,p1,a,b,c,b2,z,v2,v3,t2,t3,e,g;

  if (typ(x) != t_QFI)
    err(talker,"not an imaginary quadratic form in nudupl");
  d1=bezout((GEN)x[2],(GEN)x[1],&u,&v);
  a=divii((GEN)x[1],d1); b=divii((GEN)x[2],d1);
  c=modii(negi(mulii(u,(GEN)x[3])),a); p1=subii(a,c);
  if (cmpii(c,p1)>0) c=negi(p1);
  v=gzero; d=a; v2=gun; v3=c;
  for (cz=0; absi_cmp(v3,l) > 0; cz++)
  {
    p1=dvmdii(d,v3,&t3); t2=subii(v,mulii(p1,v2));
    v=v2; d=v3; v2=t2; v3=t3;
  }
  z=cgetg(4,t_QFI);
  if (!cz)
  {
    g=divii(addii(mulii(v3,b),(GEN)x[3]),d);
    z[1]=(long)sqri(d);
    z[2]=laddii((GEN)x[2],shifti(mulii(d,v3),1));
    z[3]=laddii(sqri(v3),mulii(g,d1));
  }
  else
  {
    if (cz&1) { v=negi(v); d=negi(d); }
    e=divii(addii(mulii((GEN)x[3],v),mulii(b,d)),a);
    g=divii(subii(mulii(e,v2),b),v);
    b2=addii(mulii(e,v2),mulii(v,g));
    if (!gcmp1(d1)) { v2=mulii(d1,v2); v=mulii(d1,v); b2=mulii(d1,b2); }
    z[1]=laddii(sqri(d),mulii(e,v));
    z[2]=laddii(b2,shifti(mulii(d,v3),1));
    z[3]=laddii(sqri(v3),mulii(g,v2));
  }
  tetpil=avma; return gerepile(av,tetpil,redimag(z));
}

static GEN
mul_nucomp(void *l, GEN x, GEN y) { return nucomp(x, y, (GEN)l); }
static GEN
mul_nudupl(void *l, GEN x) { return nudupl(x, (GEN)l); }

GEN
nupow(GEN x, GEN n)
{
  pari_sp av;
  GEN y, l;

  if (typ(n) != t_INT) err(talker,"not an integer exponent in nupow");
  if (gcmp1(n)) return gcopy(x);
  av = avma; y = imag_unit_form(x);
  if (!signe(n)) return y;

  l = gclone( racine(shifti(racine((GEN)y[3]),1)) );
  avma = av;
  y = leftright_pow(x, n, (void*)l, &mul_nudupl, &mul_nucomp);
  gunclone(l);
  if (signe(n) < 0 && !egalii((GEN)y[1],(GEN)y[2])
                   && !egalii((GEN)y[1],(GEN)y[3])) setsigne(y[2],-signe(y[2]));
  return y;
}

/* reduction */

static GEN
abs_dvmdii(GEN b, GEN p1, GEN *pt, long s)
{
  if (s<0) setsigne(b, 1); p1 = dvmdii(b,p1,pt);
  if (s<0) setsigne(b,-1); return p1;
}

static GEN
rhoimag0(GEN x, long *flag)
{
  GEN p1,b,d,z;
  long fl, s = signe(x[2]);

  fl = cmpii((GEN)x[1], (GEN)x[3]);
  if (fl <= 0)
  {
    long fg = absi_cmp((GEN)x[1], (GEN)x[2]);
    if (fg >= 0)
    {
      *flag = (s<0 && (!fl || !fg))? 2 /* set x[2] = negi(x[2]) in caller */
                                   : 1;
      return x;
    }
  }
  p1=shifti((GEN)x[3],1); d = abs_dvmdii((GEN)x[2],p1,&b,s);
  if (s>=0)
  {
    setsigne(d,-signe(d));
    if (cmpii(b,(GEN)x[3])<=0) setsigne(b,-signe(b));
    else { d=addsi(-1,d); b=subii(p1,b); }
  }
  else if (cmpii(b,(GEN)x[3])>=0) { d=addsi(1,d); b=subii(b,p1); }

  z=cgetg(4,t_QFI);
  z[1] = x[3];
  z[3] = laddii((GEN)x[1], mulii(d,shifti(subii((GEN)x[2],b),-1)));
  if (signe(b)<0 && !fl) setsigne(b,1);
  z[2] = (long)b; *flag = 0; return z;
}

static void
fix_expo(GEN x)
{
  long e = expo(x[5]);
  if (e >= EXP220)
  {
    x[4] = laddsi(1,(GEN)x[4]);
    setexpo(x[5], e - EXP220);
  }
}

GEN
rhoreal_aux(GEN x, GEN D, GEN sqrtD, GEN isqrtD)
{
  GEN p1,p2, y = cgetg(6,t_VEC);
  GEN b = (GEN)x[2];
  GEN c = (GEN)x[3];

  y[1] = (long)c;
  p2 = (absi_cmp(isqrtD,c) >= 0)? isqrtD: absi(c);
  p1 = shifti(c,1);
  if (p1 == gzero) err(talker, "reducible form in rhoreal");
  setsigne(p1,1); /* |2c| */
  p2 = divii(addii(p2,b), p1);
  y[2] = lsubii(mulii(p2,p1), b);

  p1 = shifti(subii(sqri((GEN)y[2]),D),-2);
  y[3] = ldivii(p1,(GEN)y[1]);

  if (lg(x) <= 5) setlg(y,4);
  else
  {
    y[4] = x[4];
    y[5] = x[5];
    if (signe(b))
    {
      p1 = divrr(addir(b,sqrtD), subir(b,sqrtD));
      y[5] = lmulrr(p1, (GEN)y[5]);
      fix_expo(y);
    }
  }
  return y;
}

#define qf_NOD  2
#define qf_STEP 1

GEN
codeform5(GEN x, long prec)
{
  GEN y = cgetg(6,t_VEC);
  y[1] = x[1];
  y[2] = x[2];
  y[3] = x[3];
  y[4] = zero;
  y[5] = (long)realun(prec); return y;
}

static GEN
add_distance(GEN x, GEN d0)
{
  GEN y = cgetg(5, t_QFR);
  y[1] = licopy((GEN)x[1]);
  y[2] = licopy((GEN)x[2]);
  y[3] = licopy((GEN)x[3]);
  y[4] = lcopy(d0); return y;
}

/* d0 = initial distance, assume |n| > 1 */
static GEN
decodeform(GEN x, GEN d0)
{
  GEN p1,p2;

  if (lg(x) < 6) return add_distance(x,d0);
  /* x = (a,b,c, expo(d), d), d = exp(2*distance) */
  p1 = absr((GEN)x[5]);
  p2 = (GEN)x[4];
  if (signe(p2))
  {
    long e = expo(p1);
    p1 = shiftr(p1,-e);
    p2 = addis(mulsi(EXP220,p2), e);
    p1 = mplog(p1);
    p1 = mpadd(p1, mulir(p2, mplog2(lg(d0))));
  }
  else
  { /* to avoid loss of precision */
    p1 = gcmp1(p1)? NULL: mplog(p1);
  }
  if (p1)
    d0 = addrr(d0, shiftr(p1,-1));
  return add_distance(x, d0);
}

static long
get_prec(GEN d)
{
  long k = lg(d);
  long l = ((BITS_IN_LONG-1-expo(d))>>TWOPOTBITS_IN_LONG)+2;
  if (l < k) l = k;
  if (l < 3) l = 3;
  return l;
}

static int
real_isreduced(GEN x, GEN isqrtD)
{
  GEN a = (GEN)x[1];
  GEN b = (GEN)x[2];
  if (signe(b) > 0 && cmpii(b,isqrtD) <= 0 )
  {
    GEN p1 = subii(isqrtD, shifti(absi(a),1));
    long l = absi_cmp(b, p1);
    if (l > 0 || (l == 0 && signe(p1) < 0)) return 1;
  }
  return 0;
}

GEN
redrealform5(GEN x, GEN D, GEN sqrtD, GEN isqrtD)
{
  while (!real_isreduced(x,isqrtD))
    x = rhoreal_aux(x,D,sqrtD,isqrtD);
  return x;
}

static GEN
redreal0(GEN x, long flag, GEN D, GEN isqrtD, GEN sqrtD)
{
  pari_sp av = avma;
  long prec;
  GEN d0;

  if (typ(x) != t_QFR) err(talker,"not a real quadratic form in redreal");

  if (!D)
    D = qf_disc(x,NULL,NULL);
  else
    if (typ(D) != t_INT) err(arither1);

  d0 = (GEN)x[4]; prec = get_prec(d0);
  x = codeform5(x,prec);
  if ((flag & qf_NOD)) setlg(x,4);
  else
  {
    if (!sqrtD)
      sqrtD = gsqrt(D,prec);
    else
    {
      long l = typ(sqrtD);
      if (!is_intreal_t(l)) err(arither1);
    }
  }
  if (!isqrtD)
    isqrtD = sqrtD? mptrunc(sqrtD): racine(D);
  else
    if (typ(isqrtD) != t_INT) err(arither1);

  if (flag & qf_STEP)
    x = rhoreal_aux(x,D,sqrtD,isqrtD);
  else
    x = redrealform5(x,D,sqrtD,isqrtD);
  return gerepileupto(av, decodeform(x,d0));
}

GEN
comprealform5(GEN x, GEN y, GEN D, GEN sqrtD, GEN isqrtD)
{
  pari_sp av = avma;
  GEN z = cgetg(6,t_VEC); comp_gen(z,x,y);
  if (x == y)
  {
    z[4] = lshifti((GEN)x[4],1);
    z[5] = lsqr((GEN)x[5]);
  }
  else
  {
    z[4] = laddii((GEN)x[4],(GEN)y[4]);
    z[5] = lmulrr((GEN)x[5],(GEN)y[5]);
  }
  fix_expo(z); z = redrealform5(z,D,sqrtD,isqrtD);
  return gerepilecopy(av,z);
}

/* assume n!=0 */
GEN
powrealform(GEN x, GEN n)
{
  pari_sp av = avma;
  long i,m;
  GEN y,D,sqrtD,isqrtD,d0;

  if (typ(x) != t_QFR)
    err(talker,"not a real quadratic form in powreal");
  if (gcmp1(n)) return gcopy(x);
  if (gcmp_1(n)) return ginv(x);

  d0 = (GEN)x[4];
  D = qf_disc(x,NULL,NULL);
  sqrtD = gsqrt(D, get_prec(d0));
  isqrtD = mptrunc(sqrtD);
  if (signe(n) < 0) { x = ginv(x); d0 = (GEN)x[4]; }
  n = absi(n);
  x = codeform5(x, lg(d0)); y = NULL;
  for (i=lgefint(n)-1; i>1; i--)
  {
    m = n[i];
    for (; m; m>>=1)
    {
      if (m&1) y = y? comprealform5(y,x,D,sqrtD,isqrtD): x;
      if (m == 1 && i == 2) break;
      x = comprealform5(x,x,D,sqrtD,isqrtD);
    }
  }
  d0 = mulri(d0,n);
  return gerepileupto(av, decodeform(y,d0));
}

GEN
redimag(GEN x)
{
  pari_sp av=avma;
  long fl;
  do x = rhoimag0(x, &fl); while (fl == 0);
  x = gerepilecopy(av,x);
  if (fl == 2) setsigne(x[2], -signe(x[2]));
  return x;
}

GEN
redreal(GEN x)
{
  return redreal0(x,0,NULL,NULL,NULL);
}

GEN
rhoreal(GEN x)
{
  return redreal0(x,qf_STEP,NULL,NULL,NULL);
}

GEN
redrealnod(GEN x, GEN isqrtD)
{
  return redreal0(x,qf_NOD,NULL,isqrtD,NULL);
}

GEN
rhorealnod(GEN x, GEN isqrtD)
{
  return redreal0(x,qf_STEP|qf_NOD,NULL,isqrtD,NULL);
}

GEN
qfbred0(GEN x, long flag, GEN D, GEN isqrtD, GEN sqrtD)
{
  long tx=typ(x),fl;
  pari_sp av;

  if (!is_qf_t(tx)) err(talker,"not a quadratic form in qfbred");
  if (!D) D = qf_disc(x,NULL,NULL);
  switch(signe(D))
  {
    case 1 :
      return redreal0(x,flag,D,isqrtD,sqrtD);

    case -1:
      if (!(flag & qf_STEP)) return redimag(x);
      av = avma; x = rhoimag0(x,&fl);
      x = gerepilecopy(av,x);
      if (fl == 2) setsigne(x[2], -signe(x[2]));
      return x;
  }
  err(redpoler,"qfbred");
  return NULL; /* not reached */
}

/* special case: p = 1 return unit form */
GEN
primeform(GEN x, GEN p, long prec)
{
  pari_sp av;
  long s, sx = signe(x);
  GEN y, b, c;

  if (typ(x) != t_INT || !sx) err(arither1);
  if (typ(p) != t_INT || signe(p) <= 0) err(arither1);
  if (is_pm1(p))
    return sx<0? imag_unit_form_by_disc(x)
               : real_unit_form_by_disc(x,prec);
  s = mod8(x);
  if (sx < 0)
  {
    if (s) s = 8-s;
    y = cgetg(4, t_QFI);
  }
  else
  {
    y = cgetg(5, t_QFR);
    y[4] = (long)realzero(prec);
  }
  switch(s&3)
  {
    case 2: case 3: err(funder2,"primeform");
  }
  av = avma;
  if (egalii(p, gdeux))
  {
    switch(s)
    {
      case 0: b = gzero; break;
      case 1: b = gun;   break;
      case 4: b = gdeux; break;
      default: err(sqrter5); b = NULL; /* -Wall */
    }
    c = shifti(subsi(s,x), -3);
  }
  else
  {
    b = mpsqrtmod(x,p); if (!b) err(sqrter5);
    s &= 1; /* s = x mod 2 */
    /* mod(b) != mod2(x) ? [Warning: we may have b == 0] */
    if ((!signe(b) && s) || mod2(b) != s) b = gerepileuptoint(av, subii(p,b));

    av = avma;
    c = diviiexact(shifti(subii(sqri(b), x), -2), p);
  }
  y[3] = lpileuptoint(av, c);
  y[2] = (long)b;
  y[1] = licopy(p); return y;
}

GEN 
redimagsl2(GEN V)
{
  pari_sp ltop = avma;
  pari_sp st_lim = stack_lim(ltop, 1);
  GEN a,b,c;
  GEN z;
  GEN u1,u2,v1,v2;
  long skip = 1;
  GEN p2,p3; 
  a = (GEN) V[1]; b = (GEN) V[2]; c = (GEN) V[3];
  u1 = v2 = gun;
  u2 = v1 = gzero;
  if (cmpii(negi(a), b) < 0 && cmpii(b, a) <= 0)
    skip = 0;
  for(;;)
  {
    if (skip)
    {
      GEN a2= shifti(a, 1);
      GEN D = divrem(b, a2, -1);
      GEN q = (GEN) D[1];
      GEN r = (GEN) D[2];
      if (cmpii(r, a) > 0)
      {
        q = addis(q, 1);
        r = subii(r, a2);
      }
      c = subii(c, shifti(mulii(q, addii(b, r)), -1));
      b = r;
      u2 = subii(u2, mulii(q, u1));
      v2 = subii(v2, mulii(q, v1));
    }
    skip = 1;
    if (cmpii(a, c) > 0)
    {
      b = negi(b);
      z = a; a = c; c = z;
      z = u1; u1 = u2; u2 = negi(z);
      z = v1; v1 = v2; v2 = negi(z);
    }
    else
      break;
    if (low_stack(st_lim, stack_lim(ltop, 1)))
    {
      GEN *bptr[7];
      bptr[0]=&a; bptr[1]=&b; bptr[2]=&c;
      bptr[3]=&u1; bptr[4]=&u2;
      bptr[5]=&v1; bptr[6]=&v2;
      gerepilemany(ltop, bptr, 7);
    }
  }
  if (egalii(a, c) && (cmpis(b, 0) < 0))
  {
    b = negi(b);
    z = u1; u1 = u2; u2 = negi(z);
    z = v1; v1 = v2; v2 = negi(z);
  }
  p2 = cgetg(3, t_VEC);
  p2[1] = lgetg(4,t_QFI);
  mael(p2, 1, 1) = licopy(a);
  mael(p2, 1, 2) = licopy(b);
  mael(p2, 1, 3) = licopy(c);
  p3 = cgetg(3, t_MAT);
  p3[1] = lgetg(3, t_COL);
  coeff(p3, 1, 1) = licopy(u1);
  coeff(p3, 2, 1) = licopy(v1);
  p3[2] = lgetg(3, t_COL);
  coeff(p3, 1, 2) = licopy(u2);
  coeff(p3, 2, 2) = licopy(v2);
  p2[2] = (long) p3;
  return gerepileupto(ltop, p2);
}

GEN qfbimagsolvep(GEN Q,GEN p)
{
  pari_sp ltop=avma;
  GEN N,d;
  GEN P,M,res; 
  N=redimagsl2(Q);
  d=qf_disc(Q, NULL, NULL);
  if (kronecker(d,p)==-1) 
    return gzero;
  P=primeform(d, p, 0);
  M=redimagsl2(P);
  if (!gegal((GEN)M[1], (GEN)N[1]))
    return gzero;
  res=(GEN)gdiv((GEN)N[2], (GEN)M[2])[1];
  return gerepilecopy(ltop,res);
}

GEN qfbsolve(GEN Q,GEN n)
{
  if (typ(Q)!=t_QFI || typ(n)!=t_INT)
    err(typeer,"qfbsolve");
  return qfbimagsolvep(Q,n);
}

/*********************************************************************/
/**                                                                 **/
/**                       BINARY DECOMPOSITION                      **/
/**                                                                 **/
/*********************************************************************/

GEN
binaire(GEN x)
{
  ulong m,u;
  long i,lx,ex,ly,tx=typ(x);
  GEN y,p1,p2;

  switch(tx)
  {
    case t_INT:
    {
      GEN xp=int_MSW(x);
      lx=lgefint(x);
      if (lx==2) { y=cgetg(2,t_VEC); y[1]=zero; return y; }
      ly = BITS_IN_LONG+1; m=HIGHBIT; u=*xp;
      while (!(m & u)) { m>>=1; ly--; }
      y = cgetg(ly+((lx-3)<<TWOPOTBITS_IN_LONG),t_VEC); ly=1;
      do { y[ly] = m & u ? un : zero; ly++; } while (m>>=1);
      for (i=3; i<lx; i++)
      {
        m=HIGHBIT; xp=int_precW(xp); u=*xp;
        do { y[ly] = m & u ? un : zero; ly++; } while (m>>=1);
      }
      break;
    }
    case t_REAL:
      ex=expo(x);
      if (!signe(x))
      {
        lx=1+max(-ex,0); y=cgetg(lx,t_VEC);
        for (i=1; i<lx; i++) y[i]=zero;
        return y;
      }

      lx=lg(x); y=cgetg(3,t_VEC);
      if (ex > bit_accuracy(lx)) err(precer,"binary");
      p1 = cgetg(max(ex,0)+2,t_VEC);
      p2 = cgetg(bit_accuracy(lx)-ex,t_VEC);
      y[1] = (long) p1; y[2] = (long) p2;
      ly = -ex; ex++; m = HIGHBIT;
      if (ex<=0)
      {
        p1[1]=zero; for (i=1; i <= -ex; i++) p2[i]=zero;
        i=2;
      }
      else
      {
        ly=1;
        for (i=2; i<lx && ly<=ex; i++)
        {
          m=HIGHBIT; u=x[i];
          do
            { p1[ly] = (m & u) ? un : zero; ly++; }
          while ((m>>=1) && ly<=ex);
        }
        ly=1;
        if (m) i--; else m=HIGHBIT;
      }
      for (; i<lx; i++)
      {
        u=x[i];
        do { p2[ly] = m & u ? un : zero; ly++; } while (m>>=1);
        m=HIGHBIT;
      }
      break;

    case t_VEC: case t_COL: case t_MAT:
      lx=lg(x); y=cgetg(lx,tx);
      for (i=1; i<lx; i++) y[i]=(long)binaire((GEN)x[i]);
      break;
    default: err(typeer,"binary");
      return NULL; /* not reached */
  }
  return y;
}

/* return 1 if bit n of x is set, 0 otherwise */
long
bittest(GEN x, long n)
{
  ulong u;
  long l;

  if (!signe(x) || n < 0) return 0;
  l = n>>TWOPOTBITS_IN_LONG;
  if (l+3 > lgefint(x)) return 0;
  u = (1UL << (n & (BITS_IN_LONG-1))) & *int_W(x,l);
  return u? 1: 0;
}

GEN
gbittest(GEN x, GEN n)
{
  return arith_proto2gs(bittest,x,itos(n));
}

/***********************************************************************/
/**                                                                   **/
/**                          BITMAP OPS                               **/
/** x & y (and), x | y (or), x ^ y (xor), ~x (neg), x & ~y (negimply) **/
/**                                                                   **/
/***********************************************************************/
extern GEN int_normalize(GEN x, long known_zero_words);

INLINE GEN
inegate(GEN z) { return subsi(-1,z); }

/* Truncate a non-negative integer to a number of bits.  */
static GEN
ibittrunc(GEN x, long bits)
{
  int xl = lgefint(x) - 2;
  int len_out = ((bits + BITS_IN_LONG - 1) >> TWOPOTBITS_IN_LONG);
  int known_zero_words;

  if (xl < len_out)
      return x;
      /* Check whether mask is trivial */
  if (!(bits & (BITS_IN_LONG - 1))) {
      if (xl == len_out)
          return x;
  } else if (len_out <= xl) {
    GEN xi = int_W(x, len_out-1);
    /* Non-trival mask is given by a formula, if x is not
       normalized, this works even in the exceptional case */
    *xi = *xi & ((1 << (bits & (BITS_IN_LONG - 1))) - 1);
    if (*xi && xl == len_out) return x;
  }
  /* Normalize */
  known_zero_words = xl - len_out;
  if (known_zero_words < 0) known_zero_words = 0;
  return int_normalize(x, known_zero_words);
}

GEN
gbitneg(GEN x, long bits)
{
  const ulong uzero = 0;
  long xl, len_out, i;

  if (typ(x) != t_INT)
      err(typeer, "bitwise negation");
  if (bits < -1)
      err(talker, "negative exponent in bitwise negation");
  if (bits == -1) return inegate(x);
  if (bits == 0) return gzero;
  if (signe(x) < 0) { /* Consider as if mod big power of 2 */
    pari_sp ltop = avma;
    return gerepileuptoint(ltop, ibittrunc(inegate(x), bits));
  }
  xl = lgefint(x);
  len_out = ((bits + BITS_IN_LONG - 1) >> TWOPOTBITS_IN_LONG) + 2;
  if (len_out > xl) { /* Need to grow */
    GEN out, outp, xp = int_MSW(x);
    out = cgeti(len_out); out[1] = evalsigne(1) | evallgefint(len_out);
    outp = int_MSW(out);
    if (!(bits & (BITS_IN_LONG - 1)))
      *outp = ~uzero;
    else
      *outp = (1 << (bits & (BITS_IN_LONG - 1))) - 1;
    for (i = 3; i < len_out - xl + 2; i++)
    {
      outp = int_precW(outp); *outp = ~uzero;
    }
    for (     ; i < len_out; i++)
    {
      outp = int_precW(outp); *outp = ~*xp;
      xp   = int_precW(xp);
    }
    return out;
  }
  x = icopy(x);
  for (i = 2; i < xl; i++) x[i] = ~x[i];
  return ibittrunc(x, bits);
}

/* bitwise 'and' of two positive integers (any integers, but we ignore sign).
 * Inputs are not necessary normalized. */
GEN
ibitand(GEN x, GEN y)
{
  long lx, ly, lout;
  long *xp, *yp, *outp;
  GEN out;
  long i;

  if (!signe(x) || !signe(y)) return gzero;
  lx=lgefint(x); ly=lgefint(y);
  lout = min(lx,ly); /* > 2 */
  xp = int_LSW(x);
  yp = int_LSW(y);
  out = cgeti(lout); out[1] = evalsigne(1) | evallgefint(lout);
  outp = int_LSW(out);
  for (i=2; i<lout; i++)
  {
    *outp = (*xp) & (*yp);
    outp  = int_nextW(outp);
    xp    = int_nextW(xp);
    yp    = int_nextW(yp);
  }
  if ( !*int_MSW(out) ) out = int_normalize(out, 1);
  return out;
}

#define swaplen(x,y, nx,ny) {long _a=nx;GEN _z=x; nx=ny; ny=_a; x=y; y=_z;}

/* bitwise 'or' of absolute values of two integers */
GEN
ibitor(GEN x, GEN y)
{
  long lx, ly;
  long *xp, *yp, *outp;
  GEN  out;
  long i;
  if (!signe(x)) return absi(y);
  if (!signe(y)) return absi(x);

  lx = lgefint(x); xp = int_LSW(x);
  ly = lgefint(y); yp = int_LSW(y);
  if (lx < ly) swaplen(xp,yp,lx,ly);
  /* lx > 2 */
  out = cgeti(lx); out[1] = evalsigne(1) | evallgefint(lx);
  outp = int_LSW(out);
  for (i=2;i<ly;i++)
  {
    *outp = (*xp) | (*yp);
    outp  = int_nextW(outp);
    xp    = int_nextW(xp);
    yp    = int_nextW(yp);
  }
  for (   ;i<lx;i++)
  {
    *outp = *xp;
    outp  = int_nextW(outp);
    xp    = int_nextW(xp);
  }
  /* If input is normalized, this is not needed */
  if ( !*int_MSW(out) ) out = int_normalize(out, 1);
  return out;
}

/* bitwise 'xor' of absolute values of two integers */
GEN
ibitxor(GEN x, GEN y)
{
  long lx, ly;
  long *xp, *yp, *outp;
  GEN  out;
  long i;
  if (!signe(x)) return absi(y);
  if (!signe(y)) return absi(x);

  lx = lgefint(x); xp = int_LSW(x);
  ly = lgefint(y); yp = int_LSW(y);
  if (lx < ly) swaplen(xp,yp,lx,ly);
  /* lx > 2 */
  out = cgeti(lx); out[1] = evalsigne(1) | evallgefint(lx);
  outp = int_LSW(out);
  for (i=2;i<ly;i++)
  {
    *outp = (*xp) ^ (*yp);
    outp  = int_nextW(outp);
    xp    = int_nextW(xp);
    yp    = int_nextW(yp);
  }
  for (   ;i<lx;i++)
  {
    *outp = *xp;
    outp  = int_nextW(outp);
    xp    = int_nextW(xp);
  }
  if ( !*int_MSW(out) ) out = int_normalize(out, 1);
  return out;
}

/* bitwise 'negimply' of absolute values of two integers */
/* "negimply(x,y)" is ~(x => y) == ~(~x | y) == x & ~y   */
GEN
ibitnegimply(GEN x, GEN y)
{
  long lx, ly, lout, lin;
  long *xp, *yp, *outp;
  GEN out;
  long i;
  if (!signe(x)) return gzero;
  if (!signe(y)) return absi(x);

  lx = lgefint(x); xp = int_LSW(x);
  ly = lgefint(y); yp = int_LSW(y);
  if (lx < ly) { lin = lx; lout = ly; } else { lin = ly; lout = lx; }
  /* lout > 2 */
  out = cgeti(lout); out[1] = evalsigne(1) | evallgefint(lout);
  outp = int_LSW(out);
  for (i=2; i<lin; i++)
  {
    *outp = (*xp) & ~(*yp);
    outp  = int_nextW(outp);
    xp    = int_nextW(xp);
    yp    = int_nextW(yp);
  }
  for (   ;i<lx;i++)
  {
    *outp = *xp;
    outp  = int_nextW(outp);
    xp    = int_nextW(xp);
  }
  for (   ;i<ly;i++)
  {
    *outp = ~(*yp);
    outp  = int_nextW(outp);
    yp    = int_nextW(yp);
  }
  if ( !*int_MSW(out) ) out = int_normalize(out, 1);
  return out;
}

#define signs(x,y) (((signe(x) >= 0) << 1) | (signe(y) >= 0))

GEN
gbitor(GEN x, GEN y)
{
  pari_sp ltop = avma;
  GEN z;

  if (typ(x) != t_INT || typ(y) != t_INT) err(typeer, "bitwise or");
  switch (signs(x, y))
  {
    case 3: /*1,1*/
      return ibitor(x,y);
    case 2: /*1,-1*/
      z = ibitnegimply(inegate(y),x);
      break;
    case 1: /*-1,1*/
      z = ibitnegimply(inegate(x),y);
      break;
    case 0: /*-1,-1*/
      z = ibitand(inegate(x),inegate(y));
      break;
    default: return NULL;
  }
  return gerepileupto(ltop, inegate(z));
}

GEN
gbitand(GEN x, GEN y)
{
  pari_sp ltop = avma;
  GEN z;

  if (typ(x) != t_INT || typ(y) != t_INT) err(typeer, "bitwise and");
  switch (signs(x, y))
  {
    case 3: /*1,1*/
      return ibitand(x,y);
    case 2: /*1,-1*/
      z = ibitnegimply(x,inegate(y));
      break;
    case 1: /*-1,1*/
      z = ibitnegimply(y,inegate(x));
      break;
    case 0: /*-1,-1*/
      z = inegate(ibitor(inegate(x),inegate(y)));
      break;
    default: return NULL;
  }
  return gerepileupto(ltop, z);
}

GEN
gbitxor(GEN x, GEN y)
{
  pari_sp ltop = avma;
  GEN z;

  if (typ(x) != t_INT || typ(y) != t_INT) err(typeer, "bitwise xor");
  switch (signs(x, y))
  {
    case 3: /*1,1*/
      return ibitxor(x,y);
    case 2: /*1,-1*/
      z = inegate(ibitxor(x,inegate(y)));
      break;
    case 1: /*-1,1*/
      z = inegate(ibitxor(inegate(x),y));
      break;
    case 0: /*-1,-1*/
      z = ibitxor(inegate(x),inegate(y));
      break;
    default: return NULL;
  }
  return gerepileupto(ltop,z);
}

/* x & ~y */
GEN
gbitnegimply(GEN x, GEN y)
{
  pari_sp ltop = avma;
  GEN z;

  if (typ(x) != t_INT || typ(y) != t_INT) err(typeer, "bitwise negated imply");
  switch (signs(x, y))
  {
    case 3: /*1,1*/
      return ibitnegimply(x,y);
    case 2: /*1,-1*/
      z = ibitand(x,inegate(y));
      break;
    case 1: /*-1,1*/
      z = inegate(ibitor(y,inegate(x)));
      break;
    case 0: /*-1,-1*/
      z = ibitnegimply(inegate(y),inegate(x));
      break;
    default: return NULL;
  }
  return gerepileupto(ltop,z);
}
