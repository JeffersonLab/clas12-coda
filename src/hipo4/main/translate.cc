//******************************************************************
//*  ██╗  ██╗██╗██████╗  ██████╗     ██╗  ██╗    ██████╗
//*  ██║  ██║██║██╔══██╗██╔═══██╗    ██║  ██║   ██╔═████╗
//*  ███████║██║██████╔╝██║   ██║    ███████║   ██║██╔██║
//*  ██╔══██║██║██╔═══╝ ██║   ██║    ╚════██║   ████╔╝██║
//*  ██║  ██║██║██║     ╚██████╔╝         ██║██╗╚██████╔╝
//*  ╚═╝  ╚═╝╚═╝╚═╝      ╚═════╝          ╚═╝╚═╝ ╚═════╝
//************************ Jefferson National Lab (2023) ***********
//******************************************************************
//* Example program for writing HIPO-4 Files..
//* Includes defining schemas, opening a file with dictionary
//*-----------------------------------------------------------------
//* Author: G.Gavalian
//* Date:   02/03/2023
//******************************************************************

#include <iostream>
#include <stdlib.h>
#include "table.h"
#include "utils.h"
#include "bank.h"
#include "decoder.h"
#include "reader.h"
#include "writer.h"
#include "evioBankUtil.h"
#include "detectors.h"

#define MAXBUF 10000000
#define ONE_MB 1048576
#define ONE_KB 1024

unsigned int buf[MAXBUF];

void nextEvent(uint32_t  *buffer) { /* read next event into buffer */};
int  bankLink(uint32_t *buffer, int tag, int leaf, int num, int *length){ /* return the position of the given bank */ return 0;}

void processFrame(const char *data);
void processDrift(const char *file);
void processEcal(const char *file);
void processComponent();
/**
*  Main Program ........
*/
int main(int argc, char** argv){

    printf("--->>> example program for data translation\n");
    char inputFile[256];

   if(argc>1) {
      sprintf(inputFile,"%s",argv[1]);
      //sprintf(outputFile,"%s",argv[2]);
   } else {
      std::cout << " *** please provide a file name..." << std::endl;
     exit(0);
   }
    
    //bank.show();
    //bank.print();
    processEcal(inputFile);
    //processComponent();
}
void print(int x){
    printf("%4d ",x);
}
void processComponent(){
    

    /*coda::component cc;
    cc.setName("generic");
    cc.addBank(11,1,"bbbb",240);
    cc.addBank(11,2,"bbbbss",280);
    cc.addBank(11,3,"bbbbssffff",120);
    cc.summary();
    cc.getBanks()[1].parse(34,6,"ssffll",32);

    cc.summary();*/

    coda::component_ec ec("ec_tt.txt","ec_fadc.txt");
    ec.init();
    coda::component_dc dc("dc_tt.txt");
    dc.init();
    ec.summary();
    dc.summary();


    /*coda::decoder decoder;
    decoder.initialize();
    decoder.showKeys();
    
    decoder.show();*/

    //coda::component *tt = new coda::component_dc("dc_tt.txt");;
    //std::vector<std::set<int>> order;
    //table.read("ec_tt.txt");
    //std::set<int>  keys = tt->keys();
    //order.push_back(keys);
    //for_each(keys.begin(),keys.end(),print);
    //printf("\n");
    //coda::component_ec ec("ec_tt.txt","ec_fadc.txt");
    //coda::component_dc dc("dc_tt.txt");
    //ec.summary();
    //dc.summary();
}

