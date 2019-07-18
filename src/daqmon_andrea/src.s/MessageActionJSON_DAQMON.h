#ifndef __MESSAGE_ACTION_JSON__
#define __MESSAGE_ACTION_JSON__

#include "MessageAction.h"

#include "hbook.h"
#include "json/json.hpp"
#include <string>

#include <unistd.h>
using json = nlohmann::json;

#ifdef USE_ROOT
#endif

class MessageActionJSON : public MessageAction {

  private:

    static const int NFORMATS=1;
    std::string formats[NFORMATS] = {"{"}; /*We need to recognize json as a message starting with a parenthesis.. ugly and dirty.*/
    int formatid;

    int error;
    int debug;
    int done;
    int status;
    int statistics;
    std::string message;
    std::string myname;
    int len;
    std::string str;


    int packed;
    Hist hist;
    
    char *hnameLocalSplit;
    char hnameLocal[20];
   
    
    /*Stuff for FT*/
    vector<double> FTPosDataCalo;
    vector<double> FTPosDataHodo1;
    vector<double> FTPosDataHodo2;
    vector<double> FTPosDataCaloHodo;
    int nADCFT1PosDataCalo;
    int nADCFT2PosDataCalo;
    int nADCFT1PosDataHodo1;
    int nADCFT2PosDataHodo1;
    int nADCFT1PosDataHodo2;
    int nADCFT2PosDataHodo2;
    int nADCFT1PosDataCaloHodo;
    int nADCFT2PosDataCaloHodo;

    TH1F *phist1;
    TH2F *phist2;


#ifdef USE_ROOT
	TList *histlist;
#endif

  public:

    MessageActionJSON(){}


#ifdef USE_ROOT
    MessageActionJSON(std::string myname_, int debug_ = 0, TList *histlist_ = NULL)
    {
      
      myname = myname_;
      done = 0;
      status = 0;
      statistics = 0;
      debug = debug_;
      
      

      histlist = histlist_;
      gethostname(hnameLocal,20);
      hnameLocalSplit = strtok(hnameLocal,".");
      
      nADCFT1PosDataCalo=0;
      nADCFT2PosDataCalo=0;
      nADCFT1PosDataHodo1=0;
      nADCFT2PosDataHodo1=0;
      nADCFT1PosDataHodo2=0;
      nADCFT2PosDataHodo2=0;
      nADCFT1PosDataCaloHodo=0;
      nADCFT2PosDataCaloHodo=0;

    }
#else
    MessageActionJSON(std::string myname_, int debug_ = 0)
    {
      myname = myname_;
      done = 0;
      status = 0;
      statistics = 0;
      debug = debug_;
      gethostname(hnameLocal,20);
      hNameLocalSplit = strtok(hnameLocal,".");
     
      nADCFT1PosDataCalo=0;
      nADCFT2PosDataCalo=0;
      nADCFT1PosDataHodo1=0;
      nADCFT2PosDataHodo1=0;
      nADCFT1PosDataHodo2=0;
      nADCFT2PosDataHodo2=0;
      nADCFT1PosDataCaloHodo=0;
      nADCFT2PosDataCaloHodo=0;

    }
#endif

    ~MessageActionJSON(){}

