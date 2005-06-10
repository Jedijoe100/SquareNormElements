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
/**                                                               **/
/**                        PARI CALCULATOR                        **/
/**                                                               **/
/*******************************************************************/
#include "pari.h"
#include "paripriv.h"

#ifdef _WIN32
#  include <windows.h>
#  ifndef WINCE
#    include <process.h>
#  endif
#endif
#ifdef HAS_STRFTIME
#  include <time.h>
#endif
#include "../language/anal.h"
#include "gp.h"

#ifdef READLINE
  int readline_init = 1;
BEGINEXTERN
#  if defined(__cplusplus) && defined(__SUNPRO_CC)
  extern char* readline(char*); /* bad prototype for readline() in readline.h */
#  else
#   ifdef READLINE_LIBRARY
#     include <readline.h>
#   else
#     include <readline/readline.h>
#   endif
#  endif
ENDEXTERN
#endif

static char *DFT_PRETTYPRINTER = "tex2mail -TeX -noindent -ragged -by_par";

#define MAX_PROMPT_LEN 128
#define DFT_PROMPT "? "
#define BREAK_LOOP_PROMPT "break> "
#define COMMENTPROMPT "comment> "
#define CONTPROMPT ""
#define DFT_INPROMPT ""
static char prompt[MAX_PROMPT_LEN], prompt_cont[MAX_PROMPT_LEN];

static int handle_C_C = 0, gpsilent = 0;
static int tm_is_waiting = 0, tm_did_complete = 0;

static ulong primelimit;

#define current_buffer (bufstack?((Buffer*)(bufstack->value)):NULL)
static stack *bufstack = NULL;

#define skip_space(s) while (isspace((int)*s)) s++
#define skip_alpha(s) while (isalpha((int)*s)) s++

static void
usage(char *s)
{
  printf("### Usage: %s [options] [GP files]\n", s);
  printf("Options are:\n");
  printf("\t[-f,--fast]\tFaststart: do not read .gprc\n");
  printf("\t[-q,--quiet]\tQuiet mode: do not print banner and history numbers\n");
  printf("\t[-p,--primelimit primelimit]\n\t\t\tPrecalculate primes up to the limit\n");
  printf("\t[-s,--stacksize stacksize]\n\t\t\tStart with the PARI stack of given size (in bytes)\n");
  printf("\t[--emacs]\tRun as if in Emacs shell\n");
  printf("\t[--help]\tPrint this message\n");
  printf("\t[--test]\tTest mode. No history, wrap long lines (bench only)\n");
  printf("\t[--texmacs]\tRun as if using TeXmacs frontend\n");
  printf("\t[--version]\tOutput version info and exit\n");
  printf("\t[--version-short]\tOutput version number and exit\n\n");
  exit(0);
}

static void
init_hist(gp_hist *H, size_t l, ulong total)
{
  H->total = total;
  H->size = l;
  H->res = (GEN *) gpmalloc(l * sizeof(GEN));
  memset(H->res,0, l * sizeof(GEN));
}

static void
init_path(gp_path *path)
{
  char *p;
#if PATH_SEPARATOR == ';'
  p = ".;C:;C:/gp";
#elif defined(UNIX)
  p = ".:~:~/gp";
#else
  p = ".";
#endif
  path->PATH = pari_strdup(p);
  path->dirs = NULL;
}

static char *
init_help()
{
  char *h = os_getenv("GPHELP");
# ifdef GPHELP
  if (!h) h = GPHELP;
# endif
  if (h) h = pari_strdup(h);
  return h;
}

static pariout_t *
init_fmt()
{
  pariout_t *f = &DFLT_OUTPUT;
  f->prettyp= f_PRETTYMAT;
#ifdef LONG_IS_64BIT
  f->sigd     = 38;
#else
  f->sigd     = 28;
#endif
  return f;
}

static void
init_pp(gp_pp *p)
{
  p->cmd = pari_strdup(DFT_PRETTYPRINTER);
  p->file = NULL;
}

/* must be called BEFORE pari_init() */
static void
gp_preinit(void)
{
  static gp_data __GP_DATA;
  static gp_hist __HIST;
  static gp_pp   __PP;
  static gp_path __PATH;
  static pari_timer __T;
  long i;

  bufstack = NULL;

  primelimit = 500000;
  bot = (pari_sp)0;
  top = (pari_sp)(1000000*sizeof(long));
  strcpy(prompt,      DFT_PROMPT);
  strcpy(prompt_cont, CONTPROMPT);

  for (i=0; i<c_LAST; i++) gp_colors[i] = c_NONE;

  GP_DATA = &__GP_DATA;
#ifdef READLINE
  GP_DATA->flags = (STRICTMATCH | SIMPLIFY | USE_READLINE);
#else
  GP_DATA->flags = (STRICTMATCH | SIMPLIFY);
#endif
  GP_DATA->lim_lines = 0;
  GP_DATA->T    = &__T;
  GP_DATA->hist = &__HIST;
  GP_DATA->pp   = &__PP;
  GP_DATA->path = &__PATH;
  GP_DATA->help = init_help();
  GP_DATA->fmt  = init_fmt();
  init_hist(GP_DATA->hist, 5000, 0);
  init_path(GP_DATA->path);
  init_pp(GP_DATA->pp);
}

#ifdef MAXPATHLEN
#  define GET_SEP_SIZE MAXPATHLEN
#else
#  define GET_SEP_SIZE 128
#endif

/* Return all chars, up to next separator
 * [as strtok but must handle verbatim character string] */
static char*
get_sep(const char *t)
{
  static char buf[GET_SEP_SIZE], *lim = buf + GET_SEP_SIZE;
  char *s = buf;
  int outer = 1;

  for(;;)
  {
    switch(*s++ = *t++)
    {
      case '"':
        if (outer || (s >= buf+2 && s[-2] != '\\')) outer = !outer;
        break;
      case '\0':
        return buf;
      case ';':
	if (outer) { s[-1] = 0; return buf; } break;
    }
    if (s == lim)
      err(talker,"get_sep: argument too long (< %ld chars)", GET_SEP_SIZE);
  }
}

static ulong
safe_mul(ulong x, ulong y)
{
  ulong z;
  LOCAL_HIREMAINDER;
  z = mulll(x, y);
  return hiremainder? 0: z;
}

/* "atoul" + optional [kmg] suffix */
static ulong
my_int(char *s)
{
  ulong n = 0;
  char *p = s;

  while (isdigit((int)*p)) { 
    ulong m;
    if (n > (~0UL / 10)) err(talker2,"integer too large",s,s);
    n *= 10; m = n;
    n += *p++ - '0';
    if (n < m) err(talker2,"integer too large",s,s);
  }
  if (n)
  {
    switch(*p)
    {
      case 'k': case 'K': n = safe_mul(n,1000UL);       p++; break;
      case 'm': case 'M': n = safe_mul(n,1000000UL);    p++; break;
      case 'g': case 'G': n = safe_mul(n,1000000000UL); p++; break;
    }
    if (!n) err(talker2,"integer too large",s,s);
  }
  if (*p) err(talker2,"I was expecting an integer here", s, s);
  return n;
}

static long
get_int(const char *s, long dflt)
{
  char *p = get_sep(s);
  long n;
  int minus = 0;
  
  if (*p == '-') { minus = 1; p++; }
  if (!isdigit((int)*p)) return dflt;

  n = (long)my_int(p);
  if (n < 0) err(talker2,"integer too large",s,s);
  return minus? -n: n;
}

static ulong
get_uint(const char *s)
{
  char *p = get_sep(s);
  if (*p == '-') err(talker2,"arguments must be positive integers",s,s);
  return my_int(p);
}

/* tell TeXmacs GP will start outputing data */
static void
tm_start_output(void)
{
  if (!tm_is_waiting) { printf("%cverbatim:",DATA_BEGIN); fflush(stdout); }
  tm_is_waiting = 1;
}

/* tell TeXmacs GP is done and is waiting for new data */
static void
tm_end_output(void)
{
  if (tm_is_waiting) { printf("%c", DATA_END); fflush(stdout); }
  tm_is_waiting = 0;
}

Buffer *
new_buffer(void)
{
  Buffer *b = (Buffer*) gpmalloc(sizeof(Buffer));
  b->len = 1024;
  b->buf = gpmalloc(b->len);
  return b;
}

void
del_buffer(Buffer *b)
{
  if (!b) return;
  free(b->buf); free((void*)b);
}

static void
pop_buffer(void)
{
  Buffer *b = (Buffer*) pop_stack(&bufstack);
  del_buffer(b);
}

/* kill all buffers until B is met or nothing is left */
static void
kill_all_buffers(Buffer *B)
{
  for(;;) {
    Buffer *b = current_buffer;
    if (b == B || !b) break;
    pop_buffer();
  }
}

static void
jump_to_given_buffer(Buffer *buf)
{
  Buffer *b;
  while ( (b = current_buffer) )
  {
    if (b == buf) break;
    pop_buffer();
  }
  if (!b || !b->env) longjmp(GP_DATA->env, 0);
  longjmp(b->env, 0);
}

/********************************************************************/
/*                                                                  */
/*                            DEFAULTS                              */
/*                                                                  */
/********************************************************************/
static void
do_strftime(const char *s, char *buf, long max)
{
#ifdef HAS_STRFTIME
  time_t t = time(NULL);
  (void)strftime(buf,max,s,localtime(&t));
#else
  strcpy(buf,s);
#endif
}

static GEN
sd_toggle(const char *v, int flag, char *s, int *ptn)
{
  int state = *ptn;
  if (*v)
  {
    int n = (int)get_int(v,0);
    if (n == state) return gnil;
    if (n != !state)
    {
      char *t = stackmalloc(64 + strlen(s));
      (void)sprintf(t, "default: incorrect value for %s [0:off / 1:on]", s);
      err(talker2, t, v,v);
    }
    state = *ptn = n;
  }
  switch(flag)
  {
    case d_RETURN: return utoi(state);
    case d_ACKNOWLEDGE:
      if (state) pariputsf("   %s = 1 (on)\n", s);
      else       pariputsf("   %s = 0 (off)\n", s);
      break;
  }
  return gnil;
}

static GEN
sd_gptoggle(const char *v, int flag, char *s, ulong FLAG)
{
  int n = (GP_DATA->flags & FLAG)? 1: 0, old = n;
  GEN z = sd_toggle(v, flag, s, &n);
  if (n != old)
  {
    if (n) GP_DATA->flags |=  FLAG;
    else   GP_DATA->flags &= ~FLAG;
  }
  return z;
}

static GEN
sd_ulong(const char *v, int flag, char *s, ulong *ptn, ulong Min, ulong Max,
           char **msg)
{
  ulong n;
  if (*v == 0) n = *ptn;
  else
  {
    n = get_uint(v);
    if (*ptn == n) return gnil;
    if (n > Max || n < Min)
    {
      char *buf = stackmalloc(strlen(s) + 2 * 20 + 40);
      (void)sprintf(buf, "default: incorrect value for %s [%lu-%lu]",
                    s, Min, Max);
      err(talker2, buf, v,v);
    }
    *ptn = n;
  }
  switch(flag)
  {
    case d_RETURN: return utoi(n);
    case d_ACKNOWLEDGE:
      if (msg)
      {
	if (!*msg) msg++; /* single msg, always printed */
	else       msg += n; /* one per possible value */
	pariputsf("   %s = %lu %s\n", s, n, *msg);
      }
      else
	pariputsf("   %s = %lu\n", s, n);
    default: return gnil;
  }
}

