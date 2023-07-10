#include <arpa/inet.h>
#include <cstdio>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sstream>
#include <cstdlib>
#include <string>
#include <iostream>

#define BUFSIZE 1024

int main(int argc, char ** argv)
{
	int socket_fd;
	int port;
	std::stringstream for_port;
	struct sockaddr_in socket_addr;
	socklen_t socket_addr_len;

	struct pollfd * pollfds = (struct pollfd *) malloc(2 * sizeof(struct pollfd));
	nfds_t nfds = 2;

	std::string username;
	ssize_t rd_bytes;
	char rd_buffer[BUFSIZE];

	if (argc < 3) {
		printf("HELP:\n\t./client host port\n");
		return -1;
	}

	std::cout << "Type your username: ";
	std::cin >> username;
	username = "[" + username + "] ";

	socket_addr.sin_family = AF_INET;
	for_port << argv[2];
	for_port >> port;
	socket_addr.sin_port = htons(port);

	if (inet_pton(AF_INET, argv[1], &socket_addr.sin_addr) < 1) {
		printf("Invalid address\n");
		return -1;
	}

	socket_addr_len = sizeof(socket_addr);
	
	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Couldn't create a socket");
		return -1;
	}

	if (connect(socket_fd, (struct sockaddr *) &socket_addr, socket_addr_len) < 0) {
		perror("Couldn't connect to address");
		return -1;
	}

	pollfds[0].fd = socket_fd;
	pollfds[1].fd = STDIN_FILENO;
	pollfds[0].events = POLLIN;
	pollfds[1].events = POLLIN;

	while (1) {

		write(STDOUT_FILENO, "you: ", 5 *sizeof(char));

		if (poll(pollfds, nfds, -1) < 0) {
			perror("Poling");
			return -1;
		}

		if (pollfds[0].revents & POLLIN) {

			rd_bytes = read(socket_fd, rd_buffer, BUFSIZE);

			if (rd_bytes < 0) {
				perror("Problem while reading from socket");
				return -1;
			}

			if (rd_bytes == 0) {
				printf("The server closed the connection\n");
				return 0;
			}

			write(STDOUT_FILENO, "\r", sizeof(char));
			write(STDOUT_FILENO, rd_buffer, rd_bytes);

		}

		if (pollfds[1].revents & POLLIN) {

			rd_bytes = read(STDIN_FILENO, rd_buffer, BUFSIZE);

			if (rd_bytes < 0) {
				perror("Problem while reading from stdin");
				return -1;
			}

			if (rd_bytes == 0) {
				printf("You closed the connection with the server\n");
				return 0;
			}

			write(socket_fd, username.c_str(), username.length());
			write(socket_fd, rd_buffer, rd_bytes);

			write(STDOUT_FILENO, "\033[A", sizeof(char));
			write(STDOUT_FILENO, "sent: ", 6 * sizeof(char));
			write(STDOUT_FILENO, rd_buffer, rd_bytes);
			write(STDOUT_FILENO, "\n", sizeof(char));

		}

	}

	return 0;

}