    /*A.C. this is the worst thing we could do. fmt comes from ipc_lib.h, where a first streamMessage->readString() was called (hence, the first string was supposed to be the FORMAT of the message. Then, in the decode, I was supposed to read the SECOND string, where the message body is. However, for a reason completely unknown (and nonsense) to me, K. Livinstone didn't want to use two strings for json messages. Hence, the fmt ALREADY contains the full message!*/

int check(std::string fmt){
 
  if (debug) printf("\n===== MessageActionJSON_DAQMON check: testing fmt >%s< <\n",fmt.c_str());

  
  for(int i=0; i<NFORMATS; i++){
    std::string f = formats[i];
    if( !strncmp(f.c_str(),fmt.c_str(),strlen(f.c_str())) ){
      formatid = i;
      if (debug) printf("MessageActionJSON_DAQMON found format: %s in string: %s \n",f.c_str(),fmt.c_str());
      
      message=fmt;

      return(1);
    }
  }
  formatid = 0;
  return(0);
}



void decode(IpcConsumer& recv){
    
  //recv >> str;
  str = message;
  if(debug) printf("\nMessageActionJSON_DAQMON received >%s<\n",str.c_str());fflush(stdout);
  


      json j3 = json::parse(str);

     
      // special iterator member functions for objects
      for (json::iterator it = j3.begin(); it != j3.end(); ++it) {
	cout<<it.key()<<" "<<it.value()<<endl;
	if (j3[it.key()].is_array()){
	  vector<double> data=j3[it.key()]; //use a double vector
	  std::string htitle=it.key();
	  htitle=string(hnameLocalSplit)+":"+htitle;
	  /*Special cases*/
	  /*For FT, since both are reporting, instead of having 2 separate histograms, one for adcft1, one for adcft2, create only one here*/
	  if (it.key()=="adcft1vtp_VTPFT_CLUSTERPOSITION"){
	    decodeFTPosHist(htitle,data,1,1,0);
	  }
	  else if (it.key()=="adcft2vtp_VTPFT_CLUSTERPOSITION"){
	    decodeFTPosHist(htitle,data,2,1,0);
	  }
	  else if (it.key()=="adcft1vtp_VTPFT_CLUSTERPOSITION_HODO"){
	    decodeFTPosHist(htitle,data,1,1,1);
	  }
	  else if (it.key()=="adcft2vtp_VTPFT_CLUSTERPOSITION_HODO"){
	    decodeFTPosHist(htitle,data,2,1,1);
	  }
	  /* else if (it.key()=="adcft1vtp_HODO1"){
	    decodeFTPosHist(htitle,data,1,0,1);
	  }
	  else if (it.key()=="adcft2vtp_HODO2"){
	    decodeFTPosHist(htitle,data,2,0,1);
	  }
	  else if (it.key()=="adcft1vtp_HODO2"){
	    decodeFTPosHist(htitle,data,1,0,2);
	  }
	  else if (it.key()=="adcft2vtp_HODO2"){
	    decodeFTPosHist(htitle,data,2,0,2);
	  }
	  */
	  else if ((it.key()=="adcft1vtp_VTPFT_CLUSTERENERGY")||(it.key()=="adcft1vtp_VTPFT_CLUSTERENERGY_HODO")||(it.key()=="adcft2vtp_VTPFT_CLUSTERENERGY")||(it.key()=="adcft2vtp_VTPFT_CLUSTERENERGY_HODO")){
	    decodeDefaultHist(htitle,data,16);
	  }
	  else{
	    decodeDefaultHist(htitle,data);
	  }
	}
      }

}







    void process()
    {
      // std::cout << "MessageActionJSON: process message" << std::endl;      


#ifdef USE_ROOT
      if(error==0) hist2root();
#endif

    }

