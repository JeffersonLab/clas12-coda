
/*et_bridge_hipo.cc*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>

#include "table.h"
#include "utils.h"
#include "bank.h"
#include "decoder.h"
#include "reader.h"
#include "writer.h"
#include "evioBankUtil.h"

#include "et_private.h"
#include "et_network.h"

#include "et2hipo.h"



#define MAXBUF 10000000
#define ONE_MB 1048576
#define ONE_KB 1024
unsigned int buf[MAXBUF];



static int localET_2_localET(et_sys_id id_from, et_sys_id id_to,
		                     et_att_id att_from, et_att_id att_to,
                             et_bridge_config *config,
                             int num, int *ntransferred);

static int remoteET_2_ET(et_sys_id id_from, et_sys_id id_to,
                         et_att_id att_from, et_att_id att_to,
                         et_bridge_config *config,
                         int num, int *ntransferred);

static int ET_2_remoteET(et_sys_id id_from, et_sys_id id_to,
                         et_att_id att_from, et_att_id att_to,
                         et_bridge_config *config,
                         int num, int *ntransferred);

/*****************************************************/
/*               BRIDGE CONFIGURATION                */
/*****************************************************/

int et_bridge_config_init_hipo(et_bridgeconfig *config)
{
    et_bridge_config *bc;
  
    bc = (et_bridge_config *) malloc(sizeof(et_bridge_config));
    if (bc == NULL) {
        return ET_ERROR;
    }
  
    /* default configuration for a station */
    bc->mode_from            = ET_SLEEP;
    bc->mode_to              = ET_SLEEP;
    bc->chunk_from           = 100;
    bc->chunk_to             = 100;
    bc->timeout_from.tv_sec  = 0;
    bc->timeout_from.tv_nsec = 0;
    bc->timeout_to.tv_sec    = 0;
    bc->timeout_to.tv_nsec   = 0;
    bc->func                 = NULL;
    bc->init                 = ET_STRUCT_OK;
  
    *config = (et_bridgeconfig) bc;
    return ET_OK;
}

/*****************************************************/
int et_bridge_config_destroy_hipo(et_bridgeconfig sconfig)
{
  if (sconfig != NULL) {
    free((et_bridge_config *) sconfig);
  }
  return ET_OK;
}

/*****************************************************/
int et_bridge_config_setmodefrom_hipo(et_bridgeconfig config, int val)
{
  et_bridge_config *bc = (et_bridge_config *) config;
  
  if (bc->init != ET_STRUCT_OK) {
    return ET_ERROR;
  }
   
  if ((val != ET_SLEEP) &&
      (val != ET_TIMED) &&
      (val != ET_ASYNC))  {
    return ET_ERROR;
  }
  
  bc->mode_from = val;
  return ET_OK;
}

/*****************************************************/
int et_bridge_config_getmodefrom_hipo(et_bridgeconfig config, int *val)
{
  et_bridge_config *bc = (et_bridge_config *) config;
  
  if (val == NULL) return ET_ERROR;
  *val = bc->mode_from;
  return ET_OK;
}

/*****************************************************/
int et_bridge_config_setmodeto_hipo(et_bridgeconfig config, int val)
{
  et_bridge_config *bc = (et_bridge_config *) config;
  
  if (bc->init != ET_STRUCT_OK) {
    return ET_ERROR;
  }
   
  if ((val != ET_SLEEP) &&
      (val != ET_TIMED) &&
      (val != ET_ASYNC))  {
    return ET_ERROR;
  }
  
  bc->mode_to = val;
  return ET_OK;
}

/*****************************************************/
int et_bridge_config_getmodeto_hipo(et_bridgeconfig config, int *val)
{
  et_bridge_config *bc = (et_bridge_config *) config;
  
  if (val == NULL) return ET_ERROR;
  *val = bc->mode_to;
  return ET_OK;
}

/*****************************************************/
int et_bridge_config_setchunkfrom_hipo(et_bridgeconfig config, int val)
{
  et_bridge_config *bc = (et_bridge_config *) config;
  
  if (bc->init != ET_STRUCT_OK) {
    return ET_ERROR;
  }
   
  if (val < 1)  {
    return ET_ERROR;
  }
  
  bc->chunk_from = val;
  return ET_OK;
}

