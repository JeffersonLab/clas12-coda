
/* roc_network.h */

/* functions */

int LINK_close(int socket);
int LINK_establish(char *host, int port);
int LINK_sized_write(int fd, unsigned int *buffer, unsigned int nbytes);
int rocOpenLink(char *fromname, char *toname, char host_return[128], int *port_return, int *socketnum_return);
int rocCloseLinkxxx();