static GEN
sd_realprecision(const char *v, int flag)
{
  pariout_t *fmt = GP_DATA->fmt;
  if (*v)
  {
    long newnb = get_int(v, fmt->sigd);
    ulong newprec = (ulong)ndec2prec(newnb);

    if (fmt->sigd == newnb && prec == newprec) return gnil;
    if (newnb <= 0) err(talker,"default: realprecision must be positive");
    fmt->sigd = newnb; prec = newprec;
  }
  if (flag == d_RETURN) return stoi(fmt->sigd);
  if (flag == d_ACKNOWLEDGE)
  {
    long n = (long)bit_accuracy_mul(prec, L2SL10);
    pariputsf("   realprecision = %ld significant digits", n);
    if (n != fmt->sigd)
      pariputsf(" (%ld digits displayed)", fmt->sigd);
    pariputc('\n');
  }
  return gnil;
}

static GEN
sd_seriesprecision(const char *v, int flag)
{
  char *msg[] = {NULL, "significant terms"};
  return sd_ulong(v,flag,"seriesprecision",&precdl, 0,LGBITS,msg);
}

static GEN
sd_format(const char *v, int flag)
{
  pariout_t *fmt = GP_DATA->fmt;
  if (*v)
  {
    char c = *v;
    if (c!='e' && c!='f' && c!='g')
      err(talker2,"default: inexistent format",v,v);
    fmt->format = c; v++;

    if (isdigit((int)*v))
      { fmt->fieldw=atol(v); while (isdigit((int)*v)) v++; }
    if (*v++ == '.')
    {
      if (*v == '-') fmt->sigd = -1;
      else
	if (isdigit((int)*v)) fmt->sigd=atol(v);
    }
  }
  if (flag == d_RETURN)
  {
    char *s = stackmalloc(64);
    (void)sprintf(s, "%c%ld.%ld", fmt->format, fmt->fieldw, fmt->sigd);
    return strtoGENstr(s);
  }
  if (flag == d_ACKNOWLEDGE)
    pariputsf("   format = %c%ld.%ld\n", fmt->format, fmt->fieldw, fmt->sigd);
  return gnil;
}

static long
gp_get_color(char **st)
{
  char *s, *v = *st;
  int c, trans;
  if (isdigit((int)*v))
    { c = atol(v); trans = 1; } /* color on transparent background */
  else
  {
    if (*v == '[')
    {
      char *a[3];
      int i = 0;
      for (a[0] = s = ++v; *s && *s != ']'; s++)
        if (*s == ',') { *s = 0; a[++i] = s+1; }
      if (*s != ']') err(talker2,"expected character: ']'",s, *st);
      *s = 0; for (i++; i<3; i++) a[i] = "";
      /*    properties    |   color    | background */
      c = (atoi(a[2])<<8) | atoi(a[0]) | (atoi(a[1])<<4);
      trans = (*(a[1]) == 0);
      v = s + 1;
    }
    else { c = c_NONE; trans = 0; }
  }
  if (trans) c = c | (1<<12);
  while (*v && *v++ != ',') /* empty */;
  if (c != c_NONE) disable_color = 0;
  *st = v; return c;
}

/* 1: error, 2: history, 3: prompt, 4: input, 5: output, 6: help, 7: timer */
static GEN
sd_colors(char *v, int flag)
{
  long c,l;
  if (*v && !(GP_DATA->flags & (EMACS|TEXMACS)))
  {
    char *v0;
    disable_color=1;
    l = strlen(v);
    if (l <= 2 && strncmp(v, "no", l) == 0)
      v = "";
    if (l <= 6 && strncmp(v, "darkbg", l) == 0)
      v = "1, 5, 3, 7, 6, 2, 3"; /* Assume recent ReadLine. */
    if (l <= 7 && strncmp(v, "lightbg", l) == 0)
      v = "1, 6, 3, 4, 5, 2, 3"; /* Assume recent ReadLine. */
    if (l <= 6 && strncmp(v, "boldfg", l) == 0)	/* Good for darkbg consoles */
      v = "[1,,1], [5,,1], [3,,1], [7,,1], [6,,1], , [2,,1]";
    v0 = v = filtre(v, 0);
    for (c=c_ERR; c < c_LAST; c++)
      gp_colors[c] = gp_get_color(&v);
    free(v0);
  }
  if (flag == d_ACKNOWLEDGE || flag == d_RETURN)
  {
    char s[128], *t = s;
    int col[3], n;
    for (*t=0,c=c_ERR; c < c_LAST; c++)
    {
      n = gp_colors[c];
      if (n == c_NONE)
        sprintf(t,"no");
      else
      {
        decode_color(n,col);
        if (n & (1<<12))
        {
          if (col[0])
            sprintf(t,"[%d,,%d]",col[1],col[0]);
          else
            sprintf(t,"%d",col[1]);
        }
        else
          sprintf(t,"[%d,%d,%d]",col[1],col[2],col[0]);
      }
      t += strlen(t);
      if (c < c_LAST - 1) { *t++=','; *t++=' '; }
    }
    if (flag==d_RETURN) return strtoGENstr(s);
    pariputsf("   colors = \"%s\"\n",s);
  }
  return gnil;
}

static GEN
sd_compatible(const char *v, int flag)
{
  char *msg[] = {
    "(no backward compatibility)",
    "(warn when using obsolete functions)",
    "(use old functions, don't ignore case)",
    "(use old functions, ignore case)", NULL
  };
  ulong old = compatible;
  GEN r = sd_ulong(v,flag,"compatible",&compatible, 0,3,msg);

  if (old != compatible && flag != d_INITRC && gp_init_functions(0))
    err(warner,"user functions re-initialized");
  return r;
}

static GEN
sd_secure(const char *v, int flag)
{
  if (*v && (GP_DATA->flags & SECURE))
  {
    fprintferr("[secure mode]: Do you want to modify the 'secure' flag? (^C if not)\n");
    hit_return();
  }
  return sd_gptoggle(v,flag,"secure", SECURE);
}

static GEN
sd_debug(const char *v, int flag)
{ return sd_ulong(v,flag,"debug",&DEBUGLEVEL, 0,20,NULL); }

ulong readline_state = DO_ARGS_COMPLETE;

static GEN
sd_rl(const char *v, int flag)
{
  static const char * const msg[] = {NULL,
	"(bits 0x2/0x4 control matched-insert/arg-complete)"};
#ifdef READLINE
  ulong o_readline_state = readline_state;
#endif
  GEN res;

  res = sd_ulong(v,flag,"readline", &readline_state, 0, 7, (char**)msg);
#ifdef READLINE
  if (!readline_init && *v && *v != '0') {
    init_readline();
    readline_init = 1;
  }
  if (o_readline_state != readline_state)
    (void)sd_gptoggle(readline_state? "1": "0", d_SILENT, "readline", USE_READLINE);
#endif
  return res;
}

static GEN
sd_debugfiles(const char *v, int flag)
{ return sd_ulong(v,flag,"debugfiles",&DEBUGFILES, 0,20,NULL); }

static GEN
sd_debugmem(const char *v, int flag)
{ return sd_ulong(v,flag,"debugmem",&DEBUGMEM, 0,20,NULL); }

static GEN
sd_echo(const char *v, int flag)
{ return sd_gptoggle(v,flag,"echo", ECHO); }

static GEN
sd_lines(const char *v, int flag)
{ return sd_ulong(v,flag,"lines",&(GP_DATA->lim_lines), 0,VERYBIGINT,NULL); }

static GEN
sd_histsize(const char *v, int flag)
{
  gp_hist *H = GP_DATA->hist;
  ulong n = H->size;
  GEN r = sd_ulong(v,flag,"histsize",&n, 1,
                     (VERYBIGINT / sizeof(long)) - 1,NULL);
  if (n != H->size)
  {
    const ulong total = H->total;
    long g, h, k, kmin;
    GEN *resG = H->res, *resH; /* G = old data, H = new one */
    size_t sG = H->size, sH;

    init_hist(H, n, total);
    if (!total) return r;

    resH = H->res;
    sH   = H->size;
    /* copy relevant history entries */
    g     = (total-1) % sG;
    h = k = (total-1) % sH;
    kmin = k - min(sH, sG);
    for ( ; k > kmin; k--, g--, h--)
    {
      resH[h] = resG[g];
      resG[g] = NULL;
      if (!g) g = sG;
      if (!h) h = sH;
    }
    /* clean up */
    for ( ; resG[g]; g--)
    {
      gunclone(resG[g]);
      if (!g) g = sG;
    }
    free((void*)resG);
  }
  return r;
}

static GEN
sd_log(const char *v, int flag)
{
  static const char * const msg[] = {
      "(off)",
      "(on)",
      "(on with colors)",
      "(TeX output)", NULL
  };
  ulong oldstyle = logstyle;
  GEN res = sd_ulong(v,flag,"log", &logstyle, 0, 3, (char**)msg);

  if (!oldstyle != !logstyle)		/* Compare converts to boolean */
  { /* toggled LOG */
    if (oldstyle)
    { /* close log */
      if (flag == d_ACKNOWLEDGE)
        pariputsf("   [logfile was \"%s\"]\n", current_logfile);
      fclose(logfile); logfile = NULL;
    }
    else
    { /* open log */
      logfile = fopen(current_logfile, "a");
      if (!logfile) err(openfiler,"logfile",current_logfile);
#ifndef WINCE
      setbuf(logfile,(char *)NULL);
#endif
    }
  }
  if (logfile && oldstyle != logstyle && logstyle == logstyle_TeX)
  {
    fprintf(logfile, "%s%s%s%s%s%s%s%s%s%s\n",
      "\\ifx \\PARIbreak\\undefined\n",
      "  \\def\\PARIbreak{\\hskip 0pt plus \\hsize\\relax\\discretionary{}{}{}}\\fi\n",
      "\\ifx \\PARIpromptSTART\\undefined\n",
      "  \\def\\PARIpromptSTART|{\\vskip\\medskipamount\\bgroup\\bf}\\fi\n",
      "\\ifx \\PARIpromptEND\\undefined\n",
      "  \\def\\PARIpromptEND|{\\egroup\\bgroup\\tt}\\fi\n",
      "\\ifx \\PARIinputEND\\undefined\n",
      "  \\def\\PARIinputEND|{\\egroup}\\fi\n",
      "\\ifx \\PARIout\\undefined\n",
      "  \\def\\PARIout#1#2{\\vskip\\smallskipamount$\\displaystyle{\\tt\\%#1} = #2$}\\fi\n");
}
  return res;
}

