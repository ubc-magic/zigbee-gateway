
#ifndef XBEESYSTEM_H
#define XBEESYSTEM_H

#define PYTHON


#include "MagicXBee.h"
#include "XBee.h"
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>


#define MAX_BUFFER 100
#define DEFAULT_SAMPLE_RATE 20000
#define MAX_BUFFER 100
#define MIN_SLEEP_RATE 100
#define MAX_SLEEP_RATE 0xAF0
#define MAX_SAMPLE_RATE 0x65000
#define MIN_SAMPLE_RATE 500 // 500 ms
#define MIN_TIME_UNTIL_SLEEP 10 // forces this to 10ms
#define MAX_TIME_UNTIL_SLEEP 65000
#define DEFAULT_DISCOVERY_TIMEOUT 20000
#define END_DEVICE 2
#define ROUTER 1
#define OK 0

using namespace std; 


class XBeeSystem : public ActiveClass
{
public: 
	XBeeSystem(); // default Constructor 
	XBeeSystem(int _baud, string _serialPort); //  Initializing Constructor
	~XBeeSystem();
	int  initializeNetwork();
	int stop();
	int deleteSensor ( int index);
	string printSensors(); // for debugging in python 
	int getPAN_ID(); // return the pan id of the system 
	int read();// wrap up the process and read functions for higher level of abstraction for python
	void setDiscoveryTimeout(int timeout);
	int getTimeSinceLastSample(int index);

	int ATParamSet( string arg1, string arg2, int timeout =5000);
	int RemoteATSet(int AddrMSB, int addrLSB, string , string , int timeout=5000);
	int RemoteATSet(int index, string, string,int timeout=5000);
	int RemoteATSet(int index, string Command, int Value, int timeout=5000);
	int RemoteATSet(int AddrMSB,int AddrLSB, string Command, int Value, int timeout=5000);
	int RemoteATSetNodeID(int index, string name, int timeout=5000);
	int RemoteATSetNodeID(int AddrMSB, int AddrLSB, string name, int timeout=5000);

	int Write(int index, int timeout=5000);
	int Write(int AddrMSB, int AddrLSB, int timeout);

	vector<uint8_t> RemoteATRead(int AddrMSB, int AddrLSB, string Command,int timeout=5000);
	vector<uint8_t> RemoteATRead(int index,string Command, int timeout =5000);
	int setPAN_ID(int PAN_ID);
	int doNodeDiscovery(int timeout);

	string printAnalog(int index, string delimiter);
	string printDigital(int index,string delimiter);
	Nodes getSensors(); // returns a copy of mySensors
	int changeAnalog(int index, int channel, int max, int min);
	int getNumberSensors(){return mySensors.size();}
	Nodes get_regQ(); // returns the Sensors Q'd to be registered
	void pop_regQ(); // Call this to remove nodes from the Registration Queue after registration

	int changeSleepMode(int index, int mode, int timeout=5000);
	int changeSleepRate(int index, int rate, int timeout=5000);
	int changeTimeUntilSleep(int index, int _timeUntilSleep, int timeout =5000);
	int changeSampleRate(int index,int rate, int timeout=5000);
	int changeName(int index, string name, int timeout=5000);
	int getCoordSN_MSB();
	int getCoordSN_LSB();
	string getCoordName();
	int setDefaultSampleRate(int rate);
	void softCommissionButtonPress(int index, int presses=1);
	bool setBaud(int b);
	bool setSerial(string s);

	int changePinMode(int index, string pinNumber, int value, int timeout=5000);
	int changePinMode(int AddrMSB, int AddrLSB, string pinNumber, int value, int timeout=5000);

	int queryPinMode(int index, string pinNumber, int timeout=5000);
	int queryPinMode(int AddrMSB, int AddrLSB, string pinNumber, int timeout=5000);

	void enableDataLogging(string _fileName);
	bool checkLoggingStatus(){return dataloggingEnabled;}
	int disableDataLogging();
	int sendRxData(int AddrMSB, int AddrLSB, vector<uint8_t> Value, int timeout=5000);
	int sendRxData(int index, vector<uint8_t> Value, int timeout=5000);
	int forceSample(int index,int timeout=5000);

private:
	int dataMapping(int sensorVariable, int in_min, int in_max, float out_min, float out_max);
	void push_regQ(Sensor sensors);
	void NodeDiscovery(XBeeResponse);
	void ProcessAPI(uint8_t API, XBeeResponse);
	bool addNewSensor ( int sensorIndex);
	void ATparameterRead(uint8_t command0, uint8_t command1);
	void ATparameterSet(uint8_t command0, uint8_t command1,uint8_t Value[], uint8_t ValueLength);
	bool RemoteATRequestRead(uint32_t addressMSB, uint32_t addressLSB, uint8_t command0, uint8_t command1, int);
	bool sendRemoteATRequest(uint32_t addressMSB, uint32_t addressLSB, uint8_t command0, uint8_t command1, uint8_t Value[], uint8_t ValueLength);

