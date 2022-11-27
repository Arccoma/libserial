/**
 *  
 *  @example example_project.cpp
 */
#include <libserial/SerialPort.h>
#include <libserial/SerialStream.h>
#include <iostream>
using namespace std;
using LibSerial::SerialPort ;
SerialPort serial_port ;

// communication protocol
// byte send to connected Arduino
char FULL_BATTERY			= '0';
char LOW_BATTERY_WARNING 	= '1'; 
char VEHICLE_RTL_ALARAM 	= '2';
char RC_LOSS_ALARAM			= '3';

#define _3CELL_WARNING_LEVEL	11100 // [mV]
#define _4CELL_WARNING_LEVEL	14800 
#define _6CELL_WARNING_LEVEL	22200 
#define _12CELL_WARNING_LEVEL	44400 


///////////////////////////////////////////////////////////
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h> // for sleep() function
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#if (defined __QNX__) | (defined __QNXNTO__)
/* QNX specific headers */
#include <unix.h>
#else
/* Linux / MacOS POSIX timer headers */
#include <sys/time.h>
#include <time.h>
#include <arpa/inet.h>
#include <stdbool.h> /* required for the definition of bool in C99 */
#endif
/* This assumes you have the mavlink headers on your include path
 or in the same folder as this source file */
#include <mavlink.h>

#define BUFFER_LENGTH 2041 // minimum buffer size that can be used with qnx (I don't know why)

//////////////////////////////////// Variables  //////////////////////////////


mavlink_message_t msg;
mavlink_status_t status;
uint16_t len;
uint8_t buf[BUFFER_LENGTH];
int bytes_sent;

struct sockaddr_in gcAddr; 
struct sockaddr_in locAddr;
//struct sockaddr_in fromAddr;
ssize_t recsize;
socklen_t fromlen = sizeof(gcAddr);

int i = 0;
unsigned int cntRcv = 0;
//int success = 0;
unsigned int temp = 0;

//////////////////////////////////// Functions  //////////////////////////////
uint64_t microsSinceEpoch();
void send_mavlink_data_to_qgc(int);
void recv_mavlink_data_from_qgc(int);

/////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{   
    printf("%s", "communicate with QGC by mavlink protocol.\n");
	char help[] = "--help";
	char target_ip[100];

	// Check if --help flag was used
	if ((argc == 2) && (strcmp(argv[1], help) == 0))
    {
		printf("\n");
		printf("\tUsage:\n\n");
		printf("\t");
		printf("%s", argv[0]);
		printf(" <ip address of QGroundControl>\n");
		printf("\tDefault for localhost: udp-server 127.0.0.1\n\n");
		exit(EXIT_FAILURE);
    }
	
	// Change the target ip if parameter was given
	strcpy(target_ip, "127.0.0.1");
	if (argc == 2) {
		strcpy(target_ip, argv[1]);
    }
	
	memset(&locAddr, 0, sizeof(locAddr));
	locAddr.sin_family = AF_INET;
	locAddr.sin_addr.s_addr = INADDR_ANY;
	locAddr.sin_port = htons(14551);
	
	int sock =  socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);// main함수 전으로 빼지 못했다.
	/* Bind the socket to port 14551 - necessary to receive packets from qgroundcontrol */ 
	if (-1 == bind(sock,(struct sockaddr *)&locAddr, sizeof(struct sockaddr)))
    {
		perror("error bind failed");
		close(sock);
		exit(EXIT_FAILURE);
    } 
	
	/* Attempt to make it non blocking */
#if (defined __QNX__) | (defined __QNXNTO__)
	if (fcntl(sock, F_SETFL, O_NONBLOCK | FASYNC) < 0)
#else
	if (fcntl(sock, F_SETFL, O_NONBLOCK | O_ASYNC) < 0)
#endif

    {
		fprintf(stderr, "error setting nonblocking: %s\n", strerror(errno));
		close(sock);
		exit(EXIT_FAILURE);
    }
	
	memset(&gcAddr, 0, sizeof(gcAddr));
	gcAddr.sin_family = AF_INET;
	gcAddr.sin_addr.s_addr = inet_addr(target_ip);
	gcAddr.sin_port = htons(14550);

    ////////////////////////////////////////////////////
    std::cout << "merging mavlink_udp.c" << std::endl;
    

    // Open hardware serial ports using the Open() method.
    cout << "serial_portl.Open("<<"/dev/ttyACM0 or ttyUSB0 or 1"<<")"<< endl;
