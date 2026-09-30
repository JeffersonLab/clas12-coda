
/* dacgethostbyname.c */



#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h> // For close()

int dacgethostbyname(char *hostname, char *ip_address_str)
{
  struct addrinfo hints, *server_info, *p;
  int status;
  char ip_string_buffer[INET6_ADDRSTRLEN];
  void *addr;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;      // AF_UNSPEC allows IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM;  // Use TCP stream sockets

  if ((status = getaddrinfo(hostname, NULL, &hints, &server_info)) != 0)
  {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    return 1;
  }

  printf("IP addresses for %s:\n", hostname);

  for (p = server_info; p != NULL; p = p->ai_next)
  {
    // Get the pointer to the address itself, different fields for IPv4 and IPv6
    if (p->ai_family == AF_INET) // IPv4
    {
      struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
      addr = &(ipv4->sin_addr);
    }
    else // IPv6
    {
      struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
      addr = &(ipv6->sin6_addr);
    }

    // Convert the IP to a string and print it
    inet_ntop(p->ai_family, addr, ip_string_buffer, sizeof ip_string_buffer);
    printf("  %s\n", ip_string_buffer);
        
    // Copy the first one found to the output parameter (if provided)
    if (ip_address_str != NULL)
    {
      strncpy(ip_address_str, ip_string_buffer, INET6_ADDRSTRLEN - 1);
      ip_address_str[INET6_ADDRSTRLEN - 1] = '\0';
    }
  }

  freeaddrinfo(server_info); // Free the linked list
  return 0;
}


/*
int main() {
    char hostname[] = "www.google.com";
    char ip[INET6_ADDRSTRLEN]; // Buffer big enough for IPv6
    
    if (dacgethostbyname(hostname, ip) == 0) {
        printf("\nFirst resolved IP for %s is: %s\n", hostname, ip);
    }

    return 0;
}
*/















/* Legacy Method: gethostbyname() (IPv4 Only) */
#if 0

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int
dacgethostbyname(char *hostname, char *ipaddress)
{
  struct hostent *hp;
  char **p;
  int nnames;

  hp = gethostbyname(hostname);
  if(hp == NULL)
  {
    printf("dacgethostbyname ERROR: information for >%s< not found\n",hostname);
    return(-1);
  }

  nnames=0;
  for(p = hp->h_addr_list; *p != 0; p++)
  {
    struct in_addr in;
    char **q;

    memcpy(&in.s_addr, *p, sizeof (in.s_addr));

    if(nnames++ > 1)
    {
      printf("dacgethostbyname: WARN: found another IP address >%s< \n",inet_ntoa(in));
    }
    else
    {
      strcpy(ipaddress,inet_ntoa(in));
      printf("dacgethostbyname: INFO: host >%s< has IP address >%s< \n",hostname,ipaddress);
    }

  }

  return(0);
}

#endif