	bool checkRxResponse(XBeeResponse &Resp, int timeout=5000);
	bool checkRemoteResponse(XBeeResponse &Resp,uint8_t command0,uint8_t command1, int timeout);
	bool checkAtResponse(XBeeResponse &Resp, uint8_t command0, uint8_t command1, int timeout);
	int  getSampleRate(int index, int timeout=5000);
	bool getSampleRates(int);
	int  getSleepInfo(int index, int timeout=10000);
	void applyChanges(int);
	void applyChanges(int, int);
	bool getNodeID(int index, int timeout=5000);
	bool getSleepMode(int index, int timeout=5000 );
	bool getParentAddress(int index, int timeout=10000);
	bool getNumberOfChildren(int index,int timeout=5000);
	void printMsgToFile(string);
	void printSensorReadingToFile(int index);
	bool openFile();

	virtual int main(); // XBee System's Active Thread that handles consuming packets
	int lock(); // This Locks Certain Public Functions invoked by the user so that the active thread doesn't wrongly consumer a packet to soon
	int release();// Release the Lock over Certain public Functions and resume the active thread

	bool coordinatorOnLine; // Flag showing status of coordinator (active or inactive)
	stringstream msg; // global string stream variable that gets reused to pass message strings  to save memory
	CMutex readerMutex; // Mutex Protecting the User Invoked functions from access by Active Thread invoked Functions
	CMutex logMutex; // Mutex Protecting Reading and Writing LogFile
	Coordinator *coord; // coord object, only accessible by Xbee System  
	vector<Sensor> regQ; //  Queue of Newly Discovered sensors to be registered on sense Tecnic
	Sensor sensor; // Sensor object that Gets reused to save memory
	int sensorCount; // Keeps track of number of sensors registered on network
	int baud; // the Baud Rate Required by the Serial Port
	string serialPort; // String containing the Serial Port Being used ie: "dev/ttyUSB0"
	vector<Sensor> mySensors; // List of Sensors Registered On the Network
	unsigned long timer; // Timer Variable that gets re-used often
	uint8_t irValue[2]; // Commonly Used Variable for sending commands
	uint8_t command[2]; // Commonly Used variable for 2 character Commands
	uint8_t API; // Stores the API ID of a resonse
	bool Exit; // Exit Flag to Allow Reader Thread To terminate
	int defaultSampleRate; // Set the Default Sample Rate (in millis) if a requested Sample Rate out of bounds

	/*All Different Types of Requests and Response Variables used in the system
	 * These will Be Re-used to save memory at run-time
	 */
	AtCommandRequest atRequest; // AT Command Requests (Requests sent to Coordinator)
	RemoteAtCommandRequest remoteAtRequest; //Remote AT Requests (Sent to Remote XBee's via Coordinator)
	RemoteAtCommandResponse remoteAtResponse; // Remote AT Responses from Remote XBees (in response to requests)
	AtCommandResponse atResponse; // AT Reponses From Coord ( In response to AT Requests)
	XBeeResponse Resp; // Generic Response Variable that Gets Re-used Often for Processing Generic Requests
	// Before they are Parsed into specific Requests

	ModemStatusResponse modemStatus; // Modem Status Response (when Coord is powered on)
	ZBRxIoSampleResponse ioSample; // Response Used for incoming IO sample Responses
	ZBRxResponse rxMsg;  // Response used when Coordinator Recieves a Serial Transmission
	ZBTxStatusResponse txMsg; // Response Used when A Coordinator Sends a Serial Transmission to a Remote XBee
	ZBRxIoIdentifierResponse nodeIDResponse; // A Type of Response Given when A New Node Joins the Network ( Via Power On or Commission)

	int discoveryTimeout; // the Amount of time Spent Rigourously Discoverying New Sensors ( Blocks all other Functions)
	bool dataloggingEnabled; // a Flag denoting whether the user wants to enable data logging or not
	ofstream logFile; // the output control file
	string fileName; // sets the file name used for data logging
	bool readerExitedOk; // a flag to check that the Internal Active Thread exits Ok and has returned
	bool getCoordStatus(){return coordinatorOnLine;} // get the status of the coordinator


};

#endif
