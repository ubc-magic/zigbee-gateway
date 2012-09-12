


#include "XBeeSystem.h"
//#include "XBee.h"
XBeeSystem::XBeeSystem()
{ 
	Exit =0;
	atResponse = AtCommandResponse();
	modemStatus =ModemStatusResponse();
	ioSample =ZBRxIoSampleResponse(); // Used for IO samples
	rxMsg =ZBRxResponse();
	remoteAtResponse =RemoteAtCommandResponse();
	//nodeIDResponse = ZBRxIoIdentifierResponse();
	Resp= XBeeResponse();

	readerMutex=CMutex("ReaderMutex");
	logMutex=CMutex("LogMutex");
	sensorCount =0;
	dataloggingEnabled=false;
	mySensors.clear(); // Clears the Sensor vector
	atRequest = AtCommandRequest(command); // Initialize atCommandRequest. gets reused to save memory
	XBeeAddress64 addr = XBeeAddress64(0,0xffff); // need this to initialize RemoteATCommandReq, othewise doesn't work
	remoteAtRequest = RemoteAtCommandRequest(addr,command, irValue,sizeof(irValue)); // must initialize RemoeAtCommandRequest inorder to work
	defaultSampleRate=DEFAULT_SAMPLE_RATE;
	coordinatorOnLine=false;
	try
	{
	coord = new Coordinator(); // create new Coordinator Object (Active class)
	}
	catch (bad_alloc &memmoryAllocationException )
   {
      cout << "Memory allocation exception occurred: "
           << memmoryAllocationException.what()
           << endl<< "Could Not create new Coordinator In memory"<<endl;
	}
} 

XBeeSystem::XBeeSystem(int _baud, string _serialPort )// Take in the Serial Baud Rate and the Com port
{ 
	Exit=0;
	atResponse = AtCommandResponse();
	modemStatus =ModemStatusResponse();
	ioSample =ZBRxIoSampleResponse(); // Used for IO samples
	rxMsg =ZBRxResponse();
	remoteAtResponse =RemoteAtCommandResponse();
	//nodeIDResponse = ZBRxIoIdentifierResponse();
	Resp= XBeeResponse();

	dataloggingEnabled=false;
	sensorCount =0;
	readerMutex=CMutex("ReaderMutex");
	logMutex=CMutex("LogMutex");
	setDiscoveryTimeout(DEFAULT_DISCOVERY_TIMEOUT);
	atRequest = AtCommandRequest(command); // must Initialize
	XBeeAddress64 addr = XBeeAddress64(0,0xffff); // need this to initialize RemoteAtCommand
	remoteAtRequest = RemoteAtCommandRequest(addr,command, irValue,sizeof(irValue)); // must initialize RemoteAtCommandRequest in order to work
	defaultSampleRate=DEFAULT_SAMPLE_RATE;
	coordinatorOnLine=false;
	//  Need a safety check here for serial port availability
	// TODO  throw python errors instead of C-errors
	if(!setBaud(_baud))
		throw -1;
	//	Need a safety check here 
	if(!setSerial(_serialPort))
		throw -2;
	try
	{
		coord = new Coordinator; // create new Coordinator Object (Active class)
	}
	catch ( bad_alloc &memmoryAllocationException )
   {
		 cout << "Memory allocation exception occurred: "
           << memmoryAllocationException.what()
           << endl<< "Could Not create new Coordinator In memory"<<endl;
	}	
	catch(int exception)
	{
		if(exception =-1)
			printf("Error, Invalid Serial Port %s ...\n", _serialPort.c_str());
		else if(exception =-2)
			printf("Error, Invalid Serial Port %s ...\n", _serialPort.c_str());
	}
} 

XBeeSystem::~XBeeSystem()
	 {		while(stop()!=1);
	 	 	//while(readerExitedOk==false); // wait for the reader thread to stop
	 		//while(coord->coordinatorExitedOk==false)
	 		 	regQ.clear();
	 		 	mySensors.clear();
	 		 	delete coord;
	 } // Destructor


int XBeeSystem::stop()
	{
		Exit=true;
		coord->Exit();
		disableDataLogging();
		if(coord->coordinatorExitedOk==true&& readerExitedOk==true)
			return 1;
		else
			return 0;
	}
void XBeeSystem::ATparameterRead(uint8_t command0, uint8_t command1)
{
  // Method: Send an AT Request API Packet to Query the Coordinator for a Value 
  // Does Not Set A Value 
  // Only for an XBee Device that is directly connected Via Serial to the Arduino
	// Command0 = First ASCII Char representing an AT REQUEST CODE
	  // Command1 = Second ASCII Char represeting an AT REQUEST CODE
	//  example a request to read the Pan ID = OP-> command0 = O, command1=P
   
	command [0]=command0;
	command [1]= command1;
	atRequest.clearCommandValue();
	atRequest.setCommand(command);

	coord->serialMutex->Wait();
	coord->send(atRequest);
	coord->serialMutex->Signal();
  
}

void XBeeSystem::ATparameterSet(uint8_t command0, uint8_t command1, uint8_t Value[], uint8_t ValueLength)
{
  // Method: Send an AT Request API Packet to the Coordinator to Set a Value 
  // Sets A new Value based on value[]. 
  // Only for an XBee Device that is directly connected Via Serial to the Arduino
  // Command0 = First ASCII Char representing an AT REQUEST CODE 
  // Command1 = Second ASCII Char represeting an AT REQUEST CODE 
  
	atRequest.clearCommandValue();
	command [0]= command0;
	command [1]= command1;

	atRequest.setCommand(command);
	atRequest.setCommandValue(Value);
	atRequest.setCommandValueLength(ValueLength);

	coord->serialMutex->Wait();
	coord->send(atRequest);
	coord->serialMutex->Signal();

}

bool XBeeSystem::RemoteATRequestRead(uint32_t addressMSB, uint32_t addressLSB, uint8_t command0, uint8_t command1, int timeout=5000)
// Method: Remote AT request to Read a Parameter 
// Assembles A Remote AT Command API Packet Frame, that queries the destination coord for a value without changing it 
// Command0 = First ASCII Char representing an AT Paramter CODE 
// Command1 = Second ASCII Char represeting a AT Paramter CODE 
// Waits until the Desired Packet is Recieved and Read 
// Returns True if The read was successful. 
// Returns False Otherwise. 

// Notes: Other types of Packets might be in the serial buffer, so 2-3 Reads could be necessary to read the desired. 
// Boolean Return Type is necessary so User can ensure Reading of the right packet otherwise Possible ERRORS/ Wrong Values Read 

// Function:
//1. Set the 64-Bit Address in a special Class for 64Bits Addressing. 
// 2. Clear Previous Command Value for Safety. 
//       Must be done because Command Value could be populated with previous answer, causing User to inadvertantly change a paramter
// 3. Set the 64-Bit Address using the 64-Bit Address Object
// 4. Set the AT command Parameter 
// 7. Send The Packet to the Coordinator to Send to the Specified Address
// This function has sets a watchdog timer
{			
				//XBeeResponse Resp;
				XBeeAddress64 _remoteAddress = XBeeAddress64(addressMSB,addressLSB);
				command [0]= command0;
				command [1]= command1;
				remoteAtRequest.clearCommandValue();
				remoteAtRequest.setRemoteAddress64(_remoteAddress);
				remoteAtRequest.setCommand(command);
				coord->serialMutex->Wait();
				coord->send(remoteAtRequest);
				coord->serialMutex->Signal();

				//	coord->outputBuffer.push(remoteAtRequest);

	            /*
	         Resp = readXBee();
            timer = GetTickCount();
            while((GetTickCount() - timer) < timeout)
            { // Get the Node Identifier of the New XBee Sensor 
            	Resp.getApiId();
					if(Resp.getApiId()== REMOTE_AT_COMMAND_RESPONSE)
					{
						Resp.getRemoteAtCommandResponse(remoteAtResponse);
						if ( remoteAtResponse.getCommand()[0] == command0 && remoteAtResponse.getCommand()[1]==command1)
						{
                          if (remoteAtResponse.getValueLength() > 0) 
                          {printf("[%C%C] successful\n",command0,command1);
                          	return true;
                          }
                          //else return false;
						}
					}
					else
					{
						coord->bufferMutex->Wait();
							coord->atBuffer.push(coord->atBuffer.front());
							coord->atBuffer.pop();
						coord->bufferMutex->Signal();
					 Sleep(500);
					 Resp = readXBee();
					}

			}
			return false;
			*/
            
}


bool XBeeSystem::sendRemoteATRequest(uint32_t addressMSB, uint32_t addressLSB, uint8_t command0, uint8_t command1, uint8_t *Value, uint8_t ValueLength)
// Method: Send Remote AT Request 
// Assembles and Sends A Remote AT request API Packet to a Remote Xbee with the 64-bit address: addressMSB + addressLSB to change a Configuration Parameter 
//  [
// AT Commands Represented by a 2 ASCII character CODE 
// Command0 = First ASCII Char representing a CODE 
// Command1 = Second ASCII Char represeting a CODE 
// Value [] = The Value to change the paramerter to
// Each element in Value [] is 8-bits long to meet XBee paremter requirements. 
// Functionality: 
//1. Set the 64-Bit Address in a special Class for 64Bits Addressing. 
// 2. Clear Previous Command Value for Safety. 
// 3. Set 64-Bit Address
// 4. Set the AT command Parameter 
// 5. Set the Command Value
// 6. Set the lenght of the command value for Check Sum 
// 7. Send The Packet to the Coordinator to Send to the Specified Address
{
			remoteAtRequest.clearCommandValue();
            XBeeAddress64 _remoteAddress = XBeeAddress64(addressMSB,addressLSB);
            command [0]= command0;
            command [1]= command1;
            remoteAtRequest.setRemoteAddress64(_remoteAddress);
            remoteAtRequest.setCommand(command);
            remoteAtRequest.setCommandValue(Value); 
            remoteAtRequest.setCommandValueLength(ValueLength); 
            coord->serialMutex->Wait();
            coord->send(remoteAtRequest);
            coord->serialMutex->Signal();

			/*timer =GetTickCount();
            while((GetTickCount() - timer) < 5000)
            { // Get the Node Identifier of the New XBee Sensor 
				coord->bufferMutex->Wait();
				if(coord->atBuffer.size()>0)
				{
					if(coord->atBuffer.front().getApiId()== REMOTE_AT_COMMAND_RESPONSE)
					{
						coord->atBuffer.front().getRemoteAtCommandResponse(remoteAtResponse);
						coord->bufferMutex->Signal();
						if ( remoteAtResponse.isOk()&&remoteAtResponse.getCommand()[0] == command0 && remoteAtResponse.getCommand()[1]==command1)
						{
							coord->bufferMutex->Wait();
							 if(coord->atBuffer.size()>0)
									coord->atBuffer.pop();
							coord->bufferMutex->Signal();
                            return true;
						}
						else 
						{
							coord->bufferMutex->Wait();
							 if(coord->atBuffer.size()>0)
							 {
								coord->atBuffer.push(coord->atBuffer.front());
								coord->atBuffer.pop();
								}
							coord->bufferMutex->Signal();
							return false;
						}
					}
					else 
					{
						coord->bufferMutex->Signal();
						return false;
					}
				}
				else
				{
					coord->bufferMutex->Signal();
				}
			}
			*/
			return true;

}

    

void XBeeSystem::NodeDiscovery(XBeeResponse Resp)

