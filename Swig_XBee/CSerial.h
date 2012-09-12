
/* this library wraps up UNIX Serial/Termios functions into functions that
 *  match the windows version of XBee System to make code portable for both systems
 *
 * Originally Class was adapted to mimic Arduino Serial functions
 */


#ifndef CSERIAL_H
#define CSERIAL_H


#include <sys/time.h>                                   // Used for TimeOut operations
#include <string.h>
#include <sys/ioctl.h> // contains control Defs for some Termios Functions
#include <stdlib.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <iostream>
#include <fcntl.h>  // contains control Defs for Termios Functions
#include <stdio.h>// File control definitions



using namespace std; 

class CSerial
{
public:
    CSerial	(); // Default Constructor 
	~CSerial (); // Destructor 

    // ::: Configuration and initialization :::

    bool    Open    (const char* myDevice,const int Bauds);      // Open a device
    bool    Close(void);                                                        // Close the device

    // ::: Read/Write operation on characters :::

    int    WriteChar   (const char Byte);                                             // Write a 
    int    ReadChar    (char *pByte,const int TimeOut_ms=NULL);         // Read a char (with timeout)
    int    ReadDataWaiting(void);
	bool Flush();

	bool	IsOpened(void){return opened;}
 
    // ::: Read/Write operation on bytes :::

    char    SendData    (const void *Buffer, const unsigned int NbBytes); // Write an array of bytes
    int     ReadData     (void *Buffer,unsigned int MaxNbBytes);


private:
    bool opened;
    int  fd;
    int _Bauds;


};

class TimeOut
{
public:
    TimeOut();                                                      // Constructor
    void                InitTimer();                                // Init the timer
    unsigned long int   ElapsedTime_ms();                           // Return the elapsed time since initialization
private:
    struct timeval      PreviousTime;
};

#endif // SERIALIB_H


