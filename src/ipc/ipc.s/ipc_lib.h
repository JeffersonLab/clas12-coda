
/* ipc_lib.h */

#define MAX_TOPIC_LENGTH 1024

#define DEFAULT_APPLICATION        "clastest"
#define DEFAULT_STATUS_POLL_GROUP  "clas_monitor"
#define DEFAULT_DISCONNECT_MODE    "gmd_failure"

#define DEBUG

/* 
* define ipc error codes for "perror" functions 
*
* NOTE: codes <0 indicate fatal errors
*             >0 are warnings and can possibly be ignored
*
*/

typedef enum{

  IPC_ERR_UNIQUE_DATAGROUP           	= -1000,
  IPC_ERR_LICENSE_FILE               	=  1001,
  IPC_ERR_SERVER_NAMES               	=  1002,
  IPC_ERR_APPLICATION                	=  1003,
  IPC_ERR_SET_UNIQUE_DATAGROUP       	= -1004,
  IPC_ERR_SERVER_DISCONNECT_MODE     	=  1005,   
  IPC_ERR_SERVER_DELIVERY_TIMEOUT    	=  1006,
  IPC_ERR_SIGTERM                    	=  1007,
  IPC_ERR_SIGQUIT                    	=  1008,
  IPC_ERR_SIGINT                     	=  1009,
  IPC_ERR_SIGHUP                     	=  1010,
  IPC_ERR_IDENT_STRING               	=  1011, 
  IPC_ERR_ALARM_HANDLER              	=  1012,
  IPC_ERR_SERVER_CREATE              	= -1013,
  IPC_ERR_CONN_CALLBACK_DESTROY      	=  1014,
  IPC_ERR_SERVER_GMD_FAILURE         	=  1015,
  IPC_ERR_CONTROL_CALLBACK_DESTROY   	=  1016,
  IPC_ERR_CONTROL_CALLBACK_CREATE    	=  1017,
  IPC_ERR_STANDARD_DATAGROUPS        	=  1018,
  IPC_ERR_MT_CREATE                     = -1019,
  IPC_ERR_NULL_APPLICATION           	=  1020,
  IPC_ERR_ATEXIT                     	=  1021,
  IPC_ERR_NULL_STATUS_POLL_GROUP     	=  1022,
  IPC_ERR_NULL_DISCONNECT_MODE          =  1023,
  IPC_ERR_DEFAULT_CALLBACK_CREATE       =  1024,
  IPC_ERR_LOG_MT                        =  1025

} IPC_ERROR_CODE;


/* composite destination (works for both sender and receiver ???)
Destination destination = session.createQueue("test-queue,test-queue-foo,topic://test-topic-foo")

/topic/test01,/topic/test02,/topic/test15A
*/


/* possible options for brokerURI:
    std::string brokerURI =
        "failover:(tcp://clondb1new:61616"
        "?wireFormat=openwire"
        "&transport.useInactivityMonitor=false"
        "&connection.alwaysSyncSend=true"
        "&connection.useAsyncSend=true"
        "?transport.commandTracingEnabled=true"
        "&transport.tcpTracingEnabled=true"
        "&wireFormat.tightEncodingEnabled=true"
        ")";
*/

#define DEFAULT_BROKER_HOST "clondb1"

#define GET_BROKER \
  char *broker_host = NULL; \
  char broker_str[512]; \
  broker_host = getenv("IPC_HOST"); \
  if(broker_host==NULL) \
  { \
    printf("Cannot get broker_host name - use default >%s<\n",DEFAULT_BROKER_HOST);	\
    broker_host = strdup(DEFAULT_BROKER_HOST); \
  } \
  printf("Will use broker on host >%s<\n",broker_host);		   \
  sprintf(broker_str,"failover:(tcp://%s:61616?wireFormat=openwire&wireFormat.maxInactivityDuration=10)",broker_host); \
  brokerURI.assign(broker_str); \
  printf("brokerURI >%s<\n",brokerURI.c_str())

/*
  sprintf(broker_str,"failover:(tcp://%s:61616)"
                     "failover:(tcp://%s:61616?wireFormat=openwire&wireFormat.maxInactivityDuration=10)"
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>

#include <iostream>
#include <memory>
#include <vector>
#include <functional>

#include <activemq/library/ActiveMQCPP.h>
#include <decaf/lang/Thread.h>
#include <decaf/lang/Runnable.h>
#include <decaf/lang/Integer.h>
#include <decaf/lang/Long.h>
#include <decaf/lang/System.h>
#include <activemq/core/ActiveMQConnectionFactory.h>
#include <activemq/util/Config.h>
#include <cms/Connection.h>
#include <cms/Session.h>
#include <cms/TextMessage.h>
#include <cms/BytesMessage.h>
#include <cms/MapMessage.h>
#include <cms/StreamMessage.h>
#include <cms/ExceptionListener.h>
#include <cms/MessageListener.h>
#include <cms/Destination.h>


using namespace activemq::core;
using namespace decaf::util::concurrent;
using namespace decaf::util;
using namespace decaf::lang;
using namespace cms;
//using namespace std;


/* classes */
class IpcProducer;
class IpcConsumer;
#include "MessageAction.h"






/* 
TO MAKE SURE WE DO 'initializeLibrary' and 'shutdownLibrary' ONLY ONCE !!!
every 'init' will increase it, every 'close' will decrease it;
in 'init': if library_initialized_counter==0, will call activemq::library::ActiveMQCPP::initializeLibrary()
in 'close': if library_initialized_counter==1, will call activemq::library::ActiveMQCPP::shutdownLibrary()
 */