static GEN
sd_TeXstyle(const char *v, int flag)
{
  static const char * const msg[] = { NULL,
	"(bits 0x1/0x2/0x4 control output of \\frac/\\left/\\PARIbreak)"};
  ulong n = GP_DATA->fmt->TeXstyle;
  GEN z = sd_ulong(v,flag,"TeXstyle", &n, 0, 7, (char**)msg);
  GP_DATA->fmt->TeXstyle = n; return z;
}

static GEN
sd_output(const char *v, int flag)
{
  char *msg[] = {"(raw)", "(prettymatrix)", "(prettyprint)",
                 "(external prettyprint)", NULL};
  ulong n = GP_DATA->fmt->prettyp;
  GEN z = sd_ulong(v,flag,"output", &n, 0,3,msg);
  GP_DATA->fmt->prettyp = n;
  GP_DATA->fmt->sp = (n != f_RAW);
  return z;
}

static GEN
sd_parisize(const char *v, int flag)
{
  ulong oldn = top-bot, n = oldn;
  GEN r = sd_ulong(v,flag,"parisize",&n, 10000,VERYBIGINT,NULL);
  if (n != oldn)
  {
    if (!bot) top = (pari_sp)n; /* no stack allocated yet */
    if (flag != d_INITRC) allocatemoremem(n);
  }
  return r;
}

static GEN
sd_primelimit(const char *v, int flag)
{
  ulong n = primelimit;
  GEN r = sd_ulong(v,flag,"primelimit",&n, 0,2*(ulong)(VERYBIGINT-1024) + 1,NULL);
  if (n != primelimit)
  {
    if (flag != d_INITRC)
    {
      byteptr ptr = initprimes(n);
      free(diffptr); diffptr = ptr;
    }
    primelimit = n;
  }
  return r;
}

static GEN
sd_simplify(const char *v, int flag)
{ return sd_gptoggle(v,flag,"simplify", SIMPLIFY); }

static GEN
sd_strictmatch(const char *v, int flag)
{ return sd_gptoggle(v,flag,"strictmatch", STRICTMATCH); }

static GEN
sd_timer(const char *v, int flag)
{ return sd_gptoggle(v,flag,"timer", CHRONO); }

static GEN
sd_filename(const char *v, int flag, char *s, char **f)
{
  if (*v)
  {
    char *s, *old = *f;
    long l;
    char *ev = expand_tilde(v);
    l = strlen(ev) + 256;
    s = (char *) malloc(l);
    do_strftime(ev,s, l-1); free(ev);
    *f = pari_strdup(s); free(s); free(old);
  }
  if (flag == d_RETURN) return strtoGENstr(*f);
  if (flag == d_ACKNOWLEDGE) pariputsf("   %s = \"%s\"\n",s,*f);
  return gnil;
}

static GEN
sd_logfile(const char *v, int flag)
{
  GEN r = sd_filename(v, flag, "logfile", &current_logfile);
  if (*v && logfile)
  {
    fclose(logfile);
    logfile = fopen(current_logfile, "a");
    if (!logfile) err(openfiler,"logfile",current_logfile);
#ifndef WINCE
    setbuf(logfile,(char *)NULL);
#endif
  }
  return r;
}

static GEN
sd_factor_add_primes(char *v, int flag)
{ return sd_toggle(v,flag,"factor_add_primes", &factor_add_primes); }

static GEN
sd_new_galois_format(char *v, int flag)
{ return sd_toggle(v,flag,"new_galois_format", &new_galois_format); }

static GEN
sd_psfile(char *v, int flag)
{ return sd_filename(v, flag, "psfile", &current_psfile); }

static void
err_secure(char *d, char *v)
{ err(talker,"[secure mode]: can't modify '%s' default (to %s)",d,v); }

static GEN
sd_help(char *v, int flag)
{
  const char *str;
  if (*v)
  {
    if (GP_DATA->flags & SECURE) err_secure("help",v);
    if (GP_DATA->help) free(GP_DATA->help);
    GP_DATA->help = expand_tilde(v);
  }
  str = GP_DATA->help? GP_DATA->help: "none";
  if (flag == d_RETURN) return strtoGENstr(str);
  if (flag == d_ACKNOWLEDGE)
    pariputsf("   help = \"%s\"\n", str);
  return gnil;
}

static GEN
sd_datadir(char *v, int flag)
{
  const char *str;
  if (*v)
  {
    if (pari_datadir) free(pari_datadir);
    pari_datadir = expand_tilde(v);
  }
  str = pari_datadir? pari_datadir: "none";
  if (flag == d_RETURN) return strtoGENstr(str);
  if (flag == d_ACKNOWLEDGE)
    pariputsf("   datadir = \"%s\"\n", str);
  return gnil;
}

static GEN
sd_path(char *v, int flag)
{
  gp_path *p = GP_DATA->path;
  if (*v)
  {
    free((void*)p->PATH);
    p->PATH = pari_strdup(v);
    if (flag == d_INITRC) return gnil;
    gp_expand_path(p);
  }
  if (flag == d_RETURN) return strtoGENstr(p->PATH);
  if (flag == d_ACKNOWLEDGE)
    pariputsf("   path = \"%s\"\n",p->PATH);
  return gnil;
}

static GEN
sd_prettyprinter(char *v, int flag)
{
  gp_pp *pp = GP_DATA->pp;
  if (*v && !(GP_DATA->flags & TEXMACS))
  {
    char *old = pp->cmd;
    int cancel = (!strcmp(v,"no"));

    if (GP_DATA->flags & SECURE) err_secure("prettyprinter",v);
    if (!strcmp(v,"yes")) v = DFT_PRETTYPRINTER;
    if (old && strcmp(old,v) && pp->file)
    {
      pariFILE *f;
      if (cancel) f = NULL;
      else
      {
        f = try_pipe(v, mf_OUT | mf_TEST);
        if (!f)
        {
          err(warner,"broken prettyprinter: '%s'",v);
          return gnil;
        }
      }
      pari_fclose(pp->file);
      pp->file = f;
    }
    pp->cmd = cancel? NULL: pari_strdup(v);
    if (old) free(old);
    if (flag == d_INITRC) return gnil;
  }
  if (flag == d_RETURN)
	  /*FIXME: The cast is needed by g++ but is a kludge but changing
	   * freadexpr to accept a const char * is non-trivial.*/
    return freadexpr(pp->cmd? pp->cmd:(char *) "");
  if (flag == d_ACKNOWLEDGE)
    pariputsf("   prettyprinter = \"%s\"\n",pp->cmd? pp->cmd: "");
  return gnil;
}

static GEN
sd_prompt_set(const char *v, int flag, char *how, char *p)
{
  if (*v)
  {
    strncpy(p,v,MAX_PROMPT_LEN);
#ifdef macintosh
    strcat(p,"\n");
#endif
  }
  if (flag == d_RETURN) return strtoGENstr(p);
  if (flag == d_ACKNOWLEDGE)
    pariputsf("   prompt%s = \"%s\"\n", how, p);
  return gnil;
}

static GEN
sd_prompt(const char *v, int flag)
{
  return sd_prompt_set(v, flag, "", prompt);
}

static GEN
sd_prompt_cont(char *v, int flag)
{
  return sd_prompt_set(v, flag, "_cont", prompt_cont);
}

default_type gp_default_list[] =
{
  {"colors",(void*)sd_colors},
  {"compatible",(void*)sd_compatible},
  {"datadir",(void*)sd_datadir},
  {"debug",(void*)sd_debug},
  {"debugfiles",(void*)sd_debugfiles},
  {"debugmem",(void*)sd_debugmem},
  {"echo",(void*)sd_echo},
  {"factor_add_primes",(void*)sd_factor_add_primes},
  {"format",(void*)sd_format},
  {"help",(void*)sd_help},
  {"histsize",(void*)sd_histsize},
  {"lines",(void*)sd_lines},
  {"log",(void*)sd_log},
  {"logfile",(void*)sd_logfile},
  {"new_galois_format",(void*)sd_new_galois_format},
  {"output",(void*)sd_output},
  {"parisize",(void*)sd_parisize},
  {"path",(void*)sd_path},
  {"primelimit",(void*)sd_primelimit},
  {"prettyprinter",(void*)sd_prettyprinter},
  {"prompt",(void*)sd_prompt},
  {"prompt_cont",(void*)sd_prompt_cont},
  {"psfile",(void*)sd_psfile},
  {"realprecision",(void*)sd_realprecision},
  {"readline",(void*)sd_rl},
  {"secure",(void*)sd_secure},
  {"seriesprecision",(void*)sd_seriesprecision},
  {"simplify",(void*)sd_simplify},
  {"strictmatch",(void*)sd_strictmatch},
  {"TeXstyle",(void *)sd_TeXstyle},
  {"timer",(void *)sd_timer},
  {NULL,NULL} /* sentinel */
};

static void
help_default(void)
{
  default_type *dft;

  for (dft=gp_default_list; dft->fun; dft++)
    ((void (*)(char*,int)) dft->fun)("", d_ACKNOWLEDGE);
}

static GEN
setdefault(char *s,char *v, int flag)
{
  default_type *dft;

  if (!*s) { help_default(); return gnil; }
  for (dft=gp_default_list; dft->fun; dft++)
    if (!strcmp(s,dft->name))
    {
      if (flag == d_EXISTS) return gen_1;
      return ((GEN (*)(char*,int)) dft->fun)(v,flag);
    }
  if (flag == d_EXISTS) return gen_0;
  err(talker,"unknown default: %s",s);
  return NULL; /* not reached */
}

/********************************************************************/
/**                                                                **/
/**                             HELP                               **/
/**                                                                **/
/********************************************************************/
static int
has_ext_help(void)
{
  if (GP_DATA->help)
  {
    char *buf = pari_strdup(GP_DATA->help), *s, *t;
    FILE *file;

    for (t = s = buf; *s; *t++ = *s++)
    {
      if (*s == '\\') s++; else if (*s == ' ') break;
    }
    *t = 0; file = fopen(buf,"r");
    free(buf);
    if (file) { fclose(file); return 1; }
  }
  return 0;
}

static int
compare_str(char **s1, char **s2) { return strcmp(*s1, *s2); }

/* Print all elements of list in columns, pausing every nbli lines
 * if nbli is non-zero.
 * list is a NULL terminated list of function names
 */
void
print_fun_list(char **list, int nbli)
{
  long i=0, j=0, maxlen=0, nbcol,len, w = term_width();
  char **l;

  while (list[i]) i++;
  qsort (list, i, sizeof(char *), (QSCOMP)compare_str);

  for (l=list; *l; l++)
  {
    len = strlen(*l);
    if (len > maxlen) maxlen=len;
  }
  maxlen++; nbcol= w / maxlen;
  if (nbcol * maxlen == w) nbcol--;
  if (!nbcol) nbcol = 1;

  pariputc('\n'); i=0;
  for (l=list; *l; l++)
  {
    pariputs(*l); i++;
    if (i >= nbcol)
    {
      i=0; pariputc('\n');
      if (nbli && j++ > nbli) { j = 0; hit_return(); }
      continue;
    }
    len = maxlen - strlen(*l);
    while (len--) pariputc(' ');
  }
  if (i) pariputc('\n');
}