{ // Method: Node Discovery 
  // Discovers New Nodes on the network for a defined duration. The lenght of time can be critical for accurately 
  // discovering all nodes 
  
  // Issues the Node Discovery AT Command ND to the coordinator. The Coordinator will search for nodes on the network 
 //  Each node will issue a response containing some information  about thier Identities and Network Addresses
 // If the proper AT response is Recieved, we have discovered a new node
       // The  AT command response is given the following format
       //  [MY] [SH] [SL] [NI] [Parent 16bit Address] [1] [Profile ID] [Manufacturer]
       // We only want MY SH SL NI and possibly Parent 16bit Address if avaiable
       // The code will read the specific parts of the request to obtain the desired info
  // Any other Responses are ignoored
			if (Resp.getApiId() == AT_COMMAND_RESPONSE)
			{
				Resp.getAtCommandResponse(atResponse);
				if (atResponse.isOk()&& atResponse.getCommand()[0] == (uint8_t)'N' && atResponse.getCommand()[1] == (uint8_t)'D' )
				{
					printf("[%C%C] was successful \n",atResponse.getCommand()[0],atResponse.getCommand()[1]);
					//sensor = new Sensor; // create new Sensor Object
					sensor = Sensor();
					/*for (int i = 0; i < atResponse.getValueLength(); i++)
					{
						printf("%X ", atResponse.getValue()[i]);
					}
					printf("\n");*/

					sensor.data.Address16 = (atResponse.getValue()[0]<<8 + atResponse.getValue()[1]);// Bytes 0 and 1 are the 16bitAddress

					for(int i=2; i<5; i++)// Bytes 2-5 are SH
						// Bytes are shifted into the 32-bit variable starting with MSB
					{
						sensor.data.SN_High += atResponse.getValue()[i];
						sensor.data.SN_High = sensor.data.SN_High<<8;
					}
					sensor.data.SN_High += atResponse.getValue()[5];

					for(int i=6; i<9; i++) //Bytes 6-9 are SL
					{
						// Bytes are shifted into the 32-bit variable starting with MSB
						sensor.data.SN_Low += atResponse.getValue()[i];
						sensor.data.SN_Low = sensor.data.SN_Low<<8;
					}
					 sensor.data.SN_Low += atResponse.getValue()[9];
					 printf("\n");

	      // Remaining Frame after position 10 has a variable length because of the Node ID is variable length. The Node ID part of the fram is truncated with a 0
	     //   set i = 10 and increment until we find the 0.
					 int i =10;
					 do
					 {
						 sensor.data.Name += (char)(atResponse.getValue()[i]);
	                    i++;
					 }while(atResponse.getValue()[i]!=0);

					 // Then The next two bytes are the Parent Address
					 sensor.data.parentAddress = (atResponse.getValue()[i+1]<<8 + atResponse.getValue()[i+2]);
					 // The next and final Byte we want is the Device Type
					 sensor.data.deviceType = atResponse.getValue()[i+3];
					 sensor.data.lastSample =Resp.getTimeStamp();

					 // We must check to see if this sensor is already discovered
					 if(mySensors.size()==0) // if no sensors discovered yet, add to the system
					 {
						 printf("This is a new Sensor, Adding it to System \n");
						 mySensors.push_back(sensor); // push into buffer
						 sensorCount=mySensors.size(); // increment the sensor count
					 }
					 else
					 {
						 for(int i=0; i<mySensors.size();i++)
						 {
							  if(mySensors.at(i).data.SN_Low == sensor.data.SN_Low ) //if sensor matches one in database
							 {
								  //if already discovered, just update the fields
								  printf("Sensor is already In the index, updating fields ...\n");
								  mySensors.at(i).data.parentAddress =sensor.data.parentAddress ;
								  mySensors.at(i).data.deviceType =sensor.data.deviceType;
								  mySensors.at(i).data.Name =sensor.data.Name;
								  mySensors.at(i).data.Address16 = sensor.data.Address16;
								//  delete sensor;// remember to delete the sensor object
								  return; //return
							 }
							  else if(mySensors.at(i)==mySensors.back())// if at end of data base
							 {
								 // if reached end of known sensors, and no match, it is a new sensor
								 printf("This is a new Sensor, Adding it to System \n");
								 mySensors.push_back(sensor); // add it to system
								 sensorCount=mySensors.size(); // update the sensor count;
								 break; // exits the for loop
							 }
						 }
					 }
					 getSampleRate(mySensors.size()-1,5000);
					 //if(sensor.data.deviceType==END_DEVICE)
						 getSleepInfo(mySensors.size()-1,5000);
					 //mySensors.back().data.sampleRate = defaultSampleRate;
					 // set the defualt analog channel readings
					 for(int j = 0; j<4; j++)
					 {   mySensors.back().data.minimum[j]=0;
					     mySensors.back().data.maximum[j]=1024; // defualt
					 }

					 if(dataloggingEnabled)
					 {
						 msg.str("");
						 msg<<"A New Sensor Joined The Network With Serial Number "
								 <<hex<<mySensors.back().data.SN_High<<";"<<hex<<mySensors.back().data.SN_Low<<";";
						 msg<<" and Name "<<mySensors.back().data.Name<<" as a ";
						 if(sensor.data.deviceType==ROUTER)
							 msg<<"ROUTER";
						 else if(sensor.data.deviceType==END_DEVICE)
							 msg<<"END_DEVICE";

						 printMsgToFile(msg.str());
					 }
					 mySensors.back().data.joined=long(getMillisecondTime()/1000);
					// sendRemoteATRequest(mySensors.back().data.SN_High, mySensors.back().data.SN_Low, 'I', 'R', irValue, sizeof(irValue));
					 push_regQ(mySensors.back()); // finally push into the registration queue for registration
					// delete sensor; // free the memory for the temp sensor object

				}
			}
		return;

}
	