/*static*/extern int library_initialized_counter; /* defined in ipc_lib.cc */












/* classes for overloading */
class MessageClearer {
};
static MessageClearer clrm;

class MessageSender {
};
static MessageSender endm;


  /********/
  /* Dima */

/* if have to distinguish between array and vector:
template<typename Test, template<typename...> class Ref>
struct is_specialization : std::false_type {};

template<template<typename...> class Ref, typename... Args>
struct is_specialization<Ref<Args...>, Ref>: std::true_type {};
*/






/*sergey*/
/// This class is used to configure how the next message is sent
struct SendConfiguration
{
  std::string topic = "";
};

/// This class changes the next message transfer configuration
class SendConfigManip {

 public:
  typedef std::function<void(SendConfiguration&)> ConfigFunction;

  ConfigFunction _action;

  explicit SendConfigManip(const ConfigFunction& action)
  {
	/*std::cout<<"SendConfigManip::Constructor"<<std::endl;*/
	_action = action;
  }
};

static
SendConfigManip SetTopic(std::string topic)
{
  //std::cout<<"=== SendConfigManip::SetTopic"<<std::endl;
  return SendConfigManip([=](SendConfiguration& conf) {
	  //std::cout<<"SetTopic: Changing message transfer configuration. SetTopic:"<<topic<<std::endl;
	  conf.topic = topic;
	  //std::cout<<"SetTopic: conf.topic: "<<conf.topic<<std::endl;
    });
}
/*sergey*/







constexpr size_t UnknownSize = static_cast<size_t>(-1);

/// This class is used to configure how the next array is sent
struct ArraySendConfiguration
{
  size_t sendItemsCount = UnknownSize;
  size_t startOffset = 0;
};

/// This class changes the next array transfer configuration
class SendArrayConfigManip {

 public:
  typedef std::function<void(ArraySendConfiguration&)> ConfigFunction;

  ConfigFunction _action;

  explicit SendArrayConfigManip(const ConfigFunction& action)
  {
	/*std::cout<<"SendArrayConfigManip::Constructor"<<std::endl;*/
	_action = action;
  }
};

static
SendArrayConfigManip SetSize(size_t  size)
{
  //std::cout<<"=== SendArrayConfigManip::SetSize"<<std::endl;
  return SendArrayConfigManip([=](ArraySendConfiguration& conf) {
	  //std::cout<<"SetSize Lambda. Changing array transfer configuration. SetSize:"<<size<<std::endl;
	  conf.sendItemsCount = size;
    });
}

static
SendArrayConfigManip SetOffset(size_t  offset)
{
  return SendArrayConfigManip([=](ArraySendConfiguration& conf){
	  //std::cout<<"SetOffset Lambda.Changing array transfer configuration. SetOffset:"<<offset<<std::endl;
	  conf.startOffset = offset;
    });
}


  /* Dima */
  /********/



/* Producer */

class IpcProducer : public Runnable {

friend class IpcServer;

private:






	/********/
	/*sergey*/

    std::vector<SendConfigManip> _nextSendManips;

	void sendManip()
    {
      //std::cout << "=== sendManip reached" << std::endl;
	  SendConfiguration conf;

	  // Go through manipulators and apply their manipulation
	  for(auto manip: _nextSendManips)
      {
		manip._action(conf);         // change configuration according to manipulator
	  }
      //std::cout << "=== sendManip: conf.topic= >" << conf.topic << "<, len=" << strlen(conf.topic.c_str()) << std::endl;

	  _nextSendManips.clear();   // clear manipulators for further ...


      // it topic is set by SetTopic() manipulator then use it, otherwise use default one
      if(strlen(conf.topic.c_str()) == 0)
	  {
        //strcpy(topic, topic_orig);
        Topic = Topic_orig;
		//std::cout << "=== SetTopic 1: set topic= >" << Topic.c_str() << "<" << std::endl;
	  }
      else
	  {
        //strcpy(topic, conf.topic.c_str());
        Topic = conf.topic;
		//std::cout << "=== SetTopic 2: set topic= >" << Topic.c_str() << "<" << std::endl;
	  }
    }

	/*sergey*/
    /********/




    /********/
    /* Dima */

    std::vector<SendArrayConfigManip> _nextArrayManips;

