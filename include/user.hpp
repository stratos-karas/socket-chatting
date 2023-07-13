#ifndef USER_H
#define USER_H

#pragma once
#include <string>
#include <sys/types.h>
#include <cstdlib>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>

#define BUFSIZE 1024

class User {

	public:
		std::string username;
		std::string hostname_to_connect;
		std::string port_to_connect;

		int socket_fd;
		struct sockaddr_in socket_addr;
		socklen_t socket_addr_len;
		bool connected;

		char buffer[BUFSIZE];
		std::string input_msg;


		// Declare constructors
		User();
		User(std::string, std::string, short);
		User(std::string, std::string, std::string);

		// Declare representation method
		static std::string rep(User &);

		// Connecting socket method
		bool connect();

		// Read from socket
		void read();
		
		// Write to socket

};


#endif
