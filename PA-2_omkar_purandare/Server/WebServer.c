/****************************************************************************************************************
Program: Webserver
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
#include<signal.h>

//the thread function
int port;
void *connection_handler(void *);
void send_bad_req(int type, int sock);
void send_bad_method(int sock);
void send_not_found(int sock);
void send_internal_error(int sock);
int special_handle(char* message);
char file_types[100][100],post_data_actual[1000];
int file_types_count=0;
char* buffer;
int keep_alive_time,socket_desc;
char header[1024];
char root_path[1000], html_file[100] ;
char* itoa(int val, int base);
void parse_conf();
void get_post_data(char* client_message);
void grace_exit(int sig);

int main(int argc , char *argv[])
{
    signal(SIGINT,grace_exit);
    int client_sock , c , *new_sock;
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
    server.sin_port = htons( port);

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

/*
  Handle connection for each client
 * */
void grace_exit(int sig)
{
  close(socket_desc);
  printf("Server closed gracefully\n");
  exit(0);
}

void *connection_handler(void *new_socket)
{
    //Get the socket descriptor
    int sock = *(int*)new_socket;
    int read_size,keep_alive_flag,post_available=0,special_counter=0;
    char *client_demand, *method, *URL, *version, client_message[2000],post_data_complete[1000],*post_data_actual_cpy;
    FILE *fp;
    struct timeval tv;  //timespec structure for timeout
    char path[1000];
    //giving default timeout timeout
    tv.tv_sec = keep_alive_time;
    tv.tv_usec = 0;
    while(1)
    {
  		if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) //setting the socket blocking for 10sec generally
      			perror("Error");
      read_size = recv(sock , client_message , 2000 , 0);
      //handling timeout or error reading
      if(read_size == 0)
      {
          puts("Client disconnected");
          fflush(stdout);
          break;
      }
      else if(read_size == -1)
      {
          perror("recv failed or keep-alive timeout occured");
          break;
      }
      printf("Received REQ..");
      //Checking for 2 req in one message
      if(strstr(client_message,"POST")!=NULL)
      {
        get_post_data(client_message);
        post_available=1;
      }
      if(post_available==0)
      {
	      if(strstr(client_message,"\n\n")==NULL)
	      {
		special_counter= special_handle(client_message);
		printf("%d\n",special_counter);
	      }
      }
      //Checking for keep alive in message
      if(strstr(client_message,"keep-alive")==NULL)
      {
        //setting timer to 0 if not found
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        keep_alive_flag=0;
      }
      else
      {
        //updating timer if found
        tv.tv_sec = keep_alive_time;
        tv.tv_usec = 0;
        keep_alive_flag=1;
      }
      //if command has POST, handle POST
      printf("Socket Handling the request:%d\n",sock);
      client_demand = strtok(client_message, "\n\n");//get first line of req
      //handle bad req
      if(client_demand==NULL)
      {
        send_bad_req(1,sock);
        return 0;
      }
      printf("Fulldemand: %s\n",client_demand);
        //handle bad req for method
      method = strtok(client_demand, "  ");
      if(method==NULL)
      {
        send_bad_req(1,sock);
        return 0;
      }
      if(strcmp(method,"GET")==0)
        printf("Method: %s\n",method);
      //checking for bad method
      else if(strcmp(method,"POST")==0)
      {
        printf("Method: %s\n",method);
      }
      else
      {
        send_bad_method(sock);
        return 0;
      }

      URL = strtok(NULL, "  ");
        //handle bad req For URI
      if(URL==NULL)
      {
        send_bad_req(2,sock);
        return 0;
      }
      if(*URL != '/')
      {
        send_bad_req(2,sock);
        return 0;
      }
      printf("URL: %s\n",URL);
      //handle bad req For Version
      version = strtok(NULL, "  ");
      if(version==NULL)
      {
        send_bad_req(3,sock);
        return 0;
      }
      if(strstr(version,"HTTP/1.1")!=NULL)
        printf("Version: %s\n\n",version);
      else if(strstr(version,"HTTP/1.0")!=NULL)
        printf("Version: %s\n",version);
      else
      {
        send_bad_req(3,sock);
        return 0;
      }
      //handling html page request
      if(strcmp(URL,"/")==0)
      {
        //pipelining by telnet
        for(int i=0; i<special_counter;i++)
        {
          //making entire path
          strcpy(path,root_path);
          strcat(path,"/");
          strcat(path,html_file);
          //trying to open file , if not found then send error
          fp=fopen(path,"rb");
          if(fp==NULL)
          {
            send_not_found(sock);
            return 0;
          }
          fseek(fp,0,SEEK_END);
          size_t file_size=ftell(fp);   //gettiing file size
          char* size = itoa(file_size,10);
          fseek(fp,0,SEEK_SET);
          buffer=(char*)malloc(file_size); //dynamic allocation of file size
          //Handling internal server error due memmory issue
          if(buffer==NULL)
          {
            send_internal_error(sock);
            return 0;
          }
          if(fread(buffer,file_size,1,fp)<=0)    //reading buffer from file
          {
          	printf("unable to copy file into buffer\n");
            send_internal_error(sock);
            return 0;
          }
          bzero(header,sizeof(header));
          //making the header to send
          if(strstr(version,"1.1") != NULL)
            strcpy(header,"HTTP/1.1");
          else
            strcpy(header,"HTTP/1.0");
          strcat(header," 200 OK\n");
          strcat(header,"Content-Type: text/html\n");
          strcat(header,"Content-Length: ");
          strcat(header,size);
          strcat(header,"\n");
          //deciding on connection field based on request
          if(keep_alive_flag==0)
            strcat(header,"Connection: Close");
          else
            strcat(header,"Connection: Keep-alive");
          strcat(header,"\n\n");
          printf("********Response sent:*******\n");
          printf("%s",header);
          //sending header
          write(sock , header , strlen(header));
          //Handling post req
          if(post_available==1)
          {
            post_available=0;
            post_data_actual_cpy=strtok(post_data_actual,"\n");
            strcpy(post_data_complete,"<html><body><pre><h1>");
            strcat(post_data_complete,post_data_actual_cpy);
            strcat(post_data_complete,"</h1></pre>");
            write(sock , post_data_complete , strlen(post_data_complete));
          }
          //send the file
          write(sock , buffer , file_size);
          printf("*************************************\n\n");
        }
      }
      else
      {
        char *file_type,*file_type_header;
        char file_type_line[100];
        int j;
        //checking for file type in the request
        for(j=0;j<=file_types_count-1;j++)
        {
          strcpy(file_type_line,file_types[j]);
          file_type=strtok(file_type_line," \n");
          if(strstr(URL,file_type)!=NULL)
          {
            file_type_header=strtok(NULL," ");
            break;
          }
        }
        bzero(path,strlen(path));
        //making path to the file
        strcpy(path,root_path);
        strcat(path,URL);
        fp=fopen(path,"rb");
        //error for file_not found
        if(fp==NULL)
        {
          send_not_found(sock);
          return 0;
        }
        fseek(fp,0,SEEK_END);
        size_t file_size=ftell(fp);   //gettiing file size
        char* size = itoa(file_size,10);
        fseek(fp,0,SEEK_SET);
        buffer=(char*)malloc(file_size);
        //internal server error handle
        if(buffer==NULL)
        {
          send_internal_error(sock);
          return 0;
        }
        if(fread(buffer,file_size,1,fp)<=0)    //reading packets from file
        {
        	printf("unable to copy file into buffer\n");
          send_internal_error(sock);
          return 0;
        }
        bzero(header,sizeof(header));
        //build the header to send
        if(strstr(version,"1.1") != NULL)
          strcpy(header,"HTTP/1.1");
        else
          strcpy(header,"HTTP/1.0");
        strcat(header," 200 OK\n");
        strcat(header,"Content-Type: ");
        strcat(header,file_type_header); //appending file type
        strcat(header,"Content-Length: ");
        strcat(header,size);
        strcat(header,"\n");
        if(keep_alive_flag==0)
          strcat(header,"Connection: Close");
        else
          strcat(header,"Connection: Keep-alive");
        strcat(header,"\n\n");
        printf("********Response sent:*******\n");
        printf("%s",header);
        //sending header
        write(sock , header , strlen(header));
        //sending file
        write(sock , buffer , file_size);
        printf("**************************************\n\n");
      }
    }
    printf("Closing Handler Socket number: %d\n\n",sock);
    //close client socket
    close(sock);
    return 0;
}