    template <typename T>
	  void sendArray(T arr, size_t arraySize, size_t arrayType_not_used)
    {
      //std::cout << "=== sendArray" << std::endl;
	  // Go through manipulators and apply their manipulation
	  ArraySendConfiguration conf;
	  for(auto manip: _nextArrayManips)
      {
		manip._action(conf);         // change configuration according to manipulator
	  }

	  _nextArrayManips.clear();   // clear manipulators for further arrays


	  // If nobody knows the array size we can't continue
	  if(conf.sendItemsCount == UnknownSize && arraySize == UnknownSize)
      {
	    throw std::logic_error("Must set array size!");
	  }

	  size_t sendingSize = arraySize;

	  // sendItemsCount is set?
	  if(conf.sendItemsCount != static_cast<size_t>(-1))
      {
		if(conf.sendItemsCount + conf.startOffset > arraySize)
		{
		  throw std::logic_error("Hey! Don't lie to me! I know array size and it is smaller than sendItemsCount + startOffset");
		}
		sendingSize = conf.sendItemsCount;
	  }

	  /* it is already taken into accout - SetSize() suppose to set the number of elements with offset subtracted 
	  // Take starting shift into the account
	  sendingSize -= conf.startOffset;
	  */

	  /*
	  // print an array
	  for(size_t i=conf.startOffset; i<conf.startOffset+sendingSize; i++)
      {
        std::cout << "===== sendArray: sending" << std::endl;
		std::cout << arr[i] << std::endl;
      }
	  */  


/* if have to distinguish between array and vector:
if(is_specialization<arr, std::vector>::value)
{
  // it is some vector
  std::vector<unsigned char> toSend;
  for(size_t i=conf.startOffset; i<conf.startOffset+sendingSize; i++) toSend.push_back((unsigned char)arr[i]);
  message->writeBytes(toSend);
}
else
{
  message->writeBytes((const unsigned char *)&arr[conf.startOffset], 0, sendingSize*sizeof(arr[0]);
}
*/


      // send an array
	/*
	  std::cout<<"=== sendArray: sendingSize="<<sendingSize<<" ELEMENT SIZE "<<sizeof(arr[0])<<" size in bytes="<<sendingSize*sizeof(arr[0])<< std::endl;
	*/
	  message->writeBytes((const unsigned char *)&arr[conf.startOffset], 0, sendingSize*sizeof(arr[0]));

    }

    /* Dima */
    /********/



private:

    std::string brokerURI;
    Connection* connection;
    Session* session;
    Destination* destination;
/*
    char* expid;
    char* sesid;
    char* sysid;
    char* unique;
*/

    Thread* producerThread;
    MessageProducer* producer;
    //StreamMessage* message;

    int waiting_for_messages;
/*
    char topic_orig[MAX_TOPIC_LENGTH];
    char topic[MAX_TOPIC_LENGTH];
*/

    std::string Topic_orig;
    std::string Topic;

    int array_size_in_bytes;


private:

    StreamMessage* message;
    int _size;


//private:
//IpcProducer(const IpcProducer&);
//IpcProducer& operator=(const IpcProducer&);



private:

    IpcProducer() :
      connection(NULL),
      session(NULL),
      destination(NULL),
	  /*
      expid(NULL),
      sesid(NULL),
      sysid(NULL),
      unique(NULL),
	  */
      producerThread(NULL),
      producer(NULL),
      message(NULL),

      waiting_for_messages(1)
    {
      //printf("================ IpcProducer created ====================================================\n");
    }

    virtual ~IpcProducer()
    {
      cleanup();
    }



	  /* singleton: in main program do
IpcProducer &sender = IpcProducer::Instance();
//IpcProducer *sender = &IpcProducer::Instance();
*/

 public:

    static IpcProducer& Instance()
	{
      static IpcProducer m_instance;
      return m_instance;
	}

    void AddSendTopic(char* expid_ = NULL, char* sesid_ = NULL, char* sysid_ = NULL, char* unique_ = NULL)
	{
      char tmp[MAX_TOPIC_LENGTH];
      char* expid = expid_;
      char* sesid = sesid_;
      char* sysid = sysid_;
      char* unique = unique_;

      if(expid==NULL)  expid = "*";
      if(sesid==NULL)  sesid = "*";
      if(sysid==NULL)  sysid = "*";
      if(unique==NULL) sysid = "*";

      sprintf(tmp,"%s.%s.%s.%s",expid,sesid,sysid,unique);

      if(Topic.length()!=0) Topic += ',';
      Topic += tmp;

      printf("IpcProducer::AddSendTopic: Topic >%s< len=%d\n",Topic.c_str(),Topic.length());
	}


    void send_init(/*char* expid_ = NULL, char* sesid_ = NULL, char* sysid_ = NULL, char* unique_ = NULL*/)
	{
	  /*
	  expid = expid_;
	  sesid = sesid_;
	  sysid = sysid_;
      unique = unique_;
	  */
      if(library_initialized_counter==0)
      {
        //printf("IpcProducer: initializeLibrary (library_initialized_counter=%d)\n",library_initialized_counter);
        activemq::library::ActiveMQCPP::initializeLibrary();
      }
      else
	  {
        //printf("IpcProducer: library_initialized_counter1=%d\n",library_initialized_counter);
	  }
      library_initialized_counter++;

      GET_BROKER;

      producerThread = new Thread(this); /* Start the producer thread */
      producerThread->start();
      this->waitUntilReady(); /* Wait for the producer to indicate that its ready to go */
	}

    void send_close()
    {
	  //printf("IpcProducer::close() reached, library_initialized_counter=%d\n",library_initialized_counter);

      this->run_exit();
	  //printf("IpcProducer::close() 1: producerThread=%x\n",producerThread);
      if(producerThread != NULL)
	  {
        producerThread->join();
        producerThread = NULL;
	  }
	  //printf("IpcProducer::close() 2\n");

      this->cleanup();
	  //printf("IpcProducer::close() 3\n");

      if(library_initialized_counter==1)
      {
        //printf("IpcProducer: shutdownLibrary (library_initialized_counter=%d)\n",library_initialized_counter);
        activemq::library::ActiveMQCPP::shutdownLibrary();
      }
      else
	  {
        //printf("IpcProducer: library_initialized_counter2=%d\n",library_initialized_counter);
	  }
      library_initialized_counter--;

	  //printf("IpcProducer::close() 4\n");
    }




