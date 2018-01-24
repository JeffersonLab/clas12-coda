/* codautil.h */

#ifdef  __cplusplus
extern "C" {
#endif

  int   get_run_number(char *mysql_database, char *session);
  char *get_run_operators(char *mysql_database, char *session);
  char *get_run_status(char *mysql_database, char *session);
  char *get_daq_config(char *mysql_database, char *configname);
  int   get_run_nevents(char *mysql_database, char *configname);
  int   get_run_nfiles(char *mysql_database, char *configname);
  int   get_run_ndata(char *mysql_database, char *configname);
  char *get_run_datafile(char *mysql_database, char *configname);
  int   get_run_time(char *mysql_database, char *configname);

#ifdef  __cplusplus
}
#endif
