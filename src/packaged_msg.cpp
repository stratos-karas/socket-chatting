#include "packaged_msg.hpp"

packaged_msg deserialize_msg(std::string serialized_msg) {

	std::string content;
	std::string from;
	std::string to;

	content = serialized_msg.substr(
			serialized_msg.find("<content>") + 9,
			serialized_msg.find("</content>") - (serialized_msg.find("<content>") + 9)
			);

	from = serialized_msg.substr(
			serialized_msg.find("<from>") + 6,
			serialized_msg.find("</from>") - (serialized_msg.find("<from>") + 6)
			);


	to = serialized_msg.substr(
			serialized_msg.find("<to>") + 4,
			(serialized_msg.find("</to>") - (serialized_msg.find("<to>") + 4))
			);

	packaged_msg deserialized_msg = {
		.content = content,
		.from = from,
		.to = to
	};

	return deserialized_msg;

}

std::string serialized_msg(packaged_msg msg) {

	std::string serialized_msg;

	serialized_msg = "<content>" + msg.content + "</content>" + 
		"<from>" + msg.from + "</from>" +
		"<to>" + msg.to + "</to>";

	return serialized_msg;

}