    /*Special flag is to handle specific cases (as FT mapping)*/

void hist2root(int specialFlag=0)
{
  Int_t id, ibinx, ibiny;
  Float_t x, y;
  Float_t *content;
  char title[40];

  printf("hist2root reached, histlist=%p\n",histlist);

  //TObject *obj = NULL;
  if(histlist==NULL)
  {
    return;
  }
  else
  {
    auto obj = histlist->FindObject(hist.title);

  /* if histogram is NOT in histlist, create and put it in, otherwise get pointer to it */
  if(obj!=NULL)
  {
    printf("FOUND obj=%p contains name >%s<\n",obj,hist.title);fflush(stdout);
    if(hist.nbiny==0/*obj->InheritsFrom("TH1F")*/)
	{
      printf("hist2root: found 1D\n");fflush(stdout);
      phist1 = (TH1F *)obj;
      //phist2 = NULL;
	}
    else /*if(obj->InheritsFrom(TH2::Class()))*/
	{
      printf("hist2root: found 2D\n");fflush(stdout);
      //phist1 = NULL;
      phist2 = (TH2F *)obj;
	}
	/*
    else
	{
      printf("UNKNOWN CLASS\n");fflush(stdout);
	}
	*/
  }
  else
  {
    if(hist.nbiny==0)
	{
      printf("hist2root: creating 1D\n");
      phist1 = new TH1F(hist.title, hist.title, hist.nbinx, hist.xmin, hist.xmax);
      histlist->Add(phist1);
	}
    else
	{
      printf("hist2root: creating 2D\n");
      phist2 = new TH2F(hist.title, hist.title, hist.nbinx, hist.xmin, hist.xmax, hist.nbiny, hist.ymin, hist.ymax);
      histlist->Add(phist2);
	}
  } 


  id = hist.id;
  if(hist.nbinx>0)
  {
    printf("hist2root: [id=%d] nbinx=%d nbiny=%d\n",id,hist.nbinx,hist.nbiny);

    if(hist.nbiny == 0) /* 1-dim */
    {
	  
      //phist1->SetEntries(hist.entries+phist1->GetEntries());
      phist1->SetEntries(hist.entries); //these are scalers, hence don't sum with previous data.
      content = phist1->GetArray();
      content[0] = (Float_t)hist.xunderflow;
      content[hist.nbinx+1] = (Float_t)hist.xoverflow;
      for(ibinx=0; ibinx<hist.nbinx; ibinx++)
	  {
        content[ibinx+1] = (Float_t)hist.buf[ibinx];
        //printf("content[%d]=%f\n",ibinx,content[ibinx+1]);
	  }
	  
	  /*
      for(ibinx=0; ibinx<hist.nbinx; ibinx++)
	  {
        printf("content[%d]=%f\n",ibinx,(float)hist.buf[ibinx]);
        phist1->Fill((float)ibinx, (float)hist.buf[ibinx]);
	  }
	  */

    }
    else /* 2-dim */
    {
	  /*
      phist2->SetEntries(hist.entries+phist2->GetEntries());
      content = phist2->GetArray();
      content[0] += (float)hist.xunderflow;
      content[hist.nbinx+1] += (float)hist.xoverflow;
      for(ibinx=0; ibinx<hist.nbinx; ibinx++)
	  {
        content[ibinx+1] += (float)hist.buf[ibinx];
        printf("content[%d]=%f\n",ibinx,content[ibinx+1]);
	  }
	  */


      printf("hist2root: 2-dim hist !!!\n"); fflush(stdout);
      phist2->Reset(); //these are scalers, hence reset
	  for(ibinx=0; ibinx<hist.nbinx; ibinx++)
      {
        x = hist.xmin + hist.dx*(Float_t)ibinx;
        for(ibiny=0; ibiny<hist.nbiny; ibiny++)
        {
          y = hist.ymin + hist.dy*(Float_t)ibiny;

          //printf("hist2root: 2DIM[%f][%f] => %f\n",x,y,(Float_t)hist.buf2[ibinx][ibiny]); fflush(stdout);
          phist2->Fill(x,y,(Float_t)hist.buf2[ibinx][ibiny]);
	    }
	  }
	  

    }
  }
  } /*netlist==NULL*/
}


 void decodeDefaultHist(std::string title,vector <double> &data,double bin_size=1){
  if (debug) printf("decodeDefaultHist \n");
  hist.ntitle=title.length();
  hist.title=strdup(title.c_str());
  hist.nbiny=0;
  hist.nbinx=data.size();
  hist.xmin=-0;5;
  hist.xmax=(data.size()-0.5)*bin_size;
  hist.dx=1;
  
  hist.buf = (float *) calloc(hist.nbinx,sizeof(float));
  if(hist.buf==NULL){
    printf("ERROR in 1DIM calloc()\n");
    error = 1;
    return;
  }
  
  for(int ibinx=0; ibinx<hist.nbinx; ibinx++) {
      hist.buf[ibinx]=data[ibinx];
  }
  error=0; 
}


