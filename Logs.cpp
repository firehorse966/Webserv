/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logs.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aiturria <aiturria@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/20 14:30:48 by aiturria          #+#    #+#             */
/*   Updated: 2025/06/11 16:05:59 by aiturria         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Logs.hpp"

Logs logs;

Logs::Logs() {
	mkdir("./logs", 0755);
    std::string date = getCurrentTime(3);
    std::string access_path = "logs/access_" + date + ".log";
    std::string error_path = "logs/error_" + date + ".log";

    _access_log.open(access_path.c_str(), std::ios::app);
    _error_log.open(error_path.c_str(), std::ios::app);

    if (!_access_log.is_open() || !_error_log.is_open()) {
        std::cerr << "Error: Could not open log files!" << std::endl;
        perror("Log file error");
    }
}

Logs::~Logs() {
	_access_log.close();
    _error_log.close();
}

void Logs::errorLog(const std::string& error) {
    if (_error_log.is_open())
		_error_log << getCurrentTime(1) << " GMT - " << error << std::endl;
    
}

void Logs::accessLog(const std::string& access) {
    if (_access_log.is_open())
        _access_log << getCurrentTime(1) << " GMT - " << access << std::endl;
}

std::string Logs::getCurrentTime(int date) {
    time_t epochtime;
    struct tm * timeinfo;
    char buffer[80];

    time(&epochtime);
    timeinfo = gmtime(&epochtime);

    if (date == 1)
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    else
        strftime(buffer, sizeof(buffer), "%Y-%m-%d", timeinfo);
    return std::string(buffer);
}

std::string Logs::intToString(int value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}