/****Function to handle multiple req*****/
int special_handle(char* message)
{
  int number_of_req=0;
  char line[256];
  FILE *req;
  //writing req in file
  req=fopen("special_handle","wb+");
  fwrite(message,1,strlen(message),req);
  fclose(req);
  req=fopen("special_handle","rb");
  //reading lines from
  while (fgets(line, sizeof(line), req))
  {
    //counting number of GET
    if(strstr(line,"GET")!=NULL)
      number_of_req++;
  }
  fclose(req);
  return number_of_req;
}

/****Function to handle posting data*****/
void get_post_data(char* client_message)
{
  int read_line=0;
  char line[256];
  FILE *req;
  //writing req to a file
  req=fopen("post_data","wb+");
  fwrite(client_message,1,strlen(client_message),req);
  fclose(req);
  req=fopen("post_data","rb");
  //get the data to post
  while (fgets(line, sizeof(line), req))
  {
    if(read_line==1)
      {
        fclose(req);
        break;
      }
    if(*line=='\n')
      read_line=1;
  }
  strcpy(post_data_actual,line);
  return;
}

/*Error 400*/
void send_bad_req(int type,int sock)
{
  char error_header[1000];
  strcpy(error_header,"HTTP/1.1 400 Bad Request\n\n");
  if(type==1)
  strcat(error_header,"<html><body>400 Bad Request ,Reason: Invalid Method </body></html>\n");
  else if(type==2)
  strcat(error_header,"<html><body>400 Bad Request ,Reason: Invalid URL</body></html>\n");
  else
  strcat(error_header,"<<html><body>400 Bad Request ,Reason: Invalid HTTP-Version</body></html>\n");
  write(sock , error_header , strlen(error_header));
  printf("Bad Request: Sending error 400....\n\n");
  close(sock);
}

