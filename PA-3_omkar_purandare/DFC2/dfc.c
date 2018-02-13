/****************************************************************************************************************
Program: DISTRIBUTED FILE SYSTEM(CLIENT)
Author: Omkar Purandare
Subject: Network Systems
*****************************************************************************************************************/
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>
#include<pthread.h> //for threading , link with lpthread
#include <openssl/md5.h>

#define MAXBUFSIZE 1024
void parse_conf(char conf_file_name[20]);
void encrypt(char* data,int length);
void decrypt(char* data,int length);
void print_list();
uint32_t file_chunk, last_chunk;
char ip_addrs[10][100],ports[10][100], Username[30], Password[30], user_pass[100],files_got[1000];
char command[MAXBUFSIZE],buffer[MAXBUFSIZE],incoming_message[MAXBUFSIZE];
char* get_files(int sock, int file_no, char* file_name, char* subfolder);
int server_count=0;
int sock1,sock2,sock3,sock4;                           //This will be our socket
struct sockaddr_in dfs1,dfs2,dfs3,dfs4;     //"Internet socket address structure"
void put_files(int sock, char* file1, char* file2, int file1_no, int file2_no, char* file_name, char* subfolder);

int main(int argc , char *argv[])
{
  if (argc < 2)
  {
    printf("USAGE:  <CONFIG_FILE>\n");
    exit(1);
  }

  parse_conf(argv[1]);
  strcpy(user_pass,Username);
  strcat(user_pass," ");
  strcat(user_pass,Password);
  //alloting addresses and ports for servers
  dfs1.sin_family = AF_INET;                 //address family
	dfs1.sin_port = htons(atoi(ports[0]));      //sets port to network byte order
	dfs1.sin_addr.s_addr = inet_addr(ip_addrs[0]); //sets remote IP address

  dfs2.sin_family = AF_INET;                 //address family
	dfs2.sin_port = htons(atoi(ports[1]));      //sets port to network byte order
	dfs2.sin_addr.s_addr = inet_addr(ip_addrs[1]); //sets remote IP address

  dfs3.sin_family = AF_INET;                 //address family
	dfs3.sin_port = htons(atoi(ports[2]));      //sets port to network byte order
	dfs3.sin_addr.s_addr = inet_addr(ip_addrs[2]); //sets remote IP address

  dfs4.sin_family = AF_INET;                 //address family
	dfs4.sin_port = htons(atoi(ports[3]));      //sets port to network byte order
	dfs4.sin_addr.s_addr = inet_addr(ip_addrs[3]); //sets remote IP address



  while(1)
	{
    //creating socket
    if ((sock1 = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  	{
  		printf("unable to create socket");
  	}
    if ((sock2 = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  	{
  		printf("unable to create socket");
  	}
    if ((sock3 = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  	{
  		printf("unable to create socket");
  	}
    if ((sock4 = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  	{
  		printf("unable to create socket");
  	}
    bzero(command,sizeof(command));
		printf("\n");
		printf("Enter any of the command:\n");
		printf("1) To download any file from server : get(file_name)/subfolder \n");
		printf("2) To send any file to the server : put(file_name)/subfolder \n");
		printf("3) To make a directory in the server : mkdir(subfolder)\n");
		printf("4) To list all files the server : list(/subfolder)\n");
		printf("/*****************************************************************/\n");
		printf("\n");
		printf("Enter the command you want to send to the server:\t");		//Messages for user
		scanf("%s",command);
    char *instruction,*file_name, *subfolder,*subfolder_ls, *subfolder_mkdir;
    instruction = strtok(command,"()");
    //put command
    if(strcmp(instruction,"put") == 0)
    {
      file_name= strtok(NULL,"()");
      subfolder= strtok(NULL,"()");
      if(file_name!=NULL)//checking for correct command
			{
        FILE *fp;
        int j;
  			fp=fopen(file_name,"rb");		//opening the file
  			bzero(buffer,sizeof(buffer));
  			if(fp!=NULL)
  			{
          /***only execute this if the file exists*****/
          MD5_CTX c;
          char buf[512],*f1,*f2,*f3,*f4;
          ssize_t bytes;
          //calculating MD5SUM
          unsigned char out[MD5_DIGEST_LENGTH];
          MD5_Init(&c);
          do
          {
                  bytes=fread(buf,1,512,fp);
                  MD5_Update(&c, buf, bytes);
          }
          while(bytes>0);
          MD5_Final(out, &c);
          int distribution = out[0]%4;
          //dividing files into buffers based on MD5SUM
          fseek(fp,0,SEEK_END);
  				size_t file_size=ftell(fp);		//Getting the file size
  				fseek(fp,0,SEEK_SET);
          file_chunk= file_size/4;
          last_chunk= file_size- (file_chunk*3);
          f1= (char*)(malloc(file_chunk));
          f2= (char*)(malloc(file_chunk));
          f3= (char*)(malloc(file_chunk));
          f4= (char*)(malloc(last_chunk));
          //dividing and encrypting
          fread(f1,file_chunk,1,fp);
          encrypt(f1,file_chunk);
          fread(f2,file_chunk,1,fp);
          encrypt(f2,file_chunk);
          fread(f3,file_chunk,1,fp);
          encrypt(f3,file_chunk);
          fread(f4,last_chunk,1,fp);
          encrypt(f4,last_chunk);
          // For MOD = 0
          if(distribution==0)
          {
            //connecting to first server
            for(j=0;j<4;j++)
            {   //Trying to connect for 1 sec
                if (connect(sock1 , (struct sockaddr *)&dfs1 , sizeof(dfs1)) < 0)
                {
                    perror("connect failed to DFS1");
                    usleep(250000);
                }
                else
                {
                  //checking for correct username and password
                  send(sock1 , user_pass , strlen(user_pass) , 0);
                  usleep(1);
                  bzero(incoming_message,sizeof(incoming_message));
                  recv(sock1 , incoming_message , sizeof(incoming_message) , 0);
                  if(strcmp(incoming_message,"valid_user") == 0)
                  {
                    //Calling function for sending the files
                    put_files(sock1, f1, f2, 1, 2,file_name,subfolder);
                  }
                  else
                  {
                    printf("Invalid user or password\n");
                  }
                  break;
                }
            }
            close(sock1);

            //Connecting to 2nd server
            for(j=0;j<4;j++)
            {
                if (connect(sock2 , (struct sockaddr *)&dfs2 , sizeof(dfs2)) < 0)
                {
                    perror("connect failed to DFS2");
                    usleep(250000);
                }
                else
                {
                  send(sock2 , user_pass , strlen(user_pass) , 0);
                  usleep(1);
                  bzero(incoming_message,sizeof(incoming_message));
                  recv(sock2 , incoming_message , sizeof(incoming_message) , 0);
                  if(strcmp(incoming_message,"valid_user") == 0)
                  {
                    put_files(sock2, f2, f3, 2, 3,file_name,subfolder);
                  }
                  else
                  {
                    printf("Invalid user or password\n");
                  }
                  break;
                }
            }
            close(sock2);

            //Connecting to 3rd server
            for(j=0;j<4;j++)
            {
                if (connect(sock3 , (struct sockaddr *)&dfs3 , sizeof(dfs3)) < 0)
                {
                    perror("connect failed to DFS3");
                    usleep(250000);
                }
                else
                {
                  send(sock3 , user_pass , strlen(user_pass) , 0);
                  usleep(1);
                  bzero(incoming_message,sizeof(incoming_message));
                  recv(sock3 , incoming_message , sizeof(incoming_message) , 0);
                  if(strcmp(incoming_message,"valid_user") == 0)
                  {
                    put_files(sock3, f3, f4, 3, 4,file_name,subfolder);
                  }
                  else
                  {
                    printf("Invalid user or password\n");
                  }
                  break;
                }
            }
            close(sock3);

            //Connecting to 4th server
            for(j=0;j<4;j++)
            {
                if (connect(sock4 , (struct sockaddr *)&dfs4 , sizeof(dfs4)) < 0)
                {
                    perror("connect failed to DFS3");
                    usleep(250000);
                }
                else
                {
                  send(sock4 , user_pass , strlen(user_pass) , 0);
                  usleep(1);
                  bzero(incoming_message,sizeof(incoming_message));
                  recv(sock4 , incoming_message , sizeof(incoming_message) , 0);
                  if(strcmp(incoming_message,"valid_user") == 0)
                  {
                    put_files(sock4, f4, f1, 4, 1,file_name,subfolder);
                  }
                  else
                  {
                    printf("Invalid user or password\n");
                  }
                  break;
                }
            }
            close(sock4);
        }
          //For MOD = 1
          else if(distribution==1)
          {

              for(j=0;j<4;j++)
              {
                  if (connect(sock1 , (struct sockaddr *)&dfs1 , sizeof(dfs1)) < 0)
                  {
                      perror("connect failed to DFS1");
                      usleep(250000);
                  }
                  else
                  {
                    send(sock1 , user_pass , strlen(user_pass) , 0);
                    usleep(1);
                    bzero(incoming_message,sizeof(incoming_message));
                    recv(sock1 , incoming_message , sizeof(incoming_message) , 0);
                    if(strcmp(incoming_message,"valid_user") == 0)
                    {
                      put_files(sock1, f4, f1, 4, 1,file_name,subfolder);
                    }
                    else
                    {
                      printf("Invalid user or password\n");
                    }
                    break;
                  }
              }
              close(sock1);

              for(j=0;j<4;j++)
              {
                  if (connect(sock2 , (struct sockaddr *)&dfs2 , sizeof(dfs2)) < 0)
                  {
                      perror("connect failed to DFS2");
                      usleep(250000);
                  }
                  else
                  {
                    send(sock2 , user_pass , strlen(user_pass) , 0);
                    usleep(1);
                    bzero(incoming_message,sizeof(incoming_message));
                    recv(sock2 , incoming_message , sizeof(incoming_message) , 0);
                    if(strcmp(incoming_message,"valid_user") == 0)
                    {
                      put_files(sock2, f1, f2, 1, 2,file_name,subfolder);
                    }
                    else
                    {
                      printf("Invalid user or password\n");
                    }
                    break;
                  }
              }
              close(sock2);

              for(j=0;j<4;j++)
              {
                  if (connect(sock3 , (struct sockaddr *)&dfs3 , sizeof(dfs3)) < 0)
                  {
                      perror("connect failed to DFS3");
                      usleep(250000);
                  }
                  else
                  {
                    send(sock3 , user_pass , strlen(user_pass) , 0);
                    usleep(1);
                    bzero(incoming_message,sizeof(incoming_message));
                    recv(sock3 , incoming_message , sizeof(incoming_message) , 0);
                    if(strcmp(incoming_message,"valid_user") == 0)
                    {
                      put_files(sock3, f2, f3, 2, 3,file_name,subfolder);
                    }
                    else
                    {
                      printf("Invalid user or password\n");
                    }
                    break;
                  }
              }
              close(sock3);

              for(j=0;j<4;j++)
              {
                  if (connect(sock4 , (struct sockaddr *)&dfs4 , sizeof(dfs4)) < 0)
                  {
                      perror("connect failed to DFS3");
                      usleep(250000);
                  }
                  else
                  {
                    send(sock4 , user_pass , strlen(user_pass) , 0);
                    usleep(1);
                    bzero(incoming_message,sizeof(incoming_message));
                    recv(sock4 , incoming_message , sizeof(incoming_message) , 0);
                    if(strcmp(incoming_message,"valid_user") == 0)
                    {
                      put_files(sock4, f3, f4, 3, 4,file_name,subfolder);
                    }
                    else
                    {
                      printf("Invalid user or password\n");
                    }
                    break;
                  }
              }
              close(sock4);
          }

          //For MOD=2
          else if(distribution==2)
          {

            for(j=0;j<4;j++)
            {
                if (connect(sock1 , (struct sockaddr *)&dfs1 , sizeof(dfs1)) < 0)
                {
                    perror("connect failed to DFS1");
                    usleep(250000);
                }
                else
                {
                  send(sock1 , user_pass , strlen(user_pass) , 0);
                  usleep(1);
                  bzero(incoming_message,sizeof(incoming_message));
                  recv(sock1 , incoming_message , sizeof(incoming_message) , 0);
                  if(strcmp(incoming_message,"valid_user") == 0)
                  {
                    put_files(sock1, f3, f4, 3, 4,file_name,subfolder);
                  }
                  else
                  {
                    printf("Invalid user or password\n");
                  }
                  break;
                }
            }
            close(sock1);

            for(j=0;j<4;j++)
            {
                if (connect(sock2 , (struct sockaddr *)&dfs2 , sizeof(dfs2)) < 0)
                {
                    perror("connect failed to DFS2");
                    usleep(250000);
                }
                else
                {
                  send(sock2 , user_pass , strlen(user_pass) , 0);
                  usleep(1);
                  bzero(incoming_message,sizeof(incoming_message));
                  recv(sock2 , incoming_message , sizeof(incoming_message) , 0);
                  if(strcmp(incoming_message,"valid_user") == 0)
                  {
                    put_files(sock2, f4, f1, 4, 1,file_name,subfolder);
                  }
                  else
                  {
                    printf("Invalid user or password\n");
                  }
                  break;
                }
            }
            close(sock2);

            for(j=0;j<4;j++)
            {
                if (connect(sock3 , (struct sockaddr *)&dfs3 , sizeof(dfs3)) < 0)
                {
                    perror("connect failed to DFS3");
                    usleep(250000);
                }
                else
                {
                  send(sock3 , user_pass , strlen(user_pass) , 0);
                  usleep(1);
                  bzero(incoming_message,sizeof(incoming_message));
                  recv(sock3 , incoming_message , sizeof(incoming_message) , 0);
                  if(strcmp(incoming_message,"valid_user") == 0)
                  {
                    put_files(sock3, f1, f2, 1, 2,file_name,subfolder);
                  }
                  else
                  {
                    printf("Invalid user or password\n");
                  }
                  break;
                }
            }
            close(sock3);

            for(j=0;j<4;j++)
            {
                if (connect(sock4 , (struct sockaddr *)&dfs4 , sizeof(dfs4)) < 0)
                {
                    perror("connect failed to DFS3");
                    usleep(250000);
                }
                else
                {
                  send(sock4 , user_pass , strlen(user_pass) , 0);
                  usleep(1);
                  bzero(incoming_message,sizeof(incoming_message));
                  recv(sock4 , incoming_message , sizeof(incoming_message) , 0);
                  if(strcmp(incoming_message,"valid_user") == 0)
                  {
                    put_files(sock4, f2, f3, 2, 3,file_name,subfolder);
                  }
                  else
                  {
                    printf("Invalid user or password\n");
                  }
                  break;
                }
            }
            close(sock4);
          }

          //For MOD = 3
          else
          {

              for(j=0;j<4;j++)
              {
                  if (connect(sock1 , (struct sockaddr *)&dfs1 , sizeof(dfs1)) < 0)
                  {
                      perror("connect failed to DFS1");
                      usleep(250000);
                  }
                  else
                  {
                    send(sock1 , user_pass , strlen(user_pass) , 0);
                    usleep(1);
                    bzero(incoming_message,sizeof(incoming_message));
                    recv(sock1 , incoming_message , sizeof(incoming_message) , 0);
                    if(strcmp(incoming_message,"valid_user") == 0)
                    {
                      put_files(sock1, f2, f3, 2, 3,file_name,subfolder);
                    }
                    else
                    {
                      printf("Invalid user or password\n");
                    }
                    break;
                  }
              }
              close(sock1);

              for(j=0;j<4;j++)
              {
                  if (connect(sock2 , (struct sockaddr *)&dfs2 , sizeof(dfs2)) < 0)
                  {
                      perror("connect failed to DFS2");
                      usleep(250000);
                  }
                  else
                  {
                    send(sock2 , user_pass , strlen(user_pass) , 0);
                    usleep(1);
                    bzero(incoming_message,sizeof(incoming_message));
                    recv(sock2 , incoming_message , sizeof(incoming_message) , 0);
                    if(strcmp(incoming_message,"valid_user") == 0)
                    {
                      put_files(sock2, f3, f4, 3, 4,file_name,subfolder);
                    }
                    else
                    {
                      printf("Invalid user or password\n");
                    }
                    break;
                  }
              }
              close(sock2);

              for(j=0;j<4;j++)
              {
                  if (connect(sock3 , (struct sockaddr *)&dfs3 , sizeof(dfs3)) < 0)
                  {
                      perror("connect failed to DFS3");
                      usleep(250000);
                  }
                  else
                  {
                    send(sock3 , user_pass , strlen(user_pass) , 0);
                    usleep(1);
                    bzero(incoming_message,sizeof(incoming_message));
                    recv(sock3 , incoming_message , sizeof(incoming_message) , 0);
                    if(strcmp(incoming_message,"valid_user") == 0)
                    {
                      put_files(sock3, f4, f1, 4, 1,file_name,subfolder);
                    }
                    else
                    {
                      printf("Invalid user or password\n");
                    }
                    break;
                  }
              }
              close(sock3);

              for(j=0;j<4;j++)
              {
                  if (connect(sock4 , (struct sockaddr *)&dfs4 , sizeof(dfs4)) < 0)
                  {
                      perror("connect failed to DFS3");
                      usleep(250000);
                  }
                  else
                  {
                    send(sock4 , user_pass , strlen(user_pass) , 0);
                    usleep(1);
                    bzero(incoming_message,sizeof(incoming_message));
                    recv(sock4 , incoming_message , sizeof(incoming_message) , 0);
                    if(strcmp(incoming_message,"valid_user") == 0)
                    {
                      put_files(sock4, f1, f2, 1, 2,file_name,subfolder);
                    }
                    else
                    {
                      printf("Invalid user or password\n");
                    }
                    break;
                  }
              }
              close(sock4);
          }
        }
        else
          printf("File doesnot exist on client\n");
      }
      else
      {
        //for wrong command
        printf("Wrong Command\n");
      }
    }

    //For get command
    else if(strcmp(instruction,"get") == 0)
    {
      bzero(files_got, sizeof(files_got));
      int j;
      char *p1, *p2, *p3, *p4, file_1[100], file_2[100], file_3[100], file_4[100];
      file_name= strtok(NULL,"()");
      subfolder= strtok(NULL,"()");
      if(file_name!=NULL)
			{
        //Making the accurate file name
        strcpy(file_1,file_name);
        strcat(file_1,".1");
        strcpy(file_2,file_name);
        strcat(file_2,".2");
        strcpy(file_3,file_name);
        strcat(file_3,".3");
        strcpy(file_4,file_name);
        strcat(file_4,".4");
        for(j=0;j<4;j++)
        {
            //Connecting to server 1 for 1 sec
            if (connect(sock1 , (struct sockaddr *)&dfs1 , sizeof(dfs1)) < 0)
            {
                perror("connect failed to DFS1");
                usleep(250000);
            }
            else
            {
              //Checking username and password
              send(sock1 , user_pass , strlen(user_pass) , 0);
              usleep(1);
              bzero(incoming_message,sizeof(incoming_message));
              recv(sock1 , incoming_message , sizeof(incoming_message) , 0);
              if(strcmp(incoming_message,"valid_user") == 0)
              {
                //Trying to get subparts from all servers seqeuncially
                write(sock1 , command , strlen(command));
                //Checking if I aldready have the file
                if(strstr(files_got,file_1)==NULL)
                    {
                      write(sock1,"goon", 10);
                      p1=  get_files(sock1, 1, file_1,subfolder);//Function to get the sub-part
                    }
                if(strstr(files_got,file_2)==NULL)
                    {
                      write(sock1,"goon", 10);
                      p2 = get_files(sock1, 2, file_2,subfolder);

                    }
                if(strstr(files_got,file_3)==NULL)
                  {
                    write(sock1,"goon", 10);
                    p3 = get_files(sock1, 3, file_3,subfolder);

                  }
                if(strstr(files_got,file_4)==NULL)
                  {
                    write(sock1,"goon", 10);
                    p4 = get_files(sock1, 4, file_4,subfolder);

                  }
                  write(sock1,"stop", 10);
              }
              else
              {
                printf("Invalid user or password\n");
              }
              break;
            }
        }
        close(sock1);

        //Connecting to second server
        for(j=0;j<4;j++)
        {
            if (connect(sock2 , (struct sockaddr *)&dfs2 , sizeof(dfs2)) < 0)
            {
                perror("connect failed to DFS2");
                usleep(250000);
            }
            else
            {
              send(sock2 , user_pass , strlen(user_pass) , 0);
              usleep(1);
              bzero(incoming_message,sizeof(incoming_message));
              recv(sock2 , incoming_message , sizeof(incoming_message) , 0);
              if(strcmp(incoming_message,"valid_user") == 0)
              {
                write(sock2 , command , strlen(command));
                if(strstr(files_got,file_1)==NULL)
                    {
                      write(sock2,"goon", 10);
                      p1=  get_files(sock2, 1, file_1,subfolder);

                    }
                if(strstr(files_got,file_2)==NULL)
                    {
                      write(sock2,"goon", 10);
                      p2 = get_files(sock2, 2, file_2,subfolder);

                    }
                if(strstr(files_got,file_3)==NULL)
                  {
                    write(sock2,"goon", 10);
                    p3 = get_files(sock2, 3, file_3,subfolder);
                  }
                if(strstr(files_got,file_4)==NULL)
                  {
                    write(sock2,"goon", 10);
                    p4 = get_files(sock2, 4, file_4,subfolder);

                  }
                  write(sock2,"stop", 10);
              }
              else
              {
                printf("Invalid user or password\n");
              }
              break;
            }
        }
        close(sock2);

        //Connecting to 3rd server
        for(j=0;j<4;j++)
        {
            if (connect(sock3 , (struct sockaddr *)&dfs3 , sizeof(dfs3)) < 0)
            {
                perror("connect failed to DFS3");
                usleep(250000);
            }
            else
            {
              send(sock3 , user_pass , strlen(user_pass) , 0);
              usleep(1);
              bzero(incoming_message,sizeof(incoming_message));
              recv(sock3 , incoming_message , sizeof(incoming_message) , 0);
              if(strcmp(incoming_message,"valid_user") == 0)
              {
                write(sock3 , command , strlen(command));
                if(strstr(files_got,file_1)==NULL)
                    {
                      write(sock3,"goon", 10);
                      p1=  get_files(sock3, 1, file_1,subfolder);

                    }
                if(strstr(files_got,file_2)==NULL)
                    {
                      write(sock3,"goon", 10);
                      p2 = get_files(sock3, 2, file_2,subfolder);

                    }
                if(strstr(files_got,file_3)==NULL)
                  {
                    write(sock3,"goon", 10);
                    p3 = get_files(sock3, 3, file_3,subfolder);

                  }
                if(strstr(files_got,file_4)==NULL)
                  {
                    write(sock3,"goon", 10);
                    p4 = get_files(sock3, 4, file_4,subfolder);

                  }
                write(sock3,"stop", 10);
              }
              else
              {
                printf("Invalid user or password\n");
              }
              break;
            }
        }
        close(sock3);

        //Connecting to 4th server
        for(j=0;j<4;j++)
        {
            if (connect(sock4 , (struct sockaddr *)&dfs4 , sizeof(dfs4)) < 0)
            {
                perror("connect failed to DFS4");
                usleep(250000);
            }
            else
            {
              send(sock4 , user_pass , strlen(user_pass) , 0);
              usleep(1);
              bzero(incoming_message,sizeof(incoming_message));
              recv(sock4 , incoming_message , sizeof(incoming_message) , 0);
              if(strcmp(incoming_message,"valid_user") == 0)
              {
                write(sock4 , command , strlen(command));
                if(strstr(files_got,file_1)==NULL)
                    {
                      write(sock4,"goon", 10);
                      p1=  get_files(sock4, 1, file_1,subfolder);

                    }
                if(strstr(files_got,file_2)==NULL)
                    {
                      write(sock4,"goon", 10);
                      p2 = get_files(sock4, 2, file_2,subfolder);

                    }
                if(strstr(files_got,file_3)==NULL)
                  {
                    write(sock4,"goon", 10);
                    p3 = get_files(sock4, 3, file_3,subfolder);

                  }
                if(strstr(files_got,file_4)==NULL)
                  {
                    write(sock4,"goon", 10);
                    p4 = get_files(sock4, 4, file_4,subfolder);

                  }
                write(sock4,"stop", 10);
              }
              else
              {
                printf("Invalid user or password\n");
              }
              break;
            }
        }
        close(sock4);

        //Check if all parts are available
        if(p1!=NULL && p2!=NULL && p3!=NULL && p4!=NULL)
        {
          //Decrypt all parts
          decrypt(p1,file_chunk);
          decrypt(p2,file_chunk);
          decrypt(p3,file_chunk);
          decrypt(p4,last_chunk);
          FILE *fp;
          fp=fopen(file_name,"w+");
          if(fp!=NULL)
          {
            //Write all the parts into the file
            fwrite(p1,1,file_chunk,fp);
            fwrite(p2,1,file_chunk,fp);
            fwrite(p3,1,file_chunk,fp);
            fwrite(p4,1,last_chunk,fp);
            fclose(fp);
            printf("File written sucess\n");
          }
          else
            printf("File write not sucessful\n");
        }
        else
          printf("File Incomplete\n");
      }
    }

    //Make directory command
    else if(strcmp(instruction,"mkdir") == 0)
    {
      int j;
      subfolder_mkdir=strtok(NULL,"()");
      if(subfolder_mkdir!=NULL)
      {
        for(j=0;j<4;j++)
        {
          //Try to connect to server 1
          if (connect(sock1 , (struct sockaddr *)&dfs1 , sizeof(dfs1)) < 0)
          {
              perror("connect failed to DFS1");
              usleep(250000);
          }
          else
          {
            //Checking for valid user and password
            send(sock1 , user_pass , strlen(user_pass) , 0);
            usleep(1);
            bzero(incoming_message,sizeof(incoming_message));
            recv(sock1 , incoming_message , sizeof(incoming_message) , 0);
            if(strcmp(incoming_message,"valid_user") == 0)
            {
              send(sock1 , command , strlen(command) , 0);
              usleep(1);
              //Passing path for the mkdir
              send(sock1,subfolder_mkdir,strlen(subfolder_mkdir),0);
            }
            else
            {
              printf("Invalid user or password SERVER1\n");
            }
          break;
        }
        }
        close(sock1);

        //Connecting to server 2
        for(j=0;j<4;j++)
        {
          if (connect(sock2 , (struct sockaddr *)&dfs2 , sizeof(dfs2)) < 0)
          {
              perror("connect failed to DFS2");
              usleep(250000);
          }
          else
          {
            send(sock2 , user_pass , strlen(user_pass) , 0);
            usleep(1);
            bzero(incoming_message,sizeof(incoming_message));
            recv(sock2 , incoming_message , sizeof(incoming_message) , 0);
            if(strcmp(incoming_message,"valid_user") == 0)
            {
              send(sock2 , command , strlen(command) , 0);
              usleep(1);
              send(sock2,subfolder_mkdir,strlen(subfolder_mkdir),0);
            }
            else
            {
              printf("Invalid user or password SERVER1\n");
            }
          break;
        }
        }
        close(sock2);

        //Connecting to 3rd server
        for(j=0;j<4;j++)
        {
          if (connect(sock3 , (struct sockaddr *)&dfs3 , sizeof(dfs3)) < 0)
          {
              perror("connect failed to DFS3");
              usleep(250000);
          }
          else
          {
            send(sock3 , user_pass , strlen(user_pass) , 0);
            usleep(1);
            bzero(incoming_message,sizeof(incoming_message));
            recv(sock3 , incoming_message , sizeof(incoming_message) , 0);
            if(strcmp(incoming_message,"valid_user") == 0)
            {
              send(sock3 , command , strlen(command) , 0);
              usleep(1);
              send(sock3,subfolder_mkdir,strlen(subfolder_mkdir),0);
            }
            else
            {
              printf("Invalid user or password SERVER3\n");
            }
          break;
        }
        }
        close(sock3);

        //Connecting to 4th server
        for(j=0;j<4;j++)
        {
          if (connect(sock4 , (struct sockaddr *)&dfs4 , sizeof(dfs4)) < 0)
          {
              perror("connect failed to DFS4");
              usleep(250000);
          }
          else
          {
            send(sock4 , user_pass , strlen(user_pass) , 0);
            usleep(1);
            bzero(incoming_message,sizeof(incoming_message));
            recv(sock4 , incoming_message , sizeof(incoming_message) , 0);
            if(strcmp(incoming_message,"valid_user") == 0)
            {
              send(sock4 , command , strlen(command) , 0);
              usleep(1);
              send(sock4,subfolder_mkdir,strlen(subfolder_mkdir),0);
            }
            else
            {
              printf("Invalid user or password SERVER4\n");
            }
          break;
        }
        }
        close(sock4);
      }
      else
        printf("Invalid mkdir command\n");
    }

    //List command
    else if(strcmp(instruction,"list") == 0)
    {
      FILE *ls;
      int i=0;
      //Opena a file to copy names into a file
      ls=fopen("list_data","wb+");
      char ls_path[100];
      subfolder_ls=strtok(NULL,"()");
      if(subfolder_ls==NULL)
        strcpy(ls_path,"/");
      else
      {
        strcpy(ls_path,subfolder_ls);
        strcat(ls_path,"/");
      }
      //Connecting server 1
      for(i=0;i<4;i++)
      {
        if (connect(sock1 , (struct sockaddr *)&dfs1 , sizeof(dfs1)) < 0)
        {
            perror("connect failed to DFS1");
            usleep(250000);
        }
        else
        {
          send(sock1 , user_pass , strlen(user_pass) , 0);
          usleep(1);
          bzero(incoming_message,sizeof(incoming_message));
          recv(sock1 , incoming_message , sizeof(incoming_message) , 0);
          if(strcmp(incoming_message,"valid_user") == 0)
          {
            send(sock1 , command , strlen(command) , 0);
            usleep(1);
            send(sock1,ls_path,strlen(ls_path),0);
            usleep(1);
            bzero(incoming_message,sizeof(incoming_message));
            recv(sock1 , incoming_message , sizeof(incoming_message) , 0);
            fwrite(incoming_message,1,strlen(incoming_message),ls);
          }
          else
          {
            printf("Invalid user or password SERVER1\n");
          }
        break;
      }
      }
      close(sock1);

      //Connecting server 2
      for(i=0;i<4;i++)
      {
        if (connect(sock2 , (struct sockaddr *)&dfs2 , sizeof(dfs2)) < 0)
        {
            perror("connect failed to DFS2");
            usleep(250000);
        }
        else
        {
          send(sock2 , user_pass , strlen(user_pass) , 0);
          usleep(1);
          bzero(incoming_message,sizeof(incoming_message));
          recv(sock2 , incoming_message , sizeof(incoming_message) , 0);
          if(strcmp(incoming_message,"valid_user") == 0)
          {
            send(sock2 , command , strlen(command) , 0);
            usleep(1);
            send(sock2,ls_path,strlen(ls_path),0);
            usleep(1);
            bzero(incoming_message,sizeof(incoming_message));
            recv(sock2 , incoming_message , sizeof(incoming_message) , 0);
            fwrite(incoming_message,1,strlen(incoming_message),ls);
          }
          else
          {
            printf("Invalid user or password SERVER2\n");
          }
          break;
        }
      }
      close(sock2);

      //Connecting server 3
      for(i=0;i<4;i++)
      {
        if (connect(sock3 , (struct sockaddr *)&dfs3 , sizeof(dfs3)) < 0)
        {
            perror("connect failed to DFS3");
            usleep(250000);
        }
        else
        {
          send(sock3 , user_pass , strlen(user_pass) , 0);
          usleep(1);
          bzero(incoming_message,sizeof(incoming_message));
          recv(sock3 , incoming_message , sizeof(incoming_message) , 0);
          if(strcmp(incoming_message,"valid_user") == 0)
          {
            send(sock3 , command , strlen(command) , 0);
            usleep(1);
            send(sock3,ls_path,strlen(ls_path),0);
            usleep(1);
            bzero(incoming_message,sizeof(incoming_message));
            recv(sock3 , incoming_message , sizeof(incoming_message) , 0);
            fwrite(incoming_message,1,strlen(incoming_message),ls);
          }
          else
          {
            printf("Invalid user or password SERVER3\n");
          }
          break;
        }
      }
      close(sock3);

      //Connecting server 4
      for(i=0;i<4;i++)
      {
        if (connect(sock4 , (struct sockaddr *)&dfs4 , sizeof(dfs4)) < 0)
        {
            perror("connect failed to DFS4");
            usleep(250000);
        }
        else
        {
          send(sock4 , user_pass , strlen(user_pass) , 0);
          usleep(1);
          bzero(incoming_message,sizeof(incoming_message));
          recv(sock4 , incoming_message , sizeof(incoming_message) , 0);
          if(strcmp(incoming_message,"valid_user") == 0)
          {
            send(sock4 , command , strlen(command) , 0);
            usleep(1);
            send(sock4,ls_path,strlen(ls_path),0);
            usleep(1);
            bzero(incoming_message,sizeof(incoming_message));
            recv(sock4 , incoming_message , sizeof(incoming_message) , 0);
            fwrite(incoming_message,1,strlen(incoming_message),ls);
          }
          else
          {
            printf("Invalid user or password SERVER4\n");
          }
          break;
        }
      }
      close(sock4);
      fclose(ls);
      print_list();
    }

    else
    {
      printf("Invalid command\n");
    }
  }
  return 0;
}

//Function to sort the list and print the file present
void print_list()
{
  char line[256], no_duplicate[100][256], files_read[100][256], *file_name, no_duplicate_copy[100];
  FILE *ls;
  int item_filled=0,add_file=1,file_count=0,ignore=0;
  ls=fopen("list_data","rb");
  //reading the files from the file
  while (fgets(line, sizeof(line), ls))
  {
    //Making a list which doesnot have duplicates
    for(int i = 0; i<item_filled; i++)
    {
      if(strcmp(no_duplicate[i],line) == 0)
      {
        add_file=0;
        break;
      }
      else
        add_file=1;
    }
    if(add_file==1)
    {
      strcpy(no_duplicate[item_filled],line);
      item_filled++;
      add_file=1;
    }
  }
  fclose(ls);

  //Logic to compute what are the files present and are they complete
  printf("The files present on server are:\n");
  for(int i=0;i<item_filled;i++)
  {
    ignore=0;
    strcpy(no_duplicate_copy,no_duplicate[i]);
    strtok(no_duplicate_copy,"..");
    file_name = strtok(NULL,"..");
    for(int j = 0; j<file_count; j++)
    {
      if(strcmp(files_read[j],file_name) == 0){
        ignore=1;
        break;
      }
    }
    if(ignore==0)
    {
        int file_part_count=0;
        for(int k=0;k<item_filled;k++)
        {
          if(strstr(no_duplicate[k],file_name)!=NULL)
            file_part_count++;
        }
        if(file_part_count==4)
        {
          printf("%s\n",file_name);
          strcpy(files_read[file_count],file_name);
          file_count++;
        }
        else
        {
          printf("%s (Incomplete)\n",file_name);
          strcpy(files_read[file_count],file_name);
          file_count++;
        }
    }
  }
}


//Function to get the files from the server
char* get_files(int sock, int file_no, char* file_name, char* subfolder)
{
  uint32_t rec_no_bytes,total_bytes;
  char file_name_send[100]=" .",file_path[100], *file_buf;
  bzero(incoming_message,sizeof(incoming_message));
  recv(sock , incoming_message , sizeof(incoming_message) , 0);
  //making the path
  strcat(file_name_send,file_name);
  {
    if(subfolder==NULL)
    {
      strcpy(file_path,"/");
      strcat(file_path,file_name_send);
    }
    else
    {
      strcpy(file_path,subfolder);
      strcat(file_path,"/");
      strcat(file_path,file_name_send);
    }
    write(sock,file_path,strlen(file_path)+1);
    bzero(incoming_message,sizeof(incoming_message));
    recv(sock , incoming_message , sizeof(incoming_message) , 0);
    if(strcmp(incoming_message,"file_not_found")==0)
    {
      return NULL;
    }
    write(sock,"OK",5);
    if(file_no == 4)
    {
      recv(sock,(char*)&last_chunk,4,0);
      file_buf=(char*)malloc(last_chunk);
      write(sock,"OK",5);
      while(total_bytes!=last_chunk)
      {
        rec_no_bytes= recv(sock,file_buf,last_chunk,0);
        total_bytes= total_bytes+rec_no_bytes;
        file_buf= file_buf+rec_no_bytes;
      }
      file_buf= file_buf-total_bytes;
      total_bytes=0;
    }
    else
    {
      recv(sock,(char*)&file_chunk,4,0);
      file_buf=(char*)malloc(file_chunk);
      write(sock,"OK",5);
      while(total_bytes!=file_chunk)
      {
        rec_no_bytes= recv(sock,file_buf,last_chunk,0);
        total_bytes= total_bytes+rec_no_bytes;
        file_buf= file_buf+rec_no_bytes;
      }
      file_buf= file_buf-total_bytes;
      total_bytes=0;
    }
    strcat(files_got,file_name);
    return file_buf;
  }
}

//Function to put hte files
void put_files(int sock, char* file1, char* file2, int file1_no, int file2_no, char* file_name, char* subfolder)
{
    char file_name_send[100]=" .",file_path[100];
    write(sock , command , strlen(command));
    usleep(100);
    strcat(file_name_send,file_name);
    if(file1_no==1)
      strcat(file_name_send,".1");
    else if (file1_no==2)
      strcat(file_name_send,".2");
    else if (file1_no==3)
      strcat(file_name_send,".3");
    else
      strcat(file_name_send,".4");
    if(subfolder==NULL)
    {
      strcpy(file_path,"/");
      strcat(file_path,file_name_send);
    }
    else
    {
      strcpy(file_path,subfolder);
      strcat(file_path,"/");
      strcat(file_path,file_name_send);
    }
    write(sock,file_path,strlen(file_path)+1);
    bzero(incoming_message,sizeof(incoming_message));
    recv(sock , incoming_message , sizeof(incoming_message) , 0);
    if(file1_no==4)
      write(sock,(char*)(&last_chunk),4);
    else
      write(sock,(char*)(&file_chunk),4);
    bzero(incoming_message,sizeof(incoming_message));
    recv(sock , incoming_message , sizeof(incoming_message) , 0);
    if(file1_no==4)
      write(sock , file1 , last_chunk );
    else
      write(sock , file1 , file_chunk);
    bzero(incoming_message,sizeof(incoming_message));
    recv(sock , incoming_message , sizeof(incoming_message) , 0);
    if(strcmp(incoming_message,"failure_write")==0)
      printf("Invalid path or writing file error for 1st file\n");
    if(file2_no==1)
      file_path[strlen(file_path)-1]='1';
    else if(file2_no==2)
      file_path[strlen(file_path)-1]='2';
    else if(file2_no==3)
      file_path[strlen(file_path)-1]='3';
    else
      file_path[strlen(file_path)-1]='4';
    send(sock,file_path,strlen(file_path)+1,0);
    bzero(incoming_message,sizeof(incoming_message));
    recv(sock , incoming_message , sizeof(incoming_message) , 0);
    if(file2_no==4)
      send(sock,(char*)(&last_chunk),4,0);
    else
      send(sock,(char*)(&file_chunk),4,0);
    bzero(incoming_message,sizeof(incoming_message));
    recv(sock , incoming_message , sizeof(incoming_message) , 0);
    if(file2_no==4)
      send(sock , file2 , last_chunk , 0);
    else
      send(sock , file2 , file_chunk , 0);
    bzero(incoming_message,sizeof(incoming_message));
    recv(sock , incoming_message , sizeof(incoming_message) , 0);
    if(strcmp(incoming_message,"failure_write")==0)
      printf("Invalid path or writing file error for 2nd file\n");
}

//Function to parse the conf file
void parse_conf(char conf_file_name[20])
{
  FILE *conf;
	conf=fopen(conf_file_name,"rb");//open file
  char line[256],line_orignal[256];
  char *info_type;
      while (fgets(line, sizeof(line), conf))
      {
          if(*line!='#')//ignore comment lines
          {
            strcpy(line_orignal,line);
            info_type = strtok(line, " \n");
            //getting port number
            if(strstr(info_type,"Server") != 0)
            {
              char* ip_port_str,*port_str,*ip_str;
              ip_port_str = strtok(NULL, " \n");
              ip_str= strtok(ip_port_str,":");
              port_str= strtok(NULL,":");
              strcpy(ip_addrs[server_count],ip_str);
              strcpy(ports[server_count],port_str);
              server_count++;
            }
            //getting the root path
            else if(strcmp(info_type,"Username") == 0)
            {
              char* Username_temp;
              Username_temp = strtok(NULL, " \n");
              strcpy(Username,Username_temp);
            }
            //getting DirectoryIndex
            else if(strcmp(info_type,"Password") == 0)
            {
                char* Password_temp;
                Password_temp = strtok(NULL, " \n");
                strcpy(Password,Password_temp);
            }
          }
      }
  return;
}


/****Function to encrypt data********/
void encrypt(char* data,int length)
{
  int j=0;
	for(int i=0;i<length;i++)
	{
		*data=*data ^Password[j];
		data++;
    j++;
    if(j%strlen(Password)==0)
      j=0;
	}
}

/****Function to encrypt data********/
void decrypt(char* data,int length)
{
  int j=0;
	for(int i=0;i<length;i++)
	{
		*data=*data ^Password[j];
		data++;
    j++;
    if(j%strlen(Password)==0)
      j=0;
	}
}
