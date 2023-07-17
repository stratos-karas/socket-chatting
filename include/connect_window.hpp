#ifndef CONNECT_WINDOW_H
#define CONNECT_WINDOW_H

#pragma once
#include "user.hpp"
#include <string>
#include <thread>
#include <ftxui/component/component_base.hpp>

typedef ftxui::Component InputComponent;
typedef std::string ButtonLabel;
typedef ftxui::Component ButtonComponent;
typedef ftxui::Component WindowComponent;


class ConnectWindow : public ftxui::Component {

	private:

		InputComponent input_username;
		InputComponent input_hostname;
		InputComponent input_port;

		ButtonLabel connect_button_label;
		ButtonComponent connect_button;
		
		std::thread reading_thread;

	public:

		ConnectWindow() = delete;
		ConnectWindow(User&);

		void connect_on_click(User&, std::thread&);


};

#endif
