#include <stdio.h>
#include <pari/pari.h>
#include <readline/readline.h>
#include <setjmp.h>

jmp_buf env;
void gp_err_recover(long numerr) { longjmp(env, numerr); }

/* History handling (%1, %2, etc.)*/
pari_stack s_history;
GEN *history;
GEN parihist(long p) 
{
  if (p > 0 && p<=s_history.n)
    return history[p-1];
  else if (p<=0 && s_history.n+p-1>=0)
    return history[s_history.n+p-1];
  pari_err(talker,"History result %ld not available [%%1-%%%ld]",p,s_history.n);
  return NULL; /* not reached */
}

int main(int argc, char **argv)
{
  char *in, *out;
  GEN z;
  entree hist={"%",0,(void*)parihist,13,"D0,L,","last history item."};
  printf("Welcome to minigp!\n");
  pari_init(8000000,500000);
  cb_pari_err_recover = gp_err_recover;
  pari_add_function(&hist);
  stack_init(&s_history,sizeof(*history),(void**)&history);
  (void)setjmp(env);
  while(1)
  {
    in = readline("? ");
    if (!in) break;
    if (!*in) continue;
    z = gp_read_str(in);
    stack_pushp(&s_history,(void*)gclone(z)); /*Add to history */
    out = GENtostr(z);
    printf("%%%ld = %s\n",s_history.n,out);
    free(in); free(out); avma=top;
  }
  return 0;
}
