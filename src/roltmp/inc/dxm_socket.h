#ifndef DXM_SOCKET_H
#define DXM_SOCKET_H

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h> 
#include <netdb.h> 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define RCE_SOCKET_DEBUG 10
#define BUF_SIZE 256

// Constants
enum rce_xml_content_type{STATUS,CONFIG,STRUCTURE,UNDEFINED};
typedef enum rce_xml_content_type RCEXMLTYPE;

typedef struct teststruct {
  int i;
} teststruct;
typedef teststruct TESTSTR;

// Structures
typedef struct rce_xml_content {
  RCEXMLTYPE type;
  char* xml_str;
} rce_xml_content;
typedef rce_xml_content RCEXML;

typedef struct rce_xml_tag {
  int valid;
  char name[BUF_SIZE];
  char str[BUF_SIZE];
} rce_xml_tag;
typedef rce_xml_tag RCEXMLTAG;

// Functions
//RCEXMLTYPE get_xml_type(char* xml_str);
//RCEXMLTAG get_xml_tag(RCEXML xml, const char* tag_name);



int close_socket(int socketfd) {
    return close(socketfd);
}

void socket_error(const char *msg)
{
    perror(msg);
    //exit(0);
}

int open_socket(char* hostname, int portno) {

    // Structure to contain the address of the server
    struct sockaddr_in serv_addr;
    // Structure defining the host computer
    struct hostent *server;
    int socketfd;

    // Create a socket  
    if(RCE_SOCKET_DEBUG > 0) printf("[ open_socket ]: Opening new socket\n"); 
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0) {
        socket_error("[ open_socket ]: [ ERROR ] Socket could not be opened\n");
        close_socket(socketfd);
        return socketfd;
    }
    if(RCE_SOCKET_DEBUG > 1) printf("[ open_socket ]: New socket (%i) has been opened\n", socketfd); 

    // Get the host information
    if(RCE_SOCKET_DEBUG>1) printf("[ open_socket ]: Getting the host information\n");
    server = gethostbyname(hostname);
    if (server == NULL) {
      char msgstr[256];
      sprintf(msgstr,"[ open_socket ]: [ ERROR ]: Host \"%s\" was not found.",hostname);
      socket_error(msgstr);
      close_socket(socketfd);
      return -1;
    }
    if(RCE_SOCKET_DEBUG > 1) printf("[ open_socket ]: Host with name %s found at address %s \n", 
                                        server->h_name, server->h_addr);
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
            (char *)&serv_addr.sin_addr.s_addr,
            server->h_length);
    serv_addr.sin_port = htons(portno);

    // Connect to the host
    if(RCE_SOCKET_DEBUG>1) printf("[ open_socket ]: Connecting to host %s ...\n", server->h_name);
    if (connect(socketfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
      char tmp[256];
      if(RCE_SOCKET_DEBUG>0) sprintf(tmp,"[ open_socket ]: [ WARNING ] Couldn't connect to host %s %d\n", hostname, portno);
      else strcpy(tmp,"");
      socket_error(tmp);
      close_socket(socketfd); 
      return -1;
    }
    if(RCE_SOCKET_DEBUG>1) printf("[ open_socket ] : return socket %d\n",socketfd);
    
    return socketfd;
}