//    serial_port.Open( "/dev/ttyACM0" ) ; //connected ARDUINO MEGA
	serial_port.Open( "/dev/ttyUSB0" ) ; //conneted ARDUINO UNO

    // Set the baud rates.
    using LibSerial::BaudRate ;
    serial_port.SetBaudRate( BaudRate::BAUD_115200 ) ;
    cout << "BAUD_115200" << endl;

    

    for (;;) {
		send_mavlink_data_to_qgc(sock); //only send hearbeat package
		recv_mavlink_data_from_qgc(sock);

		//sleep(1); // Sleep one second
		usleep(500000);
    }
	cout << "close(sock)" << endl ;
	close(sock);
    
    cout << "serial_port.Close()" << endl;
    serial_port.Close() ;

}


void recv_mavlink_data_from_qgc(int sock){
	
	mavlink_battery_status_t batteryStatus;
	mavlink_statustext_t statustext;

	memset(buf, 0, BUFFER_LENGTH);
	recsize = recvfrom(sock, (void *)buf, BUFFER_LENGTH, 0, (struct sockaddr *)&gcAddr, &fromlen);
	if (recsize > 0){// Something received - print out all bytes and parse packet
		//printf("Bytes Received: %d\nDatagram: ", (int)recsize);
		for (i = 0; i < recsize; ++i)
		{
			temp = buf[i];
			//printf("%02x ", (unsigned char)temp);
			if (mavlink_parse_char(MAVLINK_COMM_0, buf[i], &msg, &status))
			{
				switch(msg.msgid){
				case MAVLINK_MSG_ID_BATTERY_STATUS:
				{
					cntRcv ++;
					printf("[%d] ",cntRcv);
					printf("Received packet: SYS: %d, COMP: %d, LEN: %d, MSG ID: %d\n", msg.sysid, msg.compid, msg.len, msg.msgid);
    				mavlink_msg_battery_status_decode(&msg, &batteryStatus);
					printf("battery voltage:%d[mV] (",batteryStatus.voltages[0]);
					cout << unsigned(batteryStatus.battery_remaining) << "% remaining)"<<endl;
					//cout << "battery cell count:" <<sizeof(batteryStatus.voltages)/sizeof(batteryStatus.voltages[0]) << endl;
						
					if(100 == batteryStatus.battery_remaining )
					{
						cout<<"Write '0' to ARDUINO:Turn off all Warning Lamp" << endl;
               			serial_port.WriteByte(FULL_BATTERY) ;
					}
					if(_4CELL_WARNING_LEVEL > unsigned(batteryStatus.voltages[0]) ){ 
               			cout<<"Write '1' to ARDUINO:Turn on Battery Warning Lamp" << endl;
               			serial_port.WriteByte(LOW_BATTERY_WARNING) ;	
					}
					break;
				}
				case MAVLINK_MSG_ID_STATUSTEXT:
				{
					cntRcv ++;
					printf("[%d] ",cntRcv);
					printf("Received packet: SYS: %d, COMP: %d, LEN: %d, MSG ID: %d\n", msg.sysid, msg.compid, msg.len, msg.msgid);
					mavlink_msg_statustext_decode(&msg, &statustext);
					printf("\n>>>>>>>> STATUSTEXT: %s <<<<<<<<<<<\n",statustext.text);  
						
					if(!strncmp("Takeoff detected",statustext.text,16))
					{
						cout << "== Takeoff == Takeoff == Takeoff == Takeoff == Takeoff =="<< endl;
					}
					if(!strncmp("RTL HOME activated",statustext.text,strlen("RTL HOME activated")))
					{	
						cout << "== RTL HOME == RTL HOME == RTL HOME == RTL HOME == RTL HOME =="<< endl;
						cout << "Write '2' to ARDUINO:"<< endl;
						serial_port.WriteByte(VEHICLE_RTL_ALARAM) ;
					}
					if(!strncmp("Manual control lost",statustext.text,strlen("Manual control lost")))
					{
						cout << "== RC Loss == RC Loss == RC Loss == RC Loss == RC Loss =="<< endl;
						cout << "Write '3' to ARDUINO:Turn on RC Loss Lamp"<< endl;
						serial_port.WriteByte(RC_LOSS_ALARAM) ;
					}
					if(!strncmp("Disarmed by landing",statustext.text,strlen("Disarmed by landing")))
					{
						cout << "== Disarm == by Landing == Disarm == by Landing == Disarm =="<< endl;
						cout<<"Write '0' to ARDUINO:Turn off all Warning Lamp" << endl;
               			serial_port.WriteByte('0') ;
					}
					break;
				}

				default:
					break;
				}//switch(msg.msgid)
			}
		}
		printf("\n");
	}
	memset(buf, 0, BUFFER_LENGTH);
}