/*****************************************************/
int et_bridge_config_getchunkfrom_hipo(et_bridgeconfig config, int *val)
{
  et_bridge_config *bc = (et_bridge_config *) config;
  
  if (val == NULL) return ET_ERROR;
  *val = bc->chunk_from;
  return ET_OK;
}

/*****************************************************/
int et_bridge_config_setchunkto_hipo(et_bridgeconfig config, int val)
{
  et_bridge_config *bc = (et_bridge_config *) config;
  
  if (bc->init != ET_STRUCT_OK) {
    return ET_ERROR;
  }
   
  if (val < 1)  {
    return ET_ERROR;
  }
  
  bc->chunk_to = val;
  return ET_OK;
}

/*****************************************************/
int et_bridge_config_getchunkto_hipo(et_bridgeconfig config, int *val)
{
  et_bridge_config *bc = (et_bridge_config *) config;
  
  if (val == NULL) return ET_ERROR;
  *val = bc->chunk_to;
  return ET_OK;
}

/*****************************************************/
int et_bridge_config_settimeoutfrom_hipo(et_bridgeconfig config, struct timespec val)
{
  et_bridge_config *bc = (et_bridge_config *) config;
  
  if (bc->init != ET_STRUCT_OK) {
    return ET_ERROR;
  }
   
  bc->timeout_from = val;
  return ET_OK;
}

/*****************************************************/
int et_bridge_config_gettimeoutfrom_hipo(et_bridgeconfig config, struct timespec *val)
{
  et_bridge_config *bc = (et_bridge_config *) config;
  
  if (val == NULL) return ET_ERROR;
  *val = bc->timeout_from;
  return ET_OK;
}

/*****************************************************/
int et_bridge_config_settimeoutto_hipo(et_bridgeconfig config, struct timespec val)
{
  et_bridge_config *bc = (et_bridge_config *) config;
  
  if (bc->init != ET_STRUCT_OK) {
    return ET_ERROR;
  }
   
  bc->timeout_to = val;
  return ET_OK;
}

/*****************************************************/
int et_bridge_config_gettimeoutto_hipo(et_bridgeconfig config, struct timespec *val)
{
  et_bridge_config *bc = (et_bridge_config *) config;
  
  if (val == NULL) return ET_ERROR;
  *val = bc->timeout_to;
  return ET_OK;
}

/*****************************************************/
int et_bridge_config_setfunc_hipo(et_bridgeconfig config, ET_SWAP_FUNCPTR func)
{
  et_bridge_config *bc = (et_bridge_config *) config;
  
  if (bc->init != ET_STRUCT_OK) {
    return ET_ERROR;
  }
   
  bc->func = func;
  return ET_OK;
}

/*****************************************************/
int et_bridge_CODAswap_hipo(et_event *src_ev, et_event *dest_ev, int bytes, int same_endian)
{
  int nints = bytes/sizeof(int);
  
  /* DEPRECATED: Swapping is now done in evio library. */
  return ET_ERROR;
  
  et_CODAswap((int *)src_ev->pdata, (int *)dest_ev->pdata, nints, same_endian);
  return ET_OK;
}



/*****************************************************/
/*                 BRIDGE ROUTINES                   */
/*****************************************************/

