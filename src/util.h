/*
 * util.h
 *
 *  Created on: May 7, 2016
 *      Author: Rohit
 */
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#ifndef SRC_UTIL_H_
#define SRC_UTIL_H_
namespace util {
std::string runCommand(const char* cmd) {
	char buffer[128];
	std::string result = "";
	std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
	if (!pipe)
		throw std::runtime_error("popen() failed!");
	while (!feof(pipe.get())) {
		if (fgets(buffer, 128, pipe.get()) != NULL)
			result += buffer;
	}
	return result;
}
int getValue() {
	return 0;
}

}

#endif /* SRC_UTIL_H_ */
