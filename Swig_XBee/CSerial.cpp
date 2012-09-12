

#include "CSerial.h"



CSerial::CSerial()
{
 	opened =false;
}

CSerial::~CSerial()
{
    Close();
}



   //dev/ttyS0, /dev/ttyACM0, /dev/ttyUSB0 ..
/*
     \return 1 success
     \return -1 device not found
     \return -2 error while opening the device
     \return -3 error while getting port parameters
     \return -4 Speed (Bauds) not recognized
     \return -5 error while writing port parameters
     \return -6 error while writing timeout parameters
  */
bool CSerial::Open(const char* myDevice,const int Bauds)
{   
    struct termios options;						// Structure with the device's options
	opened = false;
	_Bauds = Bauds;
	//char buffer[3];
	//char myDevice[14] = "/dev/ttyUSB"; // for FTDI CHIP
    //itoa(Device,buffer,10);
    //myDevice[11]=buffer[0];
    //myDevice[12]=buffer[1];
    fd = open(myDevice, O_RDWR | O_NOCTTY | O_NONBLOCK);			// Open port
    if (fd == -1) 
	return false;						// If the device is not open, return -1
    fcntl(fd, F_SETFL, FNDELAY);					// Open the device in nonblocking mode
    // Set parameters
    tcgetattr(fd, &options);						// Get the current options of the port
    bzero(&options, sizeof(options));					// Clear all the options
    speed_t         Speed;
    switch (_Bauds)                                                      // Set the speed (Bauds)
    {
    case 110  :     Speed=B110; break;
    case 300  :     Speed=B300; break;
    case 600  :     Speed=B600; break;
    case 1200 :     Speed=B1200; break;
    case 2400 :     Speed=B2400; break;
    case 4800 :     Speed=B4800; break;
    case 9600 :     Speed=B9600; break;
    case 19200 :    Speed=B19200; break;
    case 38400 :    Speed=B38400; break;
    case 57600 :    Speed=B57600; break;
    case 115200 :   Speed=B115200; break;
    default : return false;
}
    cfsetispeed(&options, Speed);					// Set the baud rate at 115200 bauds
    cfsetospeed(&options, Speed);
    options.c_cflag |= ( CLOCAL | CREAD |  CS8);			// Configure the device : 8 bits, no parity, no control
    options.c_iflag |= ( IGNPAR | IGNBRK );
    options.c_cc[VTIME]=0;						// Timer unused
    options.c_cc[VMIN]=0;						// At least on character before satisfy reading
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSAFLUSH, &options);
	opened = true;					// Activate the settings
    return true;                                                         // Success

}


/*
   Close the connection with the current device
*/
bool CSerial::Close()
{

    close (fd);
	opened = false;
return true; 
	 
}



int CSerial::WriteChar(const char Byte)
{

    if (write(fd,&Byte,1)!=1)                                           // Write the char
        return -1;                                                      // Error while writting
    return 1;                                                           // Write operation successfull

}



char CSerial::SendData(const void *Buffer, const unsigned int NbBytes)
{

    if (write (fd,Buffer,NbBytes)!=(ssize_t)NbBytes)                              // Write data
        return -1;                                                      // Error while writing
    return 1;                                                           // Write operation successfull

}



/*!
     \brief Wait for a byte from the serial device and return the data read
     \param pByte : data read on the serial device
     \param TimeOut_ms : delay of timeout before giving up the reading
            If set to zero, timeout is disable (Optional)
     \return 1 success
     \return 0 Timeout reached
     \return -1 error while setting the Timeout
     \return -2 error while reading the byte
  */
int CSerial::ReadChar(char *pByte,const int TimeOut_ms)
{

    TimeOut         Timer;                                              // Timer used for timeout
    Timer.InitTimer();                                                  // Initialise the timer
    while (Timer.ElapsedTime_ms()<TimeOut_ms || TimeOut_ms==0)          // While Timeout is not reached
    {
        switch (read(fd,pByte,1)) {                                     // Try to read a byte on the device
        case 1  : return 1;                                             // Read successfull
        case -1 : return -2;                                            // Error while reading
        }
    }
    return 0;
}