static void
commands(int n)
{
  const size_t LIST_LEN = 1023UL;
  size_t size = LIST_LEN, s = 0;
  int i;
  entree *ep;
  char **list = (char **) gpmalloc((size+1)*sizeof(char *));

  for (i = 0; i < functions_tblsz; i++)
    for (ep = functions_hash[i]; ep; ep = ep->next)
      if ((n < 0 && ep->menu) || ep->menu == n)
      {
        list[s] = ep->name;
        if (++s >= size)
        {
	  size += (LIST_LEN+1);
          list = (char**) gprealloc(list, size*sizeof(char *));
        }
      }
  list[s] = NULL;
  print_fun_list(list, term_height()-4);
  free(list);
}

static void
print_def_arg(GEN x)
{
  if (x == gen_0) return;
  pariputc('=');
  if (typ(x)==t_STR)
    pariputs(GSTR(x)); /* otherwise it's surrounded by "" */
  else
    brute(x,'g',-1);
}

static void
print_user_fun(entree *ep)
{
  gp_args *f= (gp_args*)ep->args;
  GEN q = (GEN)ep->value, *arg = f->arg;
  int i, narg;

  q++; /* skip initial NULL */
  pariputs(ep->name); pariputc('(');
  narg = f->narg;
  for (i=1; i<=narg; i++, arg++)
  {
    entree *ep = varentries[*q++];
    pariputs(ep? ep->name:"#");
    print_def_arg(*arg);
    if (i == narg) { arg++; break; }
    pariputs(", ");
  }
  pariputs(") = ");
  narg = f->nloc;
  if (narg)
  {
    pariputs("local(");
    for (i=1; i<=narg; i++, arg++)
    {
      entree *ep = varentries[*q++];
      pariputs(ep? ep->name:"#");
      print_def_arg(*arg);
      if (i == narg) break;
      pariputs(", ");
    }
    pariputs("); ");
  }
  pariputs((char*)q);
}

static void
print_user_member(entree *ep)
{
  GEN q = (GEN)ep->value;
  entree *ep2;

  q++; /* skip initial NULL */
  ep2 = varentries[*q++];
  pariputs(ep2? ep2->name:"#");
  pariputsf(".%s = ", ep->name);
  pariputs((char*)q);
}

static void
brace_print(entree *ep, void print(entree *))
{
  pariputc('{'); print(ep);
  pariputc('}'); pariputs("\n\n");
}

static void
user_fun(void)
{
  entree *ep;
  int i;
  for (i = 0; i < functions_tblsz; i++)
    for (ep = functions_hash[i]; ep; ep = ep->next)
      if (EpVALENCE(ep) == EpUSER) brace_print(ep, &print_user_fun);
}

static void
user_member(void)
{
  entree *ep;
  int i;
  for (i = 0; i < functions_tblsz; i++)
    for (ep = members_hash[i]; ep; ep = ep->next)
      if (EpVALENCE(ep) == EpMEMBER) brace_print(ep, &print_user_member);
}

static void
center(char *s)
{
  long i, l = strlen(s), pad = term_width() - l;
  char *buf, *u;

  if (pad<0) pad=0; else pad >>= 1;
  u = buf = (char*)gpmalloc(l + pad + 2);
  for (i=0; i<pad; i++) *u++ = ' ';
  while (*s) *u++ = *s++;
  *u++ = '\n'; *u = 0;
  pariputs(buf); free(buf);
}

static void
community(void)
{
  pari_sp av = avma;
  char *s = stackmalloc(strlen(GPDATADIR) + 1024);

  (void)sprintf(s, "The standard distribution of GP/PARI includes a \
reference manual, a tutorial, a reference card and quite a few examples. They \
should have been installed in the directory '%s'. If not, ask the person \
who installed PARI on your system where they can be found. You can also \
download them from the PARI WWW site 'http://pari.math.u-bordeaux.fr/'",
GPDATADIR);
  print_text(s); avma = av;

  pariputs("\nThree mailing lists are devoted to PARI:\n\
  - pari-announce (moderated) to announce major version changes.\n\
  - pari-dev for everything related to the development of PARI, including\n\
    suggestions, technical questions, bug reports and patch submissions.\n\
  - pari-users for everything else!\n");
  print_text("\
To subscribe, send an empty message to <listname>-subscribe@list.cr.yp.to. \
An archive is kept at the WWW site mentioned above. You can also reach the \
authors directly by email: pari@math.u-bordeaux.fr (answer not guaranteed)."); }

static void
gentypes(void)
{
  pariputs("List of the PARI types:\n\
  t_INT    : long integers     [ cod1 ] [ cod2 ] [ man_1 ] ... [ man_k ]\n\
  t_REAL   : long real numbers [ cod1 ] [ cod2 ] [ man_1 ] ... [ man_k ]\n\
  t_INTMOD : integermods       [ code ] [ mod  ] [ integer ]\n\
  t_FRAC   : irred. rationals  [ code ] [ num. ] [ den. ]\n\
  t_COMPLEX: complex numbers   [ code ] [ real ] [ imag ]\n\
  t_PADIC  : p-adic numbers    [ cod1 ] [ cod2 ] [ p ] [ p^r ] [ int ]\n\
  t_QUAD   : quadratic numbers [ cod1 ] [ mod  ] [ real ] [ imag ]\n\
  t_POLMOD : poly mod          [ code ] [ mod  ] [ polynomial ]\n\
  -------------------------------------------------------------\n\
  t_POL    : polynomials       [ cod1 ] [ cod2 ] [ man_1 ] ... [ man_k ]\n\
  t_SER    : power series      [ cod1 ] [ cod2 ] [ man_1 ] ... [ man_k ]\n\
  t_RFRAC  : irred. rat. func. [ code ] [ num. ] [ den. ]\n\
  t_QFR    : real qfb          [ code ] [ a ] [ b ] [ c ] [ del ]\n\
  t_QFI    : imaginary qfb     [ code ] [ a ] [ b ] [ c ]\n\
  t_VEC    : row vector        [ code ] [  x_1  ] ... [  x_k  ]\n\
  t_COL    : column vector     [ code ] [  x_1  ] ... [  x_k  ]\n\
  t_MAT    : matrix            [ code ] [ col_1 ] ... [ col_k ]\n\
  t_LIST   : list              [ code ] [ cod2 ] [ x_1 ] ... [ x_k ]\n\
  t_STR    : string            [ code ] [ man_1 ] ... [ man_k ]\n\
  t_VECSMALL: vec. small ints  [ code ] [ x_1 ] ... [ x_k ]\n\
\n");
}

static void
menu_commands(void)
{
  pariputs("Help topics: for a list of relevant subtopics, type ?n for n in\n\
  0: user-defined identifiers (variable, alias, function)\n\
  1: Standard monadic or dyadic OPERATORS\n\
  2: CONVERSIONS and similar elementary functions\n\
  3: TRANSCENDENTAL functions\n\
  4: NUMBER THEORETICAL functions\n\
  5: Functions related to ELLIPTIC CURVES\n\
  6: Functions related to general NUMBER FIELDS\n\
  7: POLYNOMIALS and power series\n\
  8: Vectors, matrices, LINEAR ALGEBRA and sets\n\
  9: SUMS, products, integrals and similar functions\n\
 10: GRAPHIC functions\n\
 11: PROGRAMMING under GP\n\
 12: The PARI community\n\
\n\
Also:\n\
  ? functionname (short on-line help)\n\
  ?\\             (keyboard shortcuts)\n\
  ?.             (member functions)\n");
  if (has_ext_help()) pariputs("\
Extended help looks available:\n\
  ??             (opens the full user's manual in a dvi previewer)\n\
  ??  tutorial / refcard / libpari (tutorial/reference card/libpari manual)\n\
  ??  keyword    (long help text about \"keyword\" from the user's manual)\n\
  ??? keyword    (a propos: list of related functions).");
}

static void
slash_commands(void)
{
  pariputs("#       : enable/disable timer\n\
##      : print time for last result\n\
\\\\      : comment up to end of line\n\
\\a {n}  : print result in raw format (readable by PARI)\n\
\\b {n}  : print result in beautified format\n\
\\c      : list all commands (same effect as ?*)\n\
\\d      : print all defaults\n\
\\e {n}  : enable/disable echo (set echo=n)\n\
\\g {n}  : set debugging level\n\
\\gf{n}  : set file debugging level\n\
\\gm{n}  : set memory debugging level\n\
\\h {m-n}: hashtable information\n\
\\l {f}  : enable/disable logfile (set logfile=f)\n\
\\m {n}  : print result in prettymatrix format\n\
\\o {n}  : change output method (0=raw, 1=prettymatrix, 2=prettyprint, 3=2-dim)\n\
\\p {n}  : change real precision\n\
\\ps{n}  : change series precision\n\
\\q      : quit completely this GP session\n\
\\r {f}  : read in a file\n\
\\s {n}  : print stack information\n\
\\t      : print the list of PARI types\n\
\\u      : print the list of user-defined functions\n\
\\um     : print the list of user-defined member functions\n\
\\v      : print current version of GP\n\
\\w {nf} : write to a file\n\
\\x {n}  : print complete inner structure of result\n\
\\y {n}  : disable/enable automatic simplification (set simplify=n)\n\
\n\
{f}=optional filename. {n}=optional integer\n");
}

static void
member_commands(void)
{
  pariputs("\
Member functions, followed by relevant objects\n\n\
a1-a6, b2-b8, c4-c6 : coeff. of the curve.            ell\n\
area : area                                           ell\n\
bid  : big ideal                                                    bnr\n\
bnf  : big number field                                        bnf, bnr\n\
clgp : class group                   bid,                      bnf, bnr\n\
cyc  : cyclic decomposition (SNF)    bid,       clgp,          bnf, bnr\n\
diff, codiff: different and codifferent                    nf, bnf, bnr\n\
disc : discriminant                                   ell, nf, bnf, bnr\n\
e, f : inertia/residue  degree            prid\n\
fu   : fundamental units                                       bnf, bnr\n\
gen  : generators                    bid, prid, clgp,          bnf, bnr\n\
index: index                                               nf, bnf, bnr\n\
j    : j-invariant                                    ell\n");
/* split: some compilers can't handle long constant strings */
  pariputs("\
mod  : modulus                       bid,                           bnr\n\
nf   : number field                                            bnf, bnr\n\
no   : number of elements            bid,       clgp,          bnf, bnr\n\
omega, eta: [omega1,omega2] and [eta1, eta2]          ell\n\
p    : rational prime below prid          prid\n\
pol  : defining polynomial                                 nf, bnf, bnr\n\
reg  : regulator                                               bnf, bnr\n\
roots: roots                                          ell  nf, bnf, bnr\n\
sign,r1,r2 : signature                                     nf, bnf, bnr\n\
t2   : t2 matrix                                           nf, bnf, bnr\n\
tate : Tate's [u^2, u, q]                             ell\n\
tu   : torsion unit and its order                              bnf, bnr\n\
w    : Mestre's w                                     ell\n\
zk   : integral basis                                      nf, bnf, bnr\n");
}

