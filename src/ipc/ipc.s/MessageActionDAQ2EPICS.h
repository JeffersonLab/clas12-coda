#ifndef __MESSAGE_ACTION_DAQ2EPICS__
#define __MESSAGE_ACTION_DAQ2EPICS__

#include "MessageAction.h"

#define MAX_ELEM 16384 /* max epics array in bytes */

class MessageActionDAQ2EPICS : public MessageAction {

  private:

    static const int NFORMATS=1;
    std::string formats[NFORMATS] = {"epics"};
    int formatid;

    int status, ii;
    std::string sender, host, user;
    int32_t time;
    std::string caname, catype;
    int32_t nelem;
    char d_char[MAX_ELEM];
    uint8_t d_uchar[MAX_ELEM];
    int16_t  d_short[MAX_ELEM/2];
    int16_t  d_ushort[MAX_ELEM/2];
    int32_t  d_int[MAX_ELEM/4];
    int32_t d_uint[MAX_ELEM/4];
    float   d_float[MAX_ELEM/4];
    double  d_double[MAX_ELEM/8];
    std::string d_string[MAX_ELEM];

    int debug = 0;



  public:

    MessageActionDAQ2EPICS()
    {
    }

    MessageActionDAQ2EPICS(int debug_)
    {
      debug = debug_;
    }


    ~MessageActionDAQ2EPICS(){}

    int check(std::string fmt)
    {
	  printf("checkDAQ2EPICS: fmt >%s<\n",fmt.c_str());
      for(int i=0; i<NFORMATS; i++)
	  {
        std::string f = formats[i];
        if( !strncmp(f.c_str(),fmt.c_str(),strlen(f.c_str())) )
		{
          formatid = i;
          return(1);
		}
	  }

      formatid = 0;
      return(0);
    }

    void decodeMessage(IpcConsumer& msg)
    {
      if(debug) std::cout << "MessageActionDAQ2EPICS: decode EPICS message" << std::endl;      


	  //if(debug) printf("\n\nepics_msg_callback reached\n");
	  //if(debug) printf("numfields=%d\n",msg.NumFields());
	  //if(debug) printf("destination >%s<\n",msg.Dest());
	  //if(debug) printf("sender >%s<\n",msg.Sender());

	  //printf("Message:\n");

      msg >> sender >> host >> user >> time;
      if(debug) printf("  Sender >%s<\n",sender.c_str());
      if(debug) printf("  Host >%s<\n",host.c_str());
      if(debug) printf("  User >%s<\n",user.c_str());
      if(debug) printf("  Unixtime >%d<\n",time);

      msg >> caname >> catype >> nelem;

      if(debug) printf("  caname >%s<\n",caname.c_str());
      if(debug) printf("  catype >%s<\n",catype.c_str());
      if(debug) printf("  nelem >%d<\n",nelem);
      if(nelem > MAX_ELEM)
      {
        printf("WARN: nelem > %d, will set nelem=%d\n",MAX_ELEM,MAX_ELEM);
        nelem = MAX_ELEM;
      }

      if(      !strcmp(catype.c_str(),"char"))   for(ii=0; ii<nelem; ii++) {msg >> d_char[ii]; /*printf(" %c",d_char[ii]);*/}
      else if( !strcmp(catype.c_str(),"uchar"))  for(ii=0; ii<nelem; ii++) {msg >> d_uchar[ii]; /*printf(" %d",d_uchar[ii]);*/}
      else if( !strcmp(catype.c_str(),"short"))  for(ii=0; ii<nelem; ii++) {msg >> d_short[ii]; /*printf(" %d",d_short[ii]);*/}
      else if( !strcmp(catype.c_str(),"ushort")) for(ii=0; ii<nelem; ii++) {msg >> d_ushort[ii]; /*printf(" %d",d_ushort[ii]);*/}
      else if( !strcmp(catype.c_str(),"int"))    for(ii=0; ii<nelem; ii++) {msg >> d_int[ii]; /*printf(" %d",d_int[ii]);*/}
      else if( !strcmp(catype.c_str(),"uint"))   for(ii=0; ii<nelem; ii++) {msg >> d_uint[ii]; /*printf(" %d",d_uint[ii]);*/}
      else if( !strcmp(catype.c_str(),"float"))  for(ii=0; ii<nelem; ii++) {msg >> d_float[ii]; /*printf(" %f",d_float[ii]);*/}
      else if( !strcmp(catype.c_str(),"double")) for(ii=0; ii<nelem; ii++) {msg >> d_double[ii]; /*printf(" %f",d_double[ii]);*/}
      else if( !strcmp(catype.c_str(),"string")) for(ii=0; ii<nelem; ii++) {msg >> d_string[ii]; /*printf(" %s",d_string[ii]);*/}
      else
      {
        printf("epics_msg_callback: ERROR: unknown catype >%s<\n",catype.c_str());
        return;
      }
      /*printf("\n");*/

    }


    void set_debug(int debug_)
	{
      debug = debug_;
	}

};

#endif
