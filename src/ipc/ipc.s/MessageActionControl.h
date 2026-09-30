#ifndef __MESSAGE_ACTION_CONTROL__
#define __MESSAGE_ACTION_CONTROL__

#include <stdio.h>
#include <strings.h>

#include <vector>
#include "MessageAction.h"

/*example: 
  command:ipc_check status
*/

class MessageActionControl : public MessageAction {

  private:

    static const int NFORMATS = 4;
    std::string formats[NFORMATS] = {"command","status","statistics","nostats"};
    int formatid;

    int debug;
    int command;
    int status;
    int statistics;
    int nostats;
    int done;
    std::string myname;

    std::string option;
    std::string requester;

    std::vector<std::string> status_list;

  public:

    MessageActionControl(std::string myname_, int debug_ = 0)
    {
      myname = myname_;
      done = 0;
      command = 0;
      status = 0;
      statistics = 0;
      nostats = 0;
      debug = debug_;
    }

    ~MessageActionControl(){}

    void setNostats(int nostats_)
    {
      nostats = nostats_;
    }
    int getNostats()
    {
      return(nostats);
    }

    void setDone(int done_)
    {
      done = done_;
    }

    int getDone()
    {
      return(done);
    }

    void setDebug(int debug_)
    {
      debug = debug_;
    }

    int getDebug()
    {
      return(debug);
    }


    int check(std::string fmt)
    {
      //if(debug) std::cout << std::endl << "checkControl: fmt >" << fmt.c_str() << "<, debug="<< debug << std::endl;
      int n = 0;

      std::vector<std::string> list = fmtsplit(fmt, std::string(":"));
      for(std::vector<std::string>::const_iterator s=list.begin(); s!=list.end(); ++s)
      {
	if(debug)
	{
	  //std::cout << "check: n=" << n << ", s->c_str() >" << s->c_str() << "<" << std::endl;
	}
	  
	if(n==0) /* first field in 'fmt' is format, compare it with out formats */
	{
          formatid = -1;
          for(int i=0; i<NFORMATS; i++)
	  {
            std::string f = formats[i];
	    //std::cout << "==" << f.c_str() << " " << s->c_str() << " " << strlen(f.c_str()) << std::endl;
            if( !strncmp(f.c_str(),s->c_str(),strlen(f.c_str())) )
	    {
              formatid = i;
              if(debug) std::cout << "check: received command >" << f.c_str() << "< -> set formatid=" << formatid << std::endl;
              break;
	    }
	  }
          if(formatid==-1) return(0); /* unknown format */
	}
        else if(n==1) /* sender name; ignore if it is us */
	{
          //requester = *s; ?????
          requester = s->c_str(); //??????
	  
          if( !strncmp(myname.c_str(),s->c_str(),strlen(myname.c_str())) )
	  {
            if(debug) std::cout << "check: it is our own message - ignore" << std::endl;
            return(0);
	  }
          else
	  {
            if(debug) std::cout << "check: requester=" << requester.c_str() << std::endl;
	  }
	}

        n++;
      }

      if(debug) std::cout << "check: accept" << std::endl;
      return(1);
    }


	/* in old system we had following commands:
      QUIT    		 call quit callback handler
      LOG_IN_DATA [ON|OFF]	 start logging incoming data messages
      LOG_OUT_DATA [ON|OFF]	 start logging outgoing data messages
      LOG_IN_STATUS [ON|OFF]	 start logging incoming status messages
      LOG_OUT_STATUS [ON|OFF]	 start logging outgoing status messages
      GMD_RESEND    		 resend all gmd messages (not sure if this works...)
      STATUS_POLL    		 send status_poll_result to monitoring group
      RECONNECT               disconnect and reconnect to server
	*/
    void decode(IpcConsumer& server)
    {
      if(debug) std::cout << "MessageActionControl: decode CONTROL message" << std::endl;

      if(formatid==0) /* received command */
      {
	option.clear();
        server >> option;
        if(debug) std::cout << "decode: received option >" << option.c_str() << "<, strlen=" << strlen(option.c_str()) << std::endl;

	if(strlen(option.c_str()) > 0)
	{
          if(strncasecmp(option.c_str(),"quit",4)==0)
          {
            if(debug) std::cout << "decode: set done=1" << std::endl;
            done = 1;
	  }
          else if(strncasecmp(option.c_str(),"status",6)==0)
          {
            if(debug) std::cout << "decode: reporting status" << std::endl;
            status = 1;
	  }
          else if(strncasecmp(option.c_str(),"statistics",10)==0)
          {
            if(debug) std::cout << "decode: reporting statistics" << std::endl;
            statistics = 1;
	  }
          else if(strncasecmp(option.c_str(),"nostats",7)==0)
          {
            if(debug) std::cout << "decode: disable reporting statistics" << std::endl;
            nostats = 1;
	  }
          else if(strncasecmp(option.c_str(),"stats",5)==0)
          {
            if(debug) std::cout << "decode: enable reporting statistics" << std::endl;
            nostats = 0;
	  }
	}
	else
	{
	  std::cout << "decode: received command, but there is no option - do nothing" << std::endl;
	  formatid = -1;
	  return;
	}
      }
      else if(formatid==1) /* received status */
      {
        if(debug) std::cout << "decode: received status" << std::endl;
      }

      return;
    }

    void process()
    {
      if(debug) std::cout << "MessageActionControl: process CONTROL message" << std::endl;

      if(formatid==0) /* process 'command' */
      {
        if(status) /* process 'command status' - send our status to requester */
        {
          sendStatus();
          status=0;
        }
        if(statistics)
        {
          sendStatistics(0, 0, 0.0, 0.0);
          statistics=0;
        }
      }
      else if(formatid==1) /* process 'status' */
      {
        if(debug) std::cout << "MessageActionControl: status >" << requester.c_str() << "<received from "<< requester.c_str() << std::endl;

        std::vector<std::string>::iterator iter = status_list.begin();

        while (iter != status_list.end())
        {
          if(*iter == requester) iter = status_list.erase(iter);
          else                   iter++;
        }

        status_list.push_back(requester);
      }
    }

    void clearStatusList()
    {
      status_list.clear();
    }

    std::vector<std::string> getStatusList()
    {
      return(status_list);
    }

    void sendCommand(char *destination, char *command)
    {
      if(debug) std::cout << "MessageActionControl: sendCommand " << command << "to " << destination << std::endl;     
      IpcServer &server = IpcServer::Instance();
      server << clrm << "command:"+myname << command /*<< time(0)*/;
      server << endm;
    }

    void sendStatus()
    {
      char topic[1024];
      /* send message to topic expid.session.control.our_name */
      sprintf(topic,"%s.%s.%s.%s",getenv("EXPID"),getenv("SESSION"),"control",requester.c_str());
      if(debug) std::cout << "MessageActionControl: sendStatus to topic " << topic << std::endl;     
      IpcServer &server = IpcServer::Instance();
      server << clrm << "status:"+myname << myname << 0/*<< time(0)*/;
      server << SetTopic(topic) << endm;
    }

    void sendStatistics(int32_t nev_received, int32_t nev_processed,
                        double rate_received, double rate_processed)
    {
      if(debug) std::cout << "MessageActionControl: sendStatistics" << std::endl;     
      IpcServer &server = IpcServer::Instance();
      server << clrm << "statistics:"+myname << myname << 0/*<< time(0)*/;
      server << nev_received;
      server << nev_processed;
      server << rate_received;
      server << rate_processed;
      server << endm;
    }

};

#endif
