
/* hist_lib.cc */

#include <sys/time.h>

#include "root_inc.h"
#include "daqMainFrame.h"
#include "daqmon.h"
#include "hist_lib.h"

extern int button_EXIT,button_PAUSE;
extern int CurrentTAB;
extern vector<daqTab*> tabs;

/*=====================================================================*/
/*                                                                     */
/*=====================================================================*/

int
STREQ(const char*s1, const char*s2)
{
  if (strncasecmp(s1,s2,strlen(s2))) return 0;
  else return 1;
}


/*=====================================================================*/
/*                                                                     */
/*=====================================================================*/
void *
hist_find(char *name, TList* histptr)
{
  int id,nf=0; 
  TObject *obj, *hist;
  printf("hist_find()::  jist name=%s\n",name);
  TIter next1(histptr);
  while ((obj = next1()))
  {
    obj->Print();
    //--printf("hist_find():: search 1D nf=%d  name=:%s:  :%s:\n", nf,name,obj->GetName());
    if (STREQ(obj->GetName(),name)) { hist=obj; nf++; }
  }   
  if (nf==1) return (void*)hist;
  return NULL;
}

/*=====================================================================*/
/*                                                                     */
/*=====================================================================*/
daqPad *
pad_find(const char *name, daqTab* tab)
{
  int id,nf=0; 
  daqPad *pad;
  printf("pad_find():: search hname=%s\n",name);
  for ( int ipad=0; ipad<tab->pads.size(); ipad++ )
  {
    pad=tab->pads[ipad];
    printf("pad_find():: pad=%d search hist>%s<  current name>%s< \n",ipad,name,pad->Hname);
    if (STREQ(pad->Hname,name))
    { 
      nf++;
      printf("pad_find(): FOUND!! ipad=%d name=%s \n",ipad,name);
      break; 
    }
  } 
  
  if (nf==1) return pad;
  return NULL;
}

/*=====================================================================*/
/*                                                                     */
/*=====================================================================*/
daqPad *
pad_find_id(int id, daqTab* tab)
{
  int nf=0; 
  daqPad *pad;
  //printf("pad_find():: search hname=%s\n",name);
  for ( int ipad=0; ipad<tab->pads.size(); ipad++ )
  {
    pad=tab->pads[ipad];
    //printf("pad_find():: pad=%d search hist=%s  current name=:%s: \n",ipad, name,pad->hname);
    if (pad->Pad==id)
    { 
      nf++; //-- printf("pad_find_id(): FOUND!! ipad=%d id=%d, name=%s \n",ipad,id,pad->Hname);
      break; 
    }
  } 
  
  if (nf==1) return pad;
  return NULL;
}





