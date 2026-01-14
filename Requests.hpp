/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Requests.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aiturria <aiturria@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/20 14:29:43 by aiturria          #+#    #+#             */
/*   Updated: 2025/05/06 15:30:14 by aiturria         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

# include <sys/types.h> 
# include <unistd.h> 
# include <errno.h>
# include <cstring>
# include <ctime>
# include <iostream>
# include <string>
# include <sstream>
# include <map>
# include <vector>
# include <algorithm>
# include "Logs.hpp"
# include "Sockets.hpp"
# include "Response.hpp"
# include "Methods.hpp"
# include "ConfigParser.hpp"

class Response;

struct clientRequest {
    std::string method;
    std::string path;
    std::string httpVersion;
    std::map<std::string, std::string> headers;
};

class Requests {
	private:
		clientRequest _request;
		std::map<std::string, std::string> _serverResponse;
	
	public:
	
		Requests(void);
		~Requests(void);
		void handleClient(int fd, ClientInfo &client);
		void receiveRequest(int fd, ClientInfo &client);
		clientRequest *getClientRequest(void);
		std::map<std::string, std::string> *getServerResponse(void);
		void processRequest(ClientInfo &client);
		void hostParser(ClientInfo &client);
		int methodParser(void);
		int pathParser(void);
		void headersParser(void);
		void listeningPort(ClientInfo &client);
		void printRequest() const;
		void clientResponse(std::map<std::string, std::string> &response) const;
		std::string pathDecode(std::string& str);
		void printVectorCharDebug(const std::vector<char>& vec);
		
};