int et_events_bridge_hipo(et_sys_id id_from, et_sys_id id_to,
		          et_att_id att_from, et_att_id att_to,
		          et_bridgeconfig bconfig,
		          int num, int *ntransferred)
{

  et_id *idfrom = (et_id *) id_from, *idto = (et_id *) id_to;
  et_bridge_config *config;
  et_bridgeconfig   default_config = NULL;
  int status=ET_ERROR, auto_config=0;
  
  *ntransferred = 0;
  
  /* The program calling this routine has already opened 2 ET
   * systems. Therefore, both must have been compatible with 
   * the ET lib used to compile this program and are then
   * compatible with eachother as well.
   */
  
  /* if no configuration is given, use defaults */
  if (bconfig == NULL)
  {
    auto_config = 1;
    if (et_bridge_config_init_hipo(&default_config) == ET_ERROR)
    {
      if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR))
      {
        printf("ERROR: et_events_bridge_hipo, null arg for \"bconfig\", cannot use default\n");
      }
      return ET_ERROR;
    }
    bconfig = default_config;
  }
  config = (et_bridge_config *) bconfig;
  
  /* if we have a local ET to local ET transfer ... */
  if ((idfrom->locality != ET_REMOTE) && (idto->locality != ET_REMOTE))
  {
    printf("Not supported yet\n");
    exit(0);
    /*
    status = localET_2_localET(id_from, id_to, att_from, att_to, config, num, ntransferred);
    */
  }
  /* else if getting events from remote ET and sending to local ET ... */
  else if ((idfrom->locality == ET_REMOTE) && (idto->locality != ET_REMOTE))
  {
    status = remoteET_2_ET(id_from, id_to, att_from, att_to, config, num, ntransferred);
  }
  /* else if getting events from local ET and sending to remote ET or
   * else going from remote to remote systems.
   *
   * If we have a remote ET to remote ET transfer, we
   * can use either ET_2_remoteET or remoteET_2_ET.
   */
  else
  {
    printf("Not supported yet\n");
    exit(0);
    /* 
    status = ET_2_remoteET(id_from, id_to, att_from, att_to, config, num, ntransferred);
    */
  }
  
  if(auto_config)
  {
    et_bridge_config_destroy_hipo(default_config);
  }
  
  return(status);
}






static unsigned int *data[1000]; //assume event chunk from "fromET" never exceed 1000 events

static coda::table table;
static coda::decoder decoder;
static hipo::event  eout(2*ONE_MB); //event
static hipo::dataframe frame(100,8*ONE_MB); // record: frame(maximum number of events, maximum number of bytes); 
static hipo::composite bank;//(42,11,std::string("bssbsl"),1024*1024*2);

static coda::eviodata_t      evioptr;

#define DEBUG
//#define DEBUG1

