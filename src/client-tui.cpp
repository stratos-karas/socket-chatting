#include <algorithm>
#include <arpa/inet.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include "ftxui/component/captured_mouse.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/util/ref.hpp"
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

int main()
{


	ftxui::ScreenInteractive screen = ftxui::ScreenInteractive::Fullscreen();
	User user;

	// Create a thread to read from the socket
	std::thread readExecThread;
	ftxui::Elements msgs;
	ftxui::Component menu = ftxui::Menu(&(user.msg_threads), &(user.selected_msg));

	ftxui::Component my_custom_menu = ftxui::Renderer(menu, [&] {
				int begin = std::max(0, user.selected_msg - 20);
				int end = std::min<int>(user.msg_threads.size(), user.selected_msg + 20);
				ftxui::Elements elements;
				for(int i = begin; i<end; ++i) {
					PackagedMessaged msg = deserialize_msg(user.msg_threads[i]);
					ftxui::Element element = ftxui::window(
						ftxui::text(msg.from),
						ftxui::paragraph(msg.content)
					);
					if (i == user.selected_msg)
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

					// If the user couldn't connect return
					if (!user.connect()) return;

					// If the user connected then start reading
					// from the socket
					readExecThread = user.recve(screen);

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
				user.disconnect();

				readExecThread.detach();

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

				// Always check if the user is connected
				if (user.is_connected())
					connect_button_label = "Disconnect";
				else
					connect_button_label = "Connect";

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



	ftxui::InputOption input_msg_option = ftxui::InputOption();
	input_msg_option.on_enter = [&] { user.send(screen); };

	ftxui::Component input_msg = ftxui::Input(&(user.input_msg), "Type your message here", input_msg_option);

	ftxui::Component rightSide = ftxui::Renderer(

			ftxui::Container::Vertical({
				my_custom_menu, 
				input_msg
			}), 

			[&] {
			
				return ftxui::window(
						ftxui::text("Chatting Window") | ftxui::bold, 

						ftxui::vbox({
						
							my_custom_menu->Render() | ftxui::flex,
						
							ftxui::separator(),
						
							input_msg->Render()

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
