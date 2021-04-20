#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <sys/types.h>

#define SVTDAQMAXSTRLEN 256
static int cal_group = 0;
static int cal_delay = 0;
static int cal_level = 0;
static int run_type = 0;
static int dpmWithConfigDump = 51;
static int controlDpmRocId = 66;
static int writeConfig = 0;



/* find run type for SVT DAQ */
static int getRunType(char* filename) {
	int type;
    printf("LOOK for calib str in \"%s\"\n",filename);
	if(strstr(filename,"injection")!=NULL) 
       type = 1;
	else if(strstr(filename,"t0Single")!=NULL) 
       type = 3;
	else if(strstr(filename,"t0")!=NULL) 
       type = 2;
    else 
       type = 0;
	return type;
}

static int getCalLevel(int i) {
   int level;
   switch (i) {
      case 0 :
         level = 29;
         break;
      case 1 :
         level = 48;
         break;
      case 2 :
         level = 80;
         break;
      case 3 :
         level = 120;
         break;
      case 4 :
         level = 192;
         break;
      default:
         printf("this cal_level should never happen. Exit!\n");
         exit(1);                    
   }
   return level;
}




static void getFebConfigFilePath(char* conf, char* type, char* f, int MAX) {

   //initialize
   strcpy(f,"");
   
  if(strlen(conf)>0) {
     printf("Find config file from \"%s\":\n",conf);
     FILE * confFile = fopen(conf,"r");
     if (confFile == NULL) printf("Error opening file \"%s\"\n",conf);
     
     char line[SVTDAQMAXSTRLEN];
     while( fgets (line , 256 , confFile) != NULL ) {
        //printf("%s\n",line);
        //split by white space
        char **split_line = NULL;
        char* p = strtok(line," ");
        int n_spaces = 0;
        int i;
        while(p) {
           split_line = realloc(split_line, sizeof(char*)*++n_spaces);
           if(split_line==NULL) {
              printf("mem allocation for string failed!");
              exit(1);
           }
           split_line[n_spaces-1] = p;
           p = strtok(NULL," ");
        }
        if(n_spaces==2) {
           
           if((strcmp(split_line[0],"FEB_CONFIG_FILE")==0 && strcmp(type,"CONFIG")==0) || (strcmp(split_line[0],"THR_CONFIG_FILE")==0 && strcmp(type,"THRESHOLDS")==0 )) {
              if(strlen(split_line[1])>255) {
                 printf("config file path is too long: %s", split_line[1]);
                 strcpy(f,"");
                 return;
              }
              //remove carriage return of there
              char* carriage_return = strchr(split_line[1],'\n');
              if(carriage_return!=NULL) {
                 *carriage_return = '\0';
                 //printf("removed carriage return (%s)\n",split_line[1]);
              }
              strcpy(f,split_line[1]);              
           } 
           else {
              printf("[ getFebConfigFile ] : wrong type and config file combination found (type=%s file %s) -> skip\n",type,split_line[0]);                    
           }
        }
        free(split_line);
     }
     fclose(confFile);
  } else {
     printf("CONFFILE is empty.");
  }
  return;
}
     

// Extract the config file path from the config file
static void getValueFromConfig(char* confFile, const char* keyword, char* outValue, const int MAX) {
   printf("getValueFromConfig\n");

   char filename[SVTDAQMAXSTRLEN];
   char baseDir[SVTDAQMAXSTRLEN];
   char host[SVTDAQMAXSTRLEN];
   char line[SVTDAQMAXSTRLEN];
   char key[SVTDAQMAXSTRLEN];
   char value[SVTDAQMAXSTRLEN];
   int active;
   FILE* file;
   const int debug = 0;
   //baseDir = getenv("CLON_PARMS");   
   strcpy(baseDir,"/mnt/clas12/release/0.2/parms");
   if(debug>0) printf("baseDir %s\n", baseDir);
   //gethostname(host,256);
   strcpy(host,"all");
   if(debug>0) printf("host %s\n", host);
   
   //initialize to empty
   strcpy(outValue,"");

   
   if(strlen(confFile)==0 || strcmp(confFile,"none")==0) {   
      if(debug>0) printf("Extract keyword \"%s\" from default conf file.\n", keyword);
      sprintf(filename,"%s/dpm/dpm-default.trg",baseDir);
      
   } else {
      if(debug>0) printf("Extract keyword \"%s\" from conf file \"%s\".\n", keyword, confFile);
      sprintf(filename,"%s",confFile);
   }      
   
   printf("Config filename \"%s\"\n", filename);
   

   file = fopen(filename, "r");
   if (file == NULL) {
      printf("Error opening file \"%s\"\n",file);
      exit(1);
   }
   
   active = 0;
   while( fgets(line,256,file) != NULL ) {
      
      if(debug>0) printf("active %d: Processing line \"%s\"\n", active,line);
      
      if(strlen(line)>0) {
         
         if(line[0]!='#') {

            if(debug>0) printf("1\n");
            // reset
            strcpy(key,"");
            strcpy(value,"");

            sscanf(line, "%s %s", key, value);

            if(debug>0) printf("%s %s\n",key,value);
            
            // look for the DPM crate
            if(strcmp(key,"DPM_CRATE")==0) {  

               printf("got crate\n");
               if(debug>0) printf("compare host %s\n",host);
               if(debug>0) printf("compare value %s\n",value);
               
               if(strcmp(value,host)==0) {
                  active = 1;            
                  printf("active %d %s %s (%s)\n",active,key,host,value);
               } else {
                  //I'm not really using active since I exit but in the future we could.
                  //printf("line \"%s\" in config file \"%s\" is forbidden  Fix config file.\n",line,filename);
                  active = 0;
               }
            }
            else if( active==1 && strcmp(key,keyword)==0) {
               printf(" got value %s\n",value);
               if(strlen(value)<MAX) {
                  strcpy(outValue,value);
               } else {
                  printf(" ERROR: the path from the config file is too long?!. Fix config file.\n");
                  exit(1);
               }
            } else {         
               printf(" do nothing here\n");
            }      
         }
      }
   } //lines
   fclose(file);
}


static void getDpmConfigFilePath(char* confFile, char* outFilePath, const int MAX) {
   printf("getDpmConfigFilePath\n");
   getValueFromConfig(confFile,"DPM_CONFIG_FILE",outFilePath,MAX);
}

static void getDpmThresholdFilePath(char* confFile, char* outFilePath, const int MAX) {
   printf("getDpmThresholdFilePath\n");
   getValueFromConfig(confFile,"DPM_THR_CONFIG_FILE",outFilePath,MAX);
}


