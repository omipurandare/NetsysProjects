Description-
In this program a server and a client has been developed which communicate over udp and they can exchange files. Apart from file exchange the client offers a set of commands which can give a good experience to the user.

Folder structure-
PA-1_omkar_purandare-----|
			 |-----server(various image.txt pdf files,udp_server.c,Makefile) 
			 |
			 |-----client(various image.txt pdf files,udp_client.c,Makefile)

Compiling code-
Both the server and client folders have their makefiles, just run command "make" to get executables as "client" and "server"

Running code-

Server- 
Give command "./server [port number]" on the command line to run the server, further no command can be given to server. It can be only be given commands via client. 

Client-
Give command "./client [ip] [prt number]" on the command line to run the client. We can further give following commands to client and the server reacts accordingly-
 1) To download any file from server : get(file_name)
 2) To send any file to the server : put(file_name)
 3) To delete any file in the server : delete(file_name)
 4) To list all files the server : ls
 5) To shut the server down gracefully : exit
Any other command is treated as an invalid command. The way to shut the client is through ctrl+C. 
