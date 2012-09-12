

#include "MagicXBee.h"

Coordinator::Coordinator():XBee()
{ 
	printf("Initializing Coord...\n");
	exit =false;
	Name="";
	PANID=0;
	coordinatorExitedOk=false;
	uint32_t SN_High =0;
	uint32_t SN_Low=0;
	try
	{
       //uint8_t powerMode; // Keep Track of Power Mode
		bufferMutex = new CMutex("bufferMutex"); // mutex for locking the Buffer Resource
		serialMutex = new CMutex("serialMutex"); // mutex for locking the Serial Port Resource
		
	}
	catch ( bad_alloc &memmoryAllocationException )
   {
      cout << "Memory allocation exception occurred: "
           << memmoryAllocationException.what()
           << endl;
	  cout  << "Could Not create new Coordinator In memory"<<endl;
	}
} 

Coordinator::~Coordinator(){
	 Exit(); // exit the active thread
	 delete serialMutex; // clean up serial Mutex
	 delete bufferMutex; // clean up Buffer Mutex
	} // must delete the data pointer to prevent leaks


void Coordinator::Exit()
{exit =true;
}
void Coordinator::setName(string _name)
{
	Name =_name;
}
void Coordinator:: setSerialNumber(uint32_t MSB, uint32_t LSB)
{
	SN_High = MSB;
	SN_Low = LSB;
}
void Coordinator:: setPAN_ID(uint16_t PAN_ID)
{
 PANID = PAN_ID; 
}


uint16_t Coordinator::getPAN_ID()
{ 
	return PANID; 
} 



int Coordinator::coordThread(void *args)
{
	/*This thread/function polls the serial port to see if a packet is available
	 * the polling is timed to prevent starvation
	 * if no packet is available at after timeout occurs, the function returns false and
	 * we put the thread to sleep for a short period of time
	 * if serial data is available, one API frame packet is consumed and the function
	 * returns True
	 *
	 * if a Valid packet is read, we check if it is a valid response,
	 * if A valid response then we push it into one of the two buffers
	 *
	 * Buffers are Protected By a Mutex to prevent Multiple threads accessing or modifying
	 * at the same time. This allows protection of resource
	 * Also this allows the Consumer/reader Thread to Sleep when no packet is available, thus reducing CPU time
	 *
	 * We have 2 buffers to assign priority to AT Command Reponses over any other type of response
	 * Because Receiving Command Responses in a timely manner is Critical for Feedback and Reliability
	 *
	 *
	 */
		bool packetAvailable=false;
	// Serial Mutex locks the Serial Port Resource shared between process threads
		serialMutex->Wait();
		packetAvailable = readPacket(1);// timeout in 1 milliseconds to prevent starvation!!
		serialMutex->Signal();

		if(packetAvailable) // if a response packet was read check the response
		{
			//producerSemaphore->Wait();
			if(this->getResponse().isAvailable()) // only put into buffer if packet read was valid response
			{
				printf("%sCoordinator Recieved A Packet with the API ID %X %s\n",green,this->getResponse().getApiId(),normal);
					if(getResponse().getApiId()==AT_COMMAND_RESPONSE || getResponse().getApiId() ==REMOTE_AT_COMMAND_RESPONSE) // only if AT command
					{ 
							bufferMutex->Wait();
							atBuffer.push(getResponse());
							bufferMutex->Signal();
					}
					else // Any other Response is pushed into this buffer
					{

							bufferMutex->Wait();
								inputBuffer.push(getResponse());
							bufferMutex->Signal();

					}

			      // signal the consumer that there is a packet in the buffer
			}
			//consumerSemaphore->Signal();
		}
	else
		{
			Sleep(1); // if no packet read, put the thread to sleep for a short time
			// prevents overloading the CPU when polling
		}

	return 0;
	}

int Coordinator::main(void)
{ 
	// Creates a thread that will run the coordinator as active object
	// This function calls coordThread() helper function
	 coordinatorExitedOk=false;
	 
	 Sleep(10);
	 // Runs in continuouse a loop until exit is requested
	 while(exit==false)
	 {
		//Sleep(10);
	    this->coordThread(NULL);
	 }
	 WaitForThread();
	 coordinatorExitedOk = true;


return 0; 
}

// END COORD;

Sensor :: Sensor()
{ 

	try
	{
		data=XBeeData();
	}
	
	catch ( bad_alloc &memmoryAllocationException )
   {
      cout << "Memory allocation exception occurred: "
           << memmoryAllocationException.what()
           << endl
	       << "Could Not create new Sensor Node In memory"<<endl; 
	} 
}