 private:

    void waitUntilReady()
    {
      //printf("IpcProducer::waitUntilReady() reached\n");
      while(producer==NULL||session==NULL||message==NULL)
      {
        sleep(1); // it will hung if does NOT sleep here !
      }
      //printf("IpcProducer::waitUntilReady() ready !!!\n");
    }

    void run_exit()
    {
	  //printf("IpcProducer::run_exit() reached\n");
      waiting_for_messages = 0;
    }





    virtual void run()
	{
      try
      {
		/*
        if(expid==NULL)  expid = "*";
        if(sesid==NULL)  sesid = "*";
        if(sysid==NULL)  sysid = "*";
        if(unique==NULL) sysid = "*";
        sprintf(topic,"%s.%s.%s.%s",expid,sesid,sysid,unique);
        strcpy(topic_orig, topic);
		*/
        Topic_orig = Topic;
        printf("IpcProducer's topic >%s<, topic_orig >%s<\n",Topic.c_str(), Topic_orig.c_str());
		/* step 1: Create a ConnectionFactory (broker's URI specified here: protocol(TCP etc), IP address, port, optionally other params) */
		/*    username and password can be specified, for example:
				  createCMSConnectionFactory( "tcp://127.0.0.1:61616?username=${USERNAME}&password=${PASSWORD}" ) */
        std::unique_ptr<ConnectionFactory> connectionFactory(ConnectionFactory::createCMSConnectionFactory(brokerURI));

		/* step 2: Create a Connection */
        /*    username server.Flush();and password can be specified, for example: connectionFactory->createConnection( "<USERNAME>", "<PASSWORD>") */
        connection = connectionFactory->createConnection();
        connection->start();

        // Create a Session
        /*session = connection->createSession(Session::SESSION_TRANSACTED);*/
        session = connection->createSession(Session::AUTO_ACKNOWLEDGE);

        // create stream message
        message = session->createStreamMessage();






        // Create the destination (Topic or Queue)
		destination = session->createTopic(Topic.c_str());

        // Create a MessageProducer from the Session to the Topic or Queue
        producer = session->createProducer(destination);
        producer->setDeliveryMode(DeliveryMode::NON_PERSISTENT);




        /* wait here until 'waiting_for_messages' becomes 0, then return */
		while(waiting_for_messages)
		{
          //printf("IpcProducer::run: waiting_for_messages\n");
		  sleep(1);
		}

		/*
printf("IpcProducer::run: 1\n");
connection->stop();
printf("IpcProducer::run: 2\n");
connection->close();
printf("IpcProducer::run: 3\n");
		*/

        //printf("IpcProducer::run: exited 'waiting_for_messages' loop\n");

      }
      catch (CMSException& e)
      {
        e.printStackTrace();
      }

	}



 private:

    /********************/
	/* message handling */

    void clearMsg()
	{
      message->clearBody();
      return;
	}

    int sendMsg()
	{
	  /* IF TOPIC CHANGED, DESTROY destination AND producer AND RECREATE THEM WITH NEW TOPIC */
      // Destroy resources.
      try{
        if( destination != NULL ) delete destination;
      }catch ( CMSException& e ) { e.printStackTrace(); }
      destination = NULL;

      try{
        if( producer != NULL ) delete producer;
      }catch ( CMSException& e ) { e.printStackTrace(); }
      producer = NULL;

	  //printf("TOPIC >%s<\n",Topic.c_str());
	  destination = session->createTopic(Topic.c_str());
      producer = session->createProducer(destination);
      producer->setDeliveryMode(DeliveryMode::NON_PERSISTENT);
	  /*MUST BE HERE SINCE WE CHANGED TOPIC*/

      producer->send(message);

	  /*
valgrind --leak-check=full
	  */
	  /*
      delete message;
      delete session;
      session = connection->createSession(Session::AUTO_ACKNOWLEDGE);
      message = session->createStreamMessage();
	  */

      return(0);
	}



public:


	/*sergey*/	
    IpcProducer &operator << (const SendConfigManip& mm)
    {
	  _nextSendManips.push_back(mm);
	  //std::cout<<"Insert SendConfigManip"<<std::endl;
	  return *this;
    }

/*topicm
	IpcProducer &operator << (MessageTopic& topic)
    {
	  std::cout<<"IpcProducer<<(topic string)"<<std::endl;
	  setTopic("bla");
	  return *this;
    }
*/
	/*sergey*/



    /********/
    /* Dima */

    /* operators overloading for manipulators */

	/* ??????
    IpcProducer &operator<<(const std::string &val)
    {
	  std::cout << val << std::endl;
	  return *this;
    }
	*/

    IpcProducer &operator << (const SendArrayConfigManip& mm)
    {
	  _nextArrayManips.push_back(mm);
	  return *this;
    }

    /// This overloads sending pointers to an array
    /// It will throw the exception if there is no SetSize is called before
    IpcProducer &operator << (const int* arr)
	{
	  //std::cout<<"IpcProducer<<(int*). Send array with unknown size (SetSize(...) must be set)"<<" sizeof="<<sizeof(arr[0])<<std::endl;
      sendArray(arr, UnknownSize, 0);  // for pointers to array we don't know the size!
      return *this;
	}

