
/* roc_process.h */

/* functions */

int proc_copy(int *bufin, int *bufout, int pid);
int proc_tt(int *bufin, int *bufout, int pid, int *nev);
int proc_download(char *rolname, char *rolparams, int pid);
int proc_prestart(int pid);
int proc_go(int pid);
int proc_end();
int proc_poll(int *bufin, int *bufout, int pid, int *nev);