int find_system_str(char* buf, const int MAX, char** start) {
  if(RCE_SOCKET_DEBUG>1) printf("[ findSystemStr ]: finding system string from %p and %d chars len with start at %p\n",buf,MAX,*start);
  char* b;
  char* e;
  char* s;
  char* p_ending;
  char* status_tag_s;
  char* status_tag_e;

  b = buf;
  while(1!=0) {    
    s = strstr(b,"<system>");  
    p_ending = strchr(b,'\f');  

    if(s!=NULL) {
      if(p_ending!=NULL) {      
	//check that status exists
	if(RCE_SOCKET_DEBUG>1) printf("[ findSystemStr ]: found system at len %d and ending and len %d\n",s-b,p_ending-b);
	status_tag_s = strstr(b,"<status>");
	status_tag_e = strstr(b,"</status>");
	// look at this system string  if status tags are found inside the ending
	if(status_tag_s!=NULL && status_tag_e!=NULL) {
	  if(RCE_SOCKET_DEBUG>1) printf("[ findSystemStr ]: found status tags at len %d and %d\n",status_tag_s-b, status_tag_e-b);
	  if((status_tag_s-b)<(p_ending-b) && (status_tag_e-b)<(p_ending-b)) {
	    if(RCE_SOCKET_DEBUG>1) printf("[ findSystemStr ]: found status tags inside ending\n");
	    // return this
	    *start = s;
	    e = p_ending-1;
	    if(RCE_SOCKET_DEBUG>1) {
	      printf("[ findSystemStr ]: found s at %p and e at %p and *start at %p with len %d \n",s,e,*start,e-s);
	      printf("[ findSystemStr ]: last characters are:\n");
	      int ii;
	      for(ii=-50;ii<=0;++ii) {
		char ee = *(e+ii);
		printf("[ findSystemStr ]: %d: '%c'\n",ii,ee);
	      }
	    }
	    return (int)(e-s);
	  }
	} 
	else {
	  // go to next, if there is one
	  b = p_ending+1;
	  if((b-buf)>MAX) return -1;
	}
      } else {
	if(RCE_SOCKET_DEBUG>1) printf("[ findSystemStr ]: p_ending couldn't be found\n"); 
	// nothing in this string to work with
	break;
      }
    } else {
      if(RCE_SOCKET_DEBUG>1) printf("[ findSystemStr ]: <system> couldn't be found\n"); 
      // nothing in this string to work with
      break;      
    }
  }


  return -1;
}