void XBeeSystem::ProcessAPI(uint8_t API, XBeeResponse Resp)
{

  // Method: Process the API Frame Response
  // API defines the Frame Delimeter, defininge the type of Repsonse 
  // Only implemented the ones needed for this release, other options are available. 
 switch(API)
 {
      case MODEM_STATUS_RESPONSE:
         // If the coordinator comes online we want to know whether or not the it is ready 
          // If connected, the status is 6, if it is not ready, the status is 0
          // If it is connected, then we want to issue the MY command to get the SN of the Coordinator
          printf("Received  Modem Status Response : ");
		 // modemStatus = ModemStatusResponse();
          Resp.getModemStatusResponse(modemStatus);
          for(int i=0;  i < Resp.getFrameDataLength(); i++)// Prints the Status Response
          {  
                  printf("%X",Resp.getFrameData()[i]);
          }
          if(Resp.getFrameData()[0]==SUCCESS) // Coordinator is ready
          {       printf("Coordinator is ready \n");
          }
      break;
  
      case AT_COMMAND_RESPONSE: // PROCESS an AT command Response 

    	  // atResponse = AtCommandResponse();
    	   Resp.getAtCommandResponse(atResponse);
    	   // Check this
    	   if (atResponse.isOk()&& atResponse.getCommand()[0] == (uint8_t)'N' && atResponse.getCommand()[1] == (uint8_t)'D' )
    	   {
    		   NodeDiscovery(Resp);
    		   break;
    	   }
    	   else if (atResponse.isOk())
    	   {
    		   printf("Command [%C%C] was successful!\n",atResponse.getCommand()[0],atResponse.getCommand()[1]);

    		  /* if (atResponse.getValueLength() > 0)
    		   {
    			   printf("Command value length is ");
    			   printf("%d",atResponse.getValueLength());
    			   printf("Command value: ");
    			   for (int i = 0; i < atResponse.getValueLength(); i++)
    			   {
    				   printf("%X",atResponse.getValue()[i]);
    			   }
					printf("\n");
    		   }*/
    	   }

     break;
     
     case ZB_IO_NODE_IDENTIFIER_RESPONSE:
     // AT Response Frame Given when a new node is Switched on or Commissioning button is presses 
     // We want to allow the coordinator to allow the newly identified node to join the network
     // The Rest of the frame contains the Serial number of the node and Network address 
     // the new sensor and These Variables are stored into the next index of Sensor array temporarily
     // if the sensor is a new node, the sensor count is incremeneted to include the new index, otherwise it is not.
     // and therefore essentially isn't added. 
     // A Helper function is called to get some addtional information and add register the sensor on Sense Tecnic
		  sensor = Sensor(); // Create a new sensor object
		  //nodeIDResponse = ZBRxIoIdentifierResponse();
		  Resp.getZBRxIoIdentifierResponse(nodeIDResponse);
          sensor.data.SN_High = nodeIDResponse.getSN_MSB(); // get SN_High of New Node
          sensor.data.SN_Low = nodeIDResponse.getSN_LSB(); // get SN_Low of new Node
          printf("A new node is available to connect to the network with ");
          printf("Serial Number = %X%X \n",sensor.data.SN_High,sensor.data.SN_Low);
          sensor.data.Address16 = nodeIDResponse.getRemoteAddress16(); // get network address
          sensor.data.parentAddress = nodeIDResponse.getParentAddress();
          sensor.data.deviceType = nodeIDResponse.getDeviceType();
          sensor.data.Name= nodeIDResponse.getNodeID();
          sensor.data.lastSample =Resp.getTimeStamp();

          // check to see if Sensor exists in database
		  if(mySensors.size()==0) // if there is no sensors
		  {
			  printf("Adding it to the System \n");
			  mySensors.push_back(sensor);
			  sensorCount=mySensors.size();
		  }
		  else
		  {
			  for(int i=0; i<mySensors.size(); i++) // search through all sensors that have been discovered
			  {
				  if(mySensors.at(i).data.SN_Low == sensor.data.SN_Low ) // if Sensor has been discovered already
				  {

					  printf("This Sensor is already In the index, updating fields ...\n");
					  mySensors.at(i).data.parentAddress =sensor.data.parentAddress ;
					  if(mySensors.at(i).data.deviceType!=sensor.data.deviceType)
					  {
						  getSleepInfo(i,5000);
						  if(dataloggingEnabled)
						  {
							  msg.str("");
							  msg<<"Sensor With Serial Number "
									  <<hex<<mySensors.back().data.SN_High<<";"<<hex<<mySensors.back().data.SN_Low<<";";
							  msg<<" and Name "<<mySensors.back().data.Name
									  <<" Rejoined the Network as a ";
							  if(sensor.data.deviceType==ROUTER)
								  msg<<"ROUTER";
							  else if(sensor.data.deviceType==END_DEVICE)
								  msg<<"END_DEVICE";


							  printMsgToFile(msg.str());
						  }
					  }
					  mySensors.at(i).data.deviceType =sensor.data.deviceType;
					  mySensors.at(i).data.Name =sensor.data.Name;
					  mySensors.at(i).data.Address16 = sensor.data.Address16;
					  //delete sensor; // remember to delete the sensor object before exiting
					  return ;
				  }
				  else if(mySensors.at(i).data.SN_Low == mySensors.back().data.SN_Low)// if have reached the end of data base;
				  {
					  // this must be a new sensor because we have not previously discovered it
					  printf("This is a new Sensor, Adding it to the System \n");
					  // at the sensor to the system and increment the sensor count
					  mySensors.push_back(sensor);
					  sensorCount=mySensors.size();
					  break; // exit the for loop
				  }

			  }
		  }
		  //sendRemoteATRequest(mySensors.back().data.SN_High,mySensors.back().data.SN_Low, 'I', 'R', irValue, sizeof(irValue));
		  getSampleRate(sensorCount-1,5000);
		  getSleepInfo(sensorCount-1,5000);
		  if(mySensors.at(sensorCount-1).data.deviceType ==END_DEVICE)
			  getSleepInfo(sensorCount-1,5000);
		  for(int j = 0; j<4; j++)
		  {   mySensors.back().data.minimum[j]=0;
		  	  mySensors.back().data.maximum[j]=1024;
		  }
		  // save the change
		  // Finally Add to the register Queue
		  if(dataloggingEnabled)
		  {
			  msg.str("");
			  msg<<"A New Sensor Joined The Network With Serial Number "
					  <<hex<<mySensors.back().data.SN_High<<";"<<hex<<mySensors.back().data.SN_Low<<";";
					  msg<<" and Name "<<mySensors.back().data.Name<<" as a ";
			  if(sensor.data.deviceType==ROUTER)
				  msg<<"ROUTER";
			  else if(sensor.data.deviceType==END_DEVICE)
				  msg<<"END_DEVICE";
			  printMsgToFile(msg.str());
		  }

		  mySensors.back().data.lastSample=Resp.getTimeStamp();
		  mySensors.back().data.joined=long(getMillisecondTime()/1000);
		  push_regQ(mySensors.back());
		  //delete sensor;

    break;
      
    //case ZB_IO_SAMPLE_RESPONSE: 0x92
   case ZB_IO_SAMPLE_RESPONSE:
    // This is the default response for a Node that is sending Data
    // This method Gets the Address and processes the IO Data
    // an IO Sample Frame has a defined structure that contains the 64-bit address of the sending Xbee, 
    //  masks that determine which IO pins are enabled as Digital and/or Analog 
    // Followed by the Samples themselves. 
    // we want to store the channel mask information for use later as well as the actual Samples 
	      //ioSample = ZBRxIoSampleResponse();
	      int index;
	      Resp.getZBRxIoSampleResponse(ioSample);
	      printf("Recieved IO Sample from Sensor with Serial Number ");
          printf("%X%X \n",ioSample.getRemoteAddress64().getMsb(),ioSample.getRemoteAddress64().getLsb());  // Get the Serial Numbers of the Xbee

          /*we should not be getting IO samples  from a sensor we havent accounted for
          * for, but it might happen if we missed one at Startup, therefore add it to the system before reading
          */
          if(mySensors.size() == 0)
          {
        	  printf("This Sensor has not yet been added to System. Attempting to add it Now \n");
        	  //sensor = new Sensor; // Create a new sensor object and temporarily push onto list
        	  sensor = Sensor();
        	  sensor.data.SN_High = ioSample.getRemoteAddress64().getMsb(); // Use the struct variable mySensor that is already created, but sensorcount+1 (temporary)
        	  sensor.data.SN_Low = ioSample.getRemoteAddress64().getLsb();
        	  sensor.data.Address16 = ioSample.getRemoteAddress16(); // get network address
        	  sensor.data.lastSample =Resp.getTimeStamp();
        	  mySensors.push_back(sensor);
        	  if(addNewSensor(0)==false)
        	  {
        		  //delete sensor;
        		  return;// Call to the helper function
        	  }
        	  sensorCount =mySensors.size();
        	  index=0;
        	   // freem memory

          }
          else
          {
        	  for(int i=0; i<=mySensors.size(); i++)// First find which Xbee sent the Data
        	  { // If the Sensor is in the System, then the data can be stored.
        		  if(mySensors.at(i).data.SN_Low == ioSample.getRemoteAddress64().getLsb())
        		  {
        			  index = i;
        			  break;
        		  }
        		  else if( mySensors.at(i).data.SN_Low == mySensors.back().data.SN_Low)
        		  {
        			  printf("This Sensor has not yet been added to System. Attempting to add it Now \n");
        			 // sensor = new Sensor; // Create a new sensor object and temporarily push onto list
        			  sensor = Sensor();
        			  sensor.data.lastSample =Resp.getTimeStamp();
        			  sensor.data.SN_High = ioSample.getRemoteAddress64().getMsb(); // Use the struct variable mySensor that is already created, but sensorcount+1 (temporary)
        			  sensor.data.SN_Low = ioSample.getRemoteAddress64().getLsb();
        			  sensor.data.Address16 = ioSample.getRemoteAddress16(); // get network address
        			  mySensors.push_back(sensor);
        			  if(!addNewSensor(mySensors.size()-1))
        			  {// Call to the helper function to query Sensor for some data
        				  //delete sensor;
        				  return; // if failed querying sensor, software commission button is pressed to reassociate it
        			  }
        			  sensorCount =mySensors.size();
        			  //delete sensor; // freem memory
        			  index = mySensors.size()-1;
        			  break;
        		  }

        	  }
          }


    		  mySensors.at(index).data.Address16 = ioSample.getRemoteAddress16(); // get the 16 bit address
    		  mySensors.at(index).data.AnalogMask=ioSample.getAnalogMask(); // Save the Analog Mask
    		  mySensors.at(index).data.digital_data = 0;

    		  mySensors.at(index).data.timeSinceLastSample = (Resp.getTimeStamp()-mySensors.at(index).data.lastSample);
    		  mySensors.at(index).data.lastSample=Resp.getTimeStamp();
    		  if (ioSample.containsAnalog())
    		  {
    			  printf("Sample contains analog data");// If the Sample Contains Analog Data Let program know

    			  for (int j = 0; j <= 4; j++) // Get the Analog data from the sample
    			  {
    				  if (ioSample.isAnalogEnabled(j))
    				  {
    					  // print the raw Sample Data for Debugging
    					  printf("Analog (AI%d) is %d ",j,ioSample.getAnalog(j));
    					  // Samples are recorded using the dataMapping function to ensure they are scaled properly
    					  mySensors.at(index).data.analog_data[j] = dataMapping(ioSample.getAnalog(j),0,1024,  mySensors.at(index).data.minimum[j],mySensors.at(index).data.maximum[j]);
    				  }
    			  }


    		  }
    		  mySensors.at(index).data.DigitalMask = (ioSample.getDigitalMaskMsb()<<8 | ioSample.getDigitalMaskLsb());   // Save the Digital Mask
    		  if (ioSample.containsDigital())
    		  {
    			  printf("Sample contains digtal data");
    			  uint16_t tempDigital =0;
    			  for (int j = 0; j < 13; j++) // Get the Digital Samples (note the same as digital mask
    				  // Digital Samples are saved into a single Byte with MSB the highest PIN value
    			  {
    				  if (ioSample.isDigitalEnabled(j))
    				  {
    					  printf(" Digital (DI%d) is %d ",j,ioSample.isDigitalOn(j));
    				  }
    				  tempDigital +=  ((uint16_t)ioSample.isDigitalOn(j))<<j;
    			  }

    			  mySensors.at(index).data.digital_data = tempDigital;
    		  }

    		  printf("\n");
    		  if(dataloggingEnabled)
    		  {
    			  printSensorReadingToFile(index);
    		  }

      break;
     
      case REMOTE_AT_COMMAND_RESPONSE:
   //case 0x97:
      // Extract the Data in a Remote AT Command Response Frame. 
      // Stored into a global variable buffer to pass data.
    	 // remoteAtResponse =RemoteAtCommandResponse();
    	  Resp.getRemoteAtCommandResponse(remoteAtResponse);

          if (remoteAtResponse.isOk())
           {
                 printf("Command [%C%C] was successful! \n",remoteAtResponse.getCommand()[0],remoteAtResponse.getCommand()[1]);

                 if (remoteAtResponse.getValueLength() > 0) 
                 {
                        printf("Command value: ");
          
                        for (int i = 0; i < remoteAtResponse.getValueLength(); i++) 
                        {
                                printf("%x", remoteAtResponse.getValue()[i]);
                        }
                        printf(" \n");
                  }
		   }
           else 
           {
                  printf("Command returned error code: %x \n",remoteAtResponse.getStatus());
           }
      break;


	case ZB_RX_RESPONSE: //TODO Add something here for extracting RX data packets 

		Resp.getZBRxResponse(rxMsg);
		index=0;
		sensor = Sensor();
		sensor.data.SN_High = rxMsg.getRemoteAddress64().getMsb(); // Use the struct variable mySensor that is already created, but sensorcount+1 (temporary)
		sensor.data.SN_Low = rxMsg.getRemoteAddress64().getLsb();
		sensor.data.Address16 = rxMsg.getRemoteAddress16(); // get network address
		sensor.data.lastSample=Resp.getTimeStamp();
// first check if there are any sensors registered at all
		if(mySensors.size() == 0) 
		  {
					mySensors.push_back(sensor);
                     if(addNewSensor(0)==false)
                    	 	 return;

					 sensorCount=mySensors.size();
					 index=0;
					 //delete sensor; // freem memory
		  }
			 for(int i =0; i< mySensors.size(); i++)
			 {
					if(mySensors.at(i).data.SN_Low == sensor.data.SN_Low) // found the index
					{
						index=i;
						break;
					}
					else if(mySensors.at(i).data.SN_Low == mySensors.back().data.SN_Low)
					{
						// Means we've searched the sensor lookup table, and couldnot find a match
						// this is a new sensor, 
						//sensor = new Sensor; // Create a new sensor object and temporarily push onto list
						mySensors.push_back(sensor);
						if(addNewSensor(mySensors.size()-1) ==false)// Call to the help
							return;

							sensorCount =mySensors.size();
							index=i+1;
						break;
					}
			 }
			 if (rxMsg.getOption() == ZB_PACKET_ACKNOWLEDGED) // only accept data if it is first acknowledged!
			 {
				 mySensors.at(index).data.Address16 = sensor.data.Address16; // get the 16 bit address
				 mySensors.at(index).data.timeSinceLastSample = (Resp.getTimeStamp()-mySensors.at(index).data.lastSample);
				 mySensors.at(index).data.lastSample=sensor.data.lastSample;
				 mySensors.at(index).data.rxData.clear(); // erase the last rxMSG
				 for(int j=0; j<rxMsg.getDataLength(); j++)
				 {
					 mySensors.at(index).data.rxData.push_back(rxMsg.getData(j));
					 printf(" %X",rxMsg.getData()[j]);
				 }
			 }
			 if(dataloggingEnabled)
			 {
				 printSensorReadingToFile(index);
			 }

    break;

	case ZB_TX_STATUS_RESPONSE:
		txMsg = ZBTxStatusResponse();

		Resp.getZBTxStatusResponse(txMsg);
			printf("Recieved a Tx Status Response.");
			if(txMsg.getDeliveryStatus()==OK)
				printf("Delivery was Successful!\n");
			else
				printf("Delivery Failed \n");
	break;
    default:
		 {
			printf("Expected I/O Sample, but got ");
			printf("%x \n",Resp.getApiId());
		}
    break;
  
    }
}


int XBeeSystem::dataMapping(int sensorVariable, int in_min, int in_max, float out_min, float out_max)
{
// Method: Map sensor Data 
// Maps Raw analog sensor data to a useful range defined by out_min and out_max. 
// User Inputs out_min and out_max in View Sensor Schema Page on the gateway
// Returns Floating point value of the new sensor reading, however
// Any new Data can be properly mapped by calling this function. 
// By default all sensor readings are mapped using this fuction first, with defualt setting to preseve RAW values. 
// ie: Default out_min = 0, out_max = 1024. (10-bit ADC maximum value);

// NOTE: All values are returned 100 times that of their actual values, This is to assist using integer Math to represent the decimal Number 
// with 2 Decimal place Precision, instead of using floating point, which For Arduino is 32-bits precision. This is done to save memory and unessesary 
// data precision. 

  return 100*(((sensorVariable - in_min) * (out_max - out_min) / (in_max - in_min)) + out_min);
}



/**************************************/
// INITIALIZE

