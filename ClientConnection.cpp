//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//                      
//                     2ª de grado de Ingeniería Informática
//                       
//              This class processes an FTP transactions.
//						DAVID FERNANDO REDONDO DURAND
//						   alu0100851700@ull.edu.es
// 
//****************************************************************************



#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cerrno>
#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h> 
#include <iostream>
#include <fstream>
#include <dirent.h>

#include "common.h"

#include "ClientConnection.h"




ClientConnection::ClientConnection(int s, unsigned long client_addr) {

	printf("New ClientConnection\n");
    int sock = (int)(s);
  
    char buffer[MAX_BUFF];

    control_socket = s;
    // Check the Linux man pages to know what fdopen does.
    fd = fdopen(s, "a+");

    if (fd == NULL){
		std::cout << "Connection closed" << std::endl;

		fclose(fd);
		close(control_socket);
		ok = false;
		return ;
    }
    
    ok = true;
    data_socket = -1;
   	passive = false;
   	client_address = client_addr;
  
  
};


ClientConnection::~ClientConnection() {
 	fclose(fd);
	close(control_socket); 
  
}


int connect_TCP( uint32_t address,  uint16_t  port) {
     // Implement your code to define a socket here

    return -1; // You must return the socket descriptor.

}






void ClientConnection::stop() {
	std::cout << "Stoping ClientConnection" << std::endl;
    close(data_socket);
    close(control_socket);
    parar = true;
  
}





    
#define COMMAND(cmd) strcmp(command, cmd)==0

// This method processes the requests.
// Here you should implement the actions related to the FTP commands.
// See the example for the USER command.
// If you think that you have to add other commands feel free to do so. You 
// are allowed to add auxiliary methods if necessary.

