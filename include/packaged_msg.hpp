#ifndef PACKAGED_MSG_H
#define PACKAGED_MSG_H

#pragma once

#include <string>

struct packaged_msg {
	std::string content; 	// Content of the message
	std::string from; 	// Username of the sender
	std::string to; 	// TODO: choose the receiver
};

typedef struct packaged_msg packaged_msg;

packaged_msg deserialize_msg(std::string);
std::string serialized_msg(packaged_msg);

#endif