int XBeeSystem::initializeNetwork()
{
	coord->resetSerial(1000);
	// need to ensure that Serial port gets opened properly 
	// if not.. exit immediatley 
	// this is a final safety check for serial port
	try{
	if(coord->begin(baud,serialPort.c_str())!=1)
	{ 
		printf("Could Not Open Serial  Port %s....\n",serialPort.c_str());
		throw (-1);// throws a Python Error as Well
	}
	}
	//TODO Fix this to print a message  and embed in a try: Exception: block 
	catch(int e)
	{
		//printf("%s",e);
		printf("Exiting....");
		exit(-1);
	}

	coord->Resume(); // must start the coordinator object
	Sleep(20);

	mySensors.clear();
	regQ.clear();
	sensorCount =0;
	bool success =false;
	Resp = XBeeResponse();
	ATparameterRead('A','I');
	while(!success)
	{
		//coord->consumerSemaphore->Wait();
		if(checkAtResponse(Resp, 'A','I',5000))
		{
			Resp.getAtCommandResponse(atResponse);

					printf("Command [%C%C] was successful! \n",atResponse.getCommand()[0],atResponse.getCommand()[1]);
					if(atResponse.getValue()[0]==0x00) // FROM AI COORD ONLINE =0x00
					{ 
						printf("Coordinator is on-line\n");
						success = true;
						coordinatorOnLine=true;
						break;
					}
					else if (atResponse.getValue()[0]==COORDINATOR_FAILURE) // 0x2A
                  	{       
						printf("Coordinator Failure... Forcing exit of program!\n");
						exit(-1);
                  	}
		}
		else
		{
			printf("Coordinator Not Online... Trying again \n");
			if(coord->atBuffer.size()>0)
			{
				coord->bufferMutex->Wait();
				coord->atBuffer.pop();
				coord->bufferMutex->Signal();
			}
			Sleep(500);
			ATparameterRead('A','I'); // resend the command


		}
					//coord->producerSemaphore->Signal();


	}
 


  //  If Starting The Coordintaor has been successful, we  need to obtain PAN ID, SH, SL, and Determine associated NODES
  // No need to determind 16-bit address because it is 0 by default. 
  
  // Query the Coordintor for the Network PAN ID 
  // AT Command = OP. 
success=false;

ATparameterRead('O','P');

uint16_t PAN_ID=0;
while(!success)
{
	//coord->consumerSemaphore->Wait();
	if(checkAtResponse(Resp,'O','P',5000))
	{
		Resp.getAtCommandResponse(atResponse);

		PAN_ID = atResponse.getValue()[0];
		PAN_ID = PAN_ID << 8;
		PAN_ID += atResponse.getValue()[1];
		coord->setPAN_ID(PAN_ID);
		printf("PAN = %X\n", PAN_ID);
		success=true;
	}
	else{
		Sleep(500);
		ATparameterRead('O','P');
	}
}

/*
while(!success)
{
	////coord->consumerSemaphore->Wait();

	coord->bufferMutex->Wait();
	size = coord->atBuffer.size();
	coord->bufferMutex->Signal();
	if(size>0)
	{
		coord->bufferMutex->Wait();
		Resp = coord->atBuffer.front();
		coord->atBuffer.pop();
		coord->bufferMutex->Signal();

		if(Resp.getApiId() == AT_COMMAND_RESPONSE)
		{ 	//atResponse.reset();
			Resp.getAtCommandResponse(atResponse);
			if(atResponse.isOk() && atResponse.getCommand()[0] == uint8_t('O') && atResponse.getCommand()[1] == uint8_t('P'))
			{
				// Store the PAN ID
				PAN_ID = atResponse.getValue()[0];
				PAN_ID = PAN_ID << 8;
				PAN_ID += atResponse.getValue()[1];
				coord->setPAN_ID(PAN_ID);
				printf("PAN = %X\n", PAN_ID);
				success=true;
			}
			else if(atResponse.isError()&&atResponse.getCommand()[0] == uint8_t('O') && atResponse.getCommand()[1] == uint8_t('P'))
			{ // resend the command
				Sleep(500);
				ATparameterRead('O','P');
			}
		}
		else
		{
			//Push the item back on the Queue
			coord->bufferMutex->Wait();
			coord->atBuffer.push(Resp);
			coord->atBuffer.pop();
			coord->bufferMutex->Signal();
		}
	}
	//coord->producerSemaphore->Signal();
}
*/
// Now obtian the MSB of the 64-Bit Serial Number, SH of the Coordinator
  
success=false;
ATparameterRead('S','H');
while(!success)
{
	//coord->consumerSemaphore->Wait();
	if(checkAtResponse(Resp,'S','H',5000))
	{
		Resp.getAtCommandResponse(atResponse);

		for(int j=0; j<4; j++)
		{
			coord->SN_High =  coord->SN_High<<8;
			coord->SN_High += atResponse.getValue()[j];
		}
		printf("SN_H ");
		printf("%x\n", coord->SN_High);
		success=true;
	}
	else
	{
		Sleep(500);
		ATparameterRead('S','H');
	}
}
	//coord->producerSemaphore->Signal();

 
 

  // Now Obtain LSB of the 64-Bit Serial Number SL of the Coordinator 
success=false;

ATparameterRead('S','L');

while(!success)
{
	////coord->consumerSemaphore->Wait();
	if(checkAtResponse(Resp, 'S','L', 5000))
	{
		Resp.getAtCommandResponse(atResponse);
		for(int j=0; j<4; j++)
		{
			coord->SN_Low = coord->SN_Low<<8;
			coord->SN_Low += atResponse.getValue()[j];;
		}
		printf("SN_L ");
		printf("%x\n", coord->SN_Low);
		success=true;
	}
	else{
		Sleep(500);
		ATparameterRead('S','L');
	}
	//coord->producerSemaphore->Signal();
}
	
 
    
    // Get the Node ID of the Coordinator; 
    // Each Character of the Node Identifier String is Returned in the generic array object, buffer
    // We cast each byte into a character one-by-one, and piece the string together one character at a time. 
    // Note: Strings are just character arrays, however we cannot just type-cast the entire Array, therefore we must type-cast 
    // each element.
   success=false;

 ATparameterRead('N','I');
 while(!success)
 {
	 // coord->consumerSemaphore->Wait();
	 if(checkAtResponse(Resp, 'N','I', 5000))
	 {
		 Resp.getAtCommandResponse(atResponse);
		 for (int pos = 0; pos < atResponse.getValueLength(); pos++)
		 {
			 printf("%C", atResponse.getValue()[pos]); // Print Each character (for Debugging)
			 coord->Name += char(atResponse.getValue()[pos]);
		 }
		 printf("\n");
		 success=true;
	 }
	 else
	 {
		 Sleep(500);
		 ATparameterRead('N','I');
	 }

	 //coord->producerSemaphore->Signal();
 }


    
// Now we Discover any Attatched Nodes at Start-UP using the Node Discovery AT command, ND
//***************Node discovery mode ************
// This method should persist for a maximum duration of 60seconds or until no more nodes have been detected
//  WE first must issue the NT command to set maximum duration for node discovery. 
// 60seconds == 0x3C
	// This section of code sets the NT based on Discovery timeout 
// NT = 0x20-0xFF x100milliseconds
 uint8_t test[1];

 if((discoveryTimeout/100) > 0xff)
 {
	 test[0]= 0xff;
 }
 else if((discoveryTimeout/100)<0x20)
 {
	 test[0]=0x20;
 }
 else
 {
	 test[0] = (discoveryTimeout/100);
 }

 success=false;
 ATparameterSet('N', 'T', test, sizeof(test));
 while(!success)
 {
	 // coord->consumerSemaphore->Wait();
	 if(checkAtResponse(Resp,'N','T', 5000))
	 {
		 Resp.getAtCommandResponse(atResponse);

		 printf("%C%C \n",atResponse.getCommand()[0],atResponse.getCommand()[1]);
		 success=true;
		 //break;
	 }
	 else{
		 Sleep(500);
		 ATparameterSet('N', 'T', test, sizeof(test));
	 }
	 //coord->producerSemaphore->Signal();

 }

      
    // Now we issue a Node Discovery Command and Allow it to persist for a defined amount of time, reading everything that becomes available 
    // Time has an effect on the accuracy and reliabiltiy of the discovered nodes, Too little time, not all nodes will be added to the system.
    // at startup, performing node discovery for at least 30000 ms is good. anywhere from 30 s to a minute would suffice to identify all Sleeping nodes 
 
 	// ATparameterRead('N', 'D');
    // Node Discovery (int timeout in milliseconds).
    doNodeDiscovery(discoveryTimeout);
	// Set the sampling rate of every node to 5 seconds for safety. Sample rates at frequency faster than 1 second my overload the system with samples
	// Sets the value to a safe value, further changes can be made through the Gateway GUI. 
	// Set all nodes at once using  AT command broadcast
    // TODO erase this sleep when finished. we are just simulating stopping for rigourous NODE DISC.
    //Sleep(5000);

    //sendRemoteATRequest(BROADCAST_ADR_MSB, BROADCAST_ADR_LSB, 'I', 'R', irValue, sizeof(irValue)); // BroadCast Address
    //API =coord->inputBuffer.front().getApiId(); // Save to a temporary variable to avoid overwriting 
    //ProcessAPI(API,0,buffer);
   
    // Initialize the Variables for each Sensor the is Discovered.
    // Sample Rate

	for(int i=0; i<mySensors.size(); i++)
      { // Initialize the maximum and minimum Values for Analog Reading Scaling to Defaults. 
         // mySensors.at(i).data.sampleRate = defaultSampleRate;
		  for(int j= 0; j<4;j++)
			{ 
			  mySensors.at(i).data.minimum[j]=0;
			  mySensors.at(i).data.maximum[j] = 1024;
			}
	  }
	printf("Starting Reader Thread \n");
this->Resume();

	  return 1; 
        
 }






int XBeeSystem::deleteSensor( int index)
{ 
	// Method: Delete Sensor from Memory 
// Inputs: Sensor Index
// Helper Function for Delete Sensor Function that Removes An instance of a Sensor In Data Structure 
// 1. Goes straight to the index of the Sensor Array Structure
// 2. copies the next Sensor obhect in memory over to this index, thus Overwriting/Removing the sensor Instance  
// 3. Shifts Everything else over one by Repeating Process for all remaining Entries 

// 4. Finaly decrements the  sensor count index value 
	if(index >=mySensors.size()|| index<0){
		printf("ERROR... Sensor Not in Index...\n"); 
		return -1; 
	}
	mySensors.erase(mySensors.begin()+index);
	sensorCount =mySensors.size(); 
	return 1; 
}


 bool XBeeSystem::addNewSensor (int sensorIndex)
// Method: Create a and populate a New Sensor Object for a newly Discovered Sensor
// Sends a Host Of Remote AT Commands queries to a newly discovered Sensor node to determine some needed Information. 
// NI: Node Identifier String 
// SM: Sleep Mode of the XBee, Determines whether the Device is a Router or an End Device 
// MP: Parent Address of the XBee. Useful for future mesh network managing algorithms. 
// NC: Number of Children. If the device has children attatched to it, Determine how many, useful for furture mesh network managing algortithms 
// 
// Each Query is Performed for a Specified Time out Rate. If the Query is not successful, the Query is performed again until the successful or timeout occurs
// This is To ensure best we can that we obtain the needed information, 
// however timeout ensures that we can at least return to regular Gateway operation if there is an error reading.
// In the Occasion an Error Occurs, Some information may not be available. 

{
	
	 if(mySensors.back().data.SN_Low!=mySensors.at(sensorIndex).data.SN_Low)
	 {
		 printf("An Unknown error Has Occurred, the Sensor Not Added to the System \n");
		 return false;
	 }


	 if(sensorIndex<0 || sensorIndex >= mySensors.size())
	 {
		 return false;
	 }
	 bool success = false;



	 success= getNodeID(sensorIndex,5000);

	 success=getSampleRate(sensorIndex,5000);

	 success=getSleepMode(sensorIndex,5000);

	 success=getSleepInfo(sensorIndex,5000);




	 if(success)
	 {
	 if(mySensors.at(sensorIndex).data.deviceType == END_DEVICE)
		 success=getParentAddress(sensorIndex, 5000);
	 else if(mySensors.at(sensorIndex).data.deviceType == ROUTER)
		 success=getNumberOfChildren(sensorIndex, 5000);
	 }

	 if(success==false)
	 {
		 softCommissionButtonPress(sensorIndex);

		 deleteSensor(sensorIndex);
		 // this will cause the Node to Re-associate on the network
		 return false;
	 }



	 for(int j = 0; j<4; j++)
	 {   mySensors.at(sensorIndex).data.minimum[j]=0;
	 	 mySensors.at(sensorIndex).data.maximum[j]=1024;
	 }

	 if(dataloggingEnabled)
	 {
		 msg.str("");
		 msg<<"A New Sensor Joined The Network With Serial Number "
				 <<hex<<mySensors.back().data.SN_High<<";"<<hex<<mySensors.back().data.SN_Low<<";";
				 msg<<" and Name "<<mySensors.back().data.Name<<" as a ";
		 if(sensor.data.deviceType==ROUTER)
			 msg<<"ROUTER";
		 else if(sensor.data.deviceType==END_DEVICE)
			 msg<<"END_DEVICE";
		 printMsgToFile(msg.str());
	 }

	 mySensors.back().data.joined=long(getMillisecondTime()/1000);
	 push_regQ(mySensors.back());
	 return success;


}


string XBeeSystem::printSensors()
{
	msg.str("");
	msg << "Number Sensors = "; 
	msg<< sensorCount;
	   if(mySensors.size()!=0)
	   {
		for(int i = 0; i< mySensors.size(); i++)
		{ 
			msg<<"\n";
			msg<<i+1<<". ";
			msg<< hex<< mySensors.at(i).data.SN_High;
			msg<<hex<< mySensors.at(i).data.SN_Low<< " ";
			msg<< mySensors.at(i).data.Name;
			msg<<"\n";
		}
	   }
	   return msg.str();
}

int XBeeSystem::getPAN_ID()
{ 
	return coord->getPAN_ID();
}

bool XBeeSystem::setSerial(string s)
{ 
	//Need  to do some kind of safety check  for correct serial ports

	if(s== "/dev/ttyS0"||s=="/dev/ttyS1"||s=="/dev/ttyS2"||s=="/dev/ttyS3"
		||s=="/dev/ttyS4"||s=="/dev/ttyS5")
	{
			serialPort =s;
			return true;
	}
	else if (s=="/dev/ttyUSB0"||s=="/dev/ttyUSB1"||s=="/dev/ttyUSB2"||s=="/dev/ttyUSB3")
	{
		serialPort =s;
		return true;
	}
	//serialPort =s;
	//return true;
	return false;


}

bool XBeeSystem::setBaud(int b)
{
	//Need  to do some kind of safety check here for available serial ports 
	// These are the Bauds Permitted by XBee
	switch(b)
	{
		case 1200: 		break;
	    case 2400 :      break;
	    case 4800 :      break;
	    case 9600 :      break;
	    case 19200 :     break;
	    case 38400 :     break;
	    case 57600 :     break;
	    case 115200 :    break;
	    case 230400 :	 break;
	    default: return false;

	}
	baud= b; 
	return true; 
}

int XBeeSystem::read()
{ 
	uint8_t _API =0;
	//Resp = XBeeResponse();


	////coord->consumerSemaphore->Wait();

		lock();
		if(coord->atBuffer.size()>0) // gives higher priority to AT responses
		{
			coord->bufferMutex->Wait();
			printf("There are %d items in the atBuffer \n", coord->atBuffer.size());
		   _API=coord->atBuffer.front().getApiId();
	       Resp=coord->atBuffer.front();
	       coord->atBuffer.pop();
	       coord->bufferMutex->Signal();
	       //ProcessAPI(_API,Resp);
		}
		else if(coord->inputBuffer.size()>0)
		{
			coord->bufferMutex->Wait();
			printf("There are %d items in the inputBuffer \n", coord->inputBuffer.size());
			_API=coord->inputBuffer.front().getApiId();
			 Resp=coord->inputBuffer.front();
			coord->inputBuffer.pop();
			coord->bufferMutex->Signal();
			//ProcessAPI(_API,Resp);
		}
		else
		{
			Sleep(1);
			release();
			return _API;
		}
		release();
		//coord->producerSemaphore->Signal();
		ProcessAPI(_API,Resp);
		return _API;

}

