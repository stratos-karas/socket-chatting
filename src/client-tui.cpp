#include <algorithm>
#include <arpa/inet.h>
#include <cstdlib>
#include <string>
#include "ftxui/component/captured_mouse.hpp"  // for ftxui
#include "ftxui/component/component.hpp"       // for Input, Renderer, Vertical
#include "ftxui/component/component_base.hpp"  // for ComponentBase
#include "ftxui/component/component_options.hpp"  // for InputOption
#include "ftxui/component/screen_interactive.hpp"  // for Component, ScreenInteractive
#include "ftxui/dom/elements.hpp"  // for text, hbox, separator, Element, operator|, vbox, border
#include "ftxui/util/ref.hpp"  // for Ref
#include "ftxui/screen/color.hpp"
#include "ftxui/dom/linear_gradient.hpp"
#include <sys/types.h>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <unistd.h>
#include <thread>
#include "packaged_msg.hpp"

#include <user.hpp>

ftxui::Elements AddFocusBottom(ftxui::Elements list) {
  if (list.size() != 0)
    list.back() = focus(std::move(list.back()));
  return std::move(list);
}

int selected_line = 0;
std::vector<std::string> lines;
ftxui::ScreenInteractive screen = ftxui::ScreenInteractive::Fullscreen();

void read_from_socket(User & user, std::string & connect_button_label) {

	ssize_t read_bytes;

	while (((read_bytes = ::read(user.socket_fd, user.buffer, 1024)) > 0) && (user.connected)) {

		std::string incoming_message = std::string(user.buffer);
		lines.push_back(incoming_message);
		selected_line = lines.size()-1;

		// Post to ftxui that there is a new message
		screen.PostEvent(ftxui::Event::Custom);

		// Flush user's buffer
		for (int i = 0; i < 1024; i++)
			user.buffer[i] = '\0';
	}

	if (read_bytes == 0) {
		user.hostname_to_connect = "Lost connection to the server";
		user.port_to_connect = "";
		user.connected = false;
		close(user.socket_fd);
		connect_button_label = "Connect";
		screen.PostEvent(ftxui::Event::Custom);
		return;
	}

	if (read_bytes < 0) {
		user.hostname_to_connect = "Fatal error while reading from server";
		user.port_to_connect = "";
		user.connected = false;
		close(user.socket_fd);
		connect_button_label = "Connect";
		screen.PostEvent(ftxui::Event::Custom);
		return;
	}

	if (!user.connected) {
		close(user.socket_fd);
		connect_button_label = "Connect";
		screen.PostEvent(ftxui::Event::Custom);
		return;
	}

}

int main()
{


	User user;
	// Declarations for sockets
	bool join_thread = false;

	// Create a thread to read from the socket
	std::thread readExecThread;
	ftxui::Elements msgs;
	auto menu = ftxui::Menu(&lines, &selected_line);

	auto my_custom_menu = ftxui::Renderer(menu, [&] {
			int begin = std::max(0, selected_line - 20);
			int end = std::min<int>(lines.size(), selected_line + 20);
			ftxui::Elements elements;
			for(int i = begin; i<end; ++i) {
				packaged_msg msg = deserialize_msg(lines[i]);
				ftxui::Element element = ftxui::window(
			      		ftxui::text(msg.from),
			      		ftxui::paragraph(msg.content)
				);
				if (i == selected_line)
					element = element | ftxui::inverted | ftxui::select;
				elements.push_back(element);
			}
			return vbox(std::move(elements)) | ftxui::frame;
	});

	ftxui::Component input_username = ftxui::Input(&user.username, "How would you like to be called in the chat?");
	ftxui::Component input_hostname = ftxui::Input(&user.hostname_to_connect, "Insert a hostname or an IP");
	ftxui::Component input_port = ftxui::Input(&user.port_to_connect, "Insert a port number");

	std::string connect_button_label = "Connect";
	ftxui::Component connect_button = ftxui::Button(&connect_button_label, [&] {
			if (connect_button_label == "Connect") {
				if (user.hostname_to_connect != "" &&  user.port_to_connect!= "") {

					if (!user.connect()) return;
					connect_button_label = "Disconnect";

					readExecThread = std::thread(read_from_socket, user, connect_button_label);

				}

				if (user.hostname_to_connect == "") {
					user.hostname_to_connect = "Didn't provide a hostname";
					return;
				}

				if (user.port_to_connect == "") {
					user.port_to_connect = "Didn't provide a port";
					return;
				}
			}
			else {
				user.username = "";
				user.hostname_to_connect = "";
				user.port_to_connect = "";

				if (user.connected)
					readExecThread.detach();

				user.connected = false;
				connect_button_label = "Connect";
				return;
			}
		});

	ftxui::Component userhostportWindow = ftxui::Renderer(

			ftxui::Container::Vertical({
				input_username, 
				input_hostname, 
				input_port, 
				connect_button
			}),

			[&] {
				return ftxui::window(
						ftxui::text("Connect to server") | ftxui::bold,
						ftxui::vbox({

							ftxui::window(  ftxui::text("Username"), 
									input_username->Render()
							),

							ftxui::window(  ftxui::text("Hostname"), 
									input_hostname->Render()
							),

							ftxui::window(  ftxui::text("Port"), 
									input_port->Render()
							),

							connect_button->Render() | ftxui::center
						})
					);
			}
	);

	ftxui::Component connectedUsers = ftxui::Container::Vertical({
				ftxui::Button("all", []{})
			});

	ftxui::Component connectedUsersWindow = ftxui::Renderer(

			connectedUsers, 

			[&] {
				return ftxui::flex(
						ftxui::window( 	ftxui::text("Connected Users") | ftxui::bold, 
								connectedUsers->Render()
						));
			}
	);

	ftxui::Component leftSideContainers = ftxui::Container::Vertical({
				userhostportWindow,
				connectedUsersWindow
			});



	std::string msgToSend;
	ftxui::InputOption inputMsgOption = ftxui::InputOption();
	inputMsgOption.on_enter = [&] {
		if (msgToSend != "") {
			if (user.connected) {
				packaged_msg msgToSendPackage = {
					.content = msgToSend,
					.from = user.username,
					.to = ""
				};
				std::string msgToSendSerialized = serialized_msg(msgToSendPackage);
				write(user.socket_fd, msgToSendSerialized.c_str(), msgToSendSerialized.size());
				lines.push_back(msgToSendSerialized);
				selected_line = lines.size()-1;
				screen.PostEvent(ftxui::Event::Custom);
				msgToSend = "";
			}
		}
	};

	ftxui::Component inputMsg = ftxui::Input(&msgToSend, "Type your message here", inputMsgOption);

	ftxui::Component rightSide = ftxui::Renderer(ftxui::Container::Vertical({my_custom_menu, inputMsg}), [&] {
			return ftxui::window(ftxui::text("Chatting Window") | ftxui::bold, 
					ftxui::vbox({
						my_custom_menu->Render()  | ftxui::color(ftxui::LinearGradient(90, ftxui::Color::DeepPink1, ftxui::Color::DeepSkyBlue1)) | ftxui::flex,
						ftxui::separator(),
						inputMsg->Render()
						})
					);
			});


	int left_side = 50;
	ftxui::Component bothSides = ftxui::ResizableSplitLeft(leftSideContainers, rightSide, &left_side);

	ftxui::Component document = ftxui::Renderer(bothSides, [&] {
			return (bothSides->Render() | ftxui::border);
			});

	screen.Loop(document);

	return 0;
	
}
