
/* get_hostname.c - calls system 'gethostname' and always returns short hostname */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "codautil.h"

#define LEN 256

int
get_hostname(char *name, size_t len)
{
  int ret;
  char host[LEN];

  ret = gethostname(host,LEN);  /* obtain our hostname */
  if(ret!=0) return(ret);

  char *dot_ptr = strchr(host, '.'); // Find the first occurrence of '.'
  if (dot_ptr != NULL) *dot_ptr = '\0'; // Terminate the string at the dot's position

  strncpy(name,host,len);

  return(0);  
}