void XBeeSystem::setDiscoveryTimeout(int timeout)
{
	discoveryTimeout=timeout;
}


Nodes XBeeSystem::getSensors()
{
	// Copies the entire contents of the sensor array into a Nodes container
	// Returns the nodes to Python 
	Node myNode =Node();// create a node structure 
	Nodes myArray =Nodes();
		/*if(mySensors.size()== 0)
		  {
			release();
				return myArray;
		  }
		 else*/
		  //{
			int size=mySensors.size();
			for(int i=0; i<mySensors.size(); i++)
			 {

			myNode.data.Name = mySensors.at(i).data.Name;
			myNode.data.SN_High =mySensors.at(i).data.SN_High; 
			myNode.data.SN_Low=mySensors.at(i).data.SN_Low;  
			myNode.data.Address16=mySensors.at(i).data.Address16; 
			myNode.data.parentAddress =mySensors.at(i).data.parentAddress;
			myNode.data.DigitalMask =mySensors.at(i).data.DigitalMask;
			myNode.data.AnalogMask =mySensors.at(i).data.AnalogMask;
			myNode.data.digital_data=mySensors.at(i).data.digital_data; 
			myNode.data.deviceType=mySensors.at(i).data.deviceType;
			myNode.data.sampleRate=mySensors.at(i).data.sampleRate;
			myNode.data.sleepRate=mySensors.at(i).data.sleepRate;
			myNode.data.timeUntilSleep = mySensors.at(i).data.timeUntilSleep;
			myNode.data.lastSample = mySensors.at(i).data.lastSample;
			myNode.data.timeSinceLastSample =(getMillisecondTime()-mySensors.at(i).data.lastSample);
			myNode.data.numberOfChildren =mySensors.at(i).data.numberOfChildren;
			myNode.data.joined = mySensors.at(i).data.joined;
			for(int k=0; k<4; k++)
			{ 
			 myNode.data.analog_data[k]=mySensors.at(i).data.analog_data[k];// Analog data  (up to for channels ) 
			 myNode.data.minimum[k] =mySensors.at(i).data.minimum[k];
			 myNode.data.maximum[k]=mySensors.at(i).data.maximum[k];
			}
			myNode.data.rxData.clear();
			// copy the RX data if there is any
			for(int k =0; k<mySensors.at(i).data.rxData.size(); k++)
			{
				myNode.data.rxData.push_back(mySensors.at(i).data.rxData[k]);
			}

			myArray.nodes.push_back(myNode);

			}


		//}
		return myArray;
}

string XBeeSystem::printAnalog(int index,string delimiter)
{
	
	stringstream analogmsg;
	if(index >= mySensors.size()|| index<0 )
		return "ERROR... Index out of range"; 
	if(mySensors.size()==0)
		return "ERROR... No Sensors Registered in System";


    int temp1; 
    int temp2; 

    
	for(int i = 0; i<4; i++) // Look at the first pins because They may contain ANALOG DATA
                { temp1 = mySensors.at(index).data.AnalogMask; // Shift operator is Destructive so store into a temporary variable
                  temp2 = (temp1)&(1<<i); // for each pass a bit moves one position to the left. ie: 00000001->00000010->00000100
                  // Bitwise AND this Value with the Analog Mask to isolate each bit of the analog mask
                  // If value on the right is equal To the masked analog mask, then then the bit at that position in the mask is 1, 
                  // therefore that channel is available. 
                  if(temp2 == 1<<i) 
					  analogmsg <<"DIO"<<dec<<i<<"="<<dec<<(mySensors.at(index).data.analog_data[i]/100)<<"."<<dec<<abs(mySensors.at(index).data.analog_data[i]%100)<<delimiter;

				}

	return analogmsg.str();
}

string XBeeSystem::printDigital(int index,string delimiter)
{
	stringstream digitalmsg;
	int digital;
	int temp1;
	int temp2;
    int digitalTemp;

	if(index >= mySensors.size() || index <0)
	{
		printf("Error... Index Out of Range \n");
		return "";
	}
    // Check digital Pins from 1-4
	for(int i=0; i<13; i++)
	{//Note that on some embedded machines, shift operations are destructive
		// Need to ensure we avoid this fact by temporarily saving values
	// If an Analog Channel was not Available, Check if Digital is Enabled 
                  // Bit masking works Similar to Above Code, however the bit mask is 16Bits long. 
                  // We need only check the first 4 Bits of the mask Here for Pins DIO0-3;
				  digitalTemp = mySensors.at(index).data.DigitalMask;
                  digital = digitalTemp&(0x1<<i);
                  if(digital == (1<<i))
                  {    temp1= mySensors.at(index).data.digital_data;
                       temp2 = (temp1>>i)&(0x1);
                        digitalmsg<<"DIO"<<dec<<i<<"="<<temp2<<delimiter;
                  }
                
	}
                //****************************************************
                // Determine the Available Channels for  10-12
                // Using Masking Techniques as before, however, we need only look at the Digital Mask 
               // save into a Temporary Variable because Shift operator is destructive.
             
   /*    for(int i=5; i<13; i++)  // Perform a Loop Count from 4-12, Only Execute When i = 4, 10, 11, 12;
	   {
                  
                  digitalTemp = mySensors.at(index).data.DigitalMask; 
                  digital = digitalTemp&(0x01<<i); // Masked value 
                  
                  if(digital == 0x01<<i) // If the Maked Value is equal to the Value on the Right, The Pin is Available
                  {
                      temp1 = mySensors.at(index).data.digital_data; // Perform masking on the digital Data Note that the Digital Mask != Digital Data
                      temp2 = (temp1>>i)&(0x1);
                      msg<<"DIO"<<i<<"="<<temp2<<delimiter;
                     
				  }
                  
	   }*/

	   return digitalmsg.str();
               

}



int XBeeSystem::changeAnalog(int sensorIndex, int channel, int min, int max)
{ 
	if(channel <0 || channel >4)
	{
		printf("Not In Range\n"); 
		return -1; // Not in range
	}
	if((sensorIndex<0)|| (sensorIndex>mySensors.size()))
	{
		printf("Index not in Range...\n");
		return -2; 
	}
	if(channel<0||channel>4)
	{ 
		printf("Channel Not in Range...\n");
		return -3; 
	}

	mySensors.at(sensorIndex).data.minimum[channel] = min;
	mySensors.at(sensorIndex).data.maximum[channel] = max;
	return 1; 
}


#ifdef PYTHON 


//int ATParamSet( const char* arg1, const char* arg2, int len);
int XBeeSystem::ATParamSet(string Command, string commandValueString, int timeout)
{ 
	if(Command.size()!=2) // this checks if the argument passed is not 2-> throw  an error
				return -1;

		uint8_t *commandValue;
		string temp; // save the command value string into a temp variable
		std::stringstream ss;
		int commandValueLength;
		int ssOut;
		int success=0;
		//Resp =XBeeResponse();

		if(timeout<0)
		{
			timeout = 1000;
		}
		else if(timeout>25000)
		{
			timeout =25000;
		}
		if(commandValueString.length()%2==1) //If the command input an odd number we must add a leading 0.
		{
			commandValueLength = (commandValueString.length()/2) +1;

			commandValue= new uint8_t[commandValueLength];
			//commandValue= (uint8_t*)calloc(commandValueLength, sizeof(uint8_t));
			// this section of code reads the string in a forwards manner
			int j=0;
			ss<<hex<<commandValueString[0];
			ss>>ssOut;
			commandValue[0]= (uint8_t)ssOut;
			j++;
			ss.clear();
			for(int pos = 1; pos<(commandValueString.length()-1);pos+=2)
			{
				temp=commandValueString.at(pos);
				temp+=commandValueString.at((pos+1));
				ss<<hex<< temp;
				ss>>ssOut;
				commandValue[j] = ssOut;
				j++;
				ss.clear();
			}

		}
		else
		{
			commandValueLength = (commandValueString.length()/2);
			//commandValue= (uint8_t*)calloc(commandValueLength, sizeof(uint8_t));
			commandValue= new uint8_t[commandValueLength];
		    // refresh temp -> point to null at start
			// this section of code reads the string in a in a forward manner.
			int j=0;
			for(int pos = 0; pos<commandValueString.length()-1;pos+=2)
			{
				temp=commandValueString.at(pos);
				temp+=commandValueString.at(pos+1);
				ss<<hex<<temp;
				ss>>ssOut;
				commandValue[j] = ssOut;
				j++;
				ss.clear();
			}
		}
		lock();
		ATparameterSet(Command[0], Command[1], commandValue, commandValueLength);
		if(checkAtResponse(Resp, Command[0],Command[1], timeout))
		{

			success=1;

		}
		//free(commandValue);
		delete [] commandValue;
		release();
		return success;
}

int XBeeSystem::RemoteATSet(int AddrMSB, int AddrLSB, string Command, string commandValueString, int timeout)
{ 

	if(Command.size()!=2) // this checks if the argument passed is not 2-> throw  an error
			return -1;

	uint8_t *commandValue;
	string temp; // save the command value string into a temp variable
	std::stringstream ss;
	int commandValueLength;
	int ssOut;
	int success=0;
	//Resp =XBeeResponse();

	if(timeout<0)
	{
		timeout = 1000;
	}
	else if(timeout>25000)
	{
		timeout =25000;
	}
	if(commandValueString.length()%2==1) //If the command input an odd number we must add a leading 0.
	{ 
		commandValueLength = (commandValueString.length()/2) +1;
		commandValue= new uint8_t[commandValueLength];
		//commandValue= (uint8_t*)calloc(commandValueLength, sizeof(uint8_t));
		// this section of code reads the string in a forwards manner
		int j=0;
		ss<<hex<<commandValueString[0];
		ss>>ssOut;
		commandValue[0]= (uint8_t)ssOut;
		j++;
		ss.clear();
		for(int pos = 1; pos<(commandValueString.length()-1);pos+=2)
		{
			temp=commandValueString.at(pos);
			temp+=commandValueString.at((pos+1));
			ss<<hex<< temp;
			ss>>ssOut;
			commandValue[j] = ssOut;
			j++;
			ss.clear();
		}
			
	}
	else 
	{
		commandValueLength = (commandValueString.length()/2);
		commandValue= new uint8_t[commandValueLength];
		//commandValue= (uint8_t*)calloc(commandValueLength, sizeof(uint8_t));
	    // refresh temp -> point to null at start
		// this section of code reads the string in a in a forward manner.
		int j=0;
		for(int pos = 0; pos<commandValueString.length()-1;pos+=2)
		{
			temp=commandValueString.at(pos);
			temp+=commandValueString.at(pos+1);
			ss<<hex<<temp;
			ss>>ssOut;
			commandValue[j] = ssOut;
			j++;
			ss.clear();
		}
	} 
	lock();
	sendRemoteATRequest(AddrMSB, AddrLSB, Command[0], Command[1], commandValue, commandValueLength);

	if(checkRemoteResponse(Resp, Command[0],Command[1], timeout))
	{
		applyChanges(AddrMSB,AddrLSB);
		success= 1;

	}
	delete [] commandValue;
	//free(commandValue);
	release();
	return success;

}