#define QUOTE "_QUOTE"
#define DOUBQUOTE "_DOUBQUOTE"
#define BACKQUOTE "_BACKQUOTE"

static char *
_cat(char *s, char *t)
{
  *s = 0; strcat(s,t); return s + strlen(t);
}

static char *
filter_quotes(char *s)
{
  int i, l = strlen(s);
  int quote = 0;
  int backquote = 0;
  int doubquote = 0;
  char *str, *t;

  for (i=0; i < l; i++)
    switch(s[i])
    {
      case '\'': quote++; break;
      case '`' : backquote++; break;
      case '"' : doubquote++;
    }
  str = (char*)gpmalloc(l + quote * (strlen(QUOTE)-1)
                          + doubquote * (strlen(DOUBQUOTE)-1)
                          + backquote * (strlen(BACKQUOTE)-1) + 1);
  t = str;
  for (i=0; i < l; i++)
    switch(s[i])
    {
      case '\'': t = _cat(t, QUOTE); break;
      case '`' : t = _cat(t, BACKQUOTE); break;
      case '"' : t = _cat(t, DOUBQUOTE); break;
      default: *t++ = s[i];
    }
  *t = 0; return str;
}

static int
nl_read(char *s) { size_t l = strlen(s); return s[l-1] == '\n'; }

#define nbof(a) sizeof(a) / sizeof(a[0])
/* query external help program for s. num < 0 [keyword] or chapter number */
static void
external_help(char *s, int num)
{
  long nbli = term_height()-3, li = 0;
  char buf[256], ar[32], *str, *opt = "";
  pariFILE *z;
  FILE *f;

  if (!GP_DATA->help) err(talker,"no external help program");
  s = filter_quotes(s);
  str = gpmalloc(strlen(GP_DATA->help) + strlen(s) + 64);
  *ar = 0;
  if (num < 0)
    opt = "-k";
  else if (s[strlen(s)-1] != '@')
    sprintf(ar,"@%d",num);
  sprintf(str,"%s -fromgp %s %c%s%s%c",GP_DATA->help,opt, SHELL_Q,s,ar,SHELL_Q);
  z = try_pipe(str,0); f = z->file;
  free(str);
  free(s);
  while (fgets(buf, nbof(buf), f))
  {
    if (!strncmp("ugly_kludge_done",buf,16)) break;
    pariputs(buf);
    if (nl_read(buf) && ++li > nbli) { hit_return(); li = 0; }
  }
  pari_fclose(z);
}

char *keyword_list[]={
  "operator",
  "libpari",
  "member",
  "integer",
  "real",
  "readline",
  "refcard",
  "tutorial",
  "nf",
  "bnf",
  "bnr",
  "ell",
  "rnf",
  "bid",
  "modulus",
  NULL
};

static int
ok_external_help(char *s)
{
  long n;
  if (!*s) return 1;
  if (!isalpha((int)*s)) return 3; /* operator or section number */
  if (!strncmp(s,"t_",2)) return 2; /* type name */

  for (n=0; keyword_list[n]; n++)
    if (!strcmp(s,keyword_list[n])) return 3;
  return 0;
}

/* don't mess readline display */
static void
aide_print(char *s1, char *s2) { pariputsf("%s: %s\n", s1, s2); }

static void
aide0(char *s, int flag)
{
  long n, long_help = flag & h_LONG;
  entree *ep,*ep1;
  char *s1;

  s = get_sep(s);
  if (isdigit((int)*s))
  {
    n = atoi(s);
    if (n == 12) { community(); return; }
    if (n < 0 || n > 12)
      err(talker2,"no such section in help: ?",s,s);
    if (long_help) external_help(s,3); else commands(n);
    return;
  }
  /* Get meaningful entry on \ps 5 */
  if (*s == '\\') { s1 = s+1; skip_alpha(s1); *s1 = '\0';}

  if (flag & h_APROPOS) { external_help(s,-1); return; }
  if (long_help && (n = ok_external_help(s))) { external_help(s,n); return; }
  switch (*s)
  {
    case '*' : commands(-1); return;
    case '\0': menu_commands(); return;
    case '\\': slash_commands(); return;
    case '.' : member_commands(); return;
  }
  ep = is_entry(s);
  if (ep && long_help)
  {
    if (!strcmp(ep->name, "default"))
    {
      char *t = s+7, *e;
      skip_space(t);
      if (*t == '(') {
	t++; skip_space(t);
        e = t; skip_alpha(e); *e = '\0'; /* safe: get_sep() made it a copy */
	if (is_default(t)) { external_help(t, 2); return; }
      }
    }
  }
  if (!ep)
  {
    n = is_default(s)? 2: 3;
    if (long_help)
      external_help(s,n);
    else
    {
      if (n == 2) { aide_print(s,"default"); return; }
      n = whatnow(s,1);
      if (!n) { aide_print(s,"unknown identifier"); return; }
      aide_print(s, "obsolete function");
      whatnow_new_syntax(s, n);
    }
    return;
  }

  ep1 = ep;  ep = do_alias(ep);
  if (ep1 != ep) pariputsf("%s is aliased to:\n\n",s);

  switch(EpVALENCE(ep))
  {
    case EpUSER:
      if (!ep->help || long_help) print_user_fun(ep);
      if (!ep->help) return;
      if (long_help) { pariputs("\n\n"); long_help=0; }
      break;

    case EpGVAR:
    case EpVAR:
      if (!ep->help) { aide_print(s, "user defined variable"); return; }
      long_help=0; break;

    case EpINSTALL:
      if (!ep->help) { aide_print(s, "installed function"); return; }
      long_help=0; break;
  }
  if (long_help) { external_help(ep->name,3); return; }
  if (ep->help) { print_text(ep->help); return; }

  err(bugparier,"aide (no help found)");
}

void
aide(char *s, int flag)
{
  if ((flag & h_RL) == 0)
  {
    if (*s == '?') { flag |= h_LONG; s++; }
    if (*s == '?') { flag |= h_APROPOS; s++; }
  }
  term_color(c_HELP); aide0(s,flag); term_color(c_NONE);
  if ((flag & h_RL) == 0) pariputc('\n');
}

/********************************************************************/
/**                                                                **/
/**                         METACOMMANDS                           **/
/**                                                                **/
/********************************************************************/

static char *
what_readline()
{
  char *s;
#ifdef READLINE
  char *ver, *extra = stackmalloc(strlen(READLINE) + 32);
#  if defined(HAS_RL_LIBRARY_VERSION) || defined(FAKE_RL_LIBRARY_VERSION)
#    ifdef FAKE_RL_LIBRARY_VERSION
  extern char *rl_library_version;
#    endif

  if (strcmp(READLINE, rl_library_version))
  {
    ver = (char*)rl_library_version;
    (void)sprintf(extra, " [was v%s in Configure]", READLINE);
  }
  else
#  endif
  {
    ver = READLINE;
    extra[0] = 0;
  }
  s = stackmalloc(3 + strlen(ver) + 8 + strlen(extra));
  (void)sprintf(s, "v%s %s%s", ver,
            (GP_DATA->flags & USE_READLINE)? "enabled": "disabled",
            extra);
#else
  s = "not compiled in";
#endif
  return s;
}

static void
print_shortversion(void)
{
  const ulong mask = (1<<PARI_VERSION_SHIFT) - 1;
  ulong n = PARI_VERSION_CODE, major, minor, patch;

  patch = n & mask; n >>= PARI_VERSION_SHIFT;
  minor = n & mask; n >>= PARI_VERSION_SHIFT;
  major = n;
  pariputsf("%lu.%lu.%lu\n", major,minor,patch); exit(0);
}

static char *
what_cc()
{
  char *s;
#ifdef GCC_VERSION
#  ifdef __cplusplus
#    define Format "g++-%s"
#  else
#    define Format "gcc-%s"
#  endif
  s = stackmalloc(4 + strlen(GCC_VERSION) + 1);
  (void)sprintf(s, Format, GCC_VERSION);
#else
#  ifdef _MSC_VER
  s = stackmalloc(32);
  (void)sprintf(s, "MSVC-%i", _MSC_VER);
#  else
  s = NULL;
#  endif
#endif
  return s;
}

static void
print_version(void)
{
  pari_sp av = avma;
  char *buf, *ver;

  center(PARIVERSION);
  center(PARIINFO);
  ver = what_cc();
  buf = stackmalloc(strlen(__DATE__) +  32 + (ver? strlen(ver): 0));
  if (ver) (void)sprintf(buf, "compiled: %s, %s", __DATE__, ver);
  else     (void)sprintf(buf, "compiled: %s", __DATE__);
  center(buf);
  ver = what_readline();
  buf = stackmalloc(strlen(ver) + 64);
  (void)sprintf(buf, "(readline %s, extended help%s available)", ver,
                has_ext_help()? "": " not");
  center(buf); avma = av;
}

static void
gp_head(void)
{
#ifdef READLINE
  if (GP_DATA->flags & TEXMACS)
    printf("%ccommand:(cas-supports-completions-set! \"pari\")%c\n",
           DATA_BEGIN, DATA_END);
#endif
  print_version(); pariputs("\n");
  center("Copyright (C) 2000-2005 The PARI Group");
  print_text("\nPARI/GP is free software, covered by the GNU General Public \
License, and comes WITHOUT ANY WARRANTY WHATSOEVER");
  pariputs("\n\
Type ? for help, \\q to quit.\n\
Type ?12 for how to get moral (and possibly technical) support.\n\n");
  pariputsf("parisize = %lu, primelimit = %lu\n", top-bot, primelimit);
}

void
fix_buffer(Buffer *b, long newlbuf)
{
  b->len = newlbuf;
  b->buf = gprealloc(b->buf, b->len);
}

void
gp_quit(void)
{
  free_graph(); freeall();
  kill_all_buffers(NULL);
  if (INIT_SIG) pari_sig_init(SIG_DFL);
  term_color(c_NONE);
  if (!(GP_DATA->flags & QUIET)) pariputs("Goodbye!\n");
  if (GP_DATA->flags & TEXMACS) tm_end_output();
  exit(0);
}

static GEN
gpreadbin(const char *s)
{
  GEN x = readbin(s,infile);
  popinfile(); return x;
}

