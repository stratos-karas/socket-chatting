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
#include <sys/types.h>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <unistd.h>
#include <thread>
#include "packaged_msg.hpp"

ftxui::Elements AddFocusBottom(ftxui::Elements list) {
  if (list.size() != 0)
    list.back() = focus(std::move(list.back()));
  return std::move(list);
}

int main()
{

	ftxui::ScreenInteractive screen = ftxui::ScreenInteractive::Fullscreen();

	// Declarations for sockets
	int socket_fd;
	int port;
	struct sockaddr_in socket_addr;
	socklen_t socket_addr_len;
	bool connected = false;
	bool join_thread = false;

	// Create a thread to read from the socket
	std::thread readExecThread;
	ftxui::Elements msgs;
	int selected_line = 0;
	std::vector<std::string> lines;
	auto menu = ftxui::Menu(&lines, &selected_line);

	auto my_custom_menu = ftxui::Renderer(menu, [&] {
			int begin = std::max(0, selected_line - 15);
			int end = std::min<int>(lines.size(), selected_line + 15);
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

	std::string username;
	std::string hostname;
	std::string portname;
	ftxui::Component inputUsername= ftxui::Input(&username, "How would you like to be called in the chat?");
	ftxui::Component inputHost = ftxui::Input(&hostname, "Insert a hostname or an IP");
	ftxui::Component inputPort = ftxui::Input(&portname, "Insert a port number");

	std::string connectButtonLabel = "Connect";
	ftxui::Component connectButton = ftxui::Button(&connectButtonLabel, [&] {
			if (connectButtonLabel == "Connect") {
				if (hostname != "" && portname != "") {

					if (username == "")
						username = "anonymous";
					
					// Setting the family of protocols
					socket_addr.sin_family = AF_INET;

					// Setting up the port
					port = std::stoi(portname);
					socket_addr.sin_port = htons(port);

					// Setting up the hostname
					if (inet_pton(AF_INET, hostname.c_str(), &socket_addr.sin_addr) < 0) {
						hostname = "Invalid hostname!";
						return;
					}

					socket_addr_len = sizeof(socket_addr);

					if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
						hostname = "Can't create a socket!";
						return;
					}

					if (connect(socket_fd, (struct sockaddr *) &socket_addr, socket_addr_len) < 0) {
						hostname = "Can't connect to server";
						return;
					}

					connectButtonLabel = "Disconnect";
					connected = true;

					readExecThread = std::thread([&] {

								join_thread = false;

								char read_buffer[1024];
								ssize_t read_bytes;
								std::string readMsg;
								
								while (((read_bytes = read(socket_fd, read_buffer, 1024)) > 0) && (!join_thread)) {

									readMsg = std::string(read_buffer);
									lines.push_back(readMsg);
									selected_line = lines.size()-1;

									// Post to ftxui that there is a new message
									screen.PostEvent(ftxui::Event::Custom);

									// Flush read buffer
									for (int i = 0; i < 1024; i++)
										read_buffer[i] = '\0';
								}

								if (read_bytes == 0) {
									hostname = "Lost connection to the server";
									portname = "";
									connectButtonLabel = "Connect";
									connected = false;
									close(socket_fd);
									screen.PostEvent(ftxui::Event::Custom);
									return;
								}

								if (read_bytes < 0) {
									hostname = "Fatal error while reading from server";
									portname = "";
									connectButtonLabel = "Connect";
									connected = false;
									close(socket_fd);
									screen.PostEvent(ftxui::Event::Custom);
									return;
								}

								if (join_thread) {
									write(socket_fd, NULL, 0);
									close(socket_fd);
									screen.PostEvent(ftxui::Event::Custom);
									return;
								}

							});
				}

				if (hostname == "") {
					hostname = "Didn't provide a hostname";
					return;
				}

				if (portname == "") {
					portname = "Didn't provide a port";
					return;
				}
			}
			else {
				username = "";
				hostname = "";
				portname = "";
				connectButtonLabel = "Connect";
				connected = false;
				join_thread = true;
				readExecThread.detach();
				return;
			}
		});

	ftxui::Component userhostportWindow = ftxui::Renderer(
			ftxui::Container::Vertical({inputUsername, inputHost, inputPort, connectButton}),
			[&] {
				return ftxui::window(
						ftxui::text("Connect to server") | ftxui::bold,
						ftxui::vbox({
							ftxui::window(ftxui::text("Username"), inputUsername->Render()),
							ftxui::window(ftxui::text("Hostname"), inputHost->Render()),
							ftxui::window(ftxui::text("Port"), inputPort->Render()),
							connectButton->Render() | ftxui::center
							})
					);
			}
	);

	ftxui::Component connectedUsers = ftxui::Container::Vertical({
				ftxui::Button("all", []{})
			});
	ftxui::Component connectedUsersWindow = ftxui::Renderer(connectedUsers, [&] {
				return ftxui::window(
						ftxui::text("Connected Users") | ftxui::bold, 
						connectedUsers->Render()
					) | ftxui::flex;
			});

	ftxui::Component leftSideContainers = ftxui::Container::Vertical({
				userhostportWindow,
				connectedUsersWindow
			});



	std::string msgToSend;
	ftxui::InputOption inputMsgOption = ftxui::InputOption();
	inputMsgOption.on_enter = [&] {
		if (msgToSend != "") {
			if (connected) {
				packaged_msg msgToSendPackage = {
					.content = msgToSend,
					.from = username,
					.to = ""
				};
				std::string msgToSendSerialized = serialized_msg(msgToSendPackage);
				write(socket_fd, (msgToSendSerialized + "\n").c_str(), msgToSendSerialized.size() + 1);
				lines.push_back(msgToSendSerialized);
				selected_line = lines.size()-1;
				screen.PostEvent(ftxui::Event::Custom);
				// msgs.push_back(ftxui::window(
				// 			ftxui::text("user"),
				// 			ftxui::paragraph(std::string(msgToSend))
				// 			) | ftxui::inverted
				// 	      );
				msgToSend = "";
			}
		}
	};

	ftxui::Component inputMsg = ftxui::Input(&msgToSend, "Type your message here", inputMsgOption);

	ftxui::Component rightSide = ftxui::Renderer(ftxui::Container::Vertical({my_custom_menu, inputMsg}), [&] {
			return ftxui::window(ftxui::text("Chatting Window") | ftxui::bold, 
					ftxui::vbox({
						my_custom_menu->Render() | ftxui::flex,
						//ftxui::vbox(AddFocusBottom(msgs)) | ftxui::vscroll_indicator | ftxui::frame | ftxui::flex | ftxui::focus,
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