    /// This template function KNOWS the array size...
    template<typename T, size_t arrAutoSize>
	IpcProducer &operator << (T(&arr)[arrAutoSize])
    {
	  //std::cout<<"IpcProducer<<(T(&arr)[]). Send array with known size="<<arrAutoSize<<" sizeof="<<sizeof(arr[0])<<std::endl;
	  sendArray(arr, arrAutoSize, 0);
	  return *this;
    }

    /// std::vector of T implementation
    template<typename T>
	IpcProducer &operator << (std::vector<T> arr)
    {
	  //std::cout<<"IpcProducer<<(vector). Send vector with size="<<arr.size()<<" sizeof="<<sizeof(arr[0])<<std::endl;
	  sendArray(arr, arr.size(), 0);
	  return *this;
    }

    /* Dima */
    /********/


	/* operators overloading: operator<< should always return it's left hand side operand
    in order to chain calls, just like operator=.
	*/

    IpcProducer& operator << (MessageSender& abc)
    {
      sendManip();
      sendMsg();
      return(*this);
    }


    IpcProducer& operator << (MessageClearer& abc) {clearMsg(); return(*this);}

    IpcProducer& operator << (char               val) {message->writeChar(val); return(*this);}
    IpcProducer& operator << (int8_t             val) {message->writeByte(val); return(*this);}
    IpcProducer& operator << (uint8_t            val) {message->writeByte(val); return(*this);}
    IpcProducer& operator << (int16_t            val) {message->writeShort(val); return(*this);}
    IpcProducer& operator << (uint16_t           val) {message->writeUnsignedShort(val); return(*this);}
    IpcProducer& operator << (int32_t            val) {message->writeInt(val); return(*this);}
    IpcProducer& operator << (uint32_t           val) {message->writeInt(val); return(*this);}
    IpcProducer& operator << (int64_t            val) {message->writeLong(val); return(*this);}
    IpcProducer& operator << (uint64_t           val) {message->writeLong(val); return(*this);}
    IpcProducer& operator << (float              val) {message->writeFloat(val); return(*this);}
    IpcProducer& operator << (double             val) {message->writeDouble(val); return(*this);}
    IpcProducer& operator << (const std::string &val) {message->writeString(val); return(*this);}
    IpcProducer& operator << (char              *val) {message->writeString(val); return(*this);}
    IpcProducer& operator << (const char        *val) {message->writeString(val); return(*this);}
    
	/* arrays */


    /********************/
    /********************/

	/*ActiveMQ:
        virtual void writeBytes(const std::vector<unsigned char>& value) = 0;
        virtual void writeBytes(const unsigned char* value, int offset, int length) = 0;
	*/

private:

    void cleanup()
    {
	  //printf("IpcProducer::cleanup() reached\n");
      if (connection != NULL)
      {
	    //printf("IpcProducer::cleanup() 1\n");
        try
        {
	      //printf("IpcProducer::cleanup() 2\n");
          connection->close();
	      //printf("IpcProducer::cleanup() 3\n");
        }
        catch (cms::CMSException& ex)
        {
	      //printf("IpcProducer::cleanup() 4\n");
          ex.printStackTrace();
        }
      }

      // Destroy resources.
      try
      {
	    //printf("IpcProducer::cleanup() 5\n");
        delete destination;
        destination = NULL;
        delete producer;
        producer = NULL;
        delete session;
        session = NULL;
        delete connection;
        connection = NULL;
      }
      catch (CMSException& e)
      {
        e.printStackTrace();
      }
    }
};













  /* Dima-sergey */

/// This class is used to configure how the next array is received
struct ArrayRecvConfiguration
{
  size_t recvItemsCount = UnknownSize;
  size_t startOffset = 0;
};

/// This class changes the next array transfer configuration
class RecvArrayConfigManip {

 public:
  typedef std::function<void(ArrayRecvConfiguration&)> ConfigFunction;

  ConfigFunction _action;

  explicit RecvArrayConfigManip(const ConfigFunction& action)
  {
	//std::cout<<"RecvArrayConfigManip::Constructor"<<std::endl;
	_action = action;
  }
};



/* what lambda does:
int* sizeCapture;
void lambda(ArrayRecvConfiguration& conf) {      
	  
      *sizeCapture = conf.recvItemsCount;

	  std::cout<<"GetSize Lambda. Changing array transfer configuration."
		" GetSize:"<<conf.recvItemsCount<<std::endl;

    });
*/
// lambda inserts _action's, [=] grabs parameters (in that case '*size')
static
RecvArrayConfigManip GetSize(/*size_t*/int *size)
{
  //std::cout<<"RecvArrayConfigManip::GetSize"<<std::endl;

  /* what lambda does:
  sizeCapture = size;
  return Recv...
  */

  return RecvArrayConfigManip([=](ArrayRecvConfiguration& conf) {
	  
      *size = conf.recvItemsCount;

	  //std::cout<<"GetSize Lambda. Changing array transfer configuration. GetSize:"<<conf.recvItemsCount<<std::endl;

    });
}

  /* Dima-sergey */



/* Consumer */