static void
escape0(char *tch)
{
  const char *s;
  char c;

  if (compatible != NONE)
  {
    s = tch;
    while (*s)
      if (*s++ == '=')
      {
	GEN (*f)(const char*, int) = NULL;
	int len = (s-tch) - 1;
	if      (!strncmp(tch,"precision",len))    f = sd_realprecision;
	else if (!strncmp(tch,"serieslength",len)) f = sd_seriesprecision;
	else if (!strncmp(tch,"format",len))       f = sd_format;
	else if (!strncmp(tch,"prompt",len))       f = sd_prompt;
	if (f) { (void)f(get_sep(s), d_ACKNOWLEDGE); return; }
	break;
      }
  }
  s = tch;
  switch ((c = *s++))
  {
    case 'w': case 'x': case 'a': case 'b': case 'B': case 'm':
    { /* history things */
      long d;
      GEN x;
      if (c != 'w' && c != 'x') d = get_int(s,0);
      else
      {
	d = atol(s); if (*s == '-') s++;
	while (isdigit((int)*s)) s++;
      }
      x = gp_history(GP_DATA->hist, d, tch+1,tch-1);
      switch (c)
      {
	case 'B':
        { /* prettyprinter */
          gp_data G = *GP_DATA; /* copy */
          gp_hist   h = *(G.hist); /* copy */
          pariout_t f = *(G.fmt);  /* copy */

          G.hist = &h; h.total = 0; /* no hist number */
          G.fmt  = &f; f.prettyp = f_PRETTY;
          G.flags &= ~(TEST|TEXMACS);
          G.lim_lines = 0;
          gp_output(x, &G); break;
        }
	case 'a': brute   (x, GP_DATA->fmt->format, -1); break;
	case 'm': matbrute(x, GP_DATA->fmt->format, -1); break;
	case 'b': sor(x, GP_DATA->fmt->format, -1, GP_DATA->fmt->fieldw); break;
	case 'x': voir(x, get_int(s, -1)); break;
        case 'w':
	  s = get_sep(s); if (!*s) s = current_logfile;
	  write0(s, mkvec(x)); return;
      }
      pariputc('\n'); return;
    }

    case 'c': commands(-1); break;
    case 'd': help_default(); break;
    case 'e':
      s = get_sep(s);
      if (!*s) s = (GP_DATA->flags & ECHO)? "0": "1";
      (void)sd_echo(s,d_ACKNOWLEDGE); break;
    case 'g':
      switch (*s)
      {
        case 'm': (void)sd_debugmem(++s,d_ACKNOWLEDGE); break;
        case 'f': (void)sd_debugfiles(++s,d_ACKNOWLEDGE); break;
        default : (void)sd_debug(s,d_ACKNOWLEDGE); break;
      }
      break;
    case 'h': print_functions_hash(s); break;
    case 'l':
      s = get_sep(s);
      if (*s)
      {
        (void)sd_logfile(s,d_ACKNOWLEDGE);
        if (logfile) break;
      }
      (void)sd_log(logfile?"0":"1",d_ACKNOWLEDGE);
      break;
    case 'o': (void)sd_output(s,d_ACKNOWLEDGE); break;
    case 'p':
      switch (*s)
      {
        case 's': (void)sd_seriesprecision(++s,d_ACKNOWLEDGE); break;
        default : (void)sd_realprecision(s,d_ACKNOWLEDGE); break;
      }
      break;
    case 'q': gp_quit(); break;
    case 'r':
      s = get_sep(s);
      switchin(s);
      if (file_is_binary(infile))
      {
        GEN x = gpreadbin(s);
        if (isclone(x)) /* many BIN_GEN */
        {
          long i, l = lg(x);
          err(warner,"setting %ld history entries", l-1);
          for (i=1; i<l; i++) (void)set_hist_entry(GP_DATA->hist, (GEN)x[i]);
        }
      }
      break;
    case 's': etatpile(); break;
    case 't': gentypes(); break;
    case 'u':
      switch (*s)
      {
        case 'm': user_member(); break;
        default: user_fun();
      }
      break;
    case 'v': print_version(); break;
    case 'y':
      s = get_sep(s);
      if (!*s) s = (GP_DATA->flags & SIMPLIFY)? "0": "1";
      (void)sd_simplify(s,d_ACKNOWLEDGE); break;
    default: err(caracer1,tch,tch-1);
  }
}

static void
escape(char *tch)
{
  char *old = get_analyseur();
  set_analyseur(tch); /* for error messages */
  escape0(tch);
  set_analyseur(old);
}
/********************************************************************/
/*                                                                  */
/*                              GPRC                                */
/*                                                                  */
/********************************************************************/
#if defined(UNIX) || defined(__EMX__)
#  include <pwd.h>
#endif

static int get_line_from_file(char *prompt, filtre_t *F, FILE *file);
#define err_gprc(s,t,u) { fprintferr("\n"); err(talker2,s,t,u); }

static void
init_filtre(filtre_t *F, void *data)
{
  F->data = data;
  F->in_string  = 0;
  F->in_comment = 0;
  F->downcase = 0;
}

/* LOCATE GPRC */

/* return $HOME or the closest we can find */
static char *
get_home(int *free_it)
{
  char *drv, *pth = os_getenv("HOME");
  if (pth) return pth;
  if ((drv = os_getenv("HOMEDRIVE"))
   && (pth = os_getenv("HOMEPATH")))
  { /* looks like WinNT */
    char *buf = gpmalloc(strlen(pth) + strlen(drv) + 1);
    sprintf(buf, "%s%s",drv,pth);
    *free_it = 1; return buf;
  }
#if defined(__EMX__) || defined(UNIX)
  {
    struct passwd *p = getpwuid(geteuid());
    if (p) return p->pw_dir;
  }
#endif
  return ".";
}

static FILE *
gprc_chk(char *s)
{
  FILE *f = fopen(s, "r");
  if (f && !(GP_DATA->flags & QUIET)) fprintferr("Reading GPRC: %s ...", s);
  return f;
}

/* Look for [._]gprc: $GPRC, then in $HOME, ., /etc, path [ to gp binary ] */
static FILE *
gprc_get(char *path)
{
  FILE *f = NULL;
  char *str, *s, c;
  long l;
  s = os_getenv("GPRC");
  if (s) f = gprc_chk(s);
  if (!f)
  {
    int free_it = 0;
    s = get_home(&free_it); l = strlen(s); c = s[l-1];
    str = strcpy(gpmalloc(l+7), s);
    if (free_it) free(s);
    s = str + l;
    if (c != '/' && c != '\\') *s++ = '/';
#ifdef UNIX
    *s = '.'; /* .gprc */
#else
    *s = '_'; /* _gprc */
#endif
    strcpy(s+1, "gprc");
    f = gprc_chk(str); /* in $HOME */
    if (!f) f = gprc_chk(s); /* in . */
    if (!f) f = gprc_chk("/etc/gprc");
    if (!f) f = gprc_chk("C:/_gprc");
    if (!f)
    { /* in 'gp' directory */
      char *t = path + strlen(path);
      while (t > path && *t != '/') t--;
      if (*t == '/')
      {
        long l = t - path + 1;
        t = gpmalloc(l + 6);
        strncpy(t, path, l);
        strcpy(t+l, s); f = gprc_chk(t);
        free(t);
      }
    }
    free(str);
  }
  return f;
}

/* PREPROCESSOR */

static ulong
read_uint(char **s)
{
  long v = atol(*s);
  if (!isdigit((int)**s)) err_gprc("not an integer", *s, *s);
  while (isdigit((int)**s)) (*s)++;
  return v;
}
static ulong
read_dot_uint(char **s)
{
  if (**s != '.') return 0;
  (*s)++; return read_uint(s);
}
/* read a.b.c */
static long
read_version(char **s)
{
  long a, b, c;
  a = read_uint(s);
  b = read_dot_uint(s);
  c = read_dot_uint(s);
  return PARI_VERSION(a,b,c);
}

static int
get_preproc_value(char **s)
{
  if (!strncmp(*s,"EMACS",5))
  {
    *s += 5;
    return GP_DATA->flags & (EMACS|TEXMACS);
  }
  if (!strncmp(*s,"READL",5))
  {
    *s += 5;
    return GP_DATA->flags & USE_READLINE;
  }
  if (!strncmp(*s,"VERSION",7))
  {
    int less = 0, orequal = 0;
    long d;
    *s += 7;
    switch(**s)
    {
      case '<': (*s)++; less = 1; break;
      case '>': (*s)++; less = 0; break;
      default: return -1;
    }
    if (**s == '=') { (*s)++; orequal = 1; }
    d = PARI_VERSION_CODE - read_version(s);
    if (!d) return orequal;
    return less? (d < 0): (d > 0);
  }
  return -1;
}

/* PARSE GPRC */

/* 1) replace next separator by '\0' (t must be writeable)
 * 2) return the next expression ("" if none)
 * see get_sep() */
static char *
next_expr(char *t)
{
  int outer = 1;
  char *s = t;

  for(;;)
  {
    char c;
    switch ((c = *s++))
    {
      case '"':
        if (outer || (s >= t+2 && s[-2] != '\\')) outer = !outer;
        break;
      case '\0':
        return "";
      default:
        if (outer && c == ';') { s[-1] = 0; return s; }
    }
  }
}

typedef struct {
  void **v;
  long len; /* len cells allocated */
  long n; /* first n cells occupied */
} growarray;

void
grow_append(growarray *A, void *e)
{
  if (A->n == A->len-1)
  {
    A->len <<= 1;
    A->v = (void**)gprealloc(A->v, A->len * sizeof(void*));
  }
  A->v[A->n++] = e;
}

void
grow_init(growarray *A)
{
  A->len = 4;
  A->n   = 0;
  A->v   = (void**)gpmalloc(A->len * sizeof(void*));
}

static void
gp_initrc(growarray *A, char *path)
{
  char *nexts,*s,*t;
  FILE *file = gprc_get(path);
  Buffer *b;
  filtre_t F;
  VOLATILE int c = 0;

  if (!file) return;
  b = new_buffer();
  init_filtre(&F, (void*)b);
  for(;;)
  {
    if (setjmp(GP_DATA->env)) fprintferr("...skipping line %ld.\n", c);
    c++;
    if (!get_line_from_file(NULL,&F,file)) break;
    s = b->buf;
    if (*s == '#')
    { /* preprocessor directive */
      int z, NOT = 0;
      s++;
      if (strncmp(s,"if",2)) err_gprc("unknown directive",s,b->buf);
      s += 2;
      if (!strncmp(s,"not",3)) { NOT = !NOT; s += 3; }
      if (*s == '!')           { NOT = !NOT; s++; }
      t = s;
      z = get_preproc_value(&s);
      if (z < 0) err_gprc("unknown preprocessor variable",t,b->buf);
      if (NOT) z = !z;
      if (!*s)
      { /* make sure at least an expr follows the directive */
        if (!get_line_from_file(NULL,&F,file)) break;
        s = b->buf;
      }
      if (!z) continue; /* dump current line */
    }
    /* parse line */
    for ( ; *s; s = nexts)
    {
      nexts = next_expr(s);
      if (!strncmp(s,"read",4) && (s[4] == ' ' || s[4] == '\t' || s[4] == '"'))
      { /* read file */
	s += 4;
	t = gpmalloc(strlen(s) + 1);
	if (*s == '"') (void)readstring(s, t); else strcpy(t,s);
        grow_append(A, t);
      }
      else
      { /* set default */
	t = s; while (*t && *t != '=') t++;
	if (*t != '=') err_gprc("missing '='",t,b->buf);
	*t++ = 0;
	if (*t == '"') (void)readstring(t, t);
	(void)setdefault(s,t,d_INITRC);
      }
    }
  }
  del_buffer(b);
  if (!(GP_DATA->flags & QUIET)) fprintferr("Done.\n\n");
  fclose(file);
}

