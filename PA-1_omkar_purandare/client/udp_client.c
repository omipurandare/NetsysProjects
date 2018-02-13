/****************************************************************************************************************
Program: UDP client
Author: Omkar Purandare
Subject: Network Systems
*****************************************************************************************************************/
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <strings.h>

#define MAXBUFSIZE 1028		//Mac Packet size with seq number
int sock;                           //This will be our socket
struct sockaddr_in remote;     //"Internet socket address structure"
void send_file_to_server(char* data, size_t length);
void encrypt(char* data,int length);
void decrypt(char* data,int length);

int main (int argc, char * argv[])
{

	                             //this will be our socket
	char buffer[MAXBUFSIZE];
	char command[MAXBUFSIZE];

	if (argc < 3)
	{
		printf("USAGE:  <server_ip> <server_port>\n");
		exit(1);
	}

	/*Server information filled in the structure*/
	bzero(&remote,sizeof(remote));               //zero the struct
	remote.sin_family = AF_INET;                 //address family
	remote.sin_port = htons(atoi(argv[2]));      //sets port to network byte order
	remote.sin_addr.s_addr = inet_addr(argv[1]); //sets remote IP address

	/**Causes the system to create a generic socket of type UDP (datagram)***/
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("unable to create socket");
	}
	printf("Socket created\n");

	/****Client runs in a loop continuosly after it is created******/
	while(1)
	{
		struct timeval tv;
		tv.tv_sec = 600;
		tv.tv_usec = 100000;
		if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) 	//Sending a timeout of 10min for normal recvfrom() function
    			perror("Error");
		bzero(command,sizeof(command));
		printf("\n");
		printf("Enter any of the command:\n");
		printf("1) To download any file from server : get(file_name)\n");
		printf("2) To send any file to the server : put(file_name)\n");
		printf("3) To delete any file in the server : delete(file_name)\n");
		printf("4) To list all files the server : ls\n");
		printf("5) To shut the server down gracefully : exit\n");
		printf("/*****************************************************************/\n");
		printf("\n");
		printf("Enter the command you want to send to the server:\t");		//Messages for user 
		scanf("%s",command);
		if (sendto(sock, command , strlen(command), 0, (struct sockaddr *)&remote,sizeof(remote)) < 0)	//sending command to the server
		printf("error sending data to server\n");
		struct sockaddr from_addr;
		bzero(buffer,sizeof(buffer));
	  	socklen_t size = sizeof(from_addr);
		recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&from_addr, &size);	//waiting for any alert from the server

		/***handling response to the exit command sent to server*****/
		if(strcmp(buffer,"end") == 0)
		{
			printf("Server is closed\n");
		}

		/***handling response to the ls command sent to server*****/
		else if(strcmp(buffer,"list") == 0)
		{
			recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&from_addr, &size);	//listing the files in server
			printf("The files in server are:\n");
			printf("%s\n",buffer);
		}

		/***handling response to the get command sent to server*****/
		else if(strcmp(buffer,"incoming_file") == 0)
		{
			int rec_no_bytes,number_of_packets;
			uint32_t seq_no,j;
			char file_to_receive[100];
			bzero(file_to_receive,sizeof(file_to_receive));
			recvfrom(sock,file_to_receive,sizeof(file_to_receive), 0, (struct sockaddr *)&from_addr, &size); //receive name of file
			/***only excute this if file is present****/
			if(strcmp(file_to_receive,"File_nf")!=0)
			{
				recvfrom(sock,(char*)(&number_of_packets),sizeof(number_of_packets), 0, (struct sockaddr *)&from_addr, &size); //receive no. of packets
				FILE *fp1;
				fp1=fopen(file_to_receive,"w+");	//opening the file with the received name of the file
				for(uint32_t i=0;i<number_of_packets;)
				{
					bzero(buffer,sizeof(buffer));
					rec_no_bytes = recvfrom(sock,buffer,sizeof(buffer), 0, (struct sockaddr *)&from_addr, &size);	//receive packets
					memcpy((char*)&seq_no, &buffer[rec_no_bytes-4], sizeof(seq_no));		//copy the seq no from the packets
					/***if seq no is as expected execute this******/
					if(seq_no==i){
				
						sendto(sock, (char*)(&i) , sizeof(i), 0, (struct sockaddr *)&remote,sizeof(remote));	//Send back ACK with packet number just received
						decrypt(buffer,rec_no_bytes-4);			//decrypt the received packet
						if(fwrite(buffer,1,rec_no_bytes-4,fp1)<0)			//writing packets to file
							{
								printf("error writting file\n");
								exit(1);
							}
						else
							printf("Packet written sucessfully:%d\n",seq_no);
						i++;	//incrementing to receive the next packet
					}
					else
					{
						j=i-1;
						sendto(sock, (char*)(&j) , sizeof(j), 0, (struct sockaddr *)&remote,sizeof(remote));	//send back the old packet number if seq no doesn't match to expected
					}

				}
				printf("File written sucessfully to client!!\n");
				fclose(fp1);	//close the file
			}
			else
				printf("File not found in server\n");
		}

		/***handling response to the put command sent to server*****/
		else if(strcmp(buffer,"putting_file")==0)
		{
			bzero(buffer,sizeof(buffer));
			recvfrom(sock,buffer,sizeof(buffer), 0, (struct sockaddr *)&from_addr, &size);		//getting the file name to send
			FILE *fp;
			fp=fopen(buffer,"rb");		//opening the file
			bzero(buffer,sizeof(buffer));
			/***only execute this if the file exists*****/
			if(fp!=NULL)
			{
				sendto(sock, "file_der" , 9, 0, (struct sockaddr *)&remote, sizeof(remote));	//send alert to server that file exists
				fseek(fp,0,SEEK_END);
				size_t file_size=ftell(fp);		//Getting the file size
				fseek(fp,0,SEEK_SET);
				uint32_t ack_no,stop_reading;
				uint32_t Full_packets=file_size/1024;
				uint32_t actual_packets=Full_packets+1;		//calculating the number of packets based on file size
				if(sendto(sock, (char*)(&actual_packets), sizeof(actual_packets), 0, (struct sockaddr *)&remote, sizeof(remote))<0) //sending number of packets
					printf("Value not sent");
				stop_reading=0;
				struct timeval tv;
				tv.tv_sec = 0;
				tv.tv_usec = 300000;
				if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) 	//setting timeout for resending the packets 
		    			perror("Error");
				for(uint32_t i=0;i<Full_packets;)
				{
					/****only excute if ACK_no matches to sent packet***/
					if(stop_reading==0)	
					{
						bzero(buffer,sizeof(buffer));
						if(fread(buffer,1024,1,fp)<=0)	//read one packet
						{
							printf("unable to copy file into buffer\n");
							exit(1);
						}
						memcpy(&buffer[1024], (char*)&i, sizeof(i));	//append the seq_no
						encrypt(buffer,1024);				//encrypt the packet
					}
					printf("Sending packet:%d\n",i);
					send_file_to_server(buffer, 1028);  //sending all full packets
					recvfrom(sock, (char*)(&ack_no), sizeof(ack_no), 0, (struct sockaddr *)&remote, &size);	//receiving the ACK_no
					/**checking if ack_no is equal to expected**/
					if(ack_no==i)
					{
						i++;		//incrementing the expected number 
						stop_reading=0;

					}
					else
						stop_reading=1;
				}
				/**Sending the extra bytes in last packet****/
				int extra_bytes;
				bzero(buffer,sizeof(buffer));	
				extra_bytes = file_size%1024;	//calculating the extra bytes
				if(fread(buffer,extra_bytes,1,fp)<=0)	//reading extra bytes
				{
					printf("unable to copy file into buffer\n");
					exit(1);
				}
				memcpy(&buffer[extra_bytes], (char*)&Full_packets, sizeof(Full_packets));	//copying last packet number
				encrypt(buffer,extra_bytes);		//encrypting the last packet
				/***checking for last ACK****/
				while(1)
				{
					send_file_to_server(buffer, extra_bytes+4);
					bzero(command,sizeof(command));
					recvfrom(sock, (char*)(&ack_no), sizeof(ack_no), 0, (struct sockaddr *)&remote, &size);	
					if(ack_no==Full_packets)
						break;
				}
				printf("File sent sucessfully\n");
				sleep(2);
			}
			else
			{
				printf("The file doesnot exit\n");
				sendto(sock, "file_not" , 9, 0, (struct sockaddr *)&remote, sizeof(remote));
			}
		}


		/***handling response to the exit delete sent to server*****/
		else if(strcmp(buffer,"delete_file") == 0)
		{
			bzero(buffer,sizeof(buffer));
			recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&from_addr, &size);	//recive update about delete from the server
			if(strcmp(buffer,"delete_sucess") == 0)		//delete sucess
				printf("File deleted in server\n");
			else
				printf("File delete error or file not found\n");	//delete fail

		}

		/***handling response to the invalid command sent to server*****/
		else if(strcmp(buffer,"Invalid") == 0)
		{
			printf("Enter a valid command\n");
		}

	}
	close(sock);	//close the client but this never happens as I can kill the client by ctrl C

}

/***Function to send file packet to server*****/
void send_file_to_server(char* data, size_t length)
{
	if (sendto(sock, data , length, 0, (struct sockaddr *)&remote, sizeof(remote)) < 0)
	printf("error sending packet to server\n");
}


/****Function to encrypt data********/
void encrypt(char* data,int length)
{
	char key='c';
	for(int i=0;i<length;i++)
	{
		*data=*data ^key;
		data++;
	}

}


/****Function to decrypt data****/
void decrypt(char* data,int length)
{
	char key='c';
	for(int i=0;i<length;i++)
	{
		*data=*data ^key;
		data++;
	}

}
