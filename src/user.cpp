#include "user.hpp"

User::User() : 
	username(""), 
	hostname_to_connect("127.0.0.1"), 
	port_to_connect("6789") {};

User::User(std::string username, std::string hostname, short port) :
	username(username),
	hostname_to_connect(hostname),
	port_to_connect(std::to_string(port)),
	connected(false) {};

User::User(std::string username, std::string hostname, std::string port) :
	username(username),
	hostname_to_connect(hostname),
	port_to_connect(port),
	connected(false) {};

std::string User::rep(User & user) {

	return  "<username>\n\t" + user.username + "\n</username>\n" +
		"<hostname>\n\t" + user.hostname_to_connect + "\n</hostname>\n" +
		"<port>\n\t" + user.port_to_connect + "\n</port>\n";
}

bool User::connect() {

		// Setting the family of protocols
		(this->socket_addr).sin_family = AF_INET;

		// Setting up the port
		(this->socket_addr).sin_port = htons(std::stoi(this->port_to_connect));

		// Setting up the hostname
		if (inet_pton(AF_INET, (this->hostname_to_connect).c_str(), &((this->socket_addr).sin_addr)) < 0) {
			this->hostname_to_connect = "Invalid hostname!";
			return false;
		}

		this->socket_addr_len = sizeof(this->socket_addr);

		if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			this->hostname_to_connect = "Can't create a socket!";
			return false;
		}

		if (::connect(this->socket_fd, (struct sockaddr *) &(this->socket_addr), this->socket_addr_len) < 0) {
			this->hostname_to_connect = "Can't connect to server";
			return false;
		}

		this->connected = true;

		return true;

}

