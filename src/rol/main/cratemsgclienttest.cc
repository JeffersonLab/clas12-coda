
/* cratemsgclienttest.cc - testbed for cratemsgclient class
      usage example:      ./Linux_i686/bin/cratemsgclienttest hps11 6102
*/

#if 1 /*defined(Linux_vme)*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "cratemsgclient.h"

CrateMsgClient *tcp;

#define LSWAP(x)        ((((x) & 0x000000ff) << 24) | \
                         (((x) & 0x0000ff00) <<  8) | \
                         (((x) & 0x00ff0000) >>  8) | \
                         (((x) & 0xff000000) >> 24))



unsigned int *buf;

int
main(int argc, char *argv[])
{
  char hostname[256];
  int hostport, len, ii, ret, slot, chan, partype;
  unsigned int buffer[100];

  printf("argc=%d\n",argc);fflush(stdout);

  if(argc==3)
  {
    strncpy(hostname, argv[1], 255);
    hostport = atoi(argv[2]);
    printf("use arguments >%s< as hostname and >%d< as hostport\n",hostname,hostport);
  }
  else
  {
    printf("Usage: cratemsgclienttest <hostname> <port>\n");
    exit(0);
  }

  tcp = new CrateMsgClient(hostname,hostport);
exit(0);

  if(tcp->IsValid())
  {
    printf("Connected\n");
  }
  else
  {
    printf("NOT CONNECTED - EXIT\n");
    exit(0);
  }

sleep(10);
exit(0);



  printf("111\n");fflush(stdout);
  ret = tcp->GetCrateMap(&buf, &len);
  printf("222: len=%d\n",len);fflush(stdout);
  for(ii=0; ii<len; ii++) {printf("slot %2d, boardID 0x%08x\n",ii,buf[ii]);fflush(stdout);}


  /*  
  partype = SCALER_PARTYPE_THRESHOLD;
  slot = 3;
  for(ii=0; ii<16; ii++)
  {
    buf[0] = 200;
    tcp->SetChannelParams(slot, ii, partype, buf, 1);
  }
  

  slot = 3;
  partype = SCALER_PARTYPE_THRESHOLD;
  ret = tcp->GetBoardParams(slot, partype, &buf, &len);
  for(ii=0; ii<len; ii++) {printf("ch[%2d] thres1=%d\n",ii,buf[ii]);fflush(stdout);}
  */


  /*
  partype = SCALER_PARTYPE_THRESHOLD2;
  ret = tcp->GetBoardParams(slot, partype, &buf, &len);
  for(ii=0; ii<len; ii++) {printf("ch[%2d] thres2=%d\n",ii,buf[ii]);fflush(stdout);}
  */

  /*
  chan = 5;
  ret = tcp->GetChannelParams(slot, chan, partype, &buf, &len);

  ret = tcp->GetChannelParams(slot, chan, partype, &buf, &len);
  */


  sleep(1);



  /*
  printf("DELAY ...\n");fflush(stdout);
  ret = tcp->Delay(1000);
  printf("  DELAY !\n");fflush(stdout);
  */




  printf("-------------------------\n");
  while(1)
  {
    printf("BEGIN LOOP\n");fflush(stdout);


    /* check if we still connected */
    if(tcp->IsValid())
    {
      printf("Still connected\n");fflush(stdout);
    }
    else
    {
      printf("Connection lost\n");fflush(stdout);
      /*exit(0);*/
	  /*
	  printf("sleeping ..\n");fflush(stdout);
      sleep(10);
	  printf(".. slept\n");fflush(stdout);
	  */
    }


    slot = 4;
    printf("Calling tcp->GetBoardParams()\n");fflush(stdout);
    partype = SCALER_PARTYPE_THRESHOLD;
    ret = tcp->GetBoardParams(slot, partype, &buf, &len);
    printf("cratemsgclienttest:GetBoardParams ret=%d, len=%d slot=%d\n",ret,len, slot);fflush(stdout);
    if(ret != kTRUE) continue; /* skip the rst, otherwise it will segfault on 'delete [] buf' since 'buf' was not allocated in ReadScalers() if it failed ! */

    for(ii=0; ii<len; ii++) {printf("ch[%2d] thres1=%d\n",ii,buf[ii]);fflush(stdout);}



	/*
    slot=3;
    ret = tcp->ReadScalers(slot, &buf, &len);
    printf("cratemsgclienttest: ret=%d, len=%d slot=%d\n",ret,len, slot);
    for(ii=0; ii<len; ii++) printf("  [%2d] %7d 0x%08x (swap 0x%08x)\n",ii,buf[ii],buf[ii],LSWAP(buf[ii]));fflush(stdout);
	*/
    slot=4;
    printf("Calling tcp->ReadScalers()\n");fflush(stdout);
    ret = tcp->ReadScalers(slot, &buf, &len);
    printf("cratemsgclienttest:ReadScalers ret=%d, len=%d slot=%d\n",ret,len, slot);fflush(stdout);
    if(ret != kTRUE) continue; /* skip the rst, otherwise it will segfault on 'delete [] buf' since 'buf' was not allocated in ReadScalers() if it failed ! */

    for(ii=0; ii<len; ii++) printf("  [%2d] %7d 0x%08x (swap 0x%08x)\n",ii,buf[ii],buf[ii],LSWAP(buf[ii]));fflush(stdout);
	/*
    slot=5;
    ret = tcp->ReadScalers(slot, &buf, &len);
    printf("cratemsgclienttest: ret=%d, len=%d slot=%d\n",ret,len, slot);
    for(ii=0; ii<len; ii++) printf("  [%2d] %7d 0x%08x (swap 0x%08x)\n",ii,buf[ii],buf[ii],LSWAP(buf[ii]));fflush(stdout);
	*/




//    ii = 35;
//    printf("lead glass sum: %10d \n",buf[ii]);fflush(stdout);
//    ii = 36;
//    printf("total sum:      %10d \n",buf[ii]);fflush(stdout);


	printf("buf=0x%08x, calling 'delete [] buf'\n",buf);fflush(stdout);
    delete [] buf;
    printf("buf=0x%08x deleted\n",buf);fflush(stdout);

#if 0
    ret = tcp->ReadData(slot, &buf, &len);
    printf("cratemsgclienttest: ret=%d, len=%d slot=%d\n",ret,len, slot);
    //for(ii=0; ii<len; ii++) printf("  [%2d] 0x%08x (swap 0x%08x)\n",ii,buf[ii],LSWAP(buf[ii]));fflush(stdout);

//    ii = 35;
//    printf("lead glass sum: %10d \n",buf[ii]);fflush(stdout);
//    ii = 36;
//    printf("total sum:      %10d \n",buf[ii]);fflush(stdout);

    delete [] buf;
#endif






	/*
    slot=4;
    ret = tcp->ReadScalers(slot, &buf, &len);
    printf("ret=%d, len=%d slot=%d\n",ret,len, slot);
    for(ii=0; ii<len; ii++) printf("  [%2d] 0x%08x (swap 0x%08x)\n",ii,buf[ii],LSWAP(buf[ii]));fflush(stdout);
    delete [] buf;
	*/

	/*
    slot=13;
    ret = tcp->ReadScalers(slot, &buf, &len);
    printf("ret=%d, len=%d slot=%d\n",ret,len, slot);
    for(ii=0; ii<len; ii++) printf("  [%2d] 0x%08x (swap 0x%08x)\n",ii,buf[ii],LSWAP(buf[ii]));fflush(stdout);
    delete [] buf;

    slot=14;
    ret = tcp->ReadScalers(slot, &buf, &len);
    printf("ret=%d, len=%d slot=%d\n",ret,len, slot);
    for(ii=0; ii<len; ii++) printf("  [%2d] 0x%08x (swap 0x%08x)\n",ii,buf[ii],LSWAP(buf[ii]));fflush(stdout);
    delete [] buf;
	*/

	/*
    slot=15;
    ret = tcp->ReadScalers(slot, &buf, &len);
    printf("ret=%d, len=%d slot=%d\n",ret,len, slot);
    for(ii=0; ii<len; ii++) printf("  [%2d] 0x%08x (swap 0x%08x)\n",ii,buf[ii],LSWAP(buf[ii]));fflush(stdout);
    delete [] buf;

    slot=16;
    ret = tcp->ReadScalers(slot, &buf, &len);
    printf("ret=%d, len=%d slot=%d\n",ret,len, slot);
    for(ii=0; ii<len; ii++) printf("  [%2d] 0x%08x (swap 0x%08x)\n",ii,buf[ii],LSWAP(buf[ii]));fflush(stdout);
    delete [] buf;
	*/
	/*
    slot=17;
    ret = tcp->ReadScalers(slot, &buf, &len);
    printf("ret=%d, len=%d slot=%d\n",ret,len, slot);
    for(ii=0; ii<len; ii++) printf("  [%2d] 0x%08x (swap 0x%08x)\n",ii,buf[ii],LSWAP(buf[ii]));fflush(stdout);
    delete [] buf;

    slot=18;
    ret = tcp->ReadScalers(slot, &buf, &len);
    printf("ret=%d, len=%d slot=%d\n",ret,len, slot);
    for(ii=0; ii<len; ii++) printf("  [%2d] 0x%08x (swap 0x%08x)\n",ii,buf[ii],LSWAP(buf[ii]));fflush(stdout);
    delete [] buf;
	*/
	/*
    slot=10;
    ret = tcp->ReadScalers(slot, &buf, &len);
    printf("ret=%d, len=%d slot=%d\n",ret,len, slot);
    for(ii=0; ii<len; ii++) printf("  [%2d] 0x%08x (swap 0x%08x)\n",ii,buf[ii],LSWAP(buf[ii]));fflush(stdout);
    delete [] buf;
	*/

    printf("\n\n");

    sleep(1);
  }

  exit(0);
}

#else

int
main()
{
  return(0);
}

#endif
