#include "user.hpp"
#include <iostream>
#include <pthread.h>

User::User() : 
	username(""), 
	hostname_to_connect("127.0.0.1"), 
	port_to_connect("6789") {};

User::User(InputString username, InputString hostname, short port) :
	username(username),
	hostname_to_connect(hostname),
	port_to_connect(std::to_string(port)),
	connected(false) {};

User::User(InputString username, InputString hostname, InputString port) :
	username(username),
	hostname_to_connect(hostname),
	port_to_connect(port),
	connected(false) {};

Representation User::rep(User & user) {

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

		// if (fcntl(socket_fd, F_SETFL, fcntl(socket_fd, F_GETFL, 0) | O_NONBLOCK) < 0) {
		// 	perror("calling fcntl");
		// 	return false;
		// }

		if (::connect(this->socket_fd, (struct sockaddr *) &(this->socket_addr), this->socket_addr_len) < 0) {
			this->hostname_to_connect = "Can't connect to server";
			return false;
		}

		this->connected = true;

		return true;

}

void User::disconnect() {
	this->connected = false;
	return;
}

bool User::is_connected() {
	return this->connected;
}

void User::send(ftxui::ScreenInteractive & screen) {

	// If the input message is not empty
	if (this->input_msg != "") {

		// And the user is connected to a server/peer
		if (this->is_connected()) {

			// Create a packaged message (packet)
			// with the appropriate data
			PackagedMessaged packaged_msg = {
				.content = this->input_msg,
				.from = this->username,
				.to = ""
			};

			// Get the serialized form of the packaged message
			SerializedMessage serialized_input_msg = serialized_msg(packaged_msg);

			// Send the serialzed message to the server/peer
			::write(this->socket_fd, serialized_input_msg.c_str(), serialized_input_msg.size());

			// Push the new message to the front end TUI
			(this->msg_threads).push_back(serialized_input_msg);

			// Focus on the last message
			this->selected_msg = (this->msg_threads).size()-1;

			// Delete the text of the input message
			this->input_msg = "";

			// Update the screen
			screen.PostEvent(ftxui::Event::Custom);

		}
	}
}

std::thread User::recve(ftxui::ScreenInteractive & screen) {

	std::thread worker([&] {

		ssize_t read_bytes;

		while (((read_bytes = ::read(this->socket_fd, this->buffer, 1024)) > 0) && (this->is_connected())) {

			std::cerr << read_bytes << "\n";

			std::string incoming_message = std::string(this->buffer);
			this->msg_threads.push_back(incoming_message);
			this->selected_msg = this->msg_threads.size()-1;

			// Post to ftxui that there is a new message
			screen.PostEvent(ftxui::Event::Custom);

			// Flush this->s buffer
			for (int i = 0; i < 1024; i++)
				this->buffer[i] = '\0';
		}

		if (read_bytes == 0) {
			std::cerr << "read_bytes = 0\n";
			this->hostname_to_connect = "Lost connection to the server";
			this->port_to_connect = "";
			this->disconnect();
			close(this->socket_fd);
			screen.PostEvent(ftxui::Event::Custom);
			return;
		}

		if (read_bytes < 0) {
			std::cerr << "read_bytes < 0\n";
			this->hostname_to_connect = "Fatal error while reading from server";
			this->port_to_connect = "";
			this->disconnect();
			close(this->socket_fd);
			screen.PostEvent(ftxui::Event::Custom);
			return;
		}

		if (!(this->is_connected())) {
			std::cerr << "disconnected\n";
			close(this->socket_fd);
			screen.PostEvent(ftxui::Event::Custom);
			return;
		}
	});

	return worker;
}

