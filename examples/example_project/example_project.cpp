/**
 *  
 *  @example example_project.cpp
 */

#include <libserial/SerialPort.h>
#include <libserial/SerialStream.h>

#include <iostream>
#include <unistd.h> // for sleep() function
using namespace std;
using LibSerial::SerialPort ;

int main()
{   
    SerialPort serial_port ;

    // Open hardware serial ports using the Open() method.
    cout << "serial_portl.Open("<<"/dev/ttyACM0"<<")"<< endl;
    serial_port.Open( "/dev/ttyACM0" ) ; //connected ARDUINO 

    // Set the baud rates.
    using LibSerial::BaudRate ;
    serial_port.SetBaudRate( BaudRate::BAUD_115200 ) ;
    cout << "BAUD_115200" << endl;

    // Create a few variables with data we can send.
    char write_byte_1 = '1';

    for(int i=0; i <5 ; i++){
        cout<<"Write to ARDUINO:" << i+1 << endl;
        serial_port.WriteByte(write_byte_1) ;
        sleep(3);   
    }
    cout << "serial_port.Close()" << endl;
    serial_port.Close() ;
}