/********************************************************************/
/*                                                                  */
/*                           GP MAIN LOOP                           */
/*                                                                  */
/********************************************************************/
static void
gp_handle_SIGINT(void)
{
#if defined(_WIN32) || defined(__CYGWIN32__)
  win32ctrlc++;
#else
  if (GP_DATA->flags & TEXMACS) tm_start_output();
  err(siginter, gp_format_time(ti_INTERRUPT));
#endif
}

static void
gp_sighandler(int sig)
{
  char *msg;
  (void)os_signal(sig,gp_sighandler);
  switch(sig)
  {
#ifdef SIGBREAK
    case SIGBREAK: gp_handle_SIGINT(); return;
#endif
#ifdef SIGINT
    case SIGINT:   gp_handle_SIGINT(); return;
#endif

#ifdef SIGSEGV
    case SIGSEGV: msg = "GP (Segmentation Fault)"; break;
#endif
#ifdef SIGBUS
    case SIGBUS:  msg = "GP (Bus Error)"; break;
#endif
#ifdef SIGFPE
    case SIGFPE:  msg = "GP (Floating Point Exception)"; break;
#endif

#ifdef SIGPIPE
    case SIGPIPE:
    {
      pariFILE *f = GP_DATA->pp->file;
      if (f && pari_outfile == f->file)
      {
        GP_DATA->pp->file = NULL; /* to avoid oo recursion on error */
        pari_outfile = stdout; pari_fclose(f);
      }
      err(talker, "Broken Pipe, resetting file stack...");
      return; /* not reached */
    }
#endif
    default: msg = "signal handling"; break;
  }
  err(bugparier, msg);
}

static void
brace_color(char *s, int c, int force)
{
  if (disable_color || (gp_colors[c] == c_NONE && !force)) return;
#ifdef RL_PROMPT_START_IGNORE
  if (GP_DATA->flags & USE_READLINE)
    *s++ = RL_PROMPT_START_IGNORE;
#endif
  strcpy(s, term_get_color(c));
#ifdef RL_PROMPT_START_IGNORE
  if (GP_DATA->flags & USE_READLINE)
  {
    s+=strlen(s);
    *s++ = RL_PROMPT_END_IGNORE;
    *s = 0;
  }
#endif
}

static char *
do_prompt(int in_comment, char *p, char **bare)
{
  static char buf[MAX_PROMPT_LEN + 24]; /* + room for color codes */
  static char buf1[MAX_PROMPT_LEN];
  char *s;

  if (GP_DATA->flags & TEST) return prompt;
  s = buf; *s = 0;
  /* escape sequences bug readline, so use special bracing (if available) */
  brace_color(s, c_PROMPT, 0);
  s += strlen(s);
  if (in_comment)
    strcpy(s, COMMENTPROMPT);
  else {
    do_strftime(p,buf1, MAX_PROMPT_LEN-1);
    strcpy(s, buf1);
  }
  if (bare)
    *bare = buf1;
  s += strlen(s);
  brace_color(s, c_INPUT, 1); return buf;
}

static char *
fgets_texmacs(char *s, int n, FILE *f)
{
  if (!tm_did_complete)
  {
    tm_start_output(); tm_end_output(); /* tell TeXmacs we need input */
  }
  return fgets(s,n,f);
}

/* Read from file (up to '\n' or EOF) and copy at s0 (points in b->buf) */
static char *
file_input(Buffer *b, char **s0, input_method *IM)
{
  int first = 1;
  char *s = *s0;
  ulong used0, used = s - b->buf;

  used0 = used;
  for(;;)
  {
    ulong left = b->len - used, l;

    if (left < 512)
    {
      fix_buffer(b, b->len << 1);
      left = b->len - used;
      *s0 = b->buf + used0;
    }
    s = b->buf + used;
    if (! IM->fgets(s, left, IM->file))
      return first? NULL: *s0; /* EOF */

    l = strlen(s); first = 0;
    if (l+1 < left || s[l-1] == '\n') return *s0; /* \n */
    used += l;
  }
}

/* Read a "complete line" and filter it. Return: 0 if EOF, 1 otherwise */
int
input_loop(filtre_t *F, input_method *IM)
{
  Buffer *b = (Buffer*)F->data;
  char *to_read, *s = b->buf;

  /* read first line */
  handle_C_C = 0;
  while (! (to_read = IM->getline(b,&s,IM)) )
  { /* EOF */
    if (!handle_C_C) { check_filtre(F); return 0; }
    /* received ^C in getline and coming back from break_loop();
     * retry (as if "\n" were input) */
    handle_C_C = 0;
  }

  /* buffer is not empty, init filter */
  F->in_string = 0;
  F->more_input= 0;
  F->wait_for_brace = 0;
  for(;;)
  {
    F->s = to_read;
    F->t = s;
    (void)filtre0(F);
    if (IM->free) free(to_read);

    if (! F->more_input) break;

    /* read continuation line */
    s = F->end;
    if (IM->prompt) IM->prompt = do_prompt(F->in_comment, prompt_cont, NULL);
    to_read = IM->getline(b,&s, IM);
    if (!to_read) break;
  }
  return 1;
}

#ifdef READLINE
typedef struct {
  char *cmd;
  long n; /* number of args */
  char **v; /* args */
} tm_cmd;

static void
parse_texmacs_command(tm_cmd *c, char *ch)
{
  long l = strlen(ch);
  char *t, *s = ch, *send = s+l-1;
  growarray A;

  if (*s != DATA_BEGIN || *send-- != DATA_END)
    err(talker, "missing DATA_[BEGIN | END] in TeXmacs command");
  s++;
  if (strncmp(s, "special:", 8)) err(talker, "unrecognized TeXmacs command");
  s += 8;
  if (*s != '(' || *send-- != ')')
    err(talker, "missing enclosing parentheses for TeXmacs command");
  s++; t = s;
  skip_alpha(s);
  c->cmd = pari_strndup(t, s - t);
  grow_init(&A);
  for (c->n = 0; s <= send; c->n++)
  {
    char *u = gpmalloc(strlen(s) + 1);
    skip_space(s);
    if (*s == '"') s = readstring(s, u);
    else
    { /* read integer */
      t = s;
      while (isdigit((int)*s)) s++;
      strncpy(u, t, s - t); u[s-t] = 0;
    }
    grow_append(&A, (void*)u);
  }
  c->v = (char**)A.v;
}

static void
free_cmd(tm_cmd *c)
{
  while (c->n--) free((void*)c->v[c->n]);
  free((void*)c->v);
}
#endif

/* prompt = NULL --> from gprc. Return 1 if new input, and 0 if EOF */
static int
get_line_from_file(char *prompt, filtre_t *F, FILE *file)
{
  const int Texmacs_stdin = ((GP_DATA->flags & TEXMACS) && file == stdin);
  char *s;
  input_method IM;

  IM.file = file;
  IM.fgets= Texmacs_stdin? &fgets_texmacs: &fgets;
  IM.prompt = NULL;
  IM.getline= &file_input;
  IM.free = 0;
  if (! input_loop(F,&IM))
  {
    if (Texmacs_stdin) tm_start_output();
    return 0;
  }

  s = ((Buffer*)F->data)->buf;
  if (*s && prompt) /* don't echo if from gprc */
  {
    if (GP_DATA->flags & ECHO)
      { pariputs(prompt); pariputs(s); pariputc('\n'); }
    else
      if (logfile) {
	if (logstyle == logstyle_TeX)
	  fprintf(logfile,
		  "\\PARIpromptSTART|%s\\PARIpromptEND|%s\\PARIinputEND|%%\n",
		  prompt,s);
	else
	  fprintf(logfile, "%s%s\n",prompt,s);
      }
    
    pariflush();
  }
  if (GP_DATA->flags & TEXMACS) 
  {
    tm_did_complete = 0;
    if (Texmacs_stdin && *s == DATA_BEGIN)
    {
#ifdef READLINE
      tm_cmd c;
      parse_texmacs_command(&c, s);
      if (strcmp(c.cmd, "complete"))
        err(talker,"Texmacs_stdin command %s not implemented", c.cmd);
      if (c.n != 2) 
        err(talker,"was expecting 2 arguments for Texmacs_stdin command");
      texmacs_completion(c.v[0], atol(c.v[1]));
      free_cmd(&c); *s = 0;
      tm_did_complete = 1;
#else
      err(talker, "readline not available");
#endif
      return 1;
    }
    tm_start_output();
  }
  return 1;
}

static int
is_interactive(void)
{
  ulong f = GP_DATA->flags;
#if defined(UNIX) || defined(__EMX__) || defined(_WIN32)
  return (infile == stdin && !(f & TEXMACS)
                          && (f & EMACS || isatty(fileno(stdin))));
#else
  return (infile == stdin && !(f & TEXMACS));
#endif
}

/* return 0 if no line could be read (EOF) */
int
pari_read_line(filtre_t *F, char *PROMPT)
{
  Buffer *b = (Buffer*)F->data;
  int res;
  if (compatible == OLDALL) F->downcase = 1;
  if (b->len > 100000) fix_buffer(b, 100000);
  if (is_interactive())
  {
    char *bare_prompt;

    if (PROMPT)
      bare_prompt = PROMPT;
    else
      PROMPT = do_prompt(F->in_comment, prompt, &bare_prompt);
#ifdef READLINE
    if (GP_DATA->flags & USE_READLINE)
      res = get_line_from_readline(PROMPT, bare_prompt, F);
    else
#endif
    {
      pariputs(PROMPT);
      res = get_line_from_file(PROMPT,F,infile);
    }
    if (!disable_color) { term_color(c_NONE); pariflush(); }
  }
  else
    res = get_line_from_file(DFT_PROMPT,F,infile);
  return res;
}

static int
chron(char *s)
{
  if (*s)
  { /* if "#" or "##" timer metacommand. Otherwise let the parser get it */
    if (*s == '#') s++;
    if (*s) return 0;
    pariputs(gp_format_time(ti_LAST));
  }
  else { GP_DATA->flags ^= CHRONO; (void)sd_timer("",d_ACKNOWLEDGE); }
  return 1;
}

/* return 0: can't interpret *buf as a metacommand
 *        1: did interpret *buf as a metacommand or empty command */
static int
check_meta(char *buf)
{
  switch(*buf++)
  {
    case '?': aide(buf, h_REGULAR); break;
    case '#': return chron(buf);
    case '\\': escape(buf); break;
    case '\0': break;
    default: return 0;
  }
  return 1;
}

/* kill all history entries since loc */
static void
prune_history(gp_hist *H, long loc)
{
  long i, j;
  i = (H->total-1) % H->size;
  j = H->total - loc;
  for ( ; j > 0; i--,j--)
  {
    if (H->res[i])
    {
      gunclone(H->res[i]);
      H->res[i] = NULL;
    }
    if (!i) i = H->size;
  }
  H->total = loc;
}

static int
silent(void)
{
  if (gpsilent) return 1;
  { char c = get_analyseur()[1]; return separator(c); }
}
static int
is_silent(char *s) { char c = s[strlen(s) - 1]; return separator(c); }

/* If there are other buffers open (bufstack != NULL), we are doing an
 * immediate read (with read, extern...) */
