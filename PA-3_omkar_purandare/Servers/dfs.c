/****************************************************************************************************************
Program: DISTRIBUTED FILE SYSTEM(SERVER)
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
#include <pthread.h>
#include <sys/stat.h>

int sock,user_count=0;                      // socket
struct sockaddr_in remote;     //"Internet socket address structure"
char port[10];
void parse_conf();
char user_passwords[10][100];
void *connection_handler(void *new_socket);

int main (int argc, char * argv[] )
{
	int socket_desc , client_sock , c , *new_sock;
	struct sockaddr_in server , client;
	pthread_t sniffer_thread[100];
	int thread_no=0;
	parse_conf();
	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1)
	{
			printf("Could not create socket");
	}
	puts("Socket created....");

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(atoi(argv[1]));
	strcpy(port,argv[1]);
	printf("%c\n",argv[1][strlen(argv[1])-1]);
	//Bind
	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
	{
			//print the error message
			perror("bind failed. Error");
			return 1;
	}
	puts("Bind done...");

	//Listen
	listen(socket_desc , 100);

	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);
	while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
	{
			puts("Connection accepted...");
			new_sock = malloc(1);
			*new_sock = client_sock;
			//creating new threads for a new connection request from client
			if( pthread_create( &sniffer_thread[thread_no] , NULL ,  connection_handler , (void*) new_sock) < 0)
			{
					perror("could not create thread");
					return 1;
			}
			thread_no++;
			puts("Handler assigned...\n");
	}

	if (client_sock < 0)
	{
			perror("accept failed");
			return 1;
	}
	close(socket_desc);
	return 0;
}


void *connection_handler(void *new_socket)
{
	char information[100],subfolder[100], *user,*file_buf,file_path[100] = "./",file_path1[100],file_path2[100];
	int valid_user=0;           //a buffer to store our received message
	int sock = *(int*)new_socket;
	//Checking for username and password
	if(port[strlen(port)-1]=='1')
		strcat(file_path,"DFS1/");
	else if(port[strlen(port)-1]=='2')
		strcat(file_path,"DFS2/");
	else if(port[strlen(port)-1]=='3')
		strcat(file_path,"DFS3/");
	else if(port[strlen(port)-1]=='4')
		strcat(file_path,"DFS4/");
	recv(sock , information , 100 , 0);
	printf("%s\n",information);
	for(int i=0; i<user_count;i++)
			{
				if(strcmp(information,user_passwords[i]) == 0)
					valid_user=1;
			}
			if(valid_user==1)
			{

				write(sock,"valid_user",10);
				user= strtok(information," ");
				strcat(file_path,user);
				struct stat wa = {0};
				if (stat(file_path, &wa) == -1) {
				    mkdir(file_path, 0700);
				}

			}
			else
			{
				write(sock,"invalid_user",12);
				return 0;
			}
			bzero(information, sizeof(information));
			recv(sock , information , 100 , 0);
			//Handling put request
			if(strcmp(information,"put") == 0)
			{
				uint32_t rec_no_bytes,total_bytes;
				uint32_t file_chunk1,file_chunk2;
				FILE *fp1,*fp2;
				//Building the path to the file
				strcpy(file_path1,file_path);
				strcpy(file_path2,file_path);
				recv(sock,subfolder,100,0);
				write(sock,"OK",3);
				strcat(file_path1,subfolder);
				printf("file1 path: %s\n",file_path1);
				recv(sock , (char*)(&file_chunk1) , 4 , 0);
				write(sock,"OK",3);
				printf("File1 chunk:%d\n",file_chunk1);
				file_buf=(char*)malloc(file_chunk1);
				fp1=fopen(file_path1,"w+");
				//Getting the entire data
				if(fp1!=NULL)
				{
					while(total_bytes!=file_chunk1)
					{
						rec_no_bytes= recv(sock , file_buf , file_chunk1 , 0);
						fwrite(file_buf,1,rec_no_bytes,fp1);			//writing packets to file
						total_bytes= total_bytes+rec_no_bytes;
					}
					write(sock,"sucess_write",12);
					fclose(fp1);
					total_bytes=0;
				}
				else
				{
					write(sock,"failure_write",20);
				}
				free(file_buf);
				bzero(subfolder,sizeof(subfolder));
				recv(sock,subfolder,100,0);
				write(sock,"OK",3);
				strcat(file_path2,subfolder);
				printf("file2 path: %s\n",file_path2);
				recv(sock , (char*)(&file_chunk2) , 4 , 0);
				write(sock,"OK",3);
				printf("file2 chunk:%d\n",file_chunk2);
				file_buf=(char*)malloc(file_chunk2);
				fp2=fopen(file_path2,"w+");
				//Getting second file
				if(fp2!=NULL)
				{
					while(total_bytes!=file_chunk2)
					{
						rec_no_bytes=recv(sock , file_buf , file_chunk2 , 0);
						fwrite(file_buf,1,rec_no_bytes,fp2);
						total_bytes= total_bytes+rec_no_bytes;
					}
												//writing packets to file
					write(sock,"sucess_write",12);
					fclose(fp2);
					total_bytes=0;
				}
				else
				{
					write(sock,"failure_write",20);
				}
			}

			//for handling the list command
			else if(strcmp(information,"list") == 0)
			{
				char response[1000];
				recv(sock,subfolder,100,0);
				strcat(file_path,subfolder);
				DIR *d;
				struct dirent *dir;
				printf("%s\n",subfolder);
				d = opendir(file_path);
				/**Listing the files in the folder***/
				if (d)
				{
					while ((dir = readdir(d)) != NULL)
					{
						if(*dir->d_name == ' ')
						{
							strcat(response,dir->d_name);
							strcat(response,"\n");
						}
					}
					closedir(d);
				}
				write(sock,response,strlen(response));
			}

			//serving mkdir
			else if(strcmp(information,"mkdir") == 0)
			{
				recv(sock,subfolder,100,0);
				strcat(file_path,"/");
				strcat(file_path,subfolder);
				printf("%s\n",file_path);
				struct stat st = {0};

				if (stat(file_path, &st) == -1) {
				    mkdir(file_path, 0700);
				}
			}

			//Serving the get file
			else if(strcmp(information,"get") == 0)
			{
				char check[100], file_path_temp[100],*file_buf, tp[100];
				FILE *fp1;
				while(1)
				{
					bzero(check,sizeof(check));
					recv(sock,check,100,0);
					printf("%s\n",check);
					if(strcmp(check,"stop")==0)
						break;
					write(sock,"OK",3);
					bzero(file_path_temp,sizeof(file_path_temp));
					bzero(subfolder,sizeof(subfolder));
					strcpy(file_path_temp,file_path);
					recv(sock,subfolder,100,0);
					strcat(file_path_temp,subfolder);
					printf("%s\n",file_path_temp);
					fp1=fopen(file_path_temp,"rb");
					if(fp1!=NULL)
					{
						write(sock,"file_found",50);
						fseek(fp1,0,SEEK_END);
						size_t file_size=ftell(fp1);		//Getting the file size
						fseek(fp1,0,SEEK_SET);
						file_buf= (char*)(malloc(file_size));
						fread(file_buf,1,file_size,fp1);			//writing packets to file
						recv(sock,tp,100,0);
						printf("File size sent:%lu\n",file_size);
						write(sock,(char*)&file_size,4);
						recv(sock,tp,100,0);
						write(sock,file_buf,file_size);
						free(file_buf);
						fclose(fp1);
					}
					else
					{
						write(sock,"file_not_found",50);
					}
				}
			}
		close(sock);
		return 0;
}

//Parsing the conf file
void parse_conf()
{
  FILE *conf;
	conf=fopen("dfs.conf","rb");//open file
  char line[256],line_orignal[256];
  char *user;
      while (fgets(line, sizeof(line), conf))
      {
          if(*line!='#')//ignore comment lines
          {
            strcpy(line_orignal,line);
            user = strtok(line, "\n");
            strcpy(user_passwords[user_count],user);
            user_count++;
          }
      }
  return;
}