void poll_string(int socketfd, char** xml_string_out, int* len_out) {
   char* buf = NULL;
   char* buf_loop = NULL;
   int buf_len;
   int read_i;
   int read_n;
   int nempty;
   int counter;
   int n_endings;
   time_t timer;
   time_t cur_time;
   int dt;
   char *pch;
   int k;
   
   
   if(RCE_SOCKET_DEBUG>0) printf("[ pollDpmXmlString ]:  from socket %d \n", socketfd);
      
   time(&timer);
   
   
   nempty=0;
   counter=0;
   read_i=0;
   buf_len=0;
   n_endings=0;
   dt=0;
   
   // loop for 10s
   while(dt<10) { 
     
     time(&cur_time);

     dt = difftime(cur_time,timer);
     
     if(RCE_SOCKET_DEBUG>1) 
       printf("[ pollDpmXmlString ]: Try to read from socket (nempty %d read_i %d time %ds)\n",nempty,read_i,dt); //,asctime(localtime(&cur_time)));
     
     read_n = 0;
     ioctl(socketfd, FIONREAD, &read_n);
     
     if(RCE_SOCKET_DEBUG>1) {
       printf("[ pollDpmXmlString ]: %d chars available on socket\n",read_n);
     }
     


     if(read_n>0) {      
       
       // allocate memory needed
       if(RCE_SOCKET_DEBUG>1) printf("[ pollDpmXmlString ]: Allocate %d array\n",read_n);      
       
       // check that the buffer used is not weird
       if(buf_loop!=NULL) {
         printf("[ pollDpmXmlString ]: [ ERROR ]: buf_loop is not null!\n");
         exit(1);
       }
       
       // allocate space to hold the input
       buf_loop = (char*) calloc(read_n+1,sizeof(char));
       
       if(RCE_SOCKET_DEBUG>1) printf("[ pollDpmXmlString ]: Allocated buf_loop array at %p strlen %d with %d length \n",buf_loop,strlen(buf_loop),(int)sizeof(char)*(read_n+1));      


       
       // Read from socket
       read_n = read(socketfd,buf_loop,read_n);
       
       if(RCE_SOCKET_DEBUG>0) {
          printf("[ pollDpmXmlString ]: Read %d chars from socket\n",read_n);
          printf("[ pollDpmXmlString ]: buf_loop strlen is %d\n",strlen(buf_loop));
       }

       if (read_n < 0) {
         printf("[ pollDpmXmlString ]: [ ERROR ]: read %d from socket\n",read_n);
         exit(1);
       }


       
       // We only want to use the xml ending in order to avouid having problems 
       // parsing the full string with string tools. 
       // Therefore remove terminating chars.
       if(RCE_SOCKET_DEBUG>20) printf("[ pollDpmXmlString ]: fix terminations\n");
       for(k=0;k<read_n;++k) {
         if(buf_loop[k]=='\0') {
           if(RCE_SOCKET_DEBUG>2) {
	     printf("[ pollDpmXmlString ]: fix termination at idx %d in this buf_loop\n",k);
	     int kk;
	     for(kk=k-20;kk<k+20;++kk) {
	       if(kk>=0)  printf("[ pollDpmXmlString ]: char around null char kk=%d '%c'\n",kk,buf_loop[kk]);
	     }	     
	   }
           buf_loop[k]=' ';
         }
       }
       
       // search for xml endings in this buffer
       pch = strchr(buf_loop,'\f'); 
       while(pch!=NULL) { 
         if(RCE_SOCKET_DEBUG>1) printf("[ pollDpmXmlString ]: found ending at %p (array index %d) in this buf!\n",pch,pch-buf_loop); 
         n_endings++; 
         pch = strchr(pch+1,'\f'); 
       } 
       
       
       
       // copy to other buffer while looping            
       if(RCE_SOCKET_DEBUG>2) printf("[ pollDpmXmlString ]: Copy %d to other buffer (at %p before realloc) \n",read_n,buf);      
       
       // reallocate more memory
       buf = (char*) realloc(buf,sizeof(char)*(buf_len+read_n));
       if(buf==NULL) {
         printf("[ pollDpmXmlString ]: [ ERROR ]: failed to allocated buf\n");
         if(buf_loop==NULL) {
           free(buf_loop);
         }
         exit(1);
       }
       
       if(RCE_SOCKET_DEBUG>2) printf("[ pollDpmXmlString ]: Allocated longer buf at %p and copy to pointer %p (offset= %d) \n",buf,buf+buf_len,buf_len);      
       
       
       // do the copy
       memcpy(buf+buf_len,buf_loop,sizeof(char)*read_n);
       
       if(RCE_SOCKET_DEBUG>20) printf("[ pollDpmXmlString ]: memcpy done\n");
       
       //update the buffer length counter
       buf_len += read_n;      
       
       if(RCE_SOCKET_DEBUG>20) printf("[ pollDpmXmlString ]: free buf_loop\n");

       

       
       // free loop buffer for next loop
       if(buf_loop!=NULL) {
         free(buf_loop);
         buf_loop=NULL;
       }
       
       if(RCE_SOCKET_DEBUG>20) printf("[ pollDpmXmlString ]: end of read_i %d with buf strlen %d\n",read_i,strlen(buf));
       
       read_i++;
       
     } // read_n>0
     else {
       if(RCE_SOCKET_DEBUG>20) printf("[ pollDpmXmlString ]: Nothing to read from socket. Sleep a little..\n");      
       //usleep(1000);
       nempty++;
     } 
     
     
     
     if(n_endings>0) {
       if(RCE_SOCKET_DEBUG>1) printf("[ pollDpmXmlString ]: \nfound %d endings at read_i %d with at len %d and strlen %d. Stop reading from buffer\n",n_endings,read_i,buf_len,strlen(buf));      
       break;
     }
     
     
     counter++;
     
     
   } //time out
   
   
   
   if(RCE_SOCKET_DEBUG>0) {
     printf("[ pollDpmXmlString ]: Done reading from socket. Found %d endings and a buf_len of %d (dt=%d)\n",n_endings, buf_len, dt);
     if(buf!=NULL) printf("[ pollDpmXmlString ]: strlen %d\n", strlen(buf));
   }
   
   // Now find the substring of the large string buffer that contains the <system/> tags ending with the special termination char
   
   if(buf!=NULL) {

     // Check that I actually found any endings in the buffer
     if(n_endings>=1) {

       if(RCE_SOCKET_DEBUG>20) {
         printf("[ pollDpmXmlString ]: \nPick out config and status string between <system> and %d endings in string with strlen %d and buf_len %d\n",n_endings,strlen(buf),buf_len);
         //printf("[ pollDpmXmlString ]: \nbuf: \n%s\n",buf);
       }
       
       // find the <system/> tag substring start/stop pointers
       char* start = NULL;
       char* xml_str = NULL;       
       int len = find_system_str(buf, buf_len,&start);    
       
       
       // check that I got it.
       if(len>0) {               

         // find the end of the substring
         char* stop = start+len;
         
         if(RCE_SOCKET_DEBUG>20) {
           printf("[ pollDpmXmlString ]: len %d start at %p stop at %p\n",len,start, stop);
           printf("[ pollDpmXmlString ]: calloc xml string len %d\n",len+1);
         }
         
         // allocate memory for the substring
         xml_str = (char*) calloc(len+1,sizeof(char));
         
         // do the copy of the substring
         memcpy(xml_str,start,len);
         
         // terminate (remember we removed all of them inside the string)
         xml_str[len] = '\0'; 
         
         if(RCE_SOCKET_DEBUG>20) printf("[ pollDpmXmlString ]: \ncopied %d chars to %p with strlen %d\n%s\n",len+1,xml_str,strlen(xml_str),xml_str);
         
         // Set the pointer-to-pointer to the the substring pointer and the size
         *xml_string_out = xml_str;
         *len_out = len+1;
         
         if(RCE_SOCKET_DEBUG>20) printf("[ pollDpmXmlString ]: output pars are at %p and len %d\n",*xml_string_out,*len_out);
         
       } 
       else {
         if(RCE_SOCKET_DEBUG>0) printf("[ pollDpmXmlString ]: Couldn't find system and/or status string in xml buffer\n");
       }
     
     } else {
       if(RCE_SOCKET_DEBUG>-1) printf("[ pollDpmXmlString ]: Only found %d endings. Need at least one.\n",n_endings);       
     }
     
     free(buf);
     
     
   }
   else {
     if(RCE_SOCKET_DEBUG>0) printf("[ pollDpmXmlString ]: The string buffer is empty (null)\n");
   }
   
   if((*xml_string_out)==NULL) {
     if(RCE_SOCKET_DEBUG>0) printf("[ pollDpmXmlString ]: No valid xml string extracted from this poll (%d endings)\n",n_endings);
   }
   
   return;
   
}





