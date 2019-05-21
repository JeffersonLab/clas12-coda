
/* c1190compensation.cc - process 1190/1290 data and prepare compensation table (by Andrey Kim)

usage:
      c1190compensation /work/ftof/tdcftof2_001547.evio.0

output:
      htdc.pdf - TDC INL histograms for each slot and channel
      compenstation table txt file per each slot
 */

#if defined(Linux) && !defined(Linux_armv7l) && !defined(Linux_nios2)

#include <stdio.h>
#include <stdlib.h>
#include <fstream>

#include<TStyle.h>
#include<TCanvas.h>
#include<TH1F.h>


#include"evioBankIndex.hxx"
#include"evioFileChannel.hxx"

#define MAXEVENTS 1000000

int
main(int argc, char** argv)
{
  int nchan = 32; /* set default to v1290/v1290N, will change below if have v1190 */
  TH1::AddDirectory(0);
  gStyle->SetOptStat(0);

  map<int, TH1F*> htdc;



  evio::evioFileChannel *evf = new evio::evioFileChannel(argv[1], "r");
  evf->open();


  /* loop over input file */

  int ien=0;
  while(evf->readNoCopy())
  {
	if(ien++>MAXEVENTS) break;
	const uint32_t *buffer = evf->getNoCopyBuffer();
	evio::evioBankIndex b0(buffer, 0);
	if(b0.tagNumExists(evio::tagNum(0xe107,0)))
    {
	  for(int ibank=0;ibank<b0.tagNumCount(evio::tagNum(0xe107,0));ibank++)
      {
		evio::bankIndex b = b0.getBankIndex(evio::tagNum(0xe107,0));

		int arrlen = b.dataLength;
		/*printf("arrlen=%d\n",arrlen);*/
        if(arrlen>500) nchan=128; /* use data size to figure out tdc type (v1190 or v1290) */

		void *datavoid = const_cast<void *> (b.data);
		int *datachar = reinterpret_cast<int *> (datavoid);

		for(int iarr=0;iarr<arrlen;iarr++)
        {
		  int slot = (datachar[iarr]>>27)&0x1F;
		  int chan = (datachar[iarr]>>19)&0x7F;
		  int tdc = ((datachar[iarr])&0x7FFFF)%1024;
		  /*std::cout<<"slot/channel/tdc= "<<slot<<" "<<chan<<" "<<tdc<<std::endl;*/

		  int id = slot*nchan+chan;
		  if(htdc[id]==0) htdc[id] = new TH1F("htdc", Form("slot %d chan %d",slot,chan), 1024,0,1024);
		  htdc[id]->Fill(tdc);

		}
	  }
	}
  }

  printf("\nProcessed %d events\n\n",ien);
  evf->close();



  /* */

  std::ofstream ofs;

  int slot = 0;
  int chan = -1;

  TCanvas* c1 = new TCanvas("c1","c", 1100,800);
  /* c1->Divide(1,2); */
  c1->Print("htdc.pdf[");
  printf("\n\nnchan=%d\n",nchan);
  for (std::map<int, TH1F*>::iterator it=htdc.begin(); it!=htdc.end(); ++it)
  {
    int islot = it->first/nchan;
    int ichan = it->first%nchan;
    printf("it->first=%d -> islot=%d ichan=%d\n",it->first,islot,ichan);

    /* for new slot, open new file */
    if(islot!=slot)
    {

	  /* if previous file contains less then 32 channels, fill it to 32 channels by default values (v1190N) */
      if(slot>0 && chan<31)
	  {
        printf("== slot=%d chan=%d\n",slot,chan);

        for(int channel=chan+1; channel<32; channel++)
        {
          ofs<<Form("# Channel %d",channel)<<endl;
          for(int irow=0;irow<128;irow++)
          {
            for(int icol=0;icol<8;icol++)
            {
              int ibin = irow*8+icol+1;
              char iy = 4*channel + (irow/32);
              ofs<<Form("0x%02hhx  ",iy);
            }
            ofs<<endl;
          }
		}

	  }

      slot = islot;
      ofs.close();
      ofs.open(Form("compensation_slot%02d.txt",slot), std::ofstream::out);
    }

    /* for every new channel, write channel number */
    chan = ichan;
    ofs<<Form("# Channel %d",ichan)<<endl;


    TH1F* hinl = (TH1F*) it->second->Clone("INL");
    for(int ik=1;ik<=hinl->GetNbinsX();ik++)
    {                                                                                                                 
      hinl->SetBinContent(ik, ik - 1024 * it->second->Integral(1,ik) / it->second->Integral() );
    }

    c1->cd(1)->SetGrid(1);
    hinl->Draw("hist");
	/*
    c1->cd(2)->SetGrid(1);
    it->second->Draw("hist");
    */
    c1->Print("htdc.pdf");

    for(int irow=0;irow<128;irow++)
    {
      for(int icol=0;icol<8;icol++)
      {
        int ibin = irow*8+icol+1;
        double yy = hinl->GetBinContent(ibin);
        char iy = round(yy);

        iy = -iy; /*sergey */

        /*printf("yy=%f iy=%d (0x%02x)\n",yy,iy,iy);*/

        ofs<<Form("0x%02hhx  ",iy);
      }
      ofs<<endl;
    }
    /* end of one channel processing */


  }
  ofs.close();
  c1->Print("htdc.pdf]");

  return 0;
}

#else

void
c1190compensation_dummy()
{
  return;
}

#endif
