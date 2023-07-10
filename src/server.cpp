#include <cstdlib>
#include <sys/poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdio>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <cstdlib>
#include <thread>
#include <vector>
#include "packaged_msg.hpp"

#define PORT 6789
#define BUFFSIZE 1024

int main()
{
	int socket_fd;
	struct sockaddr_in socket_addr;
	int sock_len = sizeof(socket_addr);
	int opt = 1;
	char rdbuffer[BUFFSIZE];
	ssize_t read_bytes;

	std::vector<struct pollfd> pollfds;
	struct pollfd stdin_fd;
	stdin_fd.events = POLLIN;
	stdin_fd.fd = STDIN_FILENO;
	pollfds.push_back(stdin_fd);
	nfds_t polls = pollfds.size();
	//struct pollfd * pollfds = (struct pollfd *) malloc(2 * sizeof(struct pollfd));
	//nfds_t polls = 2;

	std::string servername = "[![server]!] ";

	// Create a socket
	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		std::perror("Couldn't create a listening socket");
		return -1;
	}
	// Change the options of the socket
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT | SO_KEEPALIVE, &opt, sizeof(opt)) < 0) {
		perror("Error while setting options for the socket");
		return -1;
	}

	// Setting up the address and port to bind the new socket
	socket_addr.sin_family = AF_INET;
	socket_addr.sin_addr.s_addr = INADDR_ANY;
	socket_addr.sin_port = htons(PORT);

	// Bind the socket to the given address and port
	if (bind(socket_fd, (struct sockaddr *) &socket_addr, sizeof(socket_addr)) < 0) {
		perror("Failed to bind the socket to the given address and port");
		return -1;
	}

	if (listen(socket_fd, 5) < 0) {
		perror("Error while listening");
		return -1;
	}

	std::thread connection_thread([&] {
			int conn_fd;
			while(true) {
				
				if ((conn_fd = accept(socket_fd, (struct sockaddr *) &socket_addr, (socklen_t *) &sock_len)) < 0) {
					perror("Error while accepting a new connection");
					return -1;
				}

				struct pollfd new_fd;
				new_fd.events = POLLIN;
				new_fd.fd = conn_fd;
				polls++;

				pollfds.push_back(new_fd);
			}
		});

	while (1) {

		if (poll(&(pollfds[0]), polls, 10) < 0) {
			perror("Polling");
			return -1;
		}

		if (pollfds[0].revents & POLLIN) {

			read_bytes = read(STDIN_FILENO, rdbuffer, BUFFSIZE);

			if (read_bytes < 0) {
				perror("Problem while reading");
				return -1;
			}

			if (read_bytes == 0) {
				std::cout << "You closed the server\n";
				break;
			}

			std::string msgToSend(rdbuffer);
			packaged_msg packagedMsgToSend = {
				.content = msgToSend,
				.from = "server",
				.to = "all"
			};
			std::string serializedMsg = serialized_msg(packagedMsgToSend);

			// I could flush the read buffer but fuck that shit
			for (int i = 1; i < polls; i++)
				write(pollfds[i].fd, (serializedMsg + "\n").c_str(), serializedMsg.size() + 1);

			for (int i = 0; i < BUFFSIZE; i++)
				rdbuffer[i] = '\0';

		}

		for (int i = 1; i < polls; i++) {

			if (pollfds[i].revents & POLLIN) {

				std::cout << "Size of pool = " << pollfds.size() << "\n";

				read_bytes = read(pollfds[i].fd, rdbuffer, BUFFSIZE);

				if (read_bytes < 0) {
					perror("Problem while reading");
					return -1;
				}

				if (read_bytes == 0) {
					std::cout << "A client exited\n";
					pollfds.erase(pollfds.cbegin() + i);
					polls--;
					break;
				}

				write(STDOUT_FILENO, rdbuffer, read_bytes);
				for (int j = 1; j < polls; j++)
					if (j != i)
						write(pollfds[j].fd, rdbuffer, read_bytes);

				// I could flush the read buffer but fuck that shit
				// write(conn_fd, servername.c_str(), servername.length());
				// write(conn_fd, rdbuffer, read_bytes);

			}
		}
		
	}

	for (auto pollstruct : pollfds)
		close(pollstruct.fd);

	return 0;

}