RCEXMLTAG get_xml_tag(RCEXML xml, const char* tag_name) {
  RCEXMLTAG tag;
  char* start_ptr;
  char* end_ptr;
  char start_tag_str[BUF_SIZE];
  char end_tag_str[BUF_SIZE];


  // Build the tag strings to use in the search
  sprintf(start_tag_str, "<%s>",tag_name);
  sprintf(end_tag_str, "</%s>",tag_name);

  // Search for the start and end tags
  start_ptr = strstr(xml.xml_str, start_tag_str);
  end_ptr = strstr(xml.xml_str, end_tag_str);

  // Check if they are found
  if( start_ptr == NULL || end_ptr == NULL ) {
    
    if(RCE_SOCKET_DEBUG>0) printf("[ get_xml_tag ] : couldn't find tag %s\n",tag_name);
    
    // set the valid flag
    tag.valid = 0;
    
  } else {
    
    if(RCE_SOCKET_DEBUG>0) printf("[ get_xml_tag ] : found tag %s\n",tag_name);
    
    // find the starting point of the value string
    char* value_start_ptr = start_ptr + strlen(start_tag_str);
    
    // find the number of chars to copy
    int n = end_ptr - value_start_ptr;
    
    if(RCE_SOCKET_DEBUG>0) printf("[ get_xml_tag ] : value_start_ptr %p (%c) %d %d (%c)\n",value_start_ptr,*value_start_ptr,n,strlen(start_tag_str), *(value_start_ptr + n));
    
    // Copy the tag name to the struct
    strcpy(tag.name, tag_name);
    
    // Reset the memory of the value
    memset(tag.str,'\0',BUF_SIZE);
    
    // Copy the string
    strncpy(tag.str, value_start_ptr, n);

    // set the valid flag
    tag.valid = 1;    

  }

  return tag;
  
}


