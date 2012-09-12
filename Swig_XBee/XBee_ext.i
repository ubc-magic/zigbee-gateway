/* example.i */
 %module XBee_ext
 %{
 
 /* Put header files here or function declarations like below */
 
 
 
 //#include "CSerial.h"
 #include "XBee.h"
 //#include "MagicXBee.h"
 #include "XBeeSystem.h"
 
 %}
%include "std_string.i"
%include "std_vector.i"
%include "exception.i"

typedef unsigned int uint32_t;
typedef unsigned short uint16_t; 
typedef unsigned char uint8_t;

unsigned long GetTickCount();

double getMillisecondTime();


class XBeeData{
public:
	XBeeData();
	~XBeeData();
	std::string Name; // Name of the XBee 
		double lastSample;
        uint32_t SN_High; // Serial Number MSB 
        uint32_t SN_Low;// Serial Number LSB
        uint16_t Address16; // 16Bit Network Address
        uint16_t DigitalMask; // Digital Channel Mask 
        uint8_t AnalogMask; // Analog Channel Mask
        uint16_t digital_data; // Digital Data (each bit represents our 8 possible channels
        vector<int> analog_data;// Analog data  (up to 4 channels )
        int deviceType;  // Device Type Coord =0, Router= 1, End Device = 2; 
        uint16_t parentAddress; // Address of the Parent
        int sampleRate; 
        int sleepRate; 
        int timeUntilSleep; 
        int numberOfChildren; // Number of children the device has 
        vector<int> minimum; 
        vector<int>maximum; 
	vector<uint8_t>rxData; // the RX data will be stored  in a vector
       //uint8_t powerMode; // Keep Track of Power Mode 
	double timeSinceLastSample;
	long joined;
	int rxDataSize(){return rxData.size();} // used for querying if there is any rx data 
		//before trying to read.. otherwise program will halt!
};

class ActiveClass {
public:
	ActiveClass(){};
	~ActiveClass(){};
	bool Resume();
	
	void WaitForThread();//{(void) pthread_join(thread1,NULL);} // wait until prthread join
	
	virtual int main() =0; // This is be overridden, declared virtual main 
	
	// this is needed to create an entry point to start the thread that then runs the function main 
	static void * threadEntry(void *This);
	
};

class Node
{
public:
	Node();
	~Node();
	XBeeData data; 
	//bool operator==(Node const& n) const;
};

namespace std {
  %template(NodeVector) vector<Node>;
  %template(IntVector) vector<int>;
  %template(RxData) vector<uint8_t>;
}

class Nodes
{
public:
	Nodes();
	~Nodes();
	vector<Node> nodes;
	int size();
};

%newobject XBeeSystem;
%newobject Coordinator;
%newobject XBeeResponse;
%newobject XBeeSystem::operator=;
%newobject XBeeResponse::setFrameData;
%newobject ZBRxIoIdentifierResponse;

%newobject AtCommandRequest;
%newobject RemoteAtCommandRequest;
%newobject remoteAtResponse;
%newobject atResponse;
%newobject ModemStatusResponse;
%newobject	ZBRxIoSampleResponse;
%newobject	ZBRxResponse;
%newobject	ZBTxStatusResponse;
%newobject	ZBRxIoIdentifierResponse;

%newobject XBeeSystem::ATParamSet;
%newobject XBeeSystem::RemoteATSet;
%newobject XBeeSystem::RemoteATSetNodeID;
%newobject XBeeSystem::sendRxData;


%include "XBeeSystem.h"