void ClientConnection::WaitForRequests() {
    if (!ok) {
	 return;
    }
    
    fprintf(fd, "220 Service ready\n"); fflush(fd);
  
    while(!parar) {

      fscanf(fd, "%s", command);
      ///////////////////////////////////////////////////////////////////
      /////////////                 USER                    /////////////
      ///////////////////////////////////////////////////////////////////
      if (COMMAND("USER")) {
      	std::cout << "Setting Username" << std::endl;
	    fscanf(fd, "%s", arg);
	    fprintf(fd, "331 User name ok, need password\n"); fflush(fd);
      }
      ///////////////////////////////////////////////////////////////////
      /////////////                  PWD                    /////////////
      ///////////////////////////////////////////////////////////////////
      else if (COMMAND("PWD")) {	//Print Directory
      	char path[MAX_BUFF]; 

        if (getcwd(path, sizeof(path)) != NULL)
            fprintf(fd, "257 \"%s\" \n", path);

      }

      ///////////////////////////////////////////////////////////////////
      /////////////                 PORT                    /////////////
      ///////////////////////////////////////////////////////////////////
      else if (COMMAND("PORT")) {	//Port to be used in data connection
      	passive = false;

	  	data_socket = socket(AF_INET, SOCK_STREAM, 0);
	  	int port[2];
		int ip[4];
	  	fscanf(fd, "%d,%d,%d,%d,%d,%d",&ip[0],&ip[1],&ip[2],&ip[3],&port[0],&port[1]);	//Leer campos h1,h2,h3,h4,p1,p2
	  	char ip_decimal[40];
	  	sprintf(ip_decimal, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);	//Ip a decimal
	  	int port_dec=port[0]*256+port[1];	//Puerto a decimal
	  	printf("Remote Host %s:%d\n",ip_decimal,port_dec);

	  	//Construir address
	  	struct sockaddr_in sin;
	  	sin.sin_family = AF_INET;
	    sin.sin_addr.s_addr = inet_addr(ip_decimal);
		sin.sin_port = htons(port_dec);
		if (connect(data_socket, (struct sockaddr *)&sin, sizeof(sin)) < 0){
			std::cout << "No puedo hacer el connect con el puerto" << strerror(errno) << std::endl;
			fprintf(fd, "421 No puedo hacer el connect con el puerto: %s\n", strerror(errno)); fflush(fd);
		}
		else{
			std::cout << "Connection Established" << std::endl;
			fprintf(fd, "200 OK Connection Established\n"); fflush(fd);
		}
      }

      ///////////////////////////////////////////////////////////////////
      /////////////            PASV (passive)               /////////////
      ///////////////////////////////////////////////////////////////////
      else if (COMMAND("PASV")) {	//Passive Mode
	  	std::cout << "PASV" << std::endl;
	  	int s = socket(AF_INET,SOCK_STREAM, 0);

	  	if (s<0)
			errexit("No se ha podido crear el socket: %s\n", strerror(errno));

	  	struct sockaddr_in sin, sa;
	  	socklen_t sa_len = sizeof(sa);
	  	sin.sin_family = AF_INET;
	    sin.sin_addr.s_addr = client_address;
		sin.sin_port = 0;

		if(bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
			errexit("No puedo hacer el bind con el puerto: %s\n", strerror(errno));
	    }
	    
	    if (listen(s, 1) < 0)
	        errexit("Fallo en el listen: %s\n", strerror(errno));

	    getsockname(s, (struct sockaddr *)&sa, &sa_len);
	    fprintf(fd, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d).\n",
                (unsigned int)(client_address & 0xff),
                (unsigned int)((client_address >> 8) & 0xff),
                (unsigned int)((client_address >> 16) & 0xff),
                (unsigned int)((client_address >> 24) & 0xff),
                (unsigned int)(sa.sin_port & 0xff),
                (unsigned int)(sa.sin_port >> 8));

	    data_socket = s;
	  	passive = true;
      }

      ///////////////////////////////////////////////////////////////////
      /////////////               CWD (cd)                  /////////////
      ///////////////////////////////////////////////////////////////////
      else if (COMMAND("CWD")) {	//CHANGE WORKING DIRECTORY
      	std::cout << "Changing working directory" << std::endl;
      	fscanf(fd, "%s", arg);
      	if(arg != NULL){
      		chdir(arg);
			fprintf(fd,"250 Working directory changed to %s\n", arg);	fflush(fd);
      	}
      	else{
      		fprintf(fd, "550 Failed to change directory.\n");	fflush(fd);
      	}
		
      }

      ///////////////////////////////////////////////////////////////////
      /////////////               STOR (put)                /////////////
      ///////////////////////////////////////////////////////////////////
      else if (COMMAND("STOR") ) {	//Store File
	    std::cout << "STOR" << std::endl;
	    fscanf(fd, "%s", arg);

	    struct sockaddr_in fsin;
	    socklen_t alen;

	    std::ofstream file(arg);
	    
	    char buffer[MAX_BUFF];
	    int n; 
	    
	    if(!file.is_open()){
	    	fprintf(fd, "450 Requested file action not taken. File unavaible.\n");
	    	std::cout << "File unavaible" << std::endl;
	    }
	    else{
	    	fprintf(fd, "150 File status okay; about to open data connection.\n"); fflush(fd);

	    	if(passive){
	    		data_socket = accept(data_socket, (struct sockaddr *)&fsin, &alen);
	    	}

	    	do{
	            n = recv(data_socket, buffer, MAX_BUFF, 0);	//Read MAX_BUFF bytes in data_socket and saves in buffer
	            file << buffer;
	        } while (n == MAX_BUFF);
	        fprintf(fd,"226 Closing data connection. Requested file action successful.\n");
            file.close();
            close(data_socket);
	    }
	    
      }


      ///////////////////////////////////////////////////////////////////
      /////////////                 SYST                    /////////////
      ///////////////////////////////////////////////////////////////////
      else if (COMMAND("SYST")) {	//System
	   	fprintf(fd,"215 UNIX Type: L8.Remote system type is UNIX.Using binary mode to transfer files.\n"); fflush(fd);
      }

      ///////////////////////////////////////////////////////////////////
      /////////////                 TYPE                    /////////////
      ///////////////////////////////////////////////////////////////////
      else if (COMMAND("TYPE")) {	//Specifies the representation type as described in the Section on Data Representation and Storage
      	fscanf(fd, "%s", arg);

        if (!strcmp(arg,"A"))
            fprintf(fd, "200 Switching to ASCII mode.\n");

        else if (!strcmp(arg,"I"))
            fprintf(fd, "200 Switching to Binary mode.\n");

        else if (!strcmp(arg,"L"))
            fprintf(fd, "200 Switching to Tenex mode.\n");

        else
            fprintf(fd, "501 Syntax error in parameters or arguments.\n");
      }


      ///////////////////////////////////////////////////////////////////
      /////////////               RETR (get)                /////////////
      ///////////////////////////////////////////////////////////////////
      else if (COMMAND("RETR")) {
	    std::cout << "RETR" << std::endl;

		fscanf(fd, "%s", arg);
		
		if(arg!=NULL){
			std::ifstream file;
	  		file.open(arg);

	  		struct sockaddr_in fsin;
		    socklen_t alen;
	  		if(passive)
		    		data_socket = accept(data_socket, (struct sockaddr *)&fsin, &alen);

	  		std::string line;
	  		if (file.is_open()){
			    fprintf(fd,"150 File status okay; about to open data connection.\n\n");
			    while ( file.good() ) {
					getline(file,line);
					line += "\n";
					send(data_socket, line.c_str(), strlen(line.c_str()), 0);
					line = "";
			    }

			    file.close();
			    close(data_socket);
			    fprintf(fd,"226 Closing data connection. Requested file action successful.\n");//Cierra fichero
			}
			else 
		     	fprintf(fd,"450 Requested file action not taken. File unavaible.\n");
		}
		
		else 
		    fprintf(fd,"500 Error.\n");
		
		
		close(data_socket);
      }


      ///////////////////////////////////////////////////////////////////
      /////////////                  QUIT                   /////////////
      ///////////////////////////////////////////////////////////////////
      else if (COMMAND("QUIT")) {
      	std::cout << "Quiting connection" << std::endl;
      	fprintf(fd, "221 Connection closed by the FTP client\r\n\n"); fflush(fd);
	 	stop();
      }


      ///////////////////////////////////////////////////////////////////
      /////////////               LIST (ls)                 /////////////
      ///////////////////////////////////////////////////////////////////
      else if (COMMAND("LIST")) {
      	std::cout << "Printing directory" << std::endl;

		int c = fgetc(fd);
		if (c != '\r')
		  fscanf(fd, "%s", arg);
		else
		  strcpy(arg, "");
		  
		
		std::string command = "ls ";
      	command += arg;
      	command += " > lstemp.txt";
		system(command.c_str());

		std::ifstream lsfile;
  		lsfile.open("lstemp.txt");

  		struct sockaddr_in fsin;
	    socklen_t alen;
  		if(passive)
	    		data_socket = accept(data_socket, (struct sockaddr *)&fsin, &alen);

  		std::string line;
  		if (lsfile.is_open()){
		    fprintf(fd,"125 List started OK.\n");
		    while ( lsfile.good() ) {
				getline(lsfile,line);
				line += "\n";
				send(data_socket, line.c_str(), strlen(line.c_str()), 0);
		    }

		    lsfile.close();
		    close(data_socket);
		    fprintf(fd,"250 List completed successfully.\n");//Cierra fichero
		    system("rm lstemp.txt");	//Elimina fichero
		}
		else {
		     fprintf(fd,"500 Error.\n");
		}
		
		close(data_socket);
	  }
	  else  {
	    fprintf(fd, "502 Command not implemented.\n"); fflush(fd);
	    printf("Comando : %s %s\n", command, arg);
	    printf("Error interno del servidor\n");
      
      }

	}
    fclose(fd);
    return;
  
};