/*scan evio record from et system, and call appropriate et2hipo procedures*/ 
static int
evioEtScan(unsigned int *buf)
{
  int nbytes, ind_data;
  int ind,  len,  nwf, tagf, padf, typf, numf;
  int indb, lenb, nwb, tagb, padb, typb, numb;
  int ind_save, ind2, len2, ind3, pad3;

  len = buf[0]+1;  /* event length */
  ind = 2;         /* skip event header */
  while(ind<len)
  {
#ifdef DEBUG1
    printf("evioEtScan: ind=%d len=%d\n",ind,len);fflush(stdout);
#endif
    nwf = buf[ind] + 1; /* the number of words in fragment */
    tagf = (buf[ind+1]>>16)&0xffff;
    padf = (buf[ind+1]>>14)&0x3;
    typf = (buf[ind+1]>>8)&0x3f;
    numf =  buf[ind+1]&0xff;
#ifdef DEBUG1
    printf("evioEtScan: tagf=%d(0x%04x) typf=%d nwf=%d\n",tagf,tagf,typf,nwf);fflush(stdout);
    if(tagf>=41 && tagf<=58) printf("=== DCRB fragment %d\n",tagf);
#endif

    /*bank of banks (fragment)*/
    if(typf!=0xe && typf!=0x10)
    {
      ind += nwf; /*skip regular banks on top level*/
    }
    else
    {
#ifdef DEBUG1
      printf("evioEtScan: found fragment, tagf=%d, numf=%d, nwf=%d, ind=%d\n",tagf,numf,nwf,ind);fflush(stdout);
#endif
      ind += 2;
      lenb = 2;

      /*process banks inside fragment*/    
      while(lenb < nwf)
      {
        nwb = buf[ind] + 1;
        tagb = (buf[ind+1]>>16)&0xffff;
        padb = (buf[ind+1]>>14)&0x3;
        typb = (buf[ind+1]>>8)&0x3f;
        numb =  buf[ind+1]&0xff;
#ifdef DEBUG1
        printf("   evioEtScan: found bank, tagb=%d, numb=%d, nwb=%d, ind=%d\n",tagb,numb,nwb,ind);fflush(stdout);
#endif

        if(typb != 0xf) /*non-composite bank*/
        {
#ifdef DEBUG1
          printf("   evioEtScan: non-composite bank (ind=%d, len=%d, lenb=%d)\n",ind,len,lenb);fflush(stdout);
#endif
          nbytes = (nwb-2)<<2;
          ind_data = ind+2;
        }
        else /*composite bank*/
        {
#ifdef DEBUG1
          printf("   evioEtScan: composite bank (ind=%d, len=%d, lenb=%d)\n",ind,len,lenb);fflush(stdout);
#endif
          ind2 = ind+2; /* index of the tagsegment (contains format description) */
          len2 = (buf[ind2]&0xffff) + 1; /* tagsegment length */
          ind3 = ind2 + len2; /* index of the internal bank */
          pad3 = (buf[ind3+1]>>14)&0x3; /* padding from internal bank */
          nbytes = ((nwb-(2+len2+2))<<2)-pad3; /* bank_length - bank_header_length(2) - tagsegment_length(len2) - internal_bank_header_length(2) */
          ind_data = ind+2+len2+2;

          if(tagb == 0xe116)
	  {
#ifdef DEBUG1
            printf("=== DCRB bank: %6d 0x%08x %6d %6d\n",tagf,&buf[0],ind_data*4,nbytes);
#endif
          }
        }


	/*decode*/
        evioptr.crate = tagf;
        evioptr.tag   = tagb;
        evioptr.offset = ind_data*4;
        evioptr.length = nbytes;
        evioptr.buffer = reinterpret_cast<const char*>(&buf[0]);
        decoder.decode(evioptr);
	/*decode*/
 

        ind += nwb; /* jump to the next bank */
        lenb += nwb; /*total length of all banks in fragment*/

        if(nwb<=0)
	{
          printf("evioEtScan: ERROR: nwb=%d (tagf=%d(0x%04x), tagb=%d(0x%04x), ind=%d, len=%d) - return\n",
            nwb,tagf,tagf,tagb,tagb,ind,len);fflush(stdout);
          return(0);
	}

      } /*while(lenb<nwf)*/

    }/*process fragment*/

  }/*while(ind<len)*/

  return(0);
}


static int evio_events_total = 0;
static time_t starting_time = 0;


