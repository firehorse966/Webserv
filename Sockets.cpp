/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Sockets.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aiturria <aiturria@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/20 14:30:19 by aiturria          #+#    #+#             */
/*   Updated: 2025/06/10 14:37:02 by aiturria         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Sockets.hpp"

std::map<int, ClientInfo> Sockets::_clients;
int Sockets::_epoll_fd;
struct epoll_event Sockets::_events[MAX_EVENTS];

Sockets::Sockets() {}

Sockets::~Sockets() {}

void Sockets::createSockets() {
	
	_epoll_fd = epoll_create1(0);
    if (_epoll_fd == -1) {
		logs.errorLog("Error: epoll creation failed");
        perror("epoll create1");
        return;
    }
	logs.accessLog("epoll created successfully");
	for (size_t i = 0; i < ConfigParser::servers.size(); i++) {
		
		for (std::map<in_addr_t, std::vector<int> >::iterator it = ConfigParser::servers[i].listen.begin();
			 it != ConfigParser::servers[i].listen.end(); ++it) {
			
			for (size_t j = 0; j < it->second.size(); j++) {
				
				int port = it->second[j];
				struct sockaddr_in server_addr;
				memset(&server_addr, 0, sizeof(server_addr));
				server_addr.sin_family = AF_INET;
				server_addr.sin_port = htons(port);
				server_addr.sin_addr.s_addr = it->first;
				
				int server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
				if (server_fd == -1) {
					logs.errorLog("Error: Socket creation failed for port :" + logs.intToString(port));
					perror("Socket creation failed");
					continue;
				}
				
				int optval = 1;
				if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optval, sizeof(optval)) == -1) {
					logs.errorLog("Error: Setting REUSEADDR / REUSEPORT failed for port :" + logs.intToString(port));
					perror("Setting SO_REUSEADDR / REUSEPORT failed");
					close(server_fd);
					continue;
				}
				
				if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
					logs.errorLog("Error: Binding failed for port :" + logs.intToString(port));
					perror("Socket Binding failed");
					close(server_fd);
					continue;
				}
				
				if (listen(server_fd, MAX_EVENTS) < 0) {
					logs.errorLog("Error: Listening failed for port :" + logs.intToString(port));
					perror("Listen failed");
					close(server_fd);
					continue;
				}
				_sockets[server_fd] = SOCKET;
				if (modifyEpoll(server_fd, EPOLL_CTL_ADD, EPOLLIN | EPOLLRDHUP) == false) {
					perror("epoll_ctl");
					close(server_fd);
					continue;	
				}
				logs.accessLog("Socket listening on " + std::string(inet_ntoa(server_addr.sin_addr)) + ":" + logs.intToString(port));
			}
		}
	}
}


ClientInfo* Sockets::getClient(int fd) {
    std::map<int, ClientInfo>::const_iterator it = _clients.find(fd);
    if (it == _clients.end())
        return NULL;
    return const_cast<ClientInfo*>(&it->second); 
}

std::map<int, int> *Sockets::getSockets(void) {
	return &_sockets;
}

std::map<int, ClientInfo>* Sockets::getClients(void) {
	return &_clients;
}

void Sockets::addClient(int incomingFD) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int client_fd = accept(incomingFD, (struct sockaddr*)&client_addr, &client_len);
	fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL, 0) | O_NONBLOCK);
    if (client_fd == -1) {
        logs.errorLog("Error: client failed to connect");
        perror("accept");
        return;
    }

	if (modifyEpoll(client_fd, EPOLL_CTL_ADD, EPOLLIN | EPOLLRDHUP) == false) {
		perror("epoll_ctl");
		close(client_fd);
		return;
	}
	
	
	ClientInfo client_info;

	char client_ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
	client_info.client_ip = std::string(client_ip);
	client_info.client_port = logs.intToString(ntohs(client_addr.sin_port));

	struct sockaddr_in server_addr;
	socklen_t server_len = sizeof(server_addr);
	getsockname(incomingFD, (struct sockaddr*)&server_addr, &server_len);

	char server_ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &server_addr.sin_addr, server_ip, INET_ADDRSTRLEN);
	client_info.server_ip = std::string(server_ip);
	client_info.server_port = logs.intToString(ntohs(server_addr.sin_port));
	
	client_info.client_fd = client_fd;
	client_info.type = CLIENT;
	client_info.cgi = false;
	client_info.bigfile = false;
	client_info.chunked = false;
	client_info.myclient = NULL;
	client_info.bytesSent = 0;
	client_info.start_time = time(NULL);
	client_info.last_activity = client_info.start_time;
	client_info.request.clear();
	client_info.finalResponse = "";
	client_info.mycgi = 0;
	client_info.response.clear();
	client_info.cgiInputFD = -1;
	client_info.cgiOutputFD = -1;
	client_info.fileFD = -1;
	client_info.error = 1;
	client_info.filesize = 0;
	client_info.serverNbr = -1;

	_clients[client_fd] = client_info;

	logs.accessLog("New client connected: IP " + client_info.client_ip + ":" + client_info.client_port + " (FD " + logs.intToString(client_fd) + ")");
}

void Sockets::removeClient(int fd) {
    modifyEpoll(fd, EPOLL_CTL_DEL, 0);
    close(fd);
	if (_clients.find(fd) != _clients.end()) {
    	_clients.erase(fd);
		logs.accessLog("removed client at FD " + logs.intToString(fd));   
	}
}


void Sockets::closeSockets() {

    for (std::map<int, ClientInfo>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        modifyEpoll(it->first, EPOLL_CTL_DEL, 0);
        close(it->first);
        logs.accessLog("Closed client connection: FD " + logs.intToString(it->first));
    }
    _clients.clear();

    for (std::map<int, int>::iterator it = _sockets.begin(); it != _sockets.end(); ++it) {
        modifyEpoll(it->first, EPOLL_CTL_DEL, 0);
        close(it->first);
        logs.accessLog("Closed listening socket: FD " + logs.intToString(it->first));
    }
    _sockets.clear();

    if (_epoll_fd != -1) {
        close(_epoll_fd);
        logs.accessLog("Closed epoll FD");
        _epoll_fd = -1;
    }
}


bool Sockets::modifyEpoll(int fd, int operation, uint32_t events) {
    struct epoll_event event;
    event.events = events;
    event.data.fd = fd;

    if (epoll_ctl(_epoll_fd, operation, fd, &event) == -1) {
        if (errno != ENOENT) {
			logs.errorLog("Error: Failed to remove FD " + logs.intToString(fd) + " from epoll");
			perror("epoll_ctl");
			return false;
		}
    }
    return true;
}

