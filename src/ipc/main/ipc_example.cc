/* Sergey: copied from activemq-cpp-library-3.8.2/src/example/main/cpp
and modified slightly, use it to test producer-broker-consumer
communication; broker must be running on $IPC_HOST machine */

/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// START SNIPPET: demo

#include <activemq/library/ActiveMQCPP.h>
#include <decaf/lang/Thread.h>
#include <decaf/lang/Runnable.h>
#include <decaf/util/concurrent/CountDownLatch.h>
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
#include <cms/ExceptionListener.h>
#include <cms/MessageListener.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <memory>

/*sergey*/
#include <cms/Destination.h>


using namespace activemq::core;
using namespace decaf::util::concurrent;
using namespace decaf::util;
using namespace decaf::lang;
using namespace cms;
using namespace std;




/* Producer */

class IpcProducer : public Runnable {
private:

    Connection* connection;
    Session* session;
    Destination* destination;
    MessageProducer* producer;
    int numMessages;
    bool useTopic;
    bool sessionTransacted;
    std::string brokerURI;

private:

    IpcProducer(const IpcProducer&);
    IpcProducer& operator=(const IpcProducer&);

public:

    IpcProducer(const std::string& brokerURI, int numMessages, bool useTopic = false, bool sessionTransacted = false) :
        connection(NULL),
        session(NULL),
        destination(NULL),
        producer(NULL),
        numMessages(numMessages),
        useTopic(useTopic),
        sessionTransacted(sessionTransacted),
        brokerURI(brokerURI) {
    }

    virtual ~IpcProducer(){
        cleanup();
    }

    void close() {
        this->cleanup();
    }

    virtual void run() {

        try {

		  /* step 1: Create a ConnectionFactory (broker's URI specified here: protocol(TCP etc), IP address, port, optionally other params) */
		  /*    username and password can be specified, for example:
				  createCMSConnectionFactory( "tcp://127.0.0.1:61616?username=${USERNAME}&password=${PASSWORD}" ) */
          unique_ptr<ConnectionFactory> connectionFactory(ConnectionFactory::createCMSConnectionFactory(brokerURI));

		  /* step 2: Create a Connection */
          /*    username and password can be specified, for example: connectionFactory->createConnection( "<USERNAME>", "<PASSWORD>") */
          connection = connectionFactory->createConnection();
          connection->start();

            // Create a Session
            if (this->sessionTransacted) {
                session = connection->createSession(Session::SESSION_TRANSACTED);
            } else {
                session = connection->createSession(Session::AUTO_ACKNOWLEDGE);
            }

            // Create the destination (Topic or Queue)
            if (useTopic) {
			  destination = session->createTopic("TEST.FOO"/*"HEART-BEAT-CHANNEL"*/);
            } else {
                destination = session->createQueue("TEST.FOO"/*"HEART-BEAT-CHANNEL"*/);
            }

            // Create a MessageProducer from the Session to the Topic or Queue
            producer = session->createProducer(destination);
            producer->setDeliveryMode(DeliveryMode::NON_PERSISTENT);

            // Create the Thread Id String
            string threadIdStr = Long::toString(Thread::currentThread()->getId());

            // Create a messages
            string text = (string) "Hello world! from thread " + threadIdStr;

            for (int ix = 0; ix < numMessages; ++ix) {
                std::unique_ptr<TextMessage> message(session->createTextMessage(text));
                message->setIntProperty("Integer", ix);
                printf("Sent message #%d from thread %s\n", ix + 1, threadIdStr.c_str());
                producer->send(message.get());
            }

        } catch (CMSException& e) {
            e.printStackTrace();
        }
    }

private:

    void cleanup() {

        if (connection != NULL) {
            try {
                connection->close();
            } catch (cms::CMSException& ex) {
                ex.printStackTrace();
            }
        }

        // Destroy resources.
        try {
            delete destination;
            destination = NULL;
            delete producer;
            producer = NULL;
            delete session;
            session = NULL;
            delete connection;
            connection = NULL;
        } catch (CMSException& e) {
            e.printStackTrace();
        }
    }
};













/* Consumer */