RCEXMLTYPE get_xml_type(char* xml_str) {
  
  char* start_ptr;
  char* end_ptr;


  // find the tags
  start_ptr = strstr(xml_str, "<status>");
  end_ptr = strstr(xml_str, "</status>");
  if( start_ptr != NULL && end_ptr != NULL )
    return STATUS;  
  
  start_ptr = strstr(xml_str, "<config>");
  end_ptr = strstr(xml_str, "</config>");
  if( start_ptr != NULL && end_ptr != NULL )
    return CONFIG;
  
  start_ptr = strstr(xml_str, "<structure>");
  end_ptr = strstr(xml_str, "</structure>");
  if( start_ptr != NULL && end_ptr != NULL )
    return STRUCTURE;
  
  return UNDEFINED;
}




static int openSocket(char* host_name, int port) {
  int socket;
  int i;
  
  // connect to socket
  socket = -1;
  i = 0;
  
  while(socket <=0 && i < 10) {
    port = port + i;
    printf("[ openSocket ] : try socket on port %d\n", port);
    socket = open_socket(host_name, port);
    i++;
  }
  
  if(socket <= 0) {
    printf("[ openSocket ] : [ ERROR ] failed to open socket\n");
  } else {
    printf("[ openSocket ] : opened socket %d\n", socket);
  }

  return socket;
  
}

static void closeSocket(int socket) {
  printf("[ closeSocket ] : close socket %d\n", socket);
  int result = close_socket(socket);
  printf("[ closeSocket ] : close socket result %d\n", result);  
}


static void getRunState(int socket, char* state_str, const int MAX) {
  char* xml_str;
  int xml_str_len;
  int n_sec;
  int i;
  
  // reset the output string
  strcpy(state_str,"");

  xml_str = NULL;
  xml_str_len = -1;

  // poll xml string
  poll_string(socket, &xml_str, &xml_str_len);
  
  printf("[ getRunState ] : Got %d XML string at %p \n", xml_str_len, xml_str);
  
  if (xml_str != NULL) {
    
    RCEXMLTYPE t;
    
    printf("[ getRunState ] : Look for type in str\n");
    t = get_xml_type( xml_str );
    
    printf("[ getRunState ] : Got type %d\n", t);
    
    RCEXML xml;
    xml.type = t;
    xml.xml_str = xml_str;
    
    printf("[ getRunState ] : Look for RunState\n");
    
    RCEXMLTAG tag;
    tag = get_xml_tag(xml,"RunState");
    
    if( tag.valid != 0) {
      printf("[ getRunState ] : Got valid tag %s value %s\n", tag.name, tag.str);
      strcpy(state_str, tag.str);
    }
    else
      printf("[ getRunState ] : No valid tag found\n");
    
    // The struct is not dynamically allocm but the string
    free(xml_str);
    
  } else {

    free(xml_str);

  }
  

}



#endif
