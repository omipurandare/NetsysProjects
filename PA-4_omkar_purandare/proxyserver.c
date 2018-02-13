/****************************************************************************************************************
Program: Proxyserver
Author: Omkar Purandare
Subject: Network Systems
*****************************************************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>
#include <pthread.h> //for threading , link with lpthread
#include <netdb.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

void *connection_handler(void *new_socket);
void send_bad_req(int type, int sock);
void send_bad_method(int sock);
void send_not_found(int sock);
void send_internal_error(int sock);
void send_dns_error(int sock);
char* check_hostname_cache(char* str);
void store_hostname_cache(char* hostname, char* IP);
void send_forbidden_error(int sock);
int check_if_blocked(char* hostname, char* ip);
int timeout;

int main(int argc , char *argv[])
{
    int socket_desc , client_sock , c , *new_sock;
    struct sockaddr_in server , client;
    pthread_t sniffer_thread[100];
    int thread_no=0;
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);//Create socket
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created....");

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( atoi(argv[1]));
    timeout = atoi(argv[2]);//reading timeout

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
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) >=0)
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
      printf("Error in socket\n");
        perror("accept failed");
        return 1;
    }
    close(socket_desc);
    return 0;
}

//Handler to handle the request
void *connection_handler(void *new_socket)
{
    //Get the socket descriptor
    struct sockaddr_in host_addr;
    struct hostent *lh;
    int sock = *(int*)new_socket, port_flag=0,port;
    int sockfd1,newsockfd;
    int read_size;
    char *client_demand, *method, *URL, *version, client_message[2000],*temp,URL_cpy[100],buffer[2000],*IP,IPcpy[100],*buffer_cache;
    read_size = recv(sock , client_message , 2000 , 0);
    if(read_size == 0)
    {
        puts("Client disconnected");
        fflush(stdout);
        return 0;
    }
    else if(read_size == -1)
    {
        perror("recv failed or keep-alive timeout occured");
        return 0;
    }
    printf("Received REQ..");
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
    else
    {
      send_bad_method(sock);
      return 0;
    }

    URL = strtok(NULL, "  ");

      //handle bad req For URL
    if(URL==NULL)
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
    
    //port assigning
    strcpy(URL_cpy,URL);
    for(int i=7;i<strlen(URL);i++)
    {
      if(URL[i]==':')
      {
        port_flag=1;
        break;
      }
    }
    temp=strtok(URL,"//");
    if(port_flag==0)
    {
      port=80;
      temp=strtok(NULL,"/");
    }
    else
    {
      temp=strtok(NULL,":");
    }

   sprintf(URL,"%s",temp);
   printf("host = %s",URL);
   struct stat st = {0};

   if (stat(URL, &st) == -1) {
       mkdir(URL, 0700);
   }
    IP = check_hostname_cache(URL); //getting IP address
    if(IP==NULL)
    {
      lh = gethostbyname(URL); //couldnt resolve IP to get 
      if(lh==NULL)
        {
          send_dns_error(sock);
          return 0;
        }
        store_hostname_cache(URL,inet_ntoa(*(struct in_addr*)(lh->h_addr)));//store in host:IP cache
    }
    else
      strcpy(IPcpy,IP);

    if(port_flag==1)
    {
      temp=strtok(NULL,"/");
      port=atoi(temp);
    }

    strcat(URL_cpy,"^]");
    temp=strtok(URL_cpy,"//");
    temp=strtok(NULL,"/");
    if(temp!=NULL)
      temp=strtok(NULL,"^]"); //finding the path
    printf("\npath = %s\nPort = %d\n",temp,port);
    char* file,path_cpy[100],file_path[100];
    if(temp == NULL)
    {
      sprintf(file_path,"./%s/index.html",URL);// making special path for index.html
    }
    else
    {
      strcpy(path_cpy,temp);
      file = strrchr(path_cpy,'/');
      if(file==NULL)
      {
        file=path_cpy;
      }
      else
        file++;
      //making file index.html
      if(strlen(file)==0)
        sprintf(file_path,"./%s/index.html",URL);
      else
        sprintf(file_path,"./%s/%s",URL,file);
    }
    FILE *cache_page;
    cache_page=fopen(file_path,"rb"); //open file
    //if file is available in cache
    if((cache_page!=NULL)&&(strstr(file_path,"index.html")==NULL))
    {
      printf("Taking contents from cache\n");
      fseek(cache_page,0,SEEK_END);
      size_t file_size=ftell(cache_page);   //gettiing file size
      fseek(cache_page,0,SEEK_SET);
      buffer_cache=(char*)malloc(file_size);
      //internal server error handle
      if(buffer_cache==NULL)
      {
        send_internal_error(sock);
        return 0;
      }
      if(fread(buffer_cache,file_size,1,cache_page)<=0)    //reading file from cache
      {
        printf("unable to copy file into buffer\n");
        send_internal_error(sock);
        return 0;
      }
      send(sock,buffer_cache,file_size,0);	//send file from cache
      fclose(cache_page);
    }
    else
    {
      bzero((char*)&host_addr,sizeof(host_addr));
      host_addr.sin_port=htons(port);
      host_addr.sin_family=AF_INET;
      if(IP==NULL)
        bcopy((char*)lh->h_addr,(char*)&host_addr.sin_addr.s_addr,lh->h_length); //copy ip address in the structure
      else
      {
        host_addr.sin_addr.s_addr=inet_addr(IPcpy);
      }
      sockfd1=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
      newsockfd=connect(sockfd1,(struct sockaddr*)&host_addr,sizeof(struct sockaddr)); //connect to remote server
      sprintf(buffer,"\nConnected to %s  IP - %s\n",URL,inet_ntoa(host_addr.sin_addr));
      if(check_if_blocked(URL,inet_ntoa(host_addr.sin_addr))==1)  //send forbidden error
      {
        send_forbidden_error(sock);
        return 0;
      }
      if(newsockfd<0)
        perror("Error in connecting to remote server");

      printf("\n%s\n",buffer);
      bzero((char*)buffer,sizeof(buffer)); //making req to remote server
      if(temp!=NULL)
        sprintf(buffer,"GET /%s %s\r\nHost: %s\r\nConnection: close\r\n\r\n",temp,version,URL);
      else
        sprintf(buffer,"GET / %s\r\nHost: %s\r\nConnection: close\r\n\r\n",version,URL);

      int n=send(sockfd1,buffer,strlen(buffer),0); //sending error to socket
      printf("\n%s\n",buffer);
      cache_page=fopen(file_path,"wb+");
      //writing the file into the cache
      if(n<0)
      {
        printf("Error wrirting socket");
        perror("Error writing to socket");
      }
      else{
        do
        {
          bzero((char*)buffer,500);
          n=recv(sockfd1,buffer,500,0);
          if(!(n<=0))
          {
            fwrite(buffer,1,n,cache_page);
            send(sock,buffer,n,0);
          }
        }while(n>0);
      }
      fclose(cache_page);
      close(sockfd1);
      close(newsockfd);
      sleep(timeout);
      remove(file_path);// remove the file after timeout
    }
    close(sock);
    return 0;
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

//dns couldnt resolve
void send_dns_error(int sock)
{
  char error_header[1000];
  strcpy(error_header,"HTTP/1.1 DNS error\n\n");
  strcat(error_header,"<html><body>DNS error: DNS could not resolve hostname </body></html>\n");
  write(sock , error_header , strlen(error_header));
  printf("DNS error: Sending error ....\n\n");
  close(sock);
}

//site in forbidden list
void send_forbidden_error(int sock)
{
  char error_header[1000];
  strcpy(error_header,"HTTP/1.1 403 forbidden error\n\n");
  strcat(error_header,"<html><body>403 forbidden error: The site is forbidden </body></html>\n");
  write(sock , error_header , strlen(error_header));
  printf("DNS error: Sending error:sending error 403 ....\n\n");
  close(sock);
}

//check file for IP mapping
char* check_hostname_cache(char* str)
{
  char *ip_tp;
  char line[256];
  FILE *req;
  //writing req to a file
  req=fopen("hostname_ip_map","rb+");
  while (fgets(line, sizeof(line), req))
  {
    if(strstr(line,str)!=NULL)
    {
      strtok(line,":\n");
      ip_tp=strtok(NULL,":\n");
      return ip_tp;
    }
  }
  return NULL;
}

//check if site is in blocked list
int check_if_blocked(char* hostname, char* ip)
{
  char line[256];
  FILE *req;
  //writing req to a file
  req=fopen("blocked_sites","rb+");
  while (fgets(line, sizeof(line), req))
  {
    if(strstr(line,hostname)!=NULL)
      return 1;
    else if(strstr(line,ip)!=NULL)
      return 1;
  }
  return 0;
}

//update the IP/cache file
void store_hostname_cache(char* hostname, char* IP)
{
  char host_ip[100];
  FILE *req;
  strcpy(host_ip,hostname);
  strcat(host_ip,":");
  strcat(host_ip,IP);
  strcat(host_ip,"\n");
  req=fopen("hostname_ip_map","a");
  fwrite(host_ip,1,strlen(host_ip),req);
  fclose(req);
  return;
}