/*=====================================================================*/
/*                                                                     */
/*=====================================================================*/
void *
hist_client2(void *arg)
{
  int Debug=10;
  
  SRV_PARAM *s_param = (SRV_PARAM*) arg;
  char* rem_host =   s_param->REM_HOST;
  TCanvas  *canvas = s_param->canvas;
  int ipad=s_param->pad;
  int myTAB=s_param->tab;
  char* myName=s_param->thread_name;
  TH1 **hruninfo=&s_param->RunInfo;
  TSocket *sock = new TSocket(rem_host,32765);

  printf(" thread:: hist_client2() connected to %s\n",rem_host);
  
  // Wait till we get the start message
  char str[256];
  sock->Recv(str, 256);
  int idx = 0;

printf("hist_client: 000\n");fflush(stdout);
printf("hist_client: str >%s<\n",str);fflush(stdout);

  // server tells us who we are
  if (STREQ(str,"go"))
  {
    idx = !strcmp(str, "go 1") ? 1 : 0;
    printf("Server:: %s \n",str);
  }
  else
  {
    printf("Server:: %s \n",str);
    return NULL;
  }
  Float_t messlen  = 0;
  Float_t cmesslen = 0;
  if (idx == 1) sock->SetCompressionLevel(1);

printf("hist_client: 11\n");fflush(stdout);
  
  int i=0;
  const int LNhist=256;
  char histname[LNhist];
  
  i++;  
  char sss[256];
  sprintf(sss,"Update = %d\n",i);
  sock->Send(sss);          // tell server we are finished
  printf("SEND >%s<\n",sss);
  int REQ_SENT=1;

printf("hist_client: 22\n");fflush(stdout);

  TMessage *mess2;
  while (button_EXIT==0)  //--  objects loop  --
  {
    if (Debug>1) printf("hist_client2(%s):: wait next  message... \n",rem_host);
    sock->Recv(mess2); 
    if (!mess2)
    {
      printf("%s: error mess2=%p , resend \n",rem_host,mess2);
      sock->Send("Update = 2\n");
      usleep(100000);
      continue;
    }

printf("hist_client2: sender host %s: RECV mess2=%p\n",rem_host,mess2);

    if (mess2->What() == kMESS_STRING)
    {
      mess2->ReadString(str, 256);
      if (Debug>0) printf("STRING: hist_client2(%s):: Client cmd: %s\n",rem_host, str);
      if ( STREQ(str,"END"))
      {
        if (--REQ_SENT) continue; //-- wait for all histograms 
	    sleep(tabs[CurrentTAB]->update_time); // break;

	    while(button_EXIT==0 && button_PAUSE==1)
        {
   	      sleep(1); // break;
	    }

        while(true)
        { 
          gStyle->SetTitleSize(0.03+0.01*sqrt((double)tabs[CurrentTAB]->pads.size()),"t");    //-- TITLE size as "fraction of the pad" ---
          for ( int ipad=0; ipad < tabs[CurrentTAB]->pads.size(); ipad++ )
          {
            char *hname=tabs[CurrentTAB]->pads[ipad]->Hname;
            if (Debug>2) printf(" Pad=%d Hname=%s \n",ipad,hname);          
            char *hh, *substr1, *substr2, hhost[256];
            substr1=hname;
            if ((substr2=strstr(hname,":")))
            {
              int Lhost=(substr2-substr1);
              strncpy(hhost,substr1,Lhost); hhost[Lhost]=0;
              hh=&substr2[1];
              if (Debug>2) printf("found /:/ host=|%s|, Lhost=%d  rem_host=%s  hist=%s\n",hhost,Lhost,rem_host,hh);
              if (strncmp(hhost,rem_host,Lhost)) continue;
              if (Debug>0) printf("===> MyHOST!  host=|%s|, Lhost=%d  rem_host=%s  hist=%s\n",hhost,Lhost,rem_host,hh);
              sprintf(sss,"Hist:%s",hname);
              sock->Send(sss);  // req. update
              REQ_SENT++;
            }
            sprintf(sss,"Hist:%s:RunInfo",rem_host);  //--  always update RunInfo,
            sock->Send(sss);  // req. update
            REQ_SENT++;
            if (Debug>0) printf("Sent  host=|%s|, str=%s REQ=%d\n",hhost,sss,REQ_SENT);
          }
          if (REQ_SENT)
          {
            usleep(100);
            break;
          }
          else sleep(1);
          
          sprintf(sss,"Hist:%s:RunInfo",rem_host);  //--  always update RunInfo, every 1 sec.
          sock->Send(sss);  // req. update
          REQ_SENT++;
          if (Debug>0) 
            printf("Sent SLOW  host=|%s|, str=%s REQ=%d\n",rem_host,sss,REQ_SENT);
        }
      }
    }
    else if (mess2->What() == kMESS_OBJECT)
    {
      if (Debug>0) printf("OBJECT: hist_client2(%s):: got object of class:%s:\n",rem_host, mess2->GetClass()->GetName());


      if (STREQ(mess2->GetClass()->GetName(),"TH1"))          //------ 1D hist -----
      {
	    TH1 *h = (TH1 *)mess2->ReadObject(mess2->GetClass());
	    daqPad* pad = pad_find(h->GetName(),tabs[CurrentTAB]);

printf("1DIM HIST ==================================================1\n");fflush(stdout);

	    if (pad)
        {
printf("1DIM HIST ==================================================2\n");fflush(stdout);
	      pad->hist1=h; gettimeofday(&(pad->Time), NULL);
TThread::Lock();
	      tabs[CurrentTAB]->canvas->cd(pad->Pad);
	      h->SetStats(pad->Stat);  // -no statistic
 	      gStyle->SetOptStat(pad->Hopt);
	      if (pad->LogY>0) gPad->SetLogy();
	      if (pad->Marker>0) h->SetMarkerStyle(pad->Marker);
	      if (pad->MSize>0)  h->SetMarkerSize(pad->MSize);
	      if (pad->MColor>0) h->SetMarkerColor(pad->MColor);
	      if (pad->TSize>0)
          {
            h->SetTitleSize(pad->TSize,"t"); 
            h->GetXaxis()->SetLabelSize(0.06); 
            h->GetYaxis()->SetLabelSize(0.06); 
          }
	      h->DrawCopy(pad->Dopt);
	      gPad->Update();

TThread::UnLock();

	      tabs[CurrentTAB]->modified++;
	    }

        //------------------------------ get hruninfo -------------------------------
        strncpy(histname,h->GetName(),LNhist);
        sprintf(sss,"%s:RunInfo",rem_host); 
        if (!strncmp(sss,histname,strlen(sss)))
        {
TThread::Lock();
          if (*hruninfo) delete *hruninfo;
          *hruninfo=h; //s_param->RunInfo=h;
TThread::UnLock();
          h=NULL;
        }
        else
        {
          delete h;       // delete histogram
        }
        //---------------------------------------------------------------------------
      }

      else if (STREQ(mess2->GetClass()->GetName(),"TH2"))
      {  //------ 2D hist -----
	    TH2F *h = (TH2F *)mess2->ReadObject(mess2->GetClass());
	    daqPad* pad = pad_find(h->GetName(),tabs[CurrentTAB]);
	    if (pad)
        {
TThread::Lock();
	      tabs[CurrentTAB]->canvas->cd(pad->Pad);
	      h->SetStats(pad->Stat);  // -no statistic
 	      gStyle->SetOptStat(pad->Hopt);
	      if (pad->LogZ>0) gPad->SetLogz();
	      if (pad->ZSize>0) h->GetZaxis()->SetLabelSize(pad->ZSize);
	      if (pad->TSize>0)
          {
            h->SetTitleSize(pad->TSize,"t");
            h->GetXaxis()->SetLabelSize(0.06); 
            h->GetYaxis()->SetLabelSize(0.06); 
          }

	      h->DrawCopy(pad->Dopt);
	      gPad->Update();
	      tabs[CurrentTAB]->modified=1;
	      TObject* X3 = gPad->GetPrimitive(h->GetName());
	      pad->hist2=(TH2*) X3;
	      gettimeofday(&(pad->Time), NULL); 
TThread::UnLock();
	    }
	    delete h;       // delete histogram         
      }

      else if (STREQ(mess2->GetClass()->GetName(),"TProfile"))
      {  //------ Profile hist -----
	    TProfile *h = (TProfile *)mess2->ReadObject(mess2->GetClass());
	    daqPad* pad = pad_find(h->GetName(),tabs[CurrentTAB]);
	    if (pad)
        {
	      pad->prof=h; gettimeofday(&(pad->Time), NULL);
TThread::Lock();
	      tabs[CurrentTAB]->canvas->cd(pad->Pad);
	      h->SetStats(pad->Stat);  // -no statistic
	      if (pad->LogZ>0) gPad->SetLogz();
	      if (pad->Marker>0) h->SetMarkerStyle(pad->Marker);
	      if (pad->MSize>0)  h->SetMarkerSize(pad->MSize);
	      if (pad->MColor>0) h->SetMarkerColor(pad->MColor);	  
	      if (pad->TSize>0)
          {
            h->SetTitleSize(pad->TSize,"t");
            h->GetXaxis()->SetLabelSize(0.06); 
            h->GetYaxis()->SetLabelSize(0.06); 
          }
	      h->DrawCopy(pad->Dopt);
	      gPad->Update();
TThread::UnLock();
	      tabs[CurrentTAB]->modified=1;
	    }
	    delete h;       // delete histogram         
      }

    } //--- if Object 
    delete mess2;
  } //--  recv hist loop
  
  sock->Send("Finished");          // tell server we are finished
  
  if (cmesslen > 0) printf("Average compression ratio: %g\n", messlen/cmesslen);
  sock->Close();
  return NULL;
}
