/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aiturria <aiturria@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/17 12:51:08 by aiturria          #+#    #+#             */
/*   Updated: 2025/06/11 15:25:53 by aiturria         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <csignal>
#include <sstream>
#include <sys/time.h>
#include "Sockets.hpp"
#include "Logs.hpp"
#include "Response.hpp"
#include "Requests.hpp"
#include "ConfigParser.hpp"

volatile bool run = true;

#include "ConfigParser.hpp"

void handleSignal(int sig) {
    if (sig == SIGINT) {
		std::cout << " Shutting down server...\n";
		logs.accessLog("Shutting down server...");
        run = false;
    }
}

int main(int argc, char **argv) {
	
	signal(SIGINT, handleSignal);
	signal(SIGCHLD, SIG_IGN);
	Sockets sockets;
	Requests request;
	Methods::mimeTypes();
	if (argc > 2){
		std::cerr << "Uso: " << argv[0] << " <archivo de configuración>\n";
		return 1;
	}
	if (argc == 1) {
		ConfigParser config("./default.cfg");
		if (!config.isValidCFG)
			exit (1);
	}
	else if (argc == 2) {
		ConfigParser config = ConfigParser(argv[1]);
		if (!config.isValidCFG)
			exit (1);
	}
	
	sockets.createSockets();
	
	while (run) {
		int activeFDs = epoll_wait(Sockets::_epoll_fd, Sockets::_events, MAX_EVENTS, 3000);
		if (activeFDs == -1) {
			if (errno == EINTR)
				continue;
			logs.errorLog("Error: epoll wait failed");
			perror("epoll wait");
			break;
		}
		struct timeval current;
        gettimeofday(&current, NULL);
		
		for(std::map<int, ClientInfo>::iterator it = Sockets::_clients.begin();
			it != Sockets::_clients.end();) {
				ClientInfo *client = &it->second;
				if (client->type == CLIENT && current.tv_sec - client->last_activity > TIMEOUT) {
					
					logs.accessLog("Client " + logs.intToString(it->first) + " timed out. Disconnected.");
					if (client->mycgi != 0 && client->cgiInputFD != 0) {
						Sockets::removeClient(client->cgiInputFD);
					}
					if (client->mycgi != 0 && client->cgiOutputFD != 0) {
						Sockets::removeClient(client->cgiOutputFD);
					}
					Sockets::removeClient(it->first);
					it = Sockets::_clients.begin();
					continue;
				}
				if ((client->type == CGI || client->type == DATA) && current.tv_sec - client->last_activity > TIMEDATA) {
					logs.accessLog("Data from " + logs.intToString(it->first) + " interrupted. Disconnected.");
					client->myclient->response["statusCode"] = "408 Request Timeout";
					Response::errorResponse(*client->myclient);
					client->myclient->finalResponse = Response::buildResponse(*client->myclient);
					client->myclient->mycgi = 0;
					Sockets::removeClient(it->first);
					it = Sockets::_clients.begin();
					continue;
				}
				else
					it++;
			}
		for (int i = 0; i < activeFDs; i++) {
			int fd = Sockets::_events[i].data.fd;
			
			if (Sockets::_events[i].events & EPOLLRDHUP) {
				logs.accessLog("Client " + logs.intToString(fd) + " closed the connection.");
				Sockets::removeClient(fd);
				continue;
			}
			
			if (sockets.getSockets()->find(fd) != sockets.getSockets()->end())
				sockets.addClient(fd);
			else if (Sockets::_events[i].events & EPOLLIN) {
				
				std::map<int, ClientInfo>::iterator it = Sockets::_clients.find(fd);
				if (it != Sockets::_clients.end()) {
            		ClientInfo *client = &it->second;
					switch (client->type) {
						case CLIENT: {
							
							request.handleClient(fd, *sockets.getClient(fd));
							break;
						}
						case CGI: 
							Methods::readFromCgi(*client);	
							break;
						case DATA:
							Methods::pipeToClient(*client);
							break;
						default:
							logs.errorLog("Unknown client type: " + logs.intToString(client->type));
							break;
					}
				}
			}
			else if (Sockets::_events[i].events & EPOLLOUT) {
				
				std::map<int, ClientInfo>::iterator it = Sockets::_clients.find(fd);
				if (it != Sockets::_clients.end()) {
            		ClientInfo *client = &it->second;
					
					switch (client->type) {
						case CLIENT:
							if (client->chunked == true)
								Response::sendChunkedData(*client);
							else
								Response::sendData(*client);
							break;
						case CGI:
							Methods::writeToCgi(*client);
							break;
						case DATA:
							Methods::readFileToPipe(*client);
							break;
						default:
							logs.errorLog("Unknown client type: " + logs.intToString(client->type));
							break;
					}
				}
			}
		}
	}
	sockets.closeSockets();	
}