void processEcal(const char *file){

    hipo::reader    r;    
    hipo::writer    w;
    coda::decoder   decoder;
    hipo::event     e;
    hipo::event     out;
    hipo::structure evio(2*ONE_MB);

    coda::eviodata_t      evioptr;
    
    r.open(file);
    w.open("output.h5");

    decoder.initialize();
    decoder.showKeys();

    int fragnum = -1; /* always '-1' */
    int banktag = 0xe101; 
    int banktag2 = 0xe116;
    int banktag3 = 0xe107;
    int nbytes, ind_data;
    int ind12; int counter = 0;

    while(r.next()==true&&counter<50000){
        decoder.reset(); counter++;
        //printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>> event # %d\n",counter);
        r.read(e);
        e.getStructure(evio,32,1);
        const unsigned int *bufptr_c = reinterpret_cast<const unsigned int *>(&evio.getAddress()[8]);
        unsigned int         *bufptr = const_cast<unsigned int *>(bufptr_c);

        for(int fragtag=1; fragtag<=58; fragtag++){
            for(int banknum=0; banknum<40; banknum++) // loop over 40 bank numbers, it can be any from 0 to 39 
                {
                    ind12 = evLinkBank(bufptr, fragtag, fragnum, banktag, banknum, &nbytes, &ind_data);
                    if(ind12>0){
                        evioptr.crate = fragtag;
                        evioptr.tag   = banktag;
                        evioptr.offset = ind_data*4;
                        evioptr.length = nbytes;
                        evioptr.buffer = reinterpret_cast<const char*>(&bufptr[0]);
                        //printf("found the ecal bank, tag = %d\n", evioptr.tag);
                        decoder.decode(evioptr);
                        //decoder.decode_fadc250(fragtag,reinterpret_cast<const char*>(&bufptr[0]), ind_data*4, nbytes, fitter, bank);
                    }
                ind12 = evLinkBank(bufptr, fragtag, fragnum, banktag2, banknum, &nbytes, &ind_data);
                if(ind12>0){
                        evioptr.crate = fragtag;
                        evioptr.tag   = banktag2;
                        evioptr.offset = ind_data*4;
                        evioptr.length = nbytes;
                        evioptr.buffer = reinterpret_cast<const char*>(&bufptr[0]);
                        //printf("found the DC bank, crate = %5d , tag = %d\n", fragtag,evioptr.tag);
                        decoder.decode(evioptr);
                        //decoder.decode_fadc250(fragtag,reinterpret_cast<const char*>(&bufptr[0]), ind_data*4, nbytes, fitter, bank);
                    }
                
                ind12 = evLinkBank(bufptr, fragtag, fragnum, banktag3, banknum, &nbytes, &ind_data);
                if(ind12>0){
                        evioptr.crate = fragtag;
                        evioptr.tag   = banktag3;
                        evioptr.offset = ind_data*4;
                        evioptr.length = nbytes;
                        evioptr.buffer = reinterpret_cast<const char*>(&bufptr[0]);
                        //printf("found the EC tdc, crate = %5d , tag = %d\n", fragtag, evioptr.tag);
                        decoder.decode(evioptr);
                        //decoder.decode_fadc250(fragtag,reinterpret_cast<const char*>(&bufptr[0]), ind_data*4, nbytes, fitter, bank);
                }
                }

        }
        //decoder.show();
        
        //printf(">>>> EVENT SHOW\n");
        //out.show();
        
        decoder.write(out);
        //out.show();
        w.addEvent(out);
    }
    w.close();
    printf("processed events %d\n",counter);

}

void processDrift(const char *inputFile){

    coda::table table;
    table.read("dc_tt.txt");
    
    coda::decoder decoder;

    hipo::reader r;

    hipo::event  e(24*ONE_KB);
    hipo::event  eout(24*ONE_KB);

    hipo::structure evio(2*ONE_MB);

    r.open(inputFile);
    
    hipo::writer w;
    w.open("output.h5");


    hipo::dataframe    frame(50,4*ONE_MB); // frame(maximum number of events, maximum number of bytes); 

    int fragnum = -1; /* always '-1' */
    int banktag = 0xe116; 
    int nbytes, ind_data;
    int ind12; int counter = 0;

    hipo::composite bank;//(42,11,std::string("bssbsl"),1024*1024*2);
    bank.parse(42,11,std::string("bssbsl"),2*ONE_MB);
    //bank.parse(std::string("bssbsl"));

    while(r.next()&&counter<450000){

        counter++;
        r.read(e);
        e.getStructure(evio,32,1);        
        //int size = evio.getSize();                
        bank.reset();
        const unsigned int *bufptr_c = reinterpret_cast<const unsigned int *>(&evio.getAddress()[8]);
        unsigned int         *bufptr = const_cast<unsigned int *>(bufptr_c);
        //printf("evio buffer size = %d , in bytes = %d\n",bufptr[0],bufptr[0]*4);
        //unsigned int *bufptr = &bufptr_k;
        for(int fragtag=41; fragtag<=58; fragtag++)
        {
            for(int banknum=0; banknum<40; banknum++) // loop over 40 bank numbers, it can be any from 0 to 39 
                {
                    ind12 = evLinkBank(bufptr, fragtag, fragnum, banktag, banknum, &nbytes, &ind_data);
                    if(ind12>0){ 
		                //printf(" --- found it");
		                //printf("[%5d] %d %d %d %d\n",ind12, fragtag,banknum, nbytes, ind_data*4);
		                decoder.decode(fragtag,reinterpret_cast<const char*>(&bufptr[0]), ind_data*4, nbytes, table, bank);
                    }
                }

        }
	    //printf("event # %d : after decoding bank rows = %d\n" ,counter,bank.getRows() );
	    eout.reset();
        eout.addStructure(bank);
    
        bool success = frame.addEvent(eout);

        if(success==false){
            processFrame(frame.buffer());
            frame.reset();
            frame.addEvent(eout);
        } 
    }
    printf("processed events %d\n",counter);
}

//=========================================================================================
//** The process routine to initiate frame from (char *) buffer, then read events one by 
//** one and read the bank and prin on the screen
//=========================================================================================

void processFrame(const char *data){

    hipo::dataframe frame;
    hipo::event     event(2*ONE_MB);

    frame.init(data);
    int size = frame.size();
    printf("frame size = %d\n",size);

    frame.summary();
    int eventCount = frame.count();
    int pos = 56;
    hipo::composite bank(2*ONE_MB);
    for(int i = 0; i < eventCount; i++){
        pos = frame.getEventAt(pos,event);
        printf("\t event # %d -> size = %d, position = %d\n",i,event.getSize(), pos);
        event.getStructure(bank,42,11);
        bank.show();
        printf(" nrows = %d\n", bank.getRows() );
        bank.print();
    }
}