int XBeeSystem::RemoteATSet(int index, string Command, string commandValueString, int timeout)
{
		if(Command.size()!=2) // this checks if the argument passed is not 2-> throw  an error
				return -1;
		if(index<0|| index>=mySensors.size())
			return -2;
		uint8_t *commandValue;
		string temp; // save the command value string into a temp variable
		std::stringstream ss;
		int commandValueLength;
		int ssOut;
		int success=0;
		//Resp =XBeeResponse();

		if(timeout<0)
		{
			timeout = 1000;
		}
		else if(timeout>25000)
		{
			timeout =25000;
		}
		if(commandValueString.length()%2==1) //If the command input an odd number we must add a leading 0.
		{
			commandValueLength = (commandValueString.length()/2) +1;
			//commandValue= (uint8_t*)calloc(commandValueLength, sizeof(uint8_t));
			commandValue= new uint8_t[commandValueLength];
			// this section of code reads the string in a forwards manner
			int j=0;
			ss<<hex<<commandValueString[0];
			ss>>ssOut;
			commandValue[0]= (uint8_t)ssOut;
			j++;
			ss.clear();
			for(int pos = 1; pos<(commandValueString.length()-1);pos+=2)
			{
				temp=commandValueString.at(pos);
				temp+=commandValueString.at((pos+1));
				ss<<hex<< temp;
				ss>>ssOut;
				commandValue[j] = ssOut;
				j++;
				ss.clear();
			}

		}
		else
		{
			commandValueLength = (commandValueString.length()/2);
			commandValue= new uint8_t[commandValueLength];
			//commandValue= (uint8_t*)calloc(commandValueLength, sizeof(uint8_t));
		    // refresh temp -> point to null at start
			// this section of code reads the string in a in a forward manner.
			int j=0;
			for(int pos = 0; pos<commandValueString.length()-1;pos+=2)
			{
				temp=commandValueString.at(pos);
				temp+=commandValueString.at(pos+1);
				ss<<hex<<temp;
				ss>>ssOut;
				commandValue[j] = ssOut;
				j++;
				ss.clear();
			}
		}
		lock();
		sendRemoteATRequest(mySensors.at(index).data.SN_High, mySensors.at(index).data.SN_Low, Command[0], Command[1], commandValue, commandValueLength);
		if(checkRemoteResponse(Resp, Command[0],Command[1], timeout))
		{
			applyChanges(index);
			success= 1;

		}
		delete []commandValue;
		//free(commandValue);
		release();

		return success;

	}


int XBeeSystem::RemoteATSet(int index, string Command, int Value, int timeout)
{
	//Resp= XBeeResponse();
	if(index <0 || index>=mySensors.size())
		return -1;
	if(Command.size()!=2)
		return -2;
	if(Value <0)
		return 0;
	int success=0;
	uint8_t commandValue[]={(Value>>24) &0xff, (Value>>16)&0xff, (Value>>8)&0xff,Value&0xff};

	lock();
	sendRemoteATRequest(mySensors.at(index).data.SN_High, mySensors.at(index).data.SN_Low, Command[0], Command[1], commandValue, sizeof(commandValue));
			if(checkRemoteResponse(Resp, Command[0],Command[1], timeout))
			{
				applyChanges(index);
				success= 1;

			}
	release();
	return success;
}

int XBeeSystem::RemoteATSet(int AddrMSB,int AddrLSB, string Command, int Value, int timeout)
{
	 //Resp= XBeeResponse();
	if(AddrMSB <0 || AddrLSB<0 )
		return -1;
	if(Command.size()!=2)
		return -2;
	if(Value <0)
		return 0;
	int success=0;
	uint8_t commandValue[]={(Value>>24) &0xff, (Value>>16)&0xff, (Value>>8)&0xff,Value&0xff};
	lock();
	sendRemoteATRequest(AddrMSB, AddrLSB, Command[0], Command[1], commandValue, sizeof(commandValue));
			if(checkRemoteResponse(Resp, Command[0],Command[1], timeout))
			{
				applyChanges(AddrMSB,AddrLSB);
				success= 1;

			}
	release();
	return success;
}


#endif

//int XBeeSystem::pop_regQ(int index)
void XBeeSystem::pop_regQ()
{
	//if(index >regQ.size() || index<0)
	//	return -1; 
    regQ.erase(regQ.begin()); // pop the front of the queue
	//regQ.erase(reqQ.begin()+index); // erase the index
}

void XBeeSystem::push_regQ(Sensor sensor)
{
	regQ.push_back(sensor);
}

Nodes XBeeSystem::get_regQ()
{
	Node myNode =Node();// create a node structure 
	Nodes myArray =Nodes();
		if(regQ.size()== 0)
		  {
				return myArray;
		  }
		 else
		  {
			for(int i=0; i<regQ.size(); i++)
			 {

			myNode.data.Name = regQ.at(i).data.Name;
			myNode.data.SN_High =regQ.at(i).data.SN_High; 
			myNode.data.SN_Low=regQ.at(i).data.SN_Low;  
			myNode.data.Address16=regQ.at(i).data.Address16; 
			myNode.data.parentAddress =regQ.at(i).data.parentAddress;
			myNode.data.DigitalMask =regQ.at(i).data.DigitalMask;
			myNode.data.AnalogMask =regQ.at(i).data.AnalogMask;
			myNode.data.digital_data=regQ.at(i).data.digital_data; 
			myNode.data.deviceType=regQ.at(i).data.deviceType;
			myNode.data.sampleRate=regQ.at(i).data.sampleRate;
			myNode.data.sleepRate=regQ.at(i).data.sleepRate;
			myNode.data.timeUntilSleep = regQ.at(i).data.timeUntilSleep;
			myNode.data.numberOfChildren =regQ.at(i).data.numberOfChildren;
			for(int k=0; k<4; k++)
			{ 
			 myNode.data.analog_data[k]=regQ.at(i).data.analog_data[k];// Analog data  (up to for channels ) 
			 myNode.data.minimum[k] =regQ.at(i).data.minimum[k];
			 myNode.data.maximum[k]=regQ.at(i).data.maximum[k];
			}
			myArray.nodes.push_back(myNode);
			}
			return myArray;
		}
}



int XBeeSystem::Write(int index, int timeout)
{
	if(index<0||index>=mySensors.size())
			return -1;
	 //Resp = XBeeResponse();
	int success =0;
	lock();
	RemoteATRequestRead(mySensors.at(index).data.SN_High, mySensors.at(index).data.SN_Low, 'W', 'R');
	if(checkRemoteResponse(Resp, 'W','R', timeout))
	{
		printf("Writing Parameters  to Sensor %X%X was Successful \n",mySensors.at(index).data.SN_High, mySensors.at(index).data.SN_Low );
		success=1;
	}
	release();
	return success;
}

int XBeeSystem::Write(int AddrMSB, int AddrLSB, int timeout)
{
	if(AddrMSB<0||AddrLSB)
			return -1;
	int success=0;
	 //Resp = XBeeResponse();
	lock();
	RemoteATRequestRead(AddrMSB, AddrLSB, 'W', 'R');
	if(checkRemoteResponse(Resp, 'W','R', timeout))
	{
		success=1;
	}
	release();
	return success;
}

int XBeeSystem::changeSleepMode(int index, int desiredMode,int timeout)
{
	//Resp =XBeeResponse();

	if(index<0||index>=mySensors.size())
		return -1;
	if(!(desiredMode==ROUTER || desiredMode==END_DEVICE)) // 1 == Router 2== End  Device
		return -2;
	if(mySensors.at(index).data.deviceType==desiredMode)
		return 0;
	if(desiredMode ==ROUTER)
	{
		if(RemoteATSet(index, "SM", 0, timeout)>0)
		{
			mySensors.at(index).data.deviceType = ROUTER;

		 return 1;
		}
	}
	if(desiredMode==END_DEVICE)
	{/*
		uint8_t temp[]={0};
		sendRemoteATRequest(mySensors.at(index).data.SN_High, mySensors.at(index).data.SN_Low,'S', 'M', temp, sizeof(temp));
		if(checkRemoteResponse(Resp,'S','M',5000))
		{
			mySensors.at(index).data.deviceType= mode;
			return 1;
		}
		*/
		if( RemoteATSet(index, "SM",5,timeout)>0)
		{
			mySensors.at(index).data.deviceType= END_DEVICE;
		}
	}
	return 0;
}

int XBeeSystem::changeSleepRate(int index, int rate, int timeout)
{
	if(index<0||index>=mySensors.size())
			return -1;  // index out of range
	if(mySensors.at(index).data.sleepRate==rate)
		return 0;
	if(rate < MIN_SLEEP_RATE)
		rate =MIN_SLEEP_RATE; // automatically set the rate to a d
	if(rate>MAX_SLEEP_RATE)
		rate =MAX_SLEEP_RATE;

	if(mySensors.at(index).data.sampleRate<rate)
		rate= mySensors.at(index).data.sampleRate-100;// Forces the sleep rate to be less than sample rate

	//sendRemoteATRequest(mySensors.at(index).data.SN_High, mySensors.at(index).data.SN_Low, 'S', 'P', irValue, sizeof(irValue));
	if(RemoteATSet(index,"SP",rate,timeout)>0)
	{
		mySensors.at(index).data.sleepRate = rate;
		return 1;
	}
		return 0;

}

int XBeeSystem::changeSampleRate(int index, int rate, int timeout)
{
	//Resp = XBeeResponse();
	if(index<0||index>=mySensors.size())
			return -1;  // index out of range
	if(mySensors.at(index).data.sampleRate==rate)
			return 0;
	if(rate < MIN_SAMPLE_RATE)
		rate =MIN_SAMPLE_RATE; // automatically set the rate to a d
	if(rate>MAX_SAMPLE_RATE)
		rate =MAX_SAMPLE_RATE;
	//irValue[0] =rate/256;
	//irValue [1]= rate%256;
	//sendRemoteATRequest(mySensors.at(index).data.SN_High, mySensors.at(index).data.SN_Low, 'I', 'R', irValue, sizeof(irValue));
	//if(checkRemoteResponse(Resp, 'I','R', 5000))
	if(RemoteATSet(index, "IR",rate,timeout)>0)
	{mySensors.at(index).data.sampleRate = rate;
		return 1;
	}
	return 0;
}

int XBeeSystem::changeTimeUntilSleep(int index, int _timeUntilSleep, int timeout)
{
	if(index<0||index>=mySensors.size())
			return -1;  // index out of range
	if(mySensors.at(index).data.timeUntilSleep==_timeUntilSleep)
		return 0;
	if(_timeUntilSleep<MIN_TIME_UNTIL_SLEEP)
		_timeUntilSleep= MIN_TIME_UNTIL_SLEEP;
	if(_timeUntilSleep >MAX_TIME_UNTIL_SLEEP)
			_timeUntilSleep =MAX_TIME_UNTIL_SLEEP;
	if(_timeUntilSleep < mySensors.at(index).data.sampleRate)
		_timeUntilSleep =mySensors.at(index).data.sampleRate+100; //should  be longer than io sample rate

		//irValue [0] =_timeUntilSleep/256;
		//irValue [1]= _timeUntilSleep%256;
	//sendRemoteATRequest(mySensors.at(index).data.SN_High, mySensors.at(index).data.SN_Low, 'S', 'T', irValue, sizeof(irValue));
	//if(checkRemoteRequest(Resp, 'S','T', 5000))
	if(RemoteATSet(index, "ST",_timeUntilSleep,timeout)>0)
	{mySensors.at(index).data.timeUntilSleep = _timeUntilSleep;
	//applyChanges(index);
	return 1;
	}
	return 0;
}

int XBeeSystem::RemoteATSetNodeID(int index, string name, int timeout)
{
	//Resp = XBeeResponse();
		if(index<0||index>=mySensors.size())
				return -1;  // index out of range
		if(name.length()==0||name.length()>20)
			return -2;
		int success=0;
	    // allocoate some memory with the size of the new name and cast to uint8_t
		uint8_t *Value= new uint8_t[name.length()];
		//uint8_t* Value = (uint8_t*)calloc(name.length(),sizeof(uint8_t)); // create a pointer in memory
		for(int i=0; i<name.length(); i++)
		{
			Value[i]= (uint8_t)name[i]; // make a copy of each character
		}
		lock();
		sendRemoteATRequest(mySensors.at(index).data.SN_High, mySensors.at(index).data.SN_Low, 'N', 'I', Value, name.length());
		if(checkRemoteResponse(Resp,'N','I',timeout))
		{
			applyChanges(index);
			success=1;
		}
		delete [] Value;
		//free(Value);
		release();
		return success;

}


int XBeeSystem::RemoteATSetNodeID(int AddrMSB, int AddrLSB, string name, int timeout)
{
	//Resp = XBeeResponse();
		if(AddrMSB < 0 || AddrLSB < 0)
			return -1;
		if(name.length()==0||name.length()>20)
			return -2;
		int success=0;
	    // allocoate some memory with the size of the new name and cast to uint8_t
		uint8_t *Value= new uint8_t[name.length()];
		//uint8_t* Value = (uint8_t*)calloc(name.length(),sizeof(uint8_t)); // create a pointer in memory
		for(int i=0; i<name.length(); i++)
		{
			Value[i]= (uint8_t)name[i]; // make a copy of each character
		}
		sendRemoteATRequest(AddrMSB, AddrLSB, 'N', 'I', Value, name.length());

		if(checkRemoteResponse(Resp,'N','I',timeout))
		{
			applyChanges(AddrMSB,AddrLSB);
			success= 1;
		}
		delete [] Value;
		//free(Value);
		release();
		return success;

}

