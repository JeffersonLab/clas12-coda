
/* libdb.h - header file */

#ifdef  __cplusplus
extern "C" {
#endif

#if defined(VXWORKS) || defined(ARM)
#include "mysql/mysql.h" /* use our mysql header(s) */
#else
#include <mysql/mysql.h> /* use standard mysql header(s) */
#endif

MYSQL *dbConnectFull(const char *host, const char *database, const char *user, const char *passwd);
MYSQL *dbConnect(const char *host, const char *database);
void dbDisconnect(MYSQL *mysql);
int  dbGetInt(MYSQL *mysql, char *str, int *value);
int  dbGetStr(MYSQL *mysql, char *str, char *strout);

#ifdef  __cplusplus
}
#endif

