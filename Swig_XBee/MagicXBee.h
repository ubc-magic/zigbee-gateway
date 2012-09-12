
#ifndef MagicXBee_h
#define MagicXBee_h

/*
 * Khol H
 * This Library Defines  Objects and functions that will be used by XBee System
 * Some Objects such as CMutex and CSemaphore server as Wrappers for some Win32 Objects
 * included in "rt.h"
 * The Original Idea was to have this library portable on both systems.
 *
 *
 */

#include <time.h>
#include <vector>
#include <list>
#include <queue>
#include "XBee.h"
//#define __linux__

const char *const green = "\033[0;32;1m"; // sets font colour  to for coordinator thread
const char *const normal = "\033[0m"; // Sets Font Colour back to normal (white)

#ifdef _WIN32  
#include "rt.h"

#endif
#if defined ( __linux__)
// If UNIX System, use pthreads for mutexes and threads
// Need Wrapper for Sleep Function 

#include <pthread.h> // threads
#include <semaphore.h> // Semaphore
#include "CSerial.h"
#include "XBee.h"



// Wrap the Sleep(Millis) Function to match Win32 Call


// Wrapper to translate binary POSIX Sempaphore into CMutex (rt.h)
// creates a layer of abstraction for creating and destroying Semaphores.


class CMutex
{
private: 
	sem_t myMutex;
	string name;
public:
	CMutex(){};
	CMutex(string _name){sem_init(&myMutex, 0, 1); name =_name;}
	~CMutex(){sem_destroy(&myMutex);}
	int Wait()
	{
		if(sem_wait(&myMutex) !=0)
		{
			printf("%sError Decrementing Mutex %s %s\n", green,name.c_str(),normal);
			return -1;
		}
		return 0;
	}
	int Signal()
	{
		if(sem_post(&myMutex)!=0)
			{
			printf("%sError Signalling Mutex %s %s\n",green, name.c_str(),normal);
			return -1;
			}
		return 0;
	}
};

class CSemaphore // Wrapper for creating and destroying Counting Semaphores
// Wraps semaphore objects into CSemaphore Objects (rt.h on Win32)
{
private:
	sem_t mySemaphore;
	string name;
public:
	CSemaphore() {};
	CSemaphore(string _name,int initialValue)
	{
		name = _name;
		sem_init(&mySemaphore,0,initialValue);
	}
	~CSemaphore(){sem_destroy(&mySemaphore);}

	int Wait()
	{
		if(sem_wait(&mySemaphore)!=0)
		{
			printf("%sError Decrementing Semaphore %s %s\n", green,name.c_str(),normal);
			return -1;
		}
	}

	int Signal()
	{
		if(sem_post(&mySemaphore)!=0)
		{
			printf("%sError Signalling Semaphore %s %s\n", green,name.c_str(),normal);
			return -1;
		}
		return 0;
	}


};
#endif


using namespace std; 


#define MAX_BYTES 100// Maximum Bytes in an API frame, determined by the XBee serial buffers 
#define BROADCAST_ADR_MSB 0x00000000
#define BROADCAST_ADR_LSB 0x0000ffff
#define MAX_IO_RESPONSES 40 // defines the maximum buffer size

/* Modified this to be a class instead of a struct strictly for use with Boost module
 * XBee Data is a class so that we can control memory allocation and Memory reserves for better
 * memory management
 */
class XBeeData{
public:
	XBeeData(){

		Name=""; // Name of the XBee
		SN_High=0; // Serial Number MSB
		SN_Low=0;// Serial Number LSB
		Address16=0; // 16Bit Network Address
		DigitalMask=0; // Digital Channel Mask
		AnalogMask=0; // Analog Channel Mask
		digital_data=0; // Digital Data (each bit represents our 8 possible channels

		deviceType=0;  // Device Type Coord =0, Router= 1, End Device = 2;
		parentAddress=0; // Address of the Parent
		sampleRate=0;
		numberSleepCycles=0;
		sleepRate=0;
		timeUntilSleep=0;
		numberOfChildren=0; // Number of children the device has


		lastSample =0;
		timeSinceLastSample=0;
		analog_data.reserve(4);
		maximum.reserve(4);
		minimum.reserve(4);
		analog_data.assign(4,0);
		maximum.assign(4,0);
		minimum.assign(4,0);
		rxData.reserve(72); // maximum potential payload per RX packet is 72 bytes
		// likely readings will be far fewer that 72, but using the max for flexibility 
	}
	~XBeeData(){maximum.clear(); minimum.clear(); rxData.clear(); analog_data.clear();}
		// Destructor forces Cleanup of Vector Classes


		string Name; // Name of the XBee 
        uint32_t SN_High; // Serial Number MSB 
        uint32_t SN_Low;// Serial Number LSB
        uint16_t Address16; // 16Bit Network Address
        uint16_t DigitalMask; // Digital Channel Mask 
        uint8_t AnalogMask; // Analog Channel Mask
        uint16_t digital_data; // Digital Data (each bit represents our 8 possible channels
        vector<int> analog_data;// Analog data  (up to 4 channels )
        int deviceType;  // Device Type Coord =0, Router= 1, End Device = 2; 
        uint16_t parentAddress; // Address of the Parent
        int sampleRate; // The sample Rate of Sensor
        int numberSleepCycles; // Total Sleep = numberSleepCycles*sleepRate
        int sleepRate; 
        int timeUntilSleep; // Time Node is awake until it sleeps again
        int numberOfChildren; // Number of children the device has 
        vector<int> minimum; 
        vector<int>maximum; 
		vector<uint8_t>rxData; // the RX data will be stored  in a vector
       //uint8_t powerMode; // Keep Track of Power Mode 
		double timeSinceLastSample;
		double lastSample; // Keeps track of time between Samples
		long joined; // time the sensor Joined The Network