/******************************************************/
static int
remoteET_2_ET(et_sys_id id_from, et_sys_id id_to, et_att_id att_from, et_att_id att_to,
              et_bridge_config *config, int num, int *ntransferred)
{
  et_id *idfrom = (et_id *) id_from, *idto = (et_id *) id_to;
  et_event *put;
  int sockfd = idfrom->sockfd;
  int i=0, ii, k, status=ET_OK, err;
  uint64_t size, len;
  int swap=0, byteordershouldbe=0, same_endian;
  int num_remains;
  int transfer[7], incoming[2], header[9+ET_STATION_SELECT_INTS]; /*ET_STATION_SELECT_INTS=6 (see et.h)*/
  int num_read=0;
  int total_put=0;
  int num_so_far=0;
  int ndata=idfrom->nevents/5+10; /*the maximum number of events in one chunk*/
  int sdata=idfrom->esize; /*maximum event size (words)*/

  static int first;

  if(first==0)
  {
    decoder.initialize();
    //table.read("dc_tt.txt");
    decoder.showKeys();

    ndata = idfrom->nevents/5+10;
    //printf("ndata=%d\n",ndata);
    for(i=0; i<ndata; i++)
    {
      data[i] = (unsigned int *) calloc(sdata,4); /*allocate event data array */
      //printf("allocated data[%d]=0x%llx, size %d words\n",i,data[i],sdata);
    }
    sleep(1);
    first = 1;
    starting_time = time(NULL);
  }
  else
  {
    frame.reset();
  }


  /* never deal with more the total num of events in the "from" ET system */
  if(num > idfrom->nevents/5)
  {
    printf("ERROR: The number of events in ETfrom is %d, requested %d events to tranfer\n",idfrom->nevents,num);
    printf("       For performance reason, cannot transfer at once more then 20%% of total events\n");
    printf("       Maximum number of events to be transfered at once is %d\n",idfrom->nevents/5);
    exit(0);
  }


  et_tcp_lock(idfrom);
 
  /* 'num' - the number of events we want to get from "from" ET;
     'num_so_far' - the number of events we've got from "from" ET so far */
  //printf("11\n");fflush(stdout);
  while (num_so_far < num)
  {
    /* First, get events from the "from" ET system.
     * We're borrowing code from etr_events_get and
     * modifying the guts.
     */

    /* 'num_remains' - the number of events we still need to get from "from" ET */
    num_remains = (num - num_so_far < config->chunk_from) ? (num - num_so_far) : config->chunk_from;

    /* prepare request to be send to "from" ET */
    transfer[0] = htonl(ET_NET_EVS_GET);
    transfer[1] = htonl(att_from);
    transfer[2] = htonl(config->mode_from & ET_WAIT_MASK);
    transfer[3] = 0; /* we are not modifying data */
    transfer[4] = htonl(num_remains);
    transfer[5] = 0;
    transfer[6] = 0;
    if (config->mode_from == ET_TIMED)
    {
      transfer[5] = htonl(config->timeout_from.tv_sec);
      transfer[6] = htonl(config->timeout_from.tv_nsec);
    }
 
    /* send request */
    if (etNetTcpWrite(sockfd, (void *) transfer, sizeof(transfer)) != sizeof(transfer))
    {
      et_tcp_unlock(idfrom);
      if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR))
      {
        printf("ERROR: et_events_bridge_hipo, write error\n");
      }
      return ET_ERROR_WRITE;
    }
 
    /* read response; if 'err' is positive, it contains the number of events we will receive */
    if (etNetTcpRead(sockfd, (void *) &err, sizeof(err)) != sizeof(err))
    {
      et_tcp_unlock(idfrom);
      if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR))
      {
        printf("ERROR: et_events_bridge_hipo, read error\n");
      }
      return ET_ERROR_READ;
    }
    err = ntohl(err);
    if (err < 0)
    {
      et_tcp_unlock(idfrom);
      return err;
    }
    
    /* read total size of data to come into 'int incoming[2]' - in bytes */
    if (etNetTcpRead(sockfd, (void *) incoming, sizeof(incoming)) != sizeof(incoming))
    {
      et_tcp_unlock(idfrom);
      if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR))
      {
        printf("ERROR: et_events_bridge_hipo, read error\n");
      }
      return ET_ERROR_READ;
    }

    /* convert to 64bit 'size' */
    size = ET_64BIT_UINT(ntohl(incoming[0]),ntohl(incoming[1]));

    num_read = err; /* the number of events we will receive */
    num_so_far += num_read; /* after we will read 'num_read' events, we will have 'num_so_far' events from "from" ET */

    /* Now that we know how many events we are about to receive,
     * get the new event from the "to" ET system & fill w/data.
     */

    /* how many new events still to get from "to" ET */
    num_remains = num_read;
    status = et_event_new(id_to, att_to, &put, config->mode_to, &config->timeout_to, sdata*num_remains);
    if (status != ET_OK)
    {
      if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR))
      {
        printf("ERROR: et_events_bridge_hipo, error (status = %d) getting new events from \"to\" ET system\n",status);
        printf("ERROR: et_events_bridge_hipo, connection to \"from\" ET system will be broken, close & reopen system\n");
      }
      goto end;
    }





    //printf("22\n");fflush(stdout);

    /* get 'num_remains' events from "from" ET */
    if(num_remains >= ndata)
    {
      printf("ERROR: num_remains=%d >= ndata=%d - exit\n",num_remains,ndata);
      exit(0);
    }

    for(i=0; i<num_remains; i++)
    {
      /* Read in the "from" ET event's header info */
      if (etNetTcpRead(sockfd, (void *) header, sizeof(header)) != sizeof(header))
      {
        if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR))
        {
          printf("ERROR: et_events_bridge_hipo, reading event header error\n");
        }
        status = ET_ERROR_READ;
        goto end;
      }
      //printf("header size %d words\n",sizeof(header)/4);

      /* data length in bytes */
      len = ET_64BIT_UINT(ntohl(header[0]),ntohl(header[1]));
      //printf("expecting evio event[%d], size is %d bytes (%d words)\n",i,len,len/4); 



      /*error: have to check is all data SO FAR is larger ???*/
      /* If data is larger than new event's memory size ... */
      if (len > put->memsize)
      {
        printf("ERROR in et_events_bridge_hipo: data is larger than new event's memory size=%d - exit\n",put->memsize);
        exit(0);
      }


    
      /*copy relevant event information  from "from" ET to "to" ET*/
      //put->length     =  len;
      put->priority   =  ntohl(header[4]) & ET_PRIORITY_MASK;
      put->datastatus = (ntohl(header[4]) & ET_DATA_MASK) >> ET_DATA_SHIFT;
      put->byteorder  =  header[7];
      for (k=0; k < idto->nselects; k++)
      {
        put->control[k] = ntohl(header[k+9]);
      }

      /* copy actual data from "from" ET to 'data[i]' array */
      if (etNetTcpRead(sockfd, data[i], len) != len)
      {
        if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR))
        {
          printf("ERROR: et_events_bridge_hipo, reading event data error\n");
        }
        status = ET_ERROR_READ;
        goto end;
      }
      //printf("received evio event[%d], internal size is %d words (data size=%d words)\n",i,data[i][0],data[i][8]); 


      unsigned int *ptr = data[i];
      /*
      for(ii=0; ii<20; ii++)
      {
        printf("pdata[%2d]=0x%08x\n",ii,ptr[ii]);
      }
      */




      unsigned int *bufptr = &ptr[8];

      //printf("11\n");fflush(stdout);

      evioEtScan(bufptr);
      //decoder.show();

      //printf("12\n");fflush(stdout);

      //printf("event # %d : after decoding bank rows = %d\n" ,counter,bank.getRows() );
      eout.reset();

      // without decoder.write - 58Hz, with - 25Hz
      decoder.write(eout);
      decoder.reset();



      //bool success = frame.addEvent(eout);
      //if(success==false)
      //{
      //  processFrame(frame.buffer());
      //  frame.reset();
      //} 

      frame.addEvent(eout);
      evio_events_total ++;

    } /*for(i=0; i<num_remains; i++)*/


    //printf("33\n");fflush(stdout);


    const int *ppp = reinterpret_cast<const int *>(frame.buffer());
    int size = frame.size(); // the number of bytes in frame
    //printf("ppp=0x%x size=%d\n",ppp,size);

    put->length = size;
    memcpy(put->pdata,ppp,size);






    /* now put[] array is filled in, we can put new events back to the "to" ET system */
    if (num_remains)
    {
      //printf("put %d evio events into single hipo record\n",num_remains);
      status = et_event_put(id_to, att_to, put);
      if (status != ET_OK)
      {
        if ((idfrom->debug >= ET_DEBUG_ERROR) || (idto->debug >= ET_DEBUG_ERROR))
        {
          printf("ERROR: et_events_bridge_hipo, error (status = %d) putting new event to \"to\" ET system\n",status);
          printf("ERROR: et_events_bridge_hipo, connection to \"from\" ET system may be broken, close & reopen system\n");
        }
        et_event_dump(id_to, att_to, put);
        goto end;
      }
      total_put += 1;
    }

  } /*while (num_so_far < num)*/

  //printf("99\n");fflush(stdout);

  et_tcp_unlock(idfrom);


  
end:

  et_tcp_unlock(idfrom);
  *ntransferred = total_put;


  time_t tdiff = time(NULL) - starting_time;
  if( tdiff>0 && (evio_events_total%20000)==0 )
  {
    //printf("tdiff=%d (starting_time=%d)\n",tdiff,starting_time);
    printf("average evio event rate is %d Hz\n",evio_events_total/tdiff);fflush(stdout);
  }

  return(status);
}