class IpcConsumer : public ExceptionListener,
                    public MessageListener,
                    public Runnable {

friend class IpcServer;

private:

    /* Dima-sergey */

    int read_bytes;
    int read_words;

    std::vector<RecvArrayConfigManip> _nextArrayManips;

    template <typename T>
	  void recvArray(T arr, size_t arraySize, int report_length)
    {
      // 'arr[]' is output array, 'arraySize' is it's size in words
      //std::cout << "=== recvArray: arraySize="<<arraySize << std::endl;

	  if(report_length)
	  {
        //std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! read_words=" << read_words << std::endl;


	    // Go through manipulators and apply their manipulation
	    ArrayRecvConfiguration conf;
        conf.recvItemsCount = read_words;
	    for(auto manip: _nextArrayManips)
        {
		  //printf("manip1\n");
		  manip._action(conf);         // change configuration according to manipulator
		  //printf("manip2\n");
	    }
	    _nextArrayManips.clear();   // clear manipulators for further arrays

	  }
	  else
	  {
        // read message from stream
        int arr_length_in_bytes = arraySize*sizeof(arr[0]);
        //std::cout << "=== recvArray: arr_length_in_bytes=" << arr_length_in_bytes << std::endl;

        // following returns # of bytes, or -1 if nothing left
	    read_bytes = streamMessage->readBytes((unsigned char *)arr, arr_length_in_bytes);
        if(sizeof(arr[0]) != 0) read_words = read_bytes/sizeof(arr[0]);

        //std::cout << "=== recvArray: read_bytes=" << read_bytes << ", read_words=" << read_words << std::endl;
        //for(int i=0; i<read_words; i++) std::cout << "--------- arr["<<i<<"]="<<arr[i]<< std::endl;
	  
	  }


	  /*
	  // If nobody knows the array size we can't continue
	  if(conf.recvItemsCount == UnknownSize && arraySize == UnknownSize)
      {
	    throw std::logic_error("Must set array size!");
	  }

	  size_t recvingSize = read_words;
      std::cout << "---- recvingSize=" << recvingSize << std::endl;

	  // recvItemsCount is set?
	  if(conf.recvItemsCount != static_cast<size_t>(-1))
      {
		if(conf.recvItemsCount + conf.startOffset > arraySize)
		{
		  throw std::logic_error("Hey! Don't lie to me! I know array size and it is smaller than recvItemsCount + startOffset");
		}
		recvingSize = conf.recvItemsCount;
	  }
	  */

	  /*
	  // print received array
	  for(size_t i=conf.startOffset; i<conf.startOffset+recvingSize; i++)
      {
        std::cout << "===================================== recving" << std::endl;
		std::cout << arr[i] << std::endl;
      }
	  */
    }

    /* Dima-sergey */






private:

    std::string brokerURI;
    Connection* connection;
    Session* session;
    Destination* destination;
	/*
    char* expid;
    char* sesid;
    char* sysid;
    char* unique;
	*/
    Thread* consumerThread;
    MessageConsumer* consumer;

    int waiting_for_messages;
    /*char topic[MAX_TOPIC_LENGTH];*/
	std::string Topic;

    std::vector<MessageAction *> actionListeners;

//private:
//    IpcConsumer(const IpcConsumer&);
//    IpcConsumer& operator=(const IpcConsumer&);


private:

    IpcConsumer() :
      connection(NULL),
      session(NULL),
      destination(NULL),
	  /*
      expid(NULL),
      sesid(NULL),
      sysid(NULL),
      unique(NULL),
	  */
      consumerThread(NULL),
      consumer(NULL),

      waiting_for_messages(0)
    {
      //printf("================ IpcConsumer created ====================================================\n");
    }

    virtual ~IpcConsumer()
    {
      cleanup();
    }


 public:

    static IpcConsumer& Instance()
	{
      static IpcConsumer m_instance;
      return m_instance;
	}

    void AddRecvTopic(char* expid_ = NULL, char* sesid_ = NULL, char* sysid_ = NULL, char* unique_ = NULL)
	{
      char tmp[MAX_TOPIC_LENGTH];
      char* expid = expid_;
      char* sesid = sesid_;
      char* sysid = sysid_;
      char* unique = unique_;

      if(expid==NULL)  expid = "*";
      if(sesid==NULL)  sesid = "*";
      if(sysid==NULL)  sysid = "*";
      if(unique==NULL) sysid = "*";

      sprintf(tmp,"%s.%s.%s.%s",expid,sesid,sysid,unique);

      if(Topic.length()!=0) Topic += ',';
      Topic += tmp;

      printf("IpcConsumer::recv_addtopic: Topic >%s< len=%d\n",Topic.c_str(),Topic.length());
	}

    void recv_init(/*char* expid_ = NULL, char* sesid_ = NULL, char* sysid_ = NULL, char* unique_ = NULL*/)
	{
	  /*
	  expid = expid_;
	  sesid = sesid_;
	  sysid = sysid_;
      unique = unique_;
	  */
      if(library_initialized_counter==0)
      {
        //printf("IpcConsumer: initializeLibrary (library_initialized_counter=%d)\n",library_initialized_counter);
        activemq::library::ActiveMQCPP::initializeLibrary();
      }
      else
	  {
        //printf("IpcConsumer: library_initialized_counter1=%d\n",library_initialized_counter);
	  }
      library_initialized_counter++;

      GET_BROKER;

      consumerThread = new Thread(this); /* Start the consumer thread */
      consumerThread->start();
      this->waitUntilReady(); /* Wait for the consumer to indicate that its ready to go */
	}

    void recv_close()
    {
	  //printf("IpcConsumer::close() reached, library_initialized_counter=%d\n",library_initialized_counter);

      this->run_exit();

      if(consumerThread != NULL)
	  {
        consumerThread->join();
        consumerThread = NULL;
	  }

      this->cleanup();

      if(library_initialized_counter==1)
      {
        //printf("IpcConsumer: shutdownLibrary (library_initialized_counter=%d)\n",library_initialized_counter);
        activemq::library::ActiveMQCPP::shutdownLibrary();
      }
      else
	  {
        //printf("IpcConsumer: library_initialized_counter2=%d\n",library_initialized_counter);
	  }
      library_initialized_counter--;
    }

	void AddCallback(MessageAction *listener)
    {
      actionListeners.push_back(listener);
    }


 private:

    void waitUntilReady()
    {
      //printf("IpcConsumer::waitUntilReady() reached\n");
      while(waiting_for_messages==0)
      {
        sleep(1); // it will hung if does NOT sleep here !
      }
      //printf("IpcConsumer::waitUntilReady() ready !!!\n");
    }

    void run_exit()
    {
	  //printf("IpcConsumer::run_exit() reached\n");
      waiting_for_messages = 0;
    }

    virtual void run()
    {
      try
      {
		/*
        if(expid==NULL)  expid = "*";
        if(sesid==NULL)  sesid = "*";
        if(sysid==NULL)  sysid = "*";
        if(unique==NULL) sysid = "*";
        sprintf(topic,"%s.%s.%s.%s",expid,sesid,sysid,unique);
		*/

        printf("IpcConsumer's topic >%s<\n",Topic.c_str());

        // Create a ConnectionFactory
        std::unique_ptr<ConnectionFactory> connectionFactory(
                ConnectionFactory::createCMSConnectionFactory(brokerURI));

        // Create a Connection
        connection = connectionFactory->createConnection();
        connection->start();
        connection->setExceptionListener(this);

        // Create a Session
        session = connection->createSession(Session::SESSION_TRANSACTED);
        /*session = connection->createSession(Session::AUTO_ACKNOWLEDGE);*/

        // Create the destination (Topic or Queue)
		destination = session->createTopic(Topic.c_str());
		/*destination = session->createQueue(topic);*/

        // Create a MessageConsumer from the Session to the Topic or Queue
        consumer = session->createConsumer(destination);

        consumer->setMessageListener(this);

        std::cout.flush();
        std::cerr.flush();

        // here we can indicate we are ready for messages - if needed ...
        

		//printf("IpcConsumer::run: waiting for messages ..\n");

        // Wait for messages
        waiting_for_messages = 1;
        while(waiting_for_messages) {sleep(1);}

		//printf("IpcConsumer::run: NOT waiting for messages any more\n");

      }
      catch (CMSException& e)
      {
        e.printStackTrace();
      }
    }




 public:

    /* operators overloading for manipulators */

	/*dima-sergey*/

    // activated by '>>GetSize()' or any other manipulator with the same type;
    // just register manipolator (GetSize) here, it will be called from recvArray()
    IpcConsumer &operator >> (const RecvArrayConfigManip& mm)
    {
	  //std::cout<<"--------------------------------------------- RecvArrayConfigManip.push_back"<<std::endl;
	  _nextArrayManips.push_back(mm);
      const int* arr; // just fake, will not be used inside following call
      recvArray(arr, 0, 1);  // to get length of the array we received before
	  return *this;
    }

    /// This overloads receiving pointers to an array
    /// It will throw the exception if there is no GetSize is called before
    IpcConsumer &operator >> (const int* arr)
	{
	  //std::cout<<"IpcConsumer>>(int*). Receive array with unknown size (GetSize(...) must be set)"<<std::endl;
      recvArray(arr, UnknownSize, 0);  // for pointers to array we don't know the size!
      return *this;
	}

    /// This template function KNOWS the array size...
    template<typename T, size_t arrAutoSize>
	IpcConsumer &operator >> (T(&arr)[arrAutoSize])
    {
	  //std::cout<<"IpcConsumer>>(T(&arr)[]). Receive array with known size="<<arrAutoSize<<std::endl;
	  recvArray(arr, arrAutoSize, 0);
	  return *this;
    }

    /// std::vector of T implementation
    template<typename T>
	IpcConsumer &operator >> (std::vector<T> arr)
    {
	  //std::cout<<"IpcConsumer>>(vector). Receive vector with size="<<arr.size()<<std::endl;
	  recvArray(arr, arr.size(), 0);
	  return *this;
    }
	/*dima-sergey*/


	/* operators overloading: operator<< should always return it's left hand side operand
    in order to chain calls, just like operator=.
	*/

	const StreamMessage* streamMessage;

    IpcConsumer& operator >> (char        &val) {val = streamMessage->readChar(); return(*this);}
    IpcConsumer& operator >> (int8_t      &val) {val = streamMessage->readByte(); return(*this);}
    IpcConsumer& operator >> (uint8_t     &val) {val = streamMessage->readByte(); return(*this);}
    IpcConsumer& operator >> (int16_t     &val) {val = streamMessage->readShort(); return(*this);}
    IpcConsumer& operator >> (uint16_t    &val) {val = streamMessage->readUnsignedShort(); return(*this);}
    IpcConsumer& operator >> (int32_t     &val) {val = streamMessage->readInt(); return(*this);}
    IpcConsumer& operator >> (uint32_t    &val) {val = streamMessage->readInt(); return(*this);}
    IpcConsumer& operator >> (int64_t     &val) {val = streamMessage->readLong(); return(*this);}
    IpcConsumer& operator >> (uint64_t    &val) {val = streamMessage->readLong(); return(*this);}
    IpcConsumer& operator >> (float       &val) {val = streamMessage->readFloat(); return(*this);}
    IpcConsumer& operator >> (double      &val) {val = streamMessage->readDouble(); return(*this);}
    IpcConsumer& operator >> (std::string &val) {val = streamMessage->readString(); return(*this);}
    IpcConsumer& operator >> (char        *val) {std::string str = streamMessage->readString(); strcpy(val,str.c_str()); return(*this);}

    /********************/
    /********************/


 private:

    /* Called from the consumer since this class is a registered MessageListener (call-back) */
    virtual void onMessage(const Message* message)
    {
      static int count = 0;

	  //std::cout << "received !!!" << std::endl;

      try
      {
        count++;
		// static_cast ??
        /*const StreamMessage* */streamMessage = dynamic_cast<const StreamMessage*> (message);
        if (streamMessage != NULL)
        {

		  //std::cout << "\n\nonMessage: message size="<<sizeof(streamMessage)<< std::endl;
		  //std::cout << "\n\nonMessage: message empty="<<streamMessage->isEmpty()<< std::endl;

          
	 std::string fmt = streamMessage->readString();
          

          /* loop over all listeners and select the one with 'format' */
          for(int i = 0; i < actionListeners.size(); i++)
          {
            //printf("onMessage: actionListener item number %d\n",i);
#if 0
            std::string f = actionListeners[i]->getFormat();
            if( !strncmp(f.c_str(),fmt.c_str(),strlen(f.c_str())) || !strncmp(f.c_str(),"*",strlen(f.c_str())) )
#else
	      if(actionListeners[i]->check(fmt)) 	  
#endif
            {
	      // std::cout << "onMessage: found listener with format '"<<fmt<<"' - processing" << std::endl;

              /* call decoder sending pointer to this class (overloaded '>>' can be used in decoder) */
              actionListeners[i]->decode(*this);

              actionListeners[i]->process();
            }
            else
			{
              //std::cout << "onMessage: does not match listener '"<<f.c_str()<<"' - ignoring" << std::endl;
			}
          }
	    }
        else
        {
          /*printf("NOT A STREAM MESSAGE !\n")*/;
        }
	   
      }
      catch (CMSException& e)
      {
        e.printStackTrace();
      }

      /* Commit all messages if session was opened as 'Session::SESSION_TRANSACTED' */
      session->commit();

    }


    // If something bad happens you see it here as this class is also been
    // registered as an ExceptionListener with the connection.
    virtual void onException(const CMSException& ex AMQCPP_UNUSED)
    {
      printf("CMS Exception occurred.  Shutting down client.\n");
      ex.printStackTrace();
      exit(1);
    }



private:


    void cleanup()
    {
      //printf("IpcConsumer::cleanup() reached\n");
      if (connection != NULL)
      {
        try
        {
          connection->close();
        }
        catch (cms::CMSException& ex)
        {
          ex.printStackTrace();
        }
      }

      // Destroy resources.
      try
      {
        delete destination;
        destination = NULL;
        delete consumer;
        consumer = NULL;
        delete session;
        session = NULL;
        delete connection;
        connection = NULL;
      }
      catch (CMSException& e)
      {
        e.printStackTrace();
      }
    }


};