static GEN
gp_main_loop(int ismain)
{
  gp_hist *H  = GP_DATA->hist;
  VOLATILE GEN z = gnil;
  VOLATILE pari_sp av = top - avma;
  Buffer *b = new_buffer();
  filtre_t F;

  init_filtre(&F, (void*)b);
  push_stack(&bufstack, (void*)b);
  for(;;)
  {
    if (ismain)
    {
      static long tloc, outtyp;
      tloc = H->total;
      outtyp = GP_DATA->fmt->prettyp;
      recover(0);
      if (setjmp(GP_DATA->env))
      { /* recover from error */
        char *s = (char*)global_err_data;
        if (s && *s) outerr(readseq(s));
	avma = top; av = 0;
        prune_history(H, tloc);
        GP_DATA->fmt->prettyp = outtyp;
        kill_all_buffers(b);
      }
    }

    if (! pari_read_line(&F, NULL))
    {
      if (popinfile()) gp_quit();
      if (ismain) continue;
      pop_buffer(); return z;
    }
    if (check_meta(b->buf)) continue;

    if (ismain)
    {
#if defined(_WIN32) || defined(__CYGWIN32__)
      win32ctrlc = 0;
#endif
      gpsilent = is_silent(b->buf);
      TIMERstart(GP_DATA->T);
    }
    avma = top - av;
    z = gpreadseq(b->buf, GP_DATA->flags & STRICTMATCH);
    if (! ismain) continue;

    if (GP_DATA->flags & CHRONO)
      pariputs(gp_format_time(ti_REGULAR));
    else
      (void)gp_format_time(ti_NOPRINT);
    if (z == gnil) continue;

    if (GP_DATA->flags & SIMPLIFY) z = simplify_i(z);
    z = set_hist_entry(H, z);
    if (!gpsilent) gp_output(z, GP_DATA);
  }
}

GEN
read0(char *s)
{
  switchin(s);
  if (file_is_binary(infile)) return gpreadbin(s);
  return gp_main_loop(0);
}

static void
check_secure(char *s)
{
  if (GP_DATA->flags & SECURE)
    err(talker, "[secure mode]: system commands not allowed\nTried to run '%s'",s);
}

GEN
extern0(char *s)
{
  check_secure(s);
  infile = try_pipe(s, mf_IN)->file;
  return gp_main_loop(0);
}

GEN
default0(char *a, char *b, long flag)
{
  return setdefault(a,b, flag? d_RETURN
                             : silent()? d_SILENT: d_ACKNOWLEDGE);
}

GEN
input0(void)
{
  Buffer *b = new_buffer();
  filtre_t F;
  GEN x;

  init_filtre(&F, (void*)b);
  push_stack(&bufstack, (void*)b);
  while (! get_line_from_file(DFT_INPROMPT,&F,infile))
    if (popinfile()) { fprintferr("no input ???"); gp_quit(); }
  x = readseq(b->buf);
  pop_buffer(); return x;
}

void
system0(char *s)
{
#if defined(UNIX) || defined(__EMX__) || defined(_WIN32)
  check_secure(s);
  system(s);
#else
  err(archer);
#endif
}

int
break_loop(long numerr)
{
  static FILE *oldinfile = NULL;
  static char *old = NULL;
  static Buffer *b = NULL;
  VOLATILE int go_on = 0;
  char *s, *t;
  filtre_t F;

  if (b) jump_to_given_buffer(b);
  b = new_buffer();
  push_stack(&bufstack, (void*)b);

  (void)&s; /* emulate volatile */
  old = s = get_analyseur();
  t = NULL;
  if (bufstack->prev)
  {
    Buffer *oldb = (Buffer*)bufstack->prev->value;
    t = oldb->buf;
    /* something fishy, probably a ^C, or we overran analyseur */
    if (!s || !s[-1] || s < t || s >= t + oldb->len) s = NULL;
  }
  oldinfile = infile;
  init_filtre(&F, (void*)b);

  term_color(c_ERR); pariputc('\n');
  errcontext("Break loop (type 'break' or Control-d to go back to GP)", s, t);
  if (s) pariputc('\n');
  term_color(c_NONE);
  if (numerr == siginter)
    pariputs("[type <Return> in empty line to continue]\n");
  infile = stdin;
  for(;;)
  {
    GEN x;
    if (setjmp(b->env)) pariputc('\n');
    if (! pari_read_line(&F, BREAK_LOOP_PROMPT))
    {
      if (popinfile()) break;
      continue;
    }
#if defined(_WIN32) || defined(__CYGWIN32__)
    win32ctrlc = 0;
#endif
    if (check_meta(b->buf))
    { /* break loop initiated by ^C? Empty input --> continue computation */
      if (numerr == siginter && *(b->buf) == 0) { handle_C_C=go_on=1; break; }
      continue;
    }
    x = readseq(b->buf);
    if (did_break()) break;
    if (x == gnil || is_silent(b->buf)) continue;

    term_color(c_OUTPUT); gen_output(x, GP_DATA->fmt);
    term_color(c_NONE); pariputc('\n');
  }
  if (old && !s) set_analyseur(old);
  b = NULL; infile = oldinfile;
  pop_buffer(); return go_on;
}

int
gp_exception_handler(long numerr)
{
  char *s = (char*)global_err_data;
  if (!s) return 0;
  if (*s) {
    /* prevent infinite recursion in case s raises an exception */
    static int recovering = 0;
    if (recovering)
      recovering = 0; 
    else
    {
      recovering = 1;
      fprintferr("\n"); outerr(readseq(s));
      recovering = 0; return 0;
    }
  }
  if (numerr == errpile) { var_make_safe(); avma = top; }
  return break_loop(numerr);
}

static void
testuint(char *s, ulong *d) { if (s) *d = get_uint(s); }

static char *
read_arg(int *nread, char *t, long argc, char **argv)
{
  int i = *nread;
  if (isdigit((int)*t)) return t;
  if (*t || i==argc) usage(argv[0]);
  *nread = i+1; return argv[i];
}

static char *
read_arg_equal(int *nread, char *t, long argc, char **argv)
{
  int i = *nread;
  if (*t=='=' && isdigit((int)t[1])) return t+1;
  if (*t || i==argc) usage(argv[0]);
  *nread = i+1; return argv[i];
}


static void
init_trivial_stack()
{
  const size_t s = 2048;
  bot = (pari_sp)gpmalloc(s);
  avma = top = bot + s;
}

void
read_opt(growarray *A, long argc, char **argv)
{
  char *b = NULL, *p = NULL, *s = NULL;
  int i = 1, initrc = 1;

  (void)&p; (void)&b; (void)&s; /* -Wall gcc-2.95 */

  pari_outfile = stderr;
  while (i < argc)
  {
    char *t = argv[i];

    if (*t++ != '-') break;
    i++;
    switch(*t++)
    {
      case 'b': b = read_arg(&i,t,argc,argv);
        err(warner, "buffersize is no longer used. -b ignored");
        break;
      case 'p': p = read_arg(&i,t,argc,argv); break;
      case 's': s = read_arg(&i,t,argc,argv); break;

      case 'e':
	if (strncmp(t,"macs",4)) usage(argv[0]); /* obsolete */
        GP_DATA->flags |= EMACS; break;
      case 'q':
        GP_DATA->flags |= QUIET; break;
      case 't':
	if (strncmp(t,"est",3)) usage(argv[0]); /* obsolete */
        GP_DATA->flags |= TEST; /* fall through */
      case 'f':
	initrc = 0; break;
      case '-':
        if (strcmp(t, "version-short") == 0) { print_shortversion(); exit(0); }
        if (strcmp(t, "version") == 0) { 
          init_trivial_stack(); print_version();
          free((void*)bot); exit(0);
        }
        if (strcmp(t, "texmacs") == 0) { GP_DATA->flags |= TEXMACS; break; }
        if (strcmp(t, "emacs") == 0) { GP_DATA->flags |= EMACS; break; }
        if (strcmp(t, "test") == 0) { GP_DATA->flags |= TEST; initrc = 0; break; }
        if (strcmp(t, "quiet") == 0) { GP_DATA->flags |= QUIET; break; }
        if (strcmp(t, "fast") == 0) { initrc = 0; break; }
        if (strncmp(t, "primelimit",10) == 0) {p = read_arg_equal(&i,t+10,argc,argv); break; }
        if (strncmp(t, "stacksize",9) == 0) {s = read_arg_equal(&i,t+9,argc,argv); break; }
       /* fall through */
      default:
	usage(argv[0]);
    }
  }
  if (GP_DATA->flags & TEXMACS) tm_start_output();
  if (GP_DATA->flags & TEST) init80col(0);
  if (initrc)
  {
    gp_initrc(A, argv[0]);
    if (setjmp(GP_DATA->env))
    {
      pariputs("### Errors on startup, exiting...\n\n");
      exit(1);
    }
  }
  for ( ; i < argc; i++) grow_append(A, pari_strdup(argv[i]));

  /* override the values from gprc */
  testuint(p, &primelimit);
  testuint(s, (ulong*)&top);
  if (GP_DATA->flags & (EMACS|TEXMACS|TEST)) disable_color = 1;
  pari_outfile = stdout;
}

#ifdef WINCE
int
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
        LPWSTR lpCmdLine, int nShowCmd)
{
  char **argv = NULL;
  int argc = 1;
#else
int
main(int argc, char **argv)
{
#endif
  growarray A;

  init_defaults(1); gp_preinit();
  if (setjmp(GP_DATA->env))
  {
    pariputs("### Errors on startup, exiting...\n\n");
    exit(1);
  }
#ifdef __MWERKS__
  argc = ccommand(&argv);
#endif
  grow_init(&A);
  read_opt(&A, argc,argv);
  pari_addfunctions(&pari_modules, functions_gp,helpmessages_gp);
  pari_addfunctions(&pari_modules, functions_highlevel,helpmessages_highlevel);
  pari_addfunctions(&pari_oldmodules, functions_oldgp,helpmessages_oldgp);

  init_graph();
  INIT_SIG_off;
  pari_init(top-bot, primelimit);
  INIT_SIG_on;
  pari_sig_init(gp_sighandler);
#ifdef READLINE
  if (GP_DATA->flags & USE_READLINE) {
    init_readline();
    readline_init = 1;
  }
#endif
  whatnow_fun = whatnow;
  default_exception_handler = gp_exception_handler;
  gp_expand_path(GP_DATA->path);

  if (!(GP_DATA->flags & QUIET)) gp_head();
  if (A.n)
  {
    VOLATILE ulong f = GP_DATA->flags;
    FILE *l = logfile;
    VOLATILE long i;
    GP_DATA->flags &= ~(CHRONO|ECHO); logfile = NULL;
    for (i = 0; i < A.n; i++) {
      if (setjmp(GP_DATA->env))
      {
        fprintferr("... skipping file '%s'\n", A.v[i]);
        i++; if (i == A.n) break;
      }
      (void)read0((char*)A.v[i]); free(A.v[i]);
    }
    GP_DATA->flags = f; logfile = l;
  }
  free(A.v);
  (void)gp_main_loop(1);
  gp_quit(); return 0; /* not reached */
}