int XBeeSystem::changeName(int index, string name, int timeout)
{
	//Resp = XBeeResponse();
	if(index < 0|| index>=mySensors.size())
			return -1;  // index out of range
	if(mySensors.at(index).data.Name==name)
		return 0;
	if(name.length()==0||name.length()>20)
		return -2;
	if(RemoteATSetNodeID(index,name,timeout)>0)
	{
		mySensors.at(index).data.Name=name;
		//applyChanges(index);
		return 1;
	}
	return 0;
}
int XBeeSystem::getTimeSinceLastSample(int index)
{
	if(index<0 ||index >=mySensors.size())
		return -1;
	if(mySensors.at(index).data.timeSinceLastSample<0)
		return 0;
	else
	return mySensors.at(index).data.timeSinceLastSample;
}

int XBeeSystem::doNodeDiscovery(int timeout)
{
	//performs node discoverty for a specified amount of time
	// set maximum to 65 seconds for safety
	int size =0;
	if(timeout<0)
	{
		timeout = 1000;
	}
	else if(timeout>65000)
	{
		timeout =65000;
	}
	//Resp =XBeeResponse();
	ATparameterRead('N', 'D');
	timer = GetTickCount();
	while(int(GetTickCount()-timer)<timeout)
	{
		////coord->consumerSemaphore->Wait();
		coord->bufferMutex->Wait();
		size = coord->atBuffer.size();
		coord->bufferMutex->Signal();
		if(size>0)
		{
			coord->bufferMutex->Wait();
			Resp = coord->atBuffer.front();
			coord->atBuffer.pop();
			coord->bufferMutex->Signal();
			if(Resp.getApiId() == AT_COMMAND_RESPONSE)
			{
				Resp.getAtCommandResponse(atResponse);
				if(atResponse.isOk()&&atResponse.getCommand()[0] == uint8_t('N') && atResponse.getCommand()[1] == uint8_t('D'))
				{
					NodeDiscovery(Resp);
				}
			}
				else
				{   // Push back onto the queue for next reading
					coord->bufferMutex->Wait();
					coord->atBuffer.push(Resp);
					coord->atBuffer.pop();
					coord->bufferMutex->Signal();
				}
			}
		////coord->producerSemaphore->Signal();
	}
	return 0;

}

string XBeeSystem::getCoordName()
{
	return coord->Name;
}

int XBeeSystem::getCoordSN_MSB()
{
	return coord->SN_High;
}

int XBeeSystem::getCoordSN_LSB()
{
	return coord->SN_Low;
}

int XBeeSystem::main()
{
	readerExitedOk=false;
#ifdef _WIN32
	 //ClassThread<Coordinator> Thread1(this, &Coordinator::coordThread, SUSPENDED,NULL);
	 //Thread1.Resume();
	 while(!Exit)
	 {
		 Sleep(100);
		this->read();
	 }
	 //Thread1.WaitForThread();
#endif

#ifdef __linux__
	 //Sleep(1000);
	 while(Exit==false)
	 {
		//Sleep(10);
		// lock();
		//this->RemoteATRequestRead(0, 0xffff,  'N','I');
	    this->read();
		// release();
	   // printf("buffer size %d \n",coord->inputBuffer.size());
	 }
	 WaitForThread();
	 readerExitedOk=true;
	 coord->Exit();

#endif
	 return 0;
}

vector<uint8_t> XBeeSystem::RemoteATRead(int AddrMSB, int AddrLSB, string Command,int timeout)
{
	vector<uint8_t> output;

	if(AddrMSB<0||AddrLSB <0)
		return output;
	if(Command.length()!=2) // this checks if the argument passed is longer than 2-> throw  an error
		return output;
		 //Resp =XBeeResponse();

	lock();
	RemoteATRequestRead(AddrMSB,AddrLSB,Command[0], Command[1]);

	if(checkRemoteResponse(Resp,Command[0],Command[1], timeout))
	{
		Resp.getRemoteAtCommandResponse(remoteAtResponse);
		for( int i=0; i< remoteAtResponse.getValueLength(); i++)
		{
			output.push_back(remoteAtResponse.getValue()[i]);
		}
	}
	release();
	return output;

}

vector<uint8_t> XBeeSystem::RemoteATRead(int index, string Command,int timeout)
{
	vector<uint8_t> output;

	if(index<0 || index>=mySensors.size())
		return output;
	if(Command.length()!=2) // this checks if the argument passed is longer than 2-> throw  an error
		return output;
	//Resp =XBeeResponse();
		lock();
		RemoteATRequestRead(mySensors.at(index).data.SN_High,mySensors.at(index).data.SN_Low,Command[0], Command[1]);
		if(checkRemoteResponse(Resp,Command[0],Command[1], timeout))
		{
			Resp.getRemoteAtCommandResponse(remoteAtResponse);
			for( int i=0; i< remoteAtResponse.getValueLength(); i++)
			{
				output.push_back(remoteAtResponse.getValue()[i]);
			}
		}
		release();
		return output;
}


int XBeeSystem::setDefaultSampleRate(int rate)
{
	if(rate<MIN_SAMPLE_RATE)
	{
		defaultSampleRate=MIN_SAMPLE_RATE;
		return -1;
	}
	if(rate>MAX_SAMPLE_RATE)
	{
		defaultSampleRate=MAX_SAMPLE_RATE;
		return -2;
	}
	defaultSampleRate = rate;
	return 1;

}

bool XBeeSystem::checkAtResponse(XBeeResponse &Resp,uint8_t command0, uint8_t command1, int timeout =5000)
{
	int size =0;
	bool success=false;
	if(timeout<0)
	{
		timeout = 1000;
	}
	else if(timeout>25000)
	{
		timeout =25000;
	}
	timer = GetTickCount();

	while(int(GetTickCount()-timer)<timeout)
	{
		////coord->consumerSemaphore->Wait();
		coord->bufferMutex->Wait();
		size = coord->atBuffer.size();
		coord->bufferMutex->Signal();
		if(size>0)
		{
			coord->bufferMutex->Wait();
			Resp = coord->atBuffer.front();
			coord->atBuffer.pop();
			coord->bufferMutex->Signal();
			if(Resp.getApiId() == AT_COMMAND_RESPONSE)
			{
				Resp.getAtCommandResponse(atResponse);
				//if(remoteAtResponse.isOk())
				if(atResponse.isOk()&&atResponse.getCommand()[0] == command0 && atResponse.getCommand()[1] == command1)
				{
					success= true;
					break;
				}
				else if(atResponse.isError()&&atResponse.getCommand()[0] == command0 && atResponse.getCommand()[1] == command1)
				{
					success =false;
					break;
				}
			}
			else
			{   // Push back onto the queue for next reading
				coord->bufferMutex->Wait();
				coord->atBuffer.push(Resp);
				//coord->atBuffer.pop();
				coord->bufferMutex->Signal();
			}
		}
		Sleep(1);
		////coord->producerSemaphore->Signal();
	}
	return success;
}



bool XBeeSystem::checkRemoteResponse(XBeeResponse &Resp,uint8_t command0, uint8_t command1, int timeout =5000)
{
	int size =0;
	bool success=false;
	if(timeout<0)
	{
		timeout = 1000;
	}
	else if(timeout>25000)
	{
		timeout =25000;
	}
	timer = GetTickCount();
	while(int(GetTickCount()-timer)<timeout)
	{
		//coord->consumerSemaphore->Wait();
		coord->bufferMutex->Wait();
		size = coord->atBuffer.size();
		coord->bufferMutex->Signal();
		if(size>0)
		{
			coord->bufferMutex->Wait();
			Resp = coord->atBuffer.front();
			coord->atBuffer.pop();
			coord->bufferMutex->Signal();
			if(Resp.getApiId() == REMOTE_AT_COMMAND_RESPONSE)
			{
				Resp.getAtCommandResponse(remoteAtResponse);
				//if(remoteAtResponse.isOk())
				if(remoteAtResponse.isOk()&&remoteAtResponse.getCommand()[0] == command0 && remoteAtResponse.getCommand()[1] == command1)
				{
					success= true;
					break;
				}
				else if(remoteAtResponse.isError()&&remoteAtResponse.getCommand()[0] == command0 && remoteAtResponse.getCommand()[1] == command1)
				{
					success= false;
					break;
				}
			}
			else
			{   // Push back onto the queue for next reading
				coord->bufferMutex->Wait();
				coord->atBuffer.push(Resp);
				//coord->atBuffer.pop();
				coord->bufferMutex->Signal();
			}
		}
		Sleep(1);
		//coord->producerSemaphore->Signal();
	}
	return success;
}

int XBeeSystem::getSampleRate(int index,int timeout)
{
	if(index<0||index>=mySensors.size())
		return -1;
	int success=0;
	// Resp = XBeeResponse();

	lock();
	RemoteATRequestRead(mySensors.at(index).data.SN_High, mySensors.at(index).data.SN_Low,'I','R');
	if(checkRemoteResponse(Resp,'I','R', timeout))
	{
		Resp.getAtCommandResponse(remoteAtResponse);
		mySensors.at(index).data.sampleRate = (int(remoteAtResponse.getValue()[0]<<8) + int(remoteAtResponse.getValue()[1]));
		success=1;
		if(mySensors.at(index).data.sampleRate<MIN_SAMPLE_RATE|| mySensors.at(index).data.sampleRate>MAX_SAMPLE_RATE)
		{
			irValue [0]=mySensors.at(index).data.sampleRate/256;
			irValue [1]=mySensors.at(index).data.sampleRate%256;
			//irValue [0]=defaultSampleRate /256;
			//irValue [1]=defaultSampleRate %256;
			sendRemoteATRequest(mySensors.at(index).data.SN_High, mySensors.at(index).data.SN_Low,'I','R',irValue, sizeof(irValue));
			if(checkRemoteResponse(Resp,'I','R',5000))
			{
				applyChanges(index);
				success= 1;
			}
		}
		else success =0;
	}
	release();
		return success;

}


bool XBeeSystem::getSampleRates(int timeout)
{
	unsigned long time;
	int fractionOfTimeout =timeout/mySensors.size();
	time=GetTickCount();
	for(int i =0; i<mySensors.size(); i++)
	{
	// RemoteATRequestRead(mySensors.at(i).data.SN_High, mySensors.at(i).data.SN_Low,'I','R');
	 getSampleRate(i,fractionOfTimeout);
	 if(int(GetTickCount()-time)>timeout)
		 return false;
	}
	return 0;

}

int XBeeSystem::getSleepInfo(int index, int timeout)
{
	if(index<0 || index>=mySensors.size())
		return -1;
	int success =0;
	//Resp =XBeeResponse();
	lock();
	RemoteATRequestRead(mySensors.at(index).data.SN_High, mySensors.at(index).data.SN_Low,'S','P');
	if(checkRemoteResponse(Resp,'S', 'P',timeout/2))
	{
		Resp.getRemoteAtCommandResponse(remoteAtResponse);
				//mySensors.at(index).data.sleepRate = mySensors.at(index).data.sleepRate
				mySensors.at(index).data.sleepRate = (int(remoteAtResponse.getValue()[0]<<8) + int(remoteAtResponse.getValue()[1]));
				success=1;
	}
	else success=0;
	RemoteATRequestRead(mySensors.at(index).data.SN_High, mySensors.at(index).data.SN_Low,'S','N');
	if(checkRemoteResponse(Resp,'S', 'N',timeout/2))
	{
		Resp.getAtCommandResponse(remoteAtResponse);
		mySensors.at(index).data.numberSleepCycles=(int(remoteAtResponse.getValue()[0]<<8) + int(remoteAtResponse.getValue()[1]));
		//mySensors.at(index).data.sleepRate = temp*mySensors.at(index).data.sleepRate;
		success=1;
	}
	else success=0;
	RemoteATRequestRead(mySensors.at(index).data.SN_High, mySensors.at(index).data.SN_Low,'S','T');
	if(checkRemoteResponse(Resp,'S', 'T',timeout/2))
		{
			Resp.getAtCommandResponse(remoteAtResponse);
		    mySensors.at(index).data.timeUntilSleep = (int(remoteAtResponse.getValue()[0]<<8) + int(remoteAtResponse.getValue()[1]));
		    int success=1;
		}
	else success=0;

	release();
	return success;
}

int XBeeSystem::setPAN_ID(int PAN_ID)
{
	 //Resp =XBeeResponse();
	if(PAN_ID <0 || PAN_ID >0xFFFF)
	{
		printf("Invalid PAN ID ... Setting PAN ID to 1234 \n");
		PAN_ID=0x1234;
	}
	int success=0;
	command [0] = PAN_ID/256;
	command [1] = PAN_ID%256;
	lock();
	ATparameterSet('O','P',command,sizeof(command));
	if(checkAtResponse(Resp,'O', 'P',5000))
	{
		printf("Successfully set PAN_ID \n");
		success= 1;
	}
	else
	{	printf("Setting PAN_ID was unsuccessful... Using default PAN_ID");

	}
	release();
	return success;
}