/*!
     \brief Read a string from the serial device (without TimeOut)
     \param String : string read on the serial device
     \param FinalChar : final char of the string
     \param MaxNbBytes : maximum allowed number of bytes read
     \return >0 success, return the number of bytes read
     \return -1 error while setting the Timeout
     \return -2 error while reading the byte
     \return -3 MaxNbBytes is reached
  */



int CSerial::ReadData (void *Buffer,unsigned int MaxNbBytes)
{

                     
    unsigned int     NbByteRead=0;
   
        unsigned char* Ptr=(unsigned char*)Buffer+NbByteRead;           // Compute the position of the current byte
        int Ret=read(fd,(void*)Ptr,MaxNbBytes-NbByteRead);              // Try to read a byte on the device
        if (Ret==-1) return -2;                                         // Error while reading
        if (Ret>0) {                                                    // One or several byte(s) has been read on the device
            NbByteRead+=Ret;                                            // Increase the number of read bytes
            if (NbByteRead>=MaxNbBytes)                                 // Success : bytes has been read
                return 1;
        }
    return 0;                                                           // Timeout reached, return 0
}


int CSerial::ReadDataWaiting(void)
{
	int bytes_waiting;
	ioctl(fd, FIONREAD, &bytes_waiting);
return bytes_waiting;
}


bool CSerial::Flush()
{
	int response;
	/*struct termios options;						// Structure with the device's options
	   				// If the device is not open, return -1
	    fcntl(fd, F_SETFL, FNDELAY);					// Open the device in nonblocking mode
	    // Set parameters
	    tcgetattr(fd, &options);						// Get the current options of the port
	    bzero(&options, sizeof(options));					// Clear all the options
	    speed_t         Speed;
	    switch (_Bauds)                                                      // Set the speed (Bauds)
	    {
	    case 110  :     Speed=B110; break;
	    case 300  :     Speed=B300; break;
	    case 600  :     Speed=B600; break;
	    case 1200 :     Speed=B1200; break;
	    case 2400 :     Speed=B2400; break;
	    case 4800 :     Speed=B4800; break;
	    case 9600 :     Speed=B9600; break;
	    case 19200 :    Speed=B19200; break;
	    case 38400 :    Speed=B38400; break;
	    case 57600 :    Speed=B57600; break;
	    case 115200 :   Speed=B115200; break;
	    default : break;
	}
	    cfsetispeed(&options, Speed);					// Set the baud rate at 115200 bauds
	    cfsetospeed(&options, Speed);
	    options.c_cflag |= ( CLOCAL | CREAD |  CS8);			// Configure the device : 8 bits, no parity, no control
	    options.c_iflag |= ( IGNPAR | IGNBRK );
	    options.c_cc[VTIME]=0;						// Timer unused
	    options.c_cc[VMIN]=0;						// At least on character before satisfy reading
	    tcflush(fd, TCIFLUSH);
	    tcsetattr(fd, TCSAFLUSH, &options);
	    */
	ioctl(fd,TCFLSH,&response);
	return true; 
}
// ******************************************
//  Class TimeOut
// ******************************************




/*!
    \brief      Constructor of the class TimeOut.
*/
// Constructor
TimeOut::TimeOut()
{}

/*!
    \brief      Initialise the timer. It writes the current time of the day in the structure PreviousTime.
*/
//Initialize the timer
void TimeOut::InitTimer()
{
    gettimeofday(&PreviousTime, NULL);
}

/*!
    \brief      Returns the time elapsed since initialization.  It write the current time of the day in the structure CurrentTime.
                Then it returns the difference between CurrentTime and PreviousTime.
    \return     The number of microseconds elapsed since the functions InitTimer was called.
  */
//Return the elapsed time since initialization
unsigned long int TimeOut::ElapsedTime_ms()
{
    struct timeval CurrentTime;
    int sec,usec;
    gettimeofday(&CurrentTime, NULL);                                   // Get current time
    sec=CurrentTime.tv_sec-PreviousTime.tv_sec;                         // Compute the number of second elapsed since last call
    usec=CurrentTime.tv_usec-PreviousTime.tv_usec;			// Compute
    if (usec<0) {														// If the previous usec is higher than the current one
        usec=1000000-PreviousTime.tv_usec+CurrentTime.tv_usec;		// Recompute the microseonds
        sec--;														// Substract one second
    }
    return sec*1000+usec/1000;
}