class IpcConsumer : public ExceptionListener,
                           public MessageListener,
                           public Runnable {

private:

    CountDownLatch latch;
    CountDownLatch doneLatch;
    Connection* connection;
    Session* session;
    Destination* destination;
    MessageConsumer* consumer;
    long waitMillis;
    bool useTopic;
    bool sessionTransacted;
    std::string brokerURI;

private:

    IpcConsumer(const IpcConsumer&);
    IpcConsumer& operator=(const IpcConsumer&);

public:

    IpcConsumer(const std::string& brokerURI, int numMessages, bool useTopic = false, bool sessionTransacted = false, int waitMillis = 30000) :
        latch(1),
        doneLatch(numMessages),
        connection(NULL),
        session(NULL),
        destination(NULL),
        consumer(NULL),
        waitMillis(waitMillis),
        useTopic(useTopic),
        sessionTransacted(sessionTransacted),
        brokerURI(brokerURI) {
    }

    virtual ~IpcConsumer() {
        cleanup();
    }

    void close() {
        this->cleanup();
    }

    void waitUntilReady() {
        latch.await();
    }

    virtual void run() {

        try {

            // Create a ConnectionFactory
            unique_ptr<ConnectionFactory> connectionFactory(
                ConnectionFactory::createCMSConnectionFactory(brokerURI));

            // Create a Connection
            connection = connectionFactory->createConnection();
            connection->start();
            connection->setExceptionListener(this);

            // Create a Session
            if (this->sessionTransacted == true) {
                session = connection->createSession(Session::SESSION_TRANSACTED);
            } else {
                session = connection->createSession(Session::AUTO_ACKNOWLEDGE);
            }

            // Create the destination (Topic or Queue)
            if (useTopic) {
			  destination = session->createTopic("TEST.FOO"/*"HEART-BEAT-CHANNEL"*//*"*"*/);
            } else {
			  destination = session->createQueue("TEST.FOO"/*"HEART-BEAT-CHANNEL"*//*"*"*/);
            }

            // Create a MessageConsumer from the Session to the Topic or Queue
            consumer = session->createConsumer(destination);

            consumer->setMessageListener(this);

            std::cout.flush();
            std::cerr.flush();

            // Indicate we are ready for messages.
            latch.countDown();

            // Wait while asynchronous messages come in.
            doneLatch.await(waitMillis);

        } catch (CMSException& e) {
            // Indicate we are ready for messages.
            latch.countDown();
            e.printStackTrace();
        }
    }




    /* Called from the consumer since this class is a registered MessageListener (call-back) */
    virtual void onMessage(const Message* message) {

        static int count = 0;

        try {
            count++;
            const TextMessage* textMessage = dynamic_cast<const TextMessage*> (message);
            string text = "";

            if (textMessage != NULL) {
                text = textMessage->getText();
            } else {
                text = "NOT A TEXTMESSAGE!";
            }

            printf("Message #%d Received: %s\n", count, text.c_str());

        } catch (CMSException& e) {
            e.printStackTrace();
        }

        // Commit all messages.
        if (this->sessionTransacted) {
            session->commit();
        }

        // No matter what, tag the count down latch until done.
        doneLatch.countDown();
    }

    // If something bad happens you see it here as this class is also been
    // registered as an ExceptionListener with the connection.
    virtual void onException(const CMSException& ex AMQCPP_UNUSED) {
        printf("CMS Exception occurred.  Shutting down client.\n");
        ex.printStackTrace();
        exit(1);
    }

private:

    void cleanup() {
        if (connection != NULL) {
            try {
                connection->close();
            } catch (cms::CMSException& ex) {
                ex.printStackTrace();
            }
        }

        // Destroy resources.
        try {
            delete destination;
            destination = NULL;
            delete consumer;
            consumer = NULL;
            delete session;
            session = NULL;
            delete connection;
            connection = NULL;
        } catch (CMSException& e) {
            e.printStackTrace();
        }
    }
};





/* main */

int
main(int argc AMQCPP_UNUSED, char* argv[] AMQCPP_UNUSED)
{
  char *broker_host = NULL;

  broker_host = getenv("IPC_HOST");
  if(broker_host==NULL)
  {
    printf("Cannot get broker_host name  - exit\n");
    exit(0);
  }
  printf("Will be looking for broker on host >%s<\n",broker_host);

    activemq::library::ActiveMQCPP::initializeLibrary();
    {
    std::cout << "=====================================================\n";
    std::cout << "Starting the example:" << std::endl;
    std::cout << "-----------------------------------------------------\n";


    // Set the URI to point to the IP Address of your broker.
    // add any optional params to the url to enable things like
    // tightMarshalling or tcp logging etc.  See the CMS web site for
    // a full list of configuration options.
    //
    //  http://activemq.apache.org/cms/
    //
    // Wire Format Options:
    // =========================
    // Use either stomp or openwire, the default ports are different for each
    //
    // Examples:
    //    tcp://127.0.0.1:61616                      default to openwire
    //    tcp://127.0.0.1:61616?wireFormat=openwire  same as above
    //    tcp://127.0.0.1:61613?wireFormat=stomp     use stomp instead
    //
    // SSL:
    // =========================
    // To use SSL you need to specify the location of the trusted Root CA or the
    // certificate for the broker you want to connect to.  Using the Root CA allows
    // you to use failover with multiple servers all using certificates signed by
    // the trusted root.  If using client authentication you also need to specify
    // the location of the client Certificate.
    //
    //     System::setProperty( "decaf.net.ssl.keyStore", "<path>/client.pem" );
    //     System::setProperty( "decaf.net.ssl.keyStorePassword", "password" );
    //     System::setProperty( "decaf.net.ssl.trustStore", "<path>/rootCA.pem" );
    //
    // The you just specify the ssl transport in the URI, for example:
    //
    //     ssl://localhost:61617
    //

    std::string brokerURI =
        "failover:(tcp://clon00:61616"
        ")";


    //============================================================
    // set to true to use topics instead of queues
    // Note in the code above that this causes createTopic or
    // createQueue to be used in both consumer an producer.
    //============================================================
    bool useTopics = true;
    bool sessionTransacted = false;
    int numMessages = 2000;

    long long startTime = System::currentTimeMillis();

    cout << "Creating producer" << endl;
    IpcProducer producer(brokerURI, numMessages, useTopics);

    cout << "Creating consumer" << endl;
    IpcConsumer consumer(brokerURI, numMessages, useTopics, sessionTransacted);

    cout << "Starting the consumer thread" << endl;
    Thread consumerThread(&consumer);
    consumerThread.start();

    cout << "Wait for the consumer to indicate that it is ready to go" << endl;
    consumer.waitUntilReady();

    cout << "Starting the producer thread" << endl;
    Thread producerThread(&producer);
    producerThread.start();





    cout << "Waiting for the threads to complete" << endl;
    producerThread.join();
    consumerThread.join();

    long long endTime = System::currentTimeMillis();
    double totalTime = (double)(endTime - startTime) / 1000.0;

    consumer.close();
    producer.close();

    std::cout << "Time to completion = " << totalTime << " seconds." << std::endl;
    std::cout << "-----------------------------------------------------\n";
    std::cout << "Finished with the example." << std::endl;
    std::cout << "=====================================================\n";

    }
    activemq::library::ActiveMQCPP::shutdownLibrary();
}

// END SNIPPET: demo