class IpcServer : public IpcProducer,
                  public IpcConsumer {

private:

  bool inited;


private:

    IpcServer() : IpcProducer(), IpcConsumer(), inited(0)
    {
      //printf("============ IpcServer reached =================\n");
    }

    virtual ~IpcServer()
    {
      //printf("============ ~IpcServer reached ================\n");
    }



public:

    static IpcServer& Instance()
	{
      static IpcServer m_instance;
      return m_instance;
	}

    int Open()
	{
	  /*
      printf("Use following: expid='%s', sesid='%s', sysid_send='%s', unique_send='%s', sysid_recv='%s', unique_recv='%s'\n",
               expid, sesid, sysid_send, unique_send, sysid_recv, unique_recv);
	  */
printf("open 1\n");fflush(stdout);
	  send_init(/*expid, sesid, sysid_send, unique_send*/);
printf("open 2\n");fflush(stdout);
	  recv_init(/*expid, sesid, sysid_recv, unique_recv*/);
printf("open 3\n");fflush(stdout);
      inited = 1;
      return(0);
	}

	/*
    int init(char* expid, char* sesid,
             char* sysid_send, char* unique_send,
             char* sysid_recv = NULL, char* unique_recv = NULL)
	{
      if(sysid_recv==NULL)  sysid_recv = sysid_send;
      if(unique_recv==NULL) unique_recv = unique_send;
      if(inited)
	  {
		std::cout<<"ERROR in IpcServer:init(): already initialized, do nothing"<<std::endl;
        return(-1);
	  }
      else
	  {
	    send_init(expid, sesid, sysid_send, unique_send);
	    recv_init(expid, sesid, sysid_recv, unique_recv);
        inited = 1;
        return(0);
	  }
	}
*/

    int Close()
    {
      if(inited)
	  {
        send_close();
        recv_close();
        inited = 0;
        return(0);
	  }
      else
	  {
		std::cout<<"ERROR in IpcServer:close(): already closed, do nothing"<<std::endl;
        return(-1);
	  }
	}
};









/* functions declarations */

extern "C" {
  int insert_msg(const char *name, const char *facility, const char *process, const char *msgclass, 
	             int severity, const char *msgstatus, int code, const char *text);
}