		int rxDataSize(){return rxData.size();} // used for querying if there is any rx data 
		//before trying to read.. otherwise program will halt!
};





#if defined(__linux__) 
/*This Class Acts as a Wrapper for Active Class Object from (rt.h) on Win32
 * If compiling in linux , we create a super class that creates an active class
*object using pthreads that is called just like Active Class primitives
*
*If a Class/object Inherits from an ActiveClass, It will have a virtual Method Called Main
* that Allows the object to create a private thread that can invoke private methods
* of that class. The class will have an active thread running from inside it
* This is my favorite way of creating threads.
* Hence the name Active Objects/Active Classes
*
* All the pthread primitives are handled by this class to make things simpler
* This is my favorite way of creating threads.
*
 */

class ActiveClass {
public:
	ActiveClass(){};//pthread_attr_init(&threadAttributes);
	/* call an appropriate functions to alter a default value */
	//pthread_attr_setschedpolicy(&threadAttributes,SCHED_RR);};
	virtual ~ActiveClass(){};
	
	bool Resume()
	{
		pthread_attr_init(&threadAttributes);
		/* call an appropriate functions to alter a default value */
		pthread_attr_setschedpolicy(&threadAttributes,SCHED_RR); // Set round robin scheduling policy
		// a call to this creats the thread in an active state
		return (pthread_create(&thread1, &threadAttributes,threadEntry,(void*)this));
		//return (pthread_create(&thread1, &threadAttributes,threadEntry,this));
	}
	
	int WaitForThread(){
		 if(pthread_join(thread1,NULL)==0)
		 	 return 1;
		 return 0;
		//pthread_attr_destroy(&threadAttributes);
	} // wait until prthread join
	
	virtual int main() =0; // This is to be overridden, virtual main
	
	// this is needed to create an entry point to start the thread that then runs the function main 
	static void * threadEntry(void *This)
	{
		static_cast<ActiveClass *>(This)->main();
		//((ActivePthreadClass *)This)->main();// this also works for class thread
		return NULL;
	}
	
protected:
	pthread_t thread1;	
	pthread_attr_t threadAttributes;
};

/*
 * Coordinator Inherits From XBee and Active Class
 * It will be able to read/send commands directly to the XBee Module on Serial Port
 * It also contains an active thread to handle all the reading functions automatically
 * All reads/and sends will be invoked through this object
 */
class Coordinator : public XBee,  public ActiveClass
{

public: 
	Coordinator();
	~Coordinator();

	void Exit();

	 // potentially mark these as private
	void setName(string _name);
    void setSerialNumber(uint32_t MSB, uint32_t LSB);
	void setPAN_ID(uint16_t PAN_ID); 


	// public helper functions 
	uint16_t getPAN_ID(); // returns the PAN_ID of the Network
	
	friend class XBeeSystem;// XBee System Must be freind of this class
	
private: 
	string Name; // Coordinator Name
	uint16_t PANID; // PAN ID of the Network
	int coordThread(void *args); // Private Thread Function for reading
	virtual int main(void); // Main Thread Function
	bool exit; // exit flag
	bool coordinatorExitedOk; // Signals that Thread terminated Ok
	uint32_t SN_High; //MSB  SN of Coordinator
	uint32_t SN_Low; //LSB SN of Coordinator

	// We use 2 Buffers so we can assign priority over AT and REMOTE AT command Responses
	// Everything else is put into input Buffer
	queue <XBeeResponse> atBuffer; // AT Command Response Buffer
	queue <XBeeResponse> inputBuffer; // Other Responses (TX and IO Responses)

	CMutex *bufferMutex; // Mutex Protecting access to Buffers
	CMutex *serialMutex;// Mutex Protecting Access to Serial Port
	
};
#endif

/*
 * This class Defines any Sensor registered on the system
 * Registered Sensors are placed into a Vector of Sensors
 */
class Sensor {
public: 
	Sensor();
	~Sensor(){};

	XBeeData data;
	bool operator==(Sensor const& s) const { return data.SN_Low == s.data.SN_Low; } // operator needed for use with boost template classes
	 
	
};



// The Node Class is explicitly created for Python use only, 
// It simple acts as a container type for XBee Sensor Nodes that can be indexed via python

class Node
{
public:
	Node()
	{ data = XBeeData();
	for(int i=0; i<4; i++)
	{
		data.analog_data[i] =0;// Analog data  (up to for channels )
		data.minimum[i] =0;
		data.maximum [i]=0;
	}
	}
	~Node(){};
	XBeeData data; 
	bool operator==(Node const& n) const{return (n.data.Name == data.Name && n.data.SN_High ==data.SN_High && n.data.SN_Low ==data.SN_Low);} 
};

// Class Nodes now contains the indexible vector of Nodes	
class Nodes
{
public:
	Nodes(){};
	~Nodes(){nodes.clear();}
	vector<Node> nodes;
	int size(){return nodes.size();}
};

#endif

