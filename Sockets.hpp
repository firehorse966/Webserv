/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Sockets.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aiturria <aiturria@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/20 14:30:07 by aiturria          #+#    #+#             */
/*   Updated: 2025/06/09 15:18:46 by aiturria         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

# include <sys/types.h> 
# include <sys/socket.h> 
# include <sys/epoll.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <unistd.h>
# include <errno.h>
# include <cstring>
# include <fcntl.h>
# include <ctime>
# include <iostream>
# include <string>
# include <map>
# include "Logs.hpp"
# include "ConfigParser.hpp"

# define MAX_EVENTS 100
# define TIMEOUT 30
# define TIMEDATA 5

extern Logs logs;

struct ClientInfo {
	int client_fd;
	int type;
	bool cgi;
	bool bigfile;
	bool chunked;
	pid_t mycgi;
	int cgiInputFD;
	int cgiOutputFD;
	int filesize;
	int fileFD;
	ClientInfo *myclient;
	std::string client_ip;
	std::string server_ip;
	std::string client_port;
	std::string server_port;
	time_t start_time;
	time_t last_activity;
	std::vector<char> request;
	int bytesSent;
	int serverNbr;
	std::map<std::string, std::string> response;
	std::string finalResponse;
	int error;
};

enum e_type {
	SOCKET,
 	CLIENT,
 	CGI,
 	DATA
 };

class Sockets {
	private:
	std::map<int, int> _sockets;
	
	public:
	
		static std::map<int, ClientInfo> _clients;
		static int _epoll_fd;
		static struct epoll_event _events[MAX_EVENTS];
		Sockets(void);
		~Sockets(void);
		void createSockets(void);
		void addClient(int incomingFD);
		ClientInfo* getClient(int fd);
		std::map<int, ClientInfo>* getClients(void);
		std::map<int, int> *getSockets(void);
		void closeSockets(void);
		static void removeClient(int fd);
		static bool modifyEpoll(int fd, int operation, uint32_t events);
	};