/*Error 501*/
void send_bad_method(int sock)
{
  char error_header[1000];
  strcpy(error_header,"HTTP/1.1 501 Not Implemented\n\n");
  strcat(error_header,"<html><body>501 Not Implemented </body></html>\n");
  write(sock , error_header , strlen(error_header));
  printf("Bad Method: Sending error 501....\n\n");
  close(sock);
}

/*Error 404*/
void send_not_found(int sock)
{
  char error_header[1000];
  strcpy(error_header,"HTTP/1.1 404 Not Found\n\n");
  strcat(error_header,"<html><body>404 Not Found Reason URL does not exist </body></html>\n");
  write(sock , error_header , strlen(error_header));
  printf("URI not found: Sending error 404....\n\n");
  close(sock);
}

/*Error 500*/
void send_internal_error(int sock)
{
  char error_header[1000];
  strcpy(error_header,"HTTP/1.1 500 Internal server error\n\n");
  write(sock , error_header , strlen(error_header));
  printf("Internal server error: Sending error 500....\n\n");
  close(sock);
}

char* itoa(int val, int base){

	static char buf[32] = {0};

	int i = 30;

	for(; val && i ; --i, val /= base)

		buf[i] = "0123456789abcdef"[val % base];

	return &buf[i+1];

}

/***Function to parse configure file***/
void parse_conf()
{
	FILE *conf;
	char conf_path[]="/home/omkar/Desktop/PA-2/Server/ws.conf";
	conf=fopen(conf_path,"rb");//open file
  char line[256],line_orignal[256];
  char *info_type;
      while (fgets(line, sizeof(line), conf))
      {
          if(*line!='#')//ignore comment lines
          {
            strcpy(line_orignal,line);
            info_type = strtok(line, " \n");
            //getting port number
            if(strcmp(info_type,"Listen") == 0)
            {
              char* port_str;
              port_str = strtok(NULL, " \n");
              port= atoi(port_str);
            }
            //getting the root path
            else if(strcmp(info_type,"DocumentRoot") == 0)
            {
              char* root_with_quotes, *root_path_ptr;
              root_path_ptr=root_path;
              root_with_quotes = strtok(NULL, " \n");
              root_with_quotes++;
              size_t n = strlen(root_with_quotes);
              for(int j=0; j<(n-1);j++)
              {
                *root_path_ptr=*root_with_quotes;
                root_with_quotes++;
                root_path_ptr++;
              }
              *root_path_ptr='\0';
            }
            //getting DirectoryIndex
            else if(strcmp(info_type,"DirectoryIndex") == 0)
            {
                char *html_file_base;
                html_file_base= strtok(NULL, " \n");
                strcpy(html_file,html_file_base);
            }
            //reading keep-alive time
            else if(strcmp(info_type,"Keep-Alive_time") == 0)
            {
              char* keep_alive_time_str;
              keep_alive_time_str= strtok(NULL, " \n");
              keep_alive_time = atoi(keep_alive_time_str);
            }
            else
            {
              //saving file type in double array
              if(*line=='.')
              {
                strcpy(file_types[file_types_count],line_orignal);
                file_types_count++;
              }
            }
          }
      }
}
