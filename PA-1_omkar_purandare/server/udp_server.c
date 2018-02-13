/****************************************************************************************************************
Program: UDP server
Author: Omkar Purandare
Subject: Network Systems
*****************************************************************************************************************/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>


#define MAXBUFSIZE 1028        //Packet size with seq number
int sock;                      // socket
struct sockaddr_in remote;     //"Internet socket address structure"
void send_msg_to_client(char* data);
void send_file_to_client(char* buffer, size_t length);
void encrypt(char* data,int length);
void decrypt(char* data,int length);


int main (int argc, char * argv[] )
{
	char buffer[MAXBUFSIZE];             //a buffer to store our received message
	char response[MAXBUFSIZE];	//Buffer to transfer messages to client
	struct sockaddr_in sin;
	if (argc != 2)
	{
		printf ("USAGE:  <port>\n");
		exit(1);
	}

	bzero(&sin,sizeof(sin));                    //zero the struct
	sin.sin_family = AF_INET;                   //address family
	sin.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order
	sin.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine

	/*Causes the system to create a generic socket of type UDP (datagram)*/
	if ((sock =  socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("unable to create socket");
	}
	printf("Socket created\n");

	/*Binding the socket in server*/
	if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		printf("unable to bind socket\n");
	}
	printf("socket bound\n");
	int check=0;

	struct timeval tv;  //timespec structure for timeout
	
	/****Running the server in a loop after it is created*********/
	while(1)
	{
		printf("Waiting for command from client....\n");
		bzero(buffer,sizeof(buffer));
		socklen_t size = sizeof(remote);
		tv.tv_sec = 600;
		tv.tv_usec = 100000;
		if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) //setting the socket blocking for 10mins generally
    			perror("Error");
		recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&remote, &size); //waits for an incoming message from client
		char* command;
		command = strtok(buffer,"()");  //parsing message from client for ()
		
		/****Handling message to list the files************/
		if(strcmp(command,"ls") == 0)
		{
			strcpy(response,"list");
			send_msg_to_client(response);  //sending alert message to server that list is coming
			strcpy(response,"");
			printf("Sending the list of files to client..\n");
			DIR *d;
			struct dirent *dir;
			d = opendir(".");
			/**Listing the files in the folder***/
			if (d)
			{
				while ((dir = readdir(d)) != NULL)
				{
					if(*dir->d_name != '.')
					{
						strcat(response,dir->d_name);
						strcat(response,"\t");
					}
				}
				closedir(d);
			}
			send_msg_to_client(response);   //sending the list as string
			printf("List sent\n");
		}

		/****Handling message to close the server************/
		else if(strcmp(command,"exit")== 0)
		{
			printf("Closing the server...\n");
			strcpy(response,"end");		//sending alert message to client that sever is closing
			send_msg_to_client(response);
			check=1;
		}

		/************Handling message to send a file from server to client******/
		else if(strcmp(command,"get")== 0)
		{
			command = strtok(NULL, "()");  //getting the file name
			if(command!=NULL)
			{
				send_msg_to_client("incoming_file");	  //send prompt for incoming file
				bzero(response,sizeof(response));
				FILE *fp;
				fp=fopen(command,"rb");        //opening file
				/**only execute if file exists***/
				if(fp!=NULL)				
				{
					send_msg_to_client(command); //send filename for incoming file
					fseek(fp,0,SEEK_END);
					size_t file_size=ftell(fp);   //gettiing file size
					fseek(fp,0,SEEK_SET);
					uint32_t ack_no,stop_reading;
					stop_reading=0;
					uint32_t Full_packets=file_size/1024;
					uint32_t actual_packets=Full_packets+1;		//calculating the number of packets based on file size
					sendto(sock, (char*)(&actual_packets), sizeof(actual_packets), 0, (struct sockaddr *)&remote, sizeof(remote)); //sending number of packets
					tv.tv_sec = 0;
					tv.tv_usec = 300000;
					if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0)   //setting timeout for resending the lost packets
    						perror("Error");

					for(uint32_t i=0;i<Full_packets;)   //running a loop sending all packets
					{
						/***check if it is repeated packet****/
						if(stop_reading==0)
						{
							bzero(response,sizeof(response));
							if(fread(response,1024,1,fp)<=0)    //reading packets from file
							{
								printf("unable to copy file into buffer\n");
								exit(1);
							}
							memcpy(&response[1024], (char*)&i, sizeof(i));  //appending the packet number
							encrypt(response,1024);				//Encrypting the data
						}
						printf("Sending packet:%d\n",i);
						send_file_to_client(response, 1028);  //sending all full packets
						recvfrom(sock,(char*)(&ack_no), sizeof(ack_no), 0, (struct sockaddr *)&remote, &size);  //waiting for ack from the client
						if(ack_no==i)		//if ACK=expected then send next packet
						{
							i++;
							stop_reading=0;
						}
						else			//send old packet
							stop_reading=1;
					}
					int extra_bytes;
					bzero(response,sizeof(response));	
					extra_bytes = file_size%1024;	//calulate extra bytes for last packet
					if(fread(response,extra_bytes,1,fp)<=0)
					{
						printf("unable to copy file into buffer\n");
						exit(1);
					}
					memcpy(&response[extra_bytes], (char*)&Full_packets, sizeof(Full_packets)); //append the last packet number
					encrypt(response,extra_bytes);    //encrypt the last packet
					/***waiting for last ACK***/
					while(1)
					{
						send_file_to_client(response, extra_bytes+4);
						bzero(buffer,sizeof(buffer));
						recvfrom(sock, (char*)(&ack_no), sizeof(ack_no), 0, (struct sockaddr *)&remote, &size);
						if(ack_no==Full_packets)
							break;
					}
					printf("File sent sucessfully\n");
					sleep(2);
				}
				else
				{
					printf("File doesnot exist\n");   
					send_msg_to_client("File_nf");	//sending message to client if file doesnot exist
				}
			}
			else
			{
				printf("Invalid command\n");
				send_msg_to_client("Invalid");
			}			
		}

		/****Handling message to receive file from the client************/
		else if(strcmp(command,"put")== 0)
		{
			
			command = strtok(NULL, "()");  //getting the file name
			if(command!=NULL)
			{
				char file_name[100];
				send_msg_to_client("putting_file");	//alert to client that is needs to send a file	
				send_msg_to_client(command);		//sending the file name
				strcpy(file_name,command);
				recvfrom(sock,buffer,sizeof(buffer), 0, (struct sockaddr *)&remote, &size);	//checking if file is present
				/***Only execute if file is present***/				
				if(strcmp(buffer,"file_der")==0)
				{
					int rec_no_bytes,number_of_packets;
					uint32_t seq_no,j;
					recvfrom(sock,(char*)(&number_of_packets),sizeof(number_of_packets), 0, (struct sockaddr *)&remote, &size); //receive no. of packets
					FILE *fp1;
					fp1=fopen(file_name,"w+");    //open file with the same name
					for(int i=0;i<number_of_packets;)
					{
						bzero(buffer,sizeof(buffer));
						rec_no_bytes = recvfrom(sock,buffer,sizeof(buffer), 0, (struct sockaddr *)&remote, &size);	//receive packets
						memcpy((char*)&seq_no, &buffer[rec_no_bytes-4], sizeof(seq_no));  //take out the seq no
						/****if seq no is what was expected then write and send back ACK*****/
						if(seq_no==i)
						{
							sendto(sock,(char*)(&i),sizeof(i),0,(struct sockaddr *)&remote,sizeof(remote));		//send ACK with packet no
							decrypt(buffer,rec_no_bytes-4);			//decrypting packets
							if(fwrite(buffer,1,rec_no_bytes-4,fp1)<0)	//Write the file with the packet
								{
									printf("error writting file\n");
									exit(1);
								}
							else
								printf("Packet written sucessfully:%d\n",seq_no);
							i++;
						}
						else	//if ACK is not as expected then send the old ACK
						{
							j=i-1;
							sendto(sock,(char*)(&j),sizeof(j),0,(struct sockaddr*)&remote,sizeof(remote));

						}

					}
					fclose(fp1);	//close the file
					printf("File written sucessfully to server!!\n");
				}
				else
					printf("File not found in client\n");
				
			}
			else
			{
				printf("Invalid command\n");
				send_msg_to_client("Invalid");
			}
		}

		/****Handling message to delete a file************/
		else if(strcmp(command,"delete")== 0)
		{
			send_msg_to_client("delete_file");	//sending alert to client that a file is going to be deleted
			command = strtok(NULL, "()");		//getting the file name
			if(command!=NULL)
			{
				char current_dir[200];
				getcwd(current_dir, 200);	//get path to current direct directory
				strcat(current_dir, "/");
				strcat(current_dir, command);
				/**Delete the file if exixsting****/
				if(remove(current_dir)==0){
					printf("File deleted\n");			
					send_msg_to_client("delete_sucess");
				}
				else{
					printf("File not found or delete error\n");	//sending message to client if file doesnot exist
					send_msg_to_client("delete_fail");
				}
			}
			else
			{
				printf("File not found or delete error\n");	//sending message to client if file doesnot exist
				send_msg_to_client("delete_fail");
			}	
		}

		/***Handing any invalid command******/
		else
		{
			printf("Invalid Command\n");
			send_msg_to_client("Invalid");		//sending message to client that a wrong command was given 

		}
		if(check==1)	//used to break loop and close server, linked to exit command
		break;
	}
	close(sock);	//close server
	printf("server closed sucessfully\n");
}

/***Function to send file packet to client*******/
void send_file_to_client(char* data, size_t length)
{
	if (sendto(sock, data , length, 0, (struct sockaddr *)&remote, sizeof(remote)) < 0)
	printf("error sending data to client\n");
}

/****Function to send string to client******/
void send_msg_to_client(char* data)
{
	if (sendto(sock, data , strlen(data), 0, (struct sockaddr *)&remote, sizeof(remote)) < 0)
	printf("error sending packet to client\n");	

}

/*****Function to encrypt the packet******/
void encrypt(char* data,int length)
{
	char key='c';
	for(int i=0;i<length;i++)	//doing XOR of all bytes
	{
		*data=*data ^key;
		data++;
	}

}

/****Function to decrypt the packet*****/
void decrypt(char* data,int length)
{
	char key='c';
	for(int i=0;i<length;i++)	//doing XOR of all bytes to decrypt 
	{
		*data=*data ^key;
		data++;
	}

}
