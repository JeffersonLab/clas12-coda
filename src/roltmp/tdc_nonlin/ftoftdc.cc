#include <stdio.h>
#include <stdlib.h>
#include <fstream>

#include<TStyle.h>
#include<TCanvas.h>
#include<TH1F.h>

#include"evioBankIndex.hxx"
#include"evioFileChannel.hxx"

int main(int argc, char** argv)
{
 TH1::AddDirectory(0);
 gStyle->SetOptStat(0);

 map<int, TH1F*> htdc;

 evio::evioFileChannel *evf = new evio::evioFileChannel(argv[1], "r");
 evf->open();

 int ien=0;
 while(evf->readNoCopy()){
	if(ien++>5000) break;
	const uint32_t *buffer = evf->getNoCopyBuffer();
	evio::evioBankIndex b0(buffer, 0);
	if(b0.tagNumExists(evio::tagNum(0xe107,0))){
		for(int ibank=0;ibank<b0.tagNumCount(evio::tagNum(0xe107,0));ibank++){
			evio::bankIndex b = b0.getBankIndex(evio::tagNum(0xe107,0));
			int arrlen = b.dataLength;
			void *datavoid = const_cast<void *> (b.data);
			int *datachar = reinterpret_cast<int *> (datavoid);

			for(int iarr=0;iarr<arrlen;iarr++){
				int slot = (datachar[iarr]>>27)&0x1F;
				int chan = (datachar[iarr]>>19)&0x7F;
				int tdc = ((datachar[iarr])&0x7FFFF)%1024;
//				std::cout<<slot<<" "<<chan<<" "<<tdc<<std::endl;

				if(slot!=16){
					int id = slot*32+chan;
					if(htdc[id]==0)
						htdc[id] = new TH1F("htdc", Form("slot %d chan %d",slot,chan), 1024,0,1024);
					htdc[id]->Fill(tdc);
				}
			}
		}
	}
 }

 evf->close();



///////////////////////////////////////////////////////////////////////////////////////
//
 std::ofstream ofs;

 int slot = 0;

 TCanvas* c1 = new TCanvas("c1","c", 1100,800);
// c1->Divide(1,2);
 c1->Print("htdc.pdf[");
 for (std::map<int, TH1F*>::iterator it=htdc.begin(); it!=htdc.end(); ++it){
     int islot = it->first/32;
     int ichan = it->first%32;


     if(islot!=slot){
          slot = islot;
          ofs.close();
          ofs.open(Form("compensation_slot%d.txt",slot), std::ofstream::out);
     }
     ofs<<Form("#Channel %d",ichan)<<endl;


     TH1F* hinl = (TH1F*) it->second->Clone("INL");
     for(int ik=1;ik<=hinl->GetNbinsX();ik++){                                                                                                                 
          hinl->SetBinContent(ik, ik - 1024*it->second->Integral(1,ik)/it->second->Integral());
     }

     c1->cd(1)->SetGrid(1);
     hinl->Draw("hist");
//     c1->cd(2)->SetGrid(1);
//     it->second->Draw("hist");
     c1->Print("htdc.pdf");
     for(int irow=0;irow<128;irow++){
          for(int icol=0;icol<8;icol++){
               int ibin = irow*8+icol+1;
               double yy = hinl->GetBinContent(ibin);
               char iy = round(yy);
               ofs<<Form("0x%02hhx ",iy);
          }
          ofs<<endl;
     }
 }
 ofs.close();
 c1->Print("htdc.pdf]");

 return 0;
}
