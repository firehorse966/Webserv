/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Requests.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aiturria <aiturria@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/25 16:00:27 by aiturria          #+#    #+#             */
/*   Updated: 2025/06/11 15:49:46 by aiturria         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Requests.hpp"

Requests::Requests() {}

Requests::~Requests() {}

void Requests::handleClient(int fd, ClientInfo &client) {

	receiveRequest(fd, client);
	
	if (client.error == 0) {
		Response::errorResponse(client);
		client.finalResponse = Response::buildResponse(client);
		Sockets::modifyEpoll(client.client_fd, EPOLL_CTL_MOD, EPOLLOUT);
	}
	if (client.error == -1) {
		Sockets::removeClient(fd);
		logs.accessLog("Client disconnected (FD " + logs.intToString(fd) + ")");
	} else if (client.error == 2) {
		Methods::decideMethod(client);
		if (client.response.at("Method") == "DELETE" && client.error > 0)
			return;
		if (client.cgi == true) {
			if (Methods::handlecgi(client) != 0)
				return;
		}
		else if (client.bigfile == true) {
			Methods::bigFiles(client);
			if (client.error != 0)
				return;
		}
		if (client.error == 0)
			Response::errorResponse(client);
		client.finalResponse = Response::buildResponse(client);
		Sockets::modifyEpoll(client.client_fd, EPOLL_CTL_MOD, EPOLLOUT | EPOLLIN);	
	}	
}


void Requests::receiveRequest(int client_fd, ClientInfo &client) {
	
	char buffer[READ];
    int bytes = recv(client_fd, buffer, sizeof(buffer), 0);
    if (bytes < 0) {
		logs.accessLog("Client " + logs.intToString(client_fd) + ": connection disconnected by the client.");
			client.error = -1;
			return;
	}
	else if (bytes == 0)
		return;
	client.request.insert(client.request.end(), buffer, buffer + bytes);
	client.last_activity = time(NULL);
	
	const char pattern[] = "\r\n\r\n";
	std::vector<char>::iterator header_end_it = std::search(
    	client.request.begin(), client.request.end(), pattern, pattern + 4);
		
	size_t header_end = std::string::npos;
	if (header_end_it != client.request.end())
		header_end = std::distance(client.request.begin(), header_end_it);

	if (client.cgi == false && header_end != std::string::npos ) {
		processRequest(client);
		if (client.error == 0)
			return;
	} else {
		if (client.cgi == true) {
			if (client.bytesSent == 0) {
				client.bytesSent = client.request.size();
			}
			else 
				client.bytesSent += bytes;
			if (bytes == 0 || (client.response.find("Body-Length") != client.response.end() && 
				client.bytesSent >= std::atoi(client.response.at("Body-Length").c_str()))) {
				Sockets::modifyEpoll(client.client_fd, EPOLL_CTL_MOD, EPOLLOUT);
				client.bytesSent = 0;
			}
			
		}
		client.error = 1;
		return;
	}
	client.error = 2;
	return;
}

clientRequest *Requests::getClientRequest(void){
	return &_request;
}

std::map<std::string, std::string> *Requests::getServerResponse(void) {
	return &_serverResponse;
}

void Requests::processRequest(ClientInfo &client) {

	_request = clientRequest();
	_serverResponse.clear();

	std::string requestString(client.request.begin(), client.request.end());
	std::istringstream requestStream(requestString);
    std::string line;

	if (std::getline(requestStream, line)) {
        std::istringstream lineStream(line);
        lineStream >> _request.method >> _request.path >> _request.httpVersion;
    }
	while (std::getline(requestStream, line) && !line.empty()) {
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string headerKey = line.substr(0, colon);
            std::string headerValue = line.substr(colon + 1);
            headerValue.erase(0, headerValue.find_first_not_of(" \t"));
			for (size_t i = 0; i < headerKey.size(); ++i) {
                headerKey[i] = std::tolower(headerKey[i]);
            }
            _request.headers[headerKey] = headerValue;
        }
	}
	//printVectorCharDebug(client.request);
	const char pattern[] = "\r\n\r\n";
	std::vector<char>::iterator header_end_it = std::search(
    	client.request.begin(), client.request.end(), pattern, pattern + 4);
		
	size_t header_end = std::string::npos;
	if (header_end_it != client.request.end())
		header_end = std::distance(client.request.begin(), header_end_it);
	
	client.request.erase(client.request.begin(), client.request.begin() + header_end + 4);
	std::map<std::string, std::string>::iterator it = _request.headers.find("content-length");
    if (it != _request.headers.end()) {
		int contentLength = 0;
		std::istringstream lengthStream(it->second);
		if (!(lengthStream >> contentLength) || contentLength < 0) {
    		_serverResponse["statusCode"] = "400 Bad Request";
			client.error = 0;
    		return;
		}
    }
	logs.accessLog("FD " + logs.intToString(client.client_fd) + " request " + _request.method + " " + _request.path);
	_serverResponse["statusCode"] = "200 OK";
	hostParser(client);
	if (methodParser() == 0 || pathParser() == 0) {
		client.response = _serverResponse;
		client.error = 0;
		return;
	}	
	headersParser();
	if (ConfigParser::servers[client.serverNbr].client_max_body_size != -1
		&& _serverResponse.find("Body-Length") != _serverResponse.end()) {
		ssize_t length = static_cast<size_t>(std::atoi(_serverResponse["Body-Length"].c_str()));
		if (length > ConfigParser::servers[client.serverNbr].client_max_body_size) {
			_serverResponse["statusCode"] = "413 Payload Too Large";
			_serverResponse["Connection"] = "close";
			client.response = _serverResponse;
			client.error = 0;
			return;
		}
	}
	client.response = _serverResponse;
	//printRequest();
	return;
}

int Requests::methodParser() {
	if (_request.httpVersion.empty() || _request.method.empty()) {
		_serverResponse["statusCode"] = "400 Bad Request";
		_serverResponse["Connection"] = "close";
		return 0;
	}
	if (_request.httpVersion != "HTTP/1.0" && _request.httpVersion != "HTTP/1.1") {
        _serverResponse["statusCode"] = "505 HTTP Version Not Supported";
		_serverResponse["Connection"] = "close";
		return 0;
	}
	if (_request.method == "GET" ||_request.method == "POST" || _request.method == "DELETE" 
	|| _request.method == "HEAD" || _request.method == "OPTIONS")
		_serverResponse["Method"] = _request.method;
	else {
		_serverResponse["Method"] = _request.method;
		_serverResponse["statusCode"] = "405 Method Not Allowed";
		_serverResponse["Connection"] = "close";
		return 0;
	}
	return 1;
}

void Requests::hostParser(ClientInfo &client) {
	if (_request.headers.find("connection") != _request.headers.end()) {
        size_t pos = _request.headers.at("connection").find('\r');
        if (pos != std::string::npos)
			_request.headers.at("connection").erase(pos);
		_serverResponse["Connection"] = _request.headers["connection"];
	}
	else
		_serverResponse["Connection"] = "keep-alive";
		
	std::map<std::string, std::string>::iterator it = _request.headers.find("host");
	if (it != _request.headers.end() && !it->second.empty()) {
		_serverResponse["Host"] = it->second;
		size_t colon = _serverResponse.at("Host").find(':');
		if (colon != std::string::npos) 
			_serverResponse["Host"] = _serverResponse.at("Host").substr(0, colon);
	}	
	else
		_serverResponse["Host"] = client.server_ip + ":" + client.server_port;
	listeningPort(client);
}


void Requests::listeningPort(ClientInfo &client) {
   
    bool serverFound = false;
    for (size_t i = 0; i < ConfigParser::servers.size(); i++) {
        for (std::map<in_addr_t, std::vector<int> >::iterator it = ConfigParser::servers[i].listen.begin();
             it != ConfigParser::servers[i].listen.end(); ++it) {
                
            char stored_ip[INET_ADDRSTRLEN];
            struct in_addr addr;
            addr.s_addr = it->first;
            if (inet_ntop(AF_INET, &addr, stored_ip, INET_ADDRSTRLEN) == NULL)
                continue;
            if (client.server_ip == std::string(stored_ip)) {
                int client_port = std::strtol(client.server_port.c_str(), NULL, 10);
                if (std::find(it->second.begin(), it->second.end(), client_port) != it->second.end()) {
					if (!serverFound && client.serverNbr == -1)
						client.serverNbr = i;
                    for (size_t j = 0; j < ConfigParser::servers[i].server_names.size(); j++ ) {
                        if (_serverResponse.at("Host") == ConfigParser::servers[i].server_names[j]) {
                            client.serverNbr = i;
                            serverFound = true;
                            break;
                        }
                    }
                }
                
            }
        }
        if (serverFound) break;
    }
    _serverResponse["Root"] = ConfigParser::servers[client.serverNbr].root;
}

std::string Requests::pathDecode(std::string& str) {
    std::string out;
    int hex;
    for (size_t i = 0; i < str.size(); ++i) {
      if (str[i] == '%' && i+2 < str.size()) {
        sscanf(str.substr(i+1,2).c_str(), "%x", &hex);
        out += char(hex);
        i += 2;
      } else if (str[i] == '+') {
        out += ' ';
      } else {
        out += str[i];
      }
    }
    return out;
}

int Requests::pathParser() {
    _request.path = pathDecode(_request.path);
    if (_request.path[0] != '/') {
        _serverResponse["statusCode"] = "400 Bad Request";
        return 0; 
    }    
    std::string Path;
    std::istringstream ss(_request.path);
    std::string dir;
    bool hasTrailingSlash = _request.path.length() > 1 && _request.path[_request.path.length() - 1] == '/';
    
    while (std::getline(ss, dir, '/'))
    {
        if (dir == "..") {
            _serverResponse["statusCode"] = "403 Forbidden";
            return 0;
        }
        else if (dir.empty() || dir == ".")
            continue;
        Path += "/" + dir;
    }
    if (Path.empty())
        Path = "/";
    else if (hasTrailingSlash)
        Path += "/";
    
    _serverResponse["Path"] = Path;
    return 1;
}
		
void Requests::headersParser(void) {
    std::map<std::string, std::string>::iterator it = _request.headers.find("accept");
    if (it != _request.headers.end()) {
        size_t comma = it->second.find(",");
        if (comma == std::string::npos)
            _serverResponse["Accept"] = it->second;
        else
            _serverResponse["Accept"] = it->second.substr(0, comma);
	}
	it = _request.headers.find("content-type");
	if (it != _request.headers.end())
		_serverResponse["Content-Type"] = it->second;

	it = _request.headers.find("content-length");
	if (it != _request.headers.end())
		_serverResponse["Body-Length"] = it->second;	
	
	it = _request.headers.find("transfer-encoding");
	if (it != _request.headers.end())
		_serverResponse["Transfer-Encoding"] = it->second;
				
	it = _request.headers.find("authorization");
	if (it != _request.headers.end())
		_serverResponse["Authorization"] = it->second;	
		
	it = _request.headers.find("cookie");
	if (it != _request.headers.end())
		_serverResponse["Cookie"] = it->second;
	
	it = _request.headers.find("referer");
	if (it != _request.headers.end())
		_serverResponse["Referer"] = it->second;
	
	it = _request.headers.find("user-agent");
	if (it != _request.headers.end())
		_serverResponse["User-Agent"] = it->second;
	
	it = _request.headers.find("purpose");
	if (it != _request.headers.end())
		_serverResponse["Purpose"] = it->second;
}

void Requests::printRequest() const {
    std::cout << "Request Line: " << _request.method << " " << _request.path << " " << _request.httpVersion << std::endl;
    std::cout << "Headers:" << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = _request.headers.begin(); it != _request.headers.end(); ++it) {
        std::cout << it->first << ": " << it->second << std::endl;
    }
    if (_request.headers.find("body") != _request.headers.end()) {
        std::cout << "Body: " << _request.headers.at("body") << std::endl;
    }
}

void Requests::clientResponse(std::map<std::string, std::string> &response) const {
	
    std::cout << "client Response:" << std::endl;
    for (std::map<std::string, std::string>::const_iterator it = response.begin(); it != response.end(); ++it) {
        std::cout << it->first << ": " << it->second << std::endl;
    }
    if (_request.headers.find("body") != _request.headers.end()) {
        std::cout << "Body: " << _request.headers.at("body") << std::endl;
    }
}

void Requests::printVectorCharDebug(const std::vector<char>& vec) {
    std::cerr << "Vector content with control chars: ";
    for (std::vector<char>::const_iterator it = vec.begin(); it != vec.end(); ++it) {
        unsigned char c = *it;
        if (c < 32 || c > 126) {
            std::cerr << "[" << (int)c << "]";
        } else {
            std::cerr << c;
        }
    }
    std::cerr << std::endl;
}