void send_mavlink_data_to_qgc(int sock){

	//Send Heartbeat : I am a Helicopter. My heart is beating.
	mavlink_msg_heartbeat_pack(1, 200, &msg, MAV_TYPE_HELICOPTER, MAV_AUTOPILOT_GENERIC, MAV_MODE_GUIDED_ARMED, 0, MAV_STATE_ACTIVE);
	len = mavlink_msg_to_send_buffer(buf, &msg);
	bytes_sent = sendto(sock, buf, len, 0, (struct sockaddr*)&gcAddr, sizeof(struct sockaddr_in));
	//printf("send heartbeat to QGC\n");
	
	//send another data..
/*	
	//베터리 상태 전송 // arccoma2022.10.04 
	
	if(0 == battery_remaining){
		battery_remaining=100;	// 
	}
	else if( battery_remaining ){
		battery_remaining-=5;	// full charging
	}
	mavlink_msg_battery_status_pack(1, 200, &msg,0,1,1,77,voltages,0,0,-1,battery_remaining,0,1,0,0,0);
	len = mavlink_msg_to_send_buffer(buf, &msg);
	bytes_sent = sendto(sock, buf, len, 0, (struct sockaddr*)&gcAddr, sizeof(struct sockaddr_in));
	printf("send battery_status to QGC\n");


	// Send Status 
	mavlink_msg_sys_status_pack(1, 200, &msg, 0, 0, 0, 500, 11000, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	len = mavlink_msg_to_send_buffer(buf, &msg);
	bytes_sent = sendto(sock, buf, len, 0, (struct sockaddr*)&gcAddr, sizeof (struct sockaddr_in));
	printf("send system_status to QGC\n");
		
	// Send Local Position 
	mavlink_msg_local_position_ned_pack(1, 200, &msg, microsSinceEpoch(), 
										position[0], position[1], position[2],
										position[3], position[4], position[5]);
	len = mavlink_msg_to_send_buffer(buf, &msg);
	bytes_sent = sendto(sock, buf, len, 0, (struct sockaddr*)&gcAddr, sizeof(struct sockaddr_in));
	printf("send local_position to QGC\n");
		
	// Send attitude 
	mavlink_msg_attitude_pack(1, 200, &msg, microsSinceEpoch(), 1.2, 1.7, 3.14, 0.01, 0.02, 0.03);
	len = mavlink_msg_to_send_buffer(buf, &msg);
	bytes_sent = sendto(sock, buf, len, 0, (struct sockaddr*)&gcAddr, sizeof(struct sockaddr_in));
	printf("send attitude to QGC\n");
*/
}

/* QNX timer version */
#if (defined __QNX__) | (defined __QNXNTO__)
uint64_t microsSinceEpoch()
{
	
	struct timespec time;
	
	uint64_t micros = 0;
	
	clock_gettime(CLOCK_REALTIME, &time);  
	micros = (uint64_t)time.tv_sec * 1000000 + time.tv_nsec/1000;
	
	return micros;
}
#else
uint64_t microsSinceEpoch()
{
	
	struct timeval tv;
	
	uint64_t micros = 0;
	
	gettimeofday(&tv, NULL);  
	micros =  ((uint64_t)tv.tv_sec) * 1000000 + tv.tv_usec;
	
	return micros;
}
#endif