 void decodeFTPosHist(std::string title,vector<double> &data, int nVTP,int isCALO,int isHODO){
   const int expectedSize=1024;
   int i=0;
   int x,y;
   vector<double> *saveData;
   int *nADCFT1,*nADCFT2;

   string shost;
   string shname;


   if (data.size()!=expectedSize){
     printf("error in decodeFTPosHist, expected: %i got: %i\n",expectedSize,data.size());
     error=1;
     return;
   }
   
   if (debug) printf("decodeFTPosHist \n");
  

   

   /*Check which data and set pointers*/
   if ((isHODO==0)&&(isCALO==1)){
     saveData=&FTPosDataCalo;
     nADCFT1=&nADCFT1PosDataCalo;
     nADCFT2=&nADCFT2PosDataCalo;
   }
   else if ((isHODO!=0)&&(isCALO==1)){
     saveData=&FTPosDataCaloHodo;
     nADCFT1=&nADCFT1PosDataCaloHodo;
     nADCFT2=&nADCFT2PosDataCaloHodo;
   }
   else if ((isHODO==1)&&(isCALO==0)){
     saveData=&FTPosDataHodo1;
     nADCFT1=&nADCFT1PosDataHodo1;
     nADCFT2=&nADCFT2PosDataHodo1;
   }
   else if ((isHODO==2)&&(isCALO==0)){
     saveData=&FTPosDataHodo2;
     nADCFT1=&nADCFT1PosDataHodo2;
     nADCFT2=&nADCFT2PosDataHodo2;
   }
   printf("--- >  %i %i < ---- \n",(*nADCFT1),(*nADCFT2));fflush(stdout);

   /*If the histogram was sent, then clear data*/
   if  (((*nADCFT1)==0) && ((*nADCFT2)==0) ){
     saveData->resize(data.size(),0.);
     saveData->clear();
   }
   



   /*Increment data count for this message*/
   if (nVTP==1){
     (*nADCFT1)++;
   }
   else{
     (*nADCFT2)++;
   }
   


   /*Now save the data - note that we can loop on full data array, since this will be just half full*/
   /*In particular, 
     adcft1 is reporting x values from 11 to 21, y from 0 to 21 
     adcft2 is reporting x values from 0 to 10, y from 0 to 21
     But the array is still reported with full data!
   */
   
   for (int ii=0;ii<data.size();ii++){
     (*saveData)[ii]+=data[ii]; //+= is to add to the 0-values of half ft-cal the others.
   }
  
  



   /*Now check: if we have data for both adcft1 and adcft2, produce the histogram, otherwise simply return (and put err=1 to avoid this being reported)*/   
   if ( ((*nADCFT1)==0) || ((*nADCFT2)==0) ){
     error=1;
     return;
   }
   
   /*If we're here, it means we should report this histogram*/
   /*Reformat the title*/
   /*it is             --> hostname:adcftN_thetitle
     and should become --> hostname:thetitle*/
  
   shost=(title.substr(0,title.find(':')));
   shname=(title.substr(title.find('_')+1));
   title=shost+":"+shname;
   
 
   /*Now fill the histogram*/
   
   hist.ntitle=title.length();
   hist.title=strdup(title.c_str());
    
   hist.nbinx=22;
   hist.xmin=-0.5;
   hist.xmax=21.5;
   hist.xunderflow=0;
   hist.xoverflow=0;
   hist.dx=1;

   hist.nbiny=22;
   hist.ymin=-0.5;
   hist.ymax=21.5;
   hist.yunderflow=0;
   hist.yoverflow=0;
   hist.dy=1;

   hist.buf2 = (float **) calloc(hist.nbinx,sizeof(float*));
   if(hist.buf2==NULL){
     printf("ERROR1 in 2DIM calloc()\n");fflush(stdout);
     error = 1;
    return;
   }

   for(i=0; i<hist.nbinx; i++){
      hist.buf2[i] = (float *) calloc(hist.nbiny,sizeof(float));
      if(hist.buf2[i]==NULL){
	printf("ERROR2 in 2DIM calloc()\n");fflush(stdout);
	error = 1;
	return;
      }
  }
   
 



  i=0;
  for(int ibiny=0; ibiny<32; ibiny++){ //We report clusters as a 5-bit value, to count from 0 to 21, first all X for given Y, then next Y... 
    for(int ibinx=0; ibinx<32; ibinx++){
      if ((ibinx<hist.nbinx)&&(ibiny<hist.nbiny)){
	y=ibiny;
	x=hist.nbinx-ibinx-1;
	/*fix normalization*/
	if (ibinx<=10){ //vtp2
	  hist.buf2[x][y]=(*saveData)[i]/(*nADCFT2); //implement reverse axis
	}
	else{ //vtp1
	  hist.buf2[x][y]=(*saveData)[i]/(*nADCFT1); //implement reverse axis
	}
      }
      i++;
    }
  }
 

  /*Just make sure error is zero and reset the counters AND the data*/
  error=0;
  (*nADCFT1)=0;
  (*nADCFT2)=0;
  saveData->clear();

    i=0;
    /*  printf("at the end %s",title.c_str());
    for(int ibiny=0; ibiny<32; ibiny++){ //We report clusters as a 5-bit value, to count from 0 to 21, first all X for given Y, then next Y... 
      for(int ibinx=0; ibinx<32; ibinx++){
	if ((ibinx<hist.nbinx)&&(ibiny<hist.nbiny)){
	  y=ibiny;
	  x=hist.nbinx-ibinx-1;
	  printf("%f - %f ;",hist.buf2[x][y],(*saveData)[i]);
	}
	i++;
      }
      }*/
 }

};

#endif

