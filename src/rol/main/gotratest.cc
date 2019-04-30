
/* cratemsgclienttest.cc - testbed for cratemsgclient class
      usage example:      ./Linux_i686/bin/cratemsgclienttest hps11 6102
*/

#if 1 /*defined(Linux_vme)*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

#include "cratemsgclient.h"

using namespace std;

CrateMsgClient *tcp;

#ifndef LSWAP
#define LSWAP(x)        ((((x) & 0x000000ff) << 24) | \
                         (((x) & 0x0000ff00) <<  8) | \
                         (((x) & 0x00ff0000) >>  8) | \
                         (((x) & 0xff000000) >> 24))
#endif


unsigned int *buf, *bufn;

int main(int argc, char *argv[])
{
  char hostname[256];
  int hostport, len, ii, ret, slot, chan, partype, lenn, retn;
  unsigned int buffer[100];
  vector<int> vslots;
//  strncpy(hostname,"svt2",255);
  hostport = 6102;

    fstream file;
    file.open("gotratest.txt", ios_base::out);
//  string hosts[] = {"svt2", "svt5", "svt7"};
//  const int NHOSTS = sizeof(hosts)/sizeof(hosts[0]);
const int NHOSTS = 3;
/*
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
*/
  for(int ih=0; ih < NHOSTS; ++ih) {
//  char *cstr = new char[hosts[ih].length() + 1];
//  strcpy(cstr, hosts[ih].c_str());
//  hostname = cstr;
//  strncpy(hostname,cstr,255);
//  delete [] cstr;
//  hostname = hosts[ih];
  if(ih==0) strncpy(hostname,"svt1",255);
  else if(ih==1) strncpy(hostname,"svt2",255);
  else if(ih==2) strncpy(hostname,"svt3",255);
  tcp = new CrateMsgClient(hostname,hostport);
  if(tcp->IsValid())
  {
    printf("Connected\n");
  }
  else
  {
    printf("NOT CONNECTED - EXIT\n");
    exit(0);
  }

  printf("111\n");fflush(stdout);
  ret = tcp->GetCrateMap(&buf, &len);
  printf("222: len=%d\n",len);fflush(stdout);
  int k = 0;
  for(ii=0; ii<len; ii++) { 
    if(buf[ii] != 0xffffffff) {
//       printf("slot %2d, boardID 0x%08x\n",ii,buf[ii]);
       vslots.push_back(ii);
       retn = tcp->ReadScalers(ii, &bufn, &lenn);
       for(int jj=0; jj<lenn; jj++) file << hostname << " " << ii << " " << jj << " " << bufn[jj] << endl;
       delete [] bufn;
    }

//    fflush(stdout);

  }

//  for(ii=0; ii<vslots.size(); ii++) {
//    printf("%d\n",vslots[ii]);
//  }
//  delete tcp;
  }

//  for(ii=0; ii<vslots.size(); ii++) {
//    printf("%d\n",vslots[ii]);
//    file << ii << " " << vslots[ii] << endl;
//  }

//  file.close();
  printf("%d vscm boards found\n", vslots.size());
//  exit(0);

//  slot=3;
//  ret = tcp->ReadScalers(slot, &buf, &len);
//  printf("cratemsgclienttest: ret=%d, len=%d slot=%d\n",ret,len, slot);
//  for(ii=0; ii<len; ii++) file << "svt 7 slot 3 " << ii << " " << buf[ii] << endl;
//  delete [] buf;

  file.close();
  exit(0);
  /*???
  partype = SCALER_PARTYPE_THRESHOLD;
  for(ii=0; ii<16; ii++)
  {
    buf[0] = 500;
    tcp->SetChannelParams(slot, ii, partype, buf, 1);
  }
  */

  slot = 3;
  partype = SCALER_PARTYPE_THRESHOLD;
  ret = tcp->GetBoardParams(slot, partype, &buf, &len);
  for(ii=0; ii<len; ii++) {printf("ch[%2d] thres1=%d\n",ii,buf[ii]);fflush(stdout);}

  partype = SCALER_PARTYPE_THRESHOLD2;
  ret = tcp->GetBoardParams(slot, partype, &buf, &len);
  for(ii=0; ii<len; ii++) {printf("ch[%2d] thres2=%d\n",ii,buf[ii]);fflush(stdout);}
  

  /*
  chan = 5;
  ret = tcp->GetChannelParams(slot, chan, partype, &buf, &len);

  ret = tcp->GetChannelParams(slot, chan, partype, &buf, &len);
  */


  sleep(3);


  printf("-------------------------\n");
  while(1)
  {
	
    slot=3;
    ret = tcp->ReadScalers(slot, &buf, &len);
    printf("cratemsgclienttest: ret=%d, len=%d slot=%d\n",ret,len, slot);
    for(ii=0; ii<len; ii++) printf("  cratemsgclienttest: [%2d] %10d 0x%08x (swap 0x%08x)\n",ii,buf[ii],buf[ii],LSWAP(buf[ii]));fflush(stdout);
    delete [] buf;

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