void XBeeSystem::applyChanges(int index)
{
	RemoteATRequestRead(mySensors.at(index).data.SN_High, mySensors.at(index).data.SN_Low,'A','C');
}

void XBeeSystem::applyChanges(int addressMSB, int addressLSB)
{
	RemoteATRequestRead(addressMSB, addressLSB,'A','C');
}

bool XBeeSystem::getNodeID(int index,int timeout)
{
	//Resp = XBeeResponse();
	bool success=false;
	lock();
	RemoteATRequestRead(mySensors.at(index).data.SN_High, mySensors.at(index).data.SN_Low,  'N','I');
	 if(checkRemoteResponse(Resp,'N','I',timeout))
	 {
		 Resp.getRemoteAtCommandResponse(remoteAtResponse);
		 for (int i = 0; i < remoteAtResponse.getValueLength(); i++)
		 {
			 printf("%c",remoteAtResponse.getValue()[i]);
			 mySensors.at(index).data.Name += (char)(remoteAtResponse.getValue()[i]);
		 }
		 success= true;
	 }
	 release();
	 return success;
}

bool XBeeSystem::getSleepMode(int index, int timeout)
{
	bool success=0;
	lock();
	RemoteATRequestRead(mySensors.at(index).data.SN_High, mySensors.at(index).data.SN_Low,  'S','M' );
	//Resp = XBeeResponse();
	if(checkRemoteResponse(Resp,'S','M',timeout))
	{
		Resp.getRemoteAtCommandResponse(remoteAtResponse);
		if(remoteAtResponse.getValue()[0]==0)
			mySensors.at(index).data.deviceType ==ROUTER;
		else if(remoteAtResponse.getValue()[0]>0)
			mySensors.at(index).data.deviceType ==END_DEVICE;
		success= true;
	}
	release();
	return success;
}

bool XBeeSystem::getParentAddress(int index, int timeout)
{
	if(index<0 || index>=mySensors.size())
		return 0;
	bool success=0;
	//Resp = XBeeResponse();
	lock();
	RemoteATRequestRead(mySensors.at(index).data.SN_High, mySensors.at(index).data.SN_Low,  'M','P');
	if(checkRemoteResponse(Resp,'M','P',timeout))
	{
		Resp.getRemoteAtCommandResponse(remoteAtResponse);
			mySensors.at(index).data.parentAddress =  (int)(remoteAtResponse.getValue()[0]<<8) + (int)remoteAtResponse.getValue()[1];
		success =true;
	}
	release();
	return false;
}

bool XBeeSystem::getNumberOfChildren(int index, int timeout)
{
	if(index<0 || index>=mySensors.size())
			return 0;
	bool success=false;
	//Resp = XBeeResponse();
	lock();
	RemoteATRequestRead(mySensors.at(index).data.SN_High, mySensors.at(index).data.SN_Low,  'N','C'); // The AT command MP returns the parent address of the XBEE
	if(checkRemoteResponse(Resp,'N','C',timeout))
	{
		Resp.getRemoteAtCommandResponse(remoteAtResponse);

			mySensors.at(index).data.numberOfChildren =  int(remoteAtResponse.getValue()[0]);
		success= true;
	}
	release();
	return success;
}

void XBeeSystem::softCommissionButtonPress(int index, int presses)
{
	if(index<0 || index>=mySensors.size())
		return;
	if(presses<0 || presses>5)
		return;
	uint8_t temp[]={presses};
	sendRemoteATRequest(mySensors.at(index).data.SN_High, mySensors.at(index).data.SN_Low,'C','B',temp, sizeof(temp));
	applyChanges(index);
}

void XBeeSystem::enableDataLogging(string _fileName)
{
	fileName=_fileName;
	dataloggingEnabled =true;
}
int XBeeSystem::disableDataLogging()
{
	if(dataloggingEnabled==false)
		return 1;
	else{
	logFile.close();
	dataloggingEnabled =false;
	return !logFile.is_open();
	}
}

bool XBeeSystem::openFile()
{
	if(dataloggingEnabled)
	{
		if(logFile.is_open())
		{
			return true;
		}
			logFile.open((fileName).c_str(), ios::out | ios::app);
			//logFile.open("fileName.txt", ios::out | ios::app);
			if(logFile.is_open())
			{

				return true;
			}
	}
	return false;
}


void XBeeSystem::printSensorReadingToFile(int index)
{
	if(index<0|| index>=mySensors.size())
		return;
	if(!dataloggingEnabled)
	{
		return;
	}
	time_t now;
	char lastSampleString[20];
	int len;
	len = sprintf(lastSampleString,"%.0f",mySensors.at(index).data.lastSample);
	if(openFile())
	{
		logMutex.Wait();
		time(&now);
		string time=ctime(&now);
		time[time.length()-1]=' ';
		logFile<<"["<<time<<";"<<lastSampleString<<";"<<index<<";"
				<<hex<<mySensors.at(index).data.SN_High<<";"
				<<hex<<mySensors.at(index).data.SN_Low<<";";
		logFile<<mySensors.at(index).data.Name<<";";
		logFile<<hex<<mySensors.at(index).data.Address16<<";";
		logFile<<dec<<mySensors.at(index).data.deviceType<<";";
		logFile<<hex<<mySensors.at(index).data.parentAddress<<";";
		logFile<<"channels:"<<dec<<mySensors.at(index).data.minimum[0]<<" "
				<<mySensors.at(index).data.maximum[0]<<","
				<<mySensors.at(index).data.minimum[1]<<" "
				<<mySensors.at(index).data.maximum[1]<<","
				<<mySensors.at(index).data.minimum[2]<<" "
				<<mySensors.at(index).data.maximum[2]<<","
				<<mySensors.at(index).data.minimum[3]<<" "
				<<mySensors.at(index).data.maximum[3]<<";";
		logFile<<printAnalog(index,";")<<printDigital(index,";")<<"RX:";
		for(int i=0; i<mySensors.at(index).data.rxData.size();i++)
		{
			logFile<<dec<<(int)(mySensors.at(index).data.rxData[i])<<":";
		}

				logFile<<";"<<dec<<mySensors.at(index).data.timeSinceLastSample<<";]"<<endl;

				logFile.close();

				logMutex.Signal();
	}


}
void XBeeSystem::printMsgToFile(string message)
{
	msg.str("");
	if(dataloggingEnabled)
	{
	time_t now;
	char timeStamp[20];
	int len;
	len = sprintf(timeStamp,"%.0f",getMillisecondTime());
	if(openFile())
		{
			logMutex.Wait();
			time(&now);
			string time=ctime(&now);
			int len=time.length();
			time[len-1]=' ';
			logFile<<"["<<time<<";"<<dec<<timeStamp<<";"<<message<<"]"<<endl;
			logFile.close();
			logMutex.Signal();
		}
	}
	msg.clear();
	msg.str("");

}

int XBeeSystem::changePinMode(int index, string pinNumber, int value, int timeout)
{
	return RemoteATSet(index, pinNumber,value,timeout);

}

int XBeeSystem::changePinMode(int AddrMSB, int AddrLSB, string pinNumber, int value, int timeout)
{
	return RemoteATSet(AddrMSB, AddrLSB, pinNumber,value,timeout);

}

int XBeeSystem::queryPinMode(int index,string pinNumber,int timeout)
{
	if(index<0 ||index<=mySensors.size())
		return -1;
	if(pinNumber.size()!=2)
		return -2;
	vector<uint8_t> value;

	value=RemoteATRead(index, pinNumber,timeout);
	if(value.size()==1)
		return (int)value[0];
	else
		return -3;

}


int XBeeSystem::queryPinMode(int AddrMSB, int AddrLSB, string pinNumber, int timeout)
{
	if(AddrMSB<0 || AddrLSB<0 )
		return -1;
	if(pinNumber.size()!=2)
		return -2;
	vector<uint8_t> value;

	value=RemoteATRead(AddrMSB,AddrLSB, pinNumber,timeout);
	if(value.size()==1)
		return (int)value[0];
	else
		return -3;

}

bool XBeeSystem::checkRxResponse(XBeeResponse &Resp, int timeout)
{
	bool success=false;
	int size=0;
	timer = GetTickCount();
		while(int(GetTickCount()-timer)<timeout)
		{
			coord->bufferMutex->Wait();
			size = coord->inputBuffer.size();
			coord->bufferMutex->Signal();
			if(size>0)
			{
				coord->bufferMutex->Wait();
				Resp = coord->inputBuffer.front();
				coord->inputBuffer.pop();
				coord->bufferMutex->Signal();
				if(Resp.getApiId() == ZB_TX_STATUS_RESPONSE)
				{   ZBTxStatusResponse txStatus = ZBTxStatusResponse();
					Resp.getZBTxStatusResponse(txStatus);
					if(txStatus.getDeliveryStatus()==OK)
					{
						success= true;
						break;
					}
				}
				else
				{   // Push back onto the queue for next reading
					coord->bufferMutex->Wait();
					coord->inputBuffer.push(Resp);
					//coord->inputBuffer.pop();
					coord->bufferMutex->Signal();
				}
			}
			else
				Sleep(1);
		}
		return success;

}
int XBeeSystem::sendRxData(int AddrMSB, int AddrLSB, vector<uint8_t> Value, int timeout)
{
	if(AddrMSB<0 || AddrLSB<0 )
			return -1;
	if(Value.size()==0)
		return -2;
	bool success=false;
	XBeeAddress64 addr64 = XBeeAddress64(AddrMSB,AddrLSB);
	uint8_t* payload=new uint8_t[Value.size()];
	for(int i=0; i<Value.size(); i++)
	{
		if(int(Value[i])>255)
			return 0;
		payload[i] = (uint8_t)Value[i];
	}
	lock();
	ZBTxRequest zbTxMsg = ZBTxRequest(addr64, payload, Value.size());
	coord->serialMutex->Wait();
	coord->send(zbTxMsg);
	coord->serialMutex->Signal();
	delete [] payload;
	if(checkRxResponse(Resp, timeout))
	{
		success= 1;
	}
	release();
	 return success;
}

int XBeeSystem::sendRxData(int index, vector<uint8_t> Value, int timeout)
{
	if(index<0 ||index>=mySensors.size())
		return -1;
	if(Value.size()==0)
		return -2;
	if(sendRxData(mySensors.at(index).data.SN_High, mySensors.at(index).data.SN_Low, Value,timeout)==1)
		return 1;
	else
		return 0;

}

int XBeeSystem::forceSample(int index,int timeout)
{
	if(index<0 ||index>=mySensors.size())
		return 0;
	lock();
	RemoteATRequestRead(mySensors.at(index).data.SN_High, mySensors.at(index).data.SN_Low, 'I','S');
	if(checkRemoteResponse(Resp, 'I', 'S', timeout))
	{
		Resp.getRemoteAtCommandResponse(remoteAtResponse);
		mySensors.at(index).data.timeSinceLastSample = (Resp.getTimeStamp()-mySensors.at(index).data.lastSample);
		mySensors.at(index).data.lastSample=Resp.getTimeStamp();
		mySensors.at(index).data.DigitalMask=(int)((remoteAtResponse.getValue()[1]&0x1c)<<8) + (int)remoteAtResponse.getValue()[2];
		mySensors.at(index).data.AnalogMask=(int)(remoteAtResponse.getValue()[3]&0x8f);
		int pos=4;
		cout<<int(mySensors.at(index).data.DigitalMask)<<endl;
		if(int(mySensors.at(index).data.DigitalMask) >0)
		{
			mySensors.at(index).data.digital_data=(int)(remoteAtResponse.getValue()[4]<<8) + (int) remoteAtResponse.getValue()[5];
			pos=6;
			cout<<" Updated Digital"<<endl;
		}
		if(mySensors.at(index).data.AnalogMask>0)
		{
			for(int i=0; i<4; i++)
			{
				uint8_t temp =mySensors.at(index).data.AnalogMask;
				int analog=(int)((remoteAtResponse.getValue()[pos]<<8)+ remoteAtResponse.getValue()[pos+1]);
				if(((temp>>i)&1) ==1)
				{
					mySensors.at(index).data.analog_data[i]= dataMapping(analog,0,1024,mySensors.at(index).data.minimum[i],mySensors.at(index).data.maximum[i]);
					pos+=2;
				}
			}
		}
	}
 release();
 if(dataloggingEnabled)
 {
	 printSensorReadingToFile(index);
 }
return 1;
}


int XBeeSystem::lock(){
	return readerMutex.Wait();

}

int XBeeSystem::release()
{
	return readerMutex.Signal();

}


