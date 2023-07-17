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
#include <vector>
#include <fcntl.h>
#include <ftxui/component/screen_interactive.hpp>
#include "packaged_msg.hpp"

#define BUFSIZE 1024

typedef std::string Representation;
typedef std::string InputString;
typedef std::string Message;
typedef std::vector<std::string> MessageThreads;
typedef int Selection;

class User {

	private:
		bool connected = false;

	public:
		InputString username;
		InputString hostname_to_connect;
		InputString port_to_connect;

		int socket_fd;
		struct sockaddr_in socket_addr;
		socklen_t socket_addr_len;

		// Thread of messages the user read and wrote
		MessageThreads msg_threads;

		// Message focused on
		Selection selected_msg;

		// Buffer to read from (sever's/peer's) socket
		bool read_data = false;
		char buffer[BUFSIZE];

		// Buffer string to write to (server's/peer's) socket
		Message input_msg;

		// Declare constructors
		User();
		User(std::string, std::string, short);
		User(std::string, std::string, std::string);

		// Declare representation method
		static std::string rep(User &);

		// Connecting socket method
		bool connect();

		// Disconnecting from socket
		void disconnect();

		// Is the user connected to a socket?
		bool is_connected();

		// Write/Send to socket
		// Find a way to dimultiplex Screen instance
		void send(ftxui::ScreenInteractive & screen);

		std::thread recve(ftxui::ScreenInteractive & screen);

};

//void recve(User & user, ftxui::ScreenInteractive & screen);

#endif
