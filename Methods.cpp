/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Methods.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aiturria <aiturria@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/13 09:24:59 by aiturria          #+#    #+#             */
/*   Updated: 2025/06/11 15:45:20 by aiturria         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Methods.hpp"

std::map<std::string, std::string> Methods::mime_types;

void Methods::decideMethod(ClientInfo &client) {

	std::map<std::string, std::string>::iterator it = client.response.find("Method");
	if (it->second == "GET" || it->second == "HEAD" || it->second == "POST")
		getMethod(client);
	else if (it->second == "OPTIONS")
		optionsMethod(client);
	else if (it->second == "DELETE")
		deleteMethod(client);
}

#pragma region get method

void Methods::getMethod(ClientInfo &client) {
	std::string folder = lastFolder(client);
	
	int check = locations(folder, client);
	if (check == 0) {
		client.error = 0;
		Response::errorResponse(client);
		return;
	}
	if (check == 3)
		return;
	if (findResource(client) == 0) {
		
		client.error = 0;
		Response::errorResponse(client);
	}
	return;	
}

std::string Methods::lastFolder(ClientInfo &client) {
    std::string folder = client.response.at("Path");
   
    if (folder.size() == 1 && folder[0] == '/')
        return folder;
    
    size_t question = folder.find('?');
    if (question != std::string::npos) {
        client.response["Query"] = folder.substr(question + 1);
        client.response["Path"] = folder.substr(0, question);
        folder = client.response["Path"];
    }
    
    if (folder.length() > 1 && folder[folder.length() - 1] == '/') {
		folder.erase(folder.length() - 1);
	}
    
    size_t last = folder.find_last_of('/');
    if (last == 0) {
        return folder;
    }
    
    size_t previous = folder.find_last_of('/', last - 1);
    if (previous == std::string::npos) {
        return folder.substr(0, last);
    }
    return folder.substr(previous, last - previous);
}

int Methods::locations(std::string folder, ClientInfo &client) {

	std::map<std::string, LocationConfig>::iterator uploads_it = ConfigParser::servers[client.serverNbr].locations.find("/uploads");
	if (uploads_it != ConfigParser::servers[client.serverNbr].locations.end()) {
		if (!uploads_it->second.root.empty())
			client.response["Upload_dir"] = uploads_it->second.root + "/uploads";
		
	}
	
	for (std::map<std::string, LocationConfig>::iterator it = ConfigParser::servers[client.serverNbr].locations.begin();
	it != ConfigParser::servers[client.serverNbr].locations.end(); ++it) {
		if (it->first == folder) {
			LocationConfig location = it->second;
			if (std::find(location.forbiddenMethods.begin(), location.forbiddenMethods.end(), client.response.at("Method")) != location.forbiddenMethods.end()) {
				client.response["statusCode"] = "405 Method Not Allowed";
				return 0;
			}
			if (!location.redirect.empty()) {
				client.response["statusCode"] = "307 Temporary Redirect";
				client.response["Location"]      = location.redirect;
				client.response["Content-Length"]= "0";
				return 3;	
			}
			if (location.autoIndexOn == true)
			client.response["AutoindexOn"] = "true";
			
			if (!location.root.empty()) {
				client.response["Root"] = location.root;
			}
			
			if (!location.try_files.empty()) {
				for ( std::vector<std::string>::iterator it = location.try_files.begin();
				it != location.try_files.end(); ++it) {
					std::string newPath = client.response.at("Root") + folder;
					if (newPath[newPath.size() - 1] != '/')
					newPath += '/';
					newPath += *it;
					std::ifstream file(newPath.c_str());
					if (file.is_open())
						client.response["Path"] = '/' + *it;
				}		
			}
			if (!location.cgi_files.empty()) {
				
				std::string cgi = client.response.at("Path");
				size_t lastslash = cgi.find_last_of('/');
				cgi = cgi.substr(lastslash + 1);
				
				if (cgi.empty() || '/' + cgi == folder) {
					cgi = getFullPath(client, 1);
				}
				if (std::find(location.cgi_files.begin(), location.cgi_files.end(), cgi) != location.cgi_files.end()) {
					client.cgi = true;
					if (client.response["Path"][client.response["Path"].size() - 1] == '/')
						client.response["Path"] += cgi;
					return 3;
				}		
			}
		}
	}
	return 1;
}
	

std::string Methods::getFullPath(ClientInfo &client, int check) {
	
	std::string fullPath;
	
	if (client.response.find("Root") != client.response.end())
		fullPath = client.response.at("Root") + client.response.at("Path");
	else
		fullPath = ConfigParser::servers[client.serverNbr].root + client.response.at("Path");
	struct stat fileInfo;
	if (stat(fullPath.c_str(), &fileInfo) == 0) {
		if (S_ISDIR(fileInfo.st_mode)) {
			if (fullPath[fullPath.size() - 1] != '/')
				fullPath = fullPath + '/';
			for ( std::vector<std::string>::iterator it = ConfigParser::servers[client.serverNbr].index.begin();
				it != ConfigParser::servers[client.serverNbr].index.end(); ++it) {
				std::string newPath = fullPath + *it;
				std::ifstream file(newPath.c_str());
				if (file.is_open()) {
					if (check == 1)
						return *it;
					return newPath;				
				}
			}
		}
	}
	return fullPath;
}

int Methods::findResource(ClientInfo &client) {
	std::string fullPath = getFullPath(client, 0);
	
	struct stat fileInfo;
	if (stat(fullPath.c_str(), &fileInfo) != 0) {
		client.response["statusCode"] = "404 Not Found";
		return 0;
	}
	if (!S_ISREG(fileInfo.st_mode)) {
		if (client.response.find("AutoindexOn") != client.response.end()) {
			Methods::autoindexOn(client, fullPath);
			return 1;
		}
		client.response["statusCode"] = "403 Forbidden";
		return 0;
	}
	if (access(fullPath.c_str(), R_OK ) != 0) {
		client.response["statusCode"] = "403 Forbidden";
		return 0;
	}
	off_t fileSize = fileInfo.st_size;
	if (fileSize > MAX_SIZE) {
		client.response["Path"] = fullPath;
		client.filesize = fileSize;
		client.bigfile = true;
		return 1;
	}
	std::ifstream file(fullPath.c_str(), std::ios::in | std::ios::binary);
	if (!file.is_open()) {
		client.response["statusCode"] = "404 Not Found";
		return 0;
	}	
	std::ostringstream fileContents;
    fileContents << file.rdbuf();
    file.close();
	if (client.response.at("Method") != "HEAD")
		client.response["Body"] = fileContents.str();
	client.response["Content-Length"] = logs.intToString(fileContents.str().size());
	size_t lastDot = fullPath.find_last_of('.');
    std::string extension = fullPath.substr(lastDot + 1);
	if (mime_types.find(extension) != mime_types.end())
		client.response["Content-Type"] = mime_types[extension];
	else
		client.response["Content-Type"] = "Unknown";
	return 1;	
}

static std::string htmlEscape(const std::string& s) {
    std::string out;
    for (std::size_t i = 0; i < s.length(); ++i) {
        switch (s[i]) {
            case '&': out += "&amp;";  break;
            case '<': out += "&lt;";   break;
            case '>': out += "&gt;";   break;
            case '"': out += "&quot;"; break;
            default:  out += s[i];     break;
        }
    }
    return out;
}

void Methods::autoindexOn(ClientInfo &client, std::string &dirPath) {
    std::ostringstream html;
    html << "<!DOCTYPE html>\n<html><head><meta charset=\"utf-8\"><title>Index of "
         << htmlEscape(client.response["Path"])
         << "</title></head><body>\n";
    html << "<h1>Index of " << htmlEscape(client.response["Path"]) << "</h1>\n";
    html << "<ul>\n";

    DIR *d = opendir(dirPath.c_str());
    if (d != NULL) {
        struct dirent *ent;
        while ((ent = readdir(d)) != NULL) {
            std::string name(ent->d_name);
            if (name == "." || name == "..")
                continue;

            std::string href = client.response["Path"];
            if (href.empty() || href[href.length() - 1] != '/')
                href += '/';
            href += name;

            html << "<li><a href=\"" << htmlEscape(href)
                 << "\">" << htmlEscape(name) << "</a></li>\n";
        }
        closedir(d);
    }

    html << "</ul>\n</body></html>\n";

    std::string body = html.str();
    client.response["statusCode"]     = "200 OK";
    client.response["Content-Type"]   = "text/html";
    client.response["Content-Length"] = logs.intToString(body.size());
    client.response["Body"]            = body;
}


void Methods::optionsMethod(ClientInfo &client) {
	std::string folder = lastFolder(client);
	std::vector<std::string> allowedMethods; 
	allowedMethods.push_back("GET");
	allowedMethods.push_back("POST");
	allowedMethods.push_back("DELETE");
	allowedMethods.push_back("HEAD");
	allowedMethods.push_back("OPTIONS");
	for (std::map<std::string, LocationConfig>::iterator it = ConfigParser::servers[client.serverNbr].locations.begin();
		it != ConfigParser::servers[client.serverNbr].locations.end(); ++it) {
		if (it->first == folder) {
			std::vector<std::string> methods = it->second.forbiddenMethods;
			for (std::vector<std::string>::iterator methodIt = methods.begin(); methodIt != methods.end(); ++methodIt) {
                allowedMethods.erase(std::remove(allowedMethods.begin(), allowedMethods.end(),
				*methodIt), allowedMethods.end());
            }
		}
	}
	
    std::string allowHeader;
    for (size_t i = 0; i < allowedMethods.size(); ++i) {
    	allowHeader += allowedMethods[i];
        if (i != allowedMethods.size() - 1)
                allowHeader += ", ";
	}
	client.response["Allow"] = allowHeader;
    client.response["statusCode"] = "204 No Content";

	client.response["Access-Control-Allow-Origin"] = "*";
    client.response["Access-Control-Allow-Methods"] = allowHeader;
    client.response["Access-Control-Allow-Headers"] = "Content-Type";
    client.response["Access-Control-Max-Age"] = "86400";
}

#pragma endregion

#pragma region cgi method
int Methods::handlecgi(ClientInfo &client) {
	
	std::string fullPath;
	if (client.response.find("Root") != client.response.end()) {
		fullPath = client.response.at("Root") + client.response.at("Path");
		client.response["fullPath"] = fullPath;
	}
	
	if (access(fullPath.c_str(), X_OK) == -1){
		client.response["statusCode"] = "403 Forbidden";
		client.error = 0;
		return 0;
	}
	ClientInfo cgi = client;
	cgi.type = CGI;
	cgi.myclient = &client;
	
	if (client.response.at("Method") == "GET") {
		if (getCgi(cgi) == 0) {
			client.error = 0;
			return 0;
		}
		Sockets::modifyEpoll(client.client_fd, EPOLL_CTL_MOD, EPOLLOUT | EPOLLIN);
		
	}		
	else if (client.response.at("Method") == "POST") {
		if (postCgi(cgi) == 0) {
			client.error = 0;
			return 0;
		}
		if (client.response.find("Body-Length") != client.response.end()) {
			if (static_cast<int>(client.request.size()) == std::atoi(client.response.at("Body-Length").c_str())) {
				Sockets::modifyEpoll(client.client_fd, EPOLL_CTL_MOD, EPOLLOUT | EPOLLIN);
				
			}
		}
	}
	return 1;	
}

int Methods::getCgi(ClientInfo &cgi) {
	
	int pipeFD[2];
	if (pipe(pipeFD) == -1) {
		cgi.myclient->response["statusCode"] = "500 Internal Server Error";
		return 0;
	}
	cgi.myclient->mycgi = fork();
	if (cgi.myclient->mycgi == -1) {
		close(pipeFD[0]);
		close(pipeFD[1]);
		cgi.myclient->response["statusCode"] = "500 Internal Server Error";
		return 0;
	}
	else if (cgi.myclient->mycgi == 0) {
		setCgiEnvironment(cgi);

		size_t slash = cgi.response.at("fullPath").find_last_of("/");
		std::string dir = cgi.response.at("fullPath").substr(0, slash);
		std::string file = cgi.response.at("fullPath").substr(slash + 1);
		chdir(dir.c_str());
		
		//std::cerr << "Current working directory: " << getcwd(NULL, 0) << std::endl;
		dup2(pipeFD[1], STDOUT_FILENO);
		close(pipeFD[0]);
		close(pipeFD[1]);

		char* argv[] = { (char*)file.c_str(), NULL };
		extern char **environ;
		execve(file.c_str(), argv, environ);
		logs.errorLog("GET child exec failed");
		exit(1);
	}
	else { 
		close(pipeFD[1]);
		Sockets::modifyEpoll(pipeFD[0], EPOLL_CTL_ADD, EPOLLIN | EPOLLRDHUP);
		logs.accessLog("Add client get cgi on FD " + logs.intToString(pipeFD[0]));
		cgi.cgiOutputFD = pipeFD[0];
		cgi.myclient->cgiOutputFD = pipeFD[0];
		Sockets::_clients[pipeFD[0]] = cgi;
	}
	return 1;	
}

int Methods::postCgi(ClientInfo &cgi) {

	int inputFD[2], outputFD[2];

	if (pipe(inputFD) == -1 || pipe(outputFD) == -1) {
		cgi.myclient->response["statusCode"] = "500 Internal Server Error";
		return 0;
	}
	
	cgi.myclient->mycgi = fork();
	if (cgi.myclient->mycgi == -1) {
		close(inputFD[0]);
		close(inputFD[1]);
		close(outputFD[0]);
		close(outputFD[1]);
		cgi.myclient->response["statusCode"] = "500 Internal Server Error";
		return 0;
	}	
	else if (cgi.myclient->mycgi == 0) {
		setCgiEnvironment(cgi);
		
		size_t slash = cgi.response.at("fullPath").find_last_of("/");
		std::string dir = cgi.response.at("fullPath").substr(0, slash);
		std::string file = cgi.response.at("fullPath").substr(slash + 1);
		chdir(dir.c_str());
		
		//std::cerr << "Current working directory: " << getcwd(NULL, 0) << std::endl;
		
		dup2(inputFD[0], STDIN_FILENO);
		dup2(outputFD[1], STDOUT_FILENO);
		close(inputFD[0]);
		close(inputFD[1]);
		close(outputFD[0]);
		close(outputFD[1]);
		
		char* argv[] = { (char*)file.c_str(), NULL };
		extern char **environ;
		execve(file.c_str(), argv, environ);
		logs.errorLog("POST child exec failed");
		exit(1);
	}
	else { 
		close(inputFD[0]);
		close(outputFD[1]);
		cgi.cgiInputFD = inputFD[1];
		cgi.cgiOutputFD = outputFD[0];
		cgi.myclient->cgiInputFD = inputFD[1];
		cgi.myclient->cgiOutputFD = outputFD[0];
		Sockets::modifyEpoll(inputFD[1], EPOLL_CTL_ADD, EPOLLOUT);
		logs.accessLog("Add client reading cgi on FD " + logs.intToString(cgi.cgiInputFD));
		Sockets::_clients[inputFD[1]] = cgi;
	}
	return 1;	
}

void Methods::writeToCgi(ClientInfo &cgi) {
	
	cgi.last_activity = time(NULL);
	size_t responseSize = cgi.myclient->request.size();
	
	const char *responseData = cgi.myclient->request.data();
	size_t toSend = responseSize > CHUNK ? CHUNK : responseSize;
	
	ssize_t bytesWritten = 0;
	while (toSend > 0) {
    	ssize_t written = write(cgi.cgiInputFD, responseData, toSend);
   		if (written == -1) {
        	cgi.myclient->response["statusCode"] = "500 Internal Server Error";
			cgi.myclient->error = 0;
        	Sockets::removeClient(cgi.cgiInputFD);
        	return;
    	}
		else if (written >= 0) {
			bytesWritten += written;
			toSend -= written;
		}
	}
	cgi.bytesSent += bytesWritten;
	cgi.myclient->request.erase(cgi.myclient->request.begin(), cgi.myclient->request.begin() + bytesWritten);
	
	if (cgi.bytesSent >= std::atoi(cgi.response.at("Body-Length").c_str())) {
		cgi.bytesSent = 0;
		Sockets::_clients[cgi.cgiOutputFD] = cgi;
        Sockets::modifyEpoll(cgi.cgiOutputFD, EPOLL_CTL_ADD, EPOLLIN | EPOLLRDHUP);
		logs.accessLog("Add client writing cgi on FD " + logs.intToString(cgi.cgiOutputFD));
		Sockets::removeClient(cgi.cgiInputFD);
    }
}

void Methods::readFromCgi(ClientInfo &cgi) {
	
	cgi.last_activity = time(NULL);
	char buffer[READ];
	ssize_t bytes = read(cgi.cgiOutputFD, buffer, sizeof(buffer) - 1);
    if (bytes < 0) {
		cgi.myclient->response["statusCode"] = "500 Internal Server Error";
		cgi.myclient->error = 0;
        Sockets::removeClient(cgi.cgiOutputFD);
        return;
	}
	else if (bytes >= 0) {
		if (cgi.bytesSent == 0) {
			cgi.myclient->finalResponse += "HTTP/1.1 ";
			cgi.bytesSent += 9;
		}
		buffer[bytes] = '\0';
		cgi.myclient->finalResponse += buffer;
	
		if (cgi.bytesSent == 9) {
			size_t status = cgi.myclient->finalResponse.find("Status: ");
			if (status != std::string::npos) {
				size_t stat_start = status + 8;
				size_t stat_end = cgi.myclient->finalResponse.find("\r\n", stat_start);
				if (stat_end != std::string::npos) {
					std::string statusCode = cgi.myclient->finalResponse.substr(stat_start, stat_end - stat_start);
					cgi.myclient->response["statusCode"] = statusCode;
					if (statusCode[0] == '4' || statusCode[0] == '5')
						cgi.myclient->error = 0;
					cgi.myclient->finalResponse.erase(status, 8);
					
				}
			}

		}
		cgi.bytesSent += bytes;	
	}
	//std::cerr << "final response " << cgi.myclient->finalResponse << "\n";

	if (cgi.myclient->filesize == 0 && cgi.myclient->error > 0) {
		size_t header_end = cgi.myclient->finalResponse.find("\r\n\r\n");
		if (header_end != std::string::npos) {
			std::istringstream stream(cgi.myclient->finalResponse.substr(0, header_end));
			std::string line;
			int contentLength = 0;
			while (std::getline(stream, line)) {
				for (size_t i = 0; i < line.size(); ++i)
					line[i] = std::tolower(line[i]);
				if (line.find("content-length:") != std::string::npos) {
					size_t pos = line.find(":");
					if (pos != std::string::npos) {
						contentLength = std::atoi(line.substr(pos + 1).c_str());
						break;
					}
				}
			}
			if (cgi.response.at("Method") == "HEAD") {
				cgi.myclient->finalResponse = cgi.myclient->finalResponse.substr(0, header_end + 4);
				cgi.myclient->filesize = header_end + 4;
			}
			else if (contentLength == 0) {
				cgi.myclient->finalResponse.insert(header_end, "Transfer-Encoding: chunked");
				cgi.myclient->chunked = true;
			}
			else
				cgi.myclient->filesize = header_end + 4 + contentLength;
		}
	}
	
	if ((cgi.myclient->filesize > 0 && cgi.bytesSent >= cgi.myclient->filesize) || bytes == 0) {
		if (cgi.myclient->chunked == true)
			cgi.myclient->filesize = cgi.bytesSent;
		cgi.myclient->cgiOutputFD = -1;
		Sockets::removeClient(cgi.cgiOutputFD);
       
    }
}
	
void Methods::setCgiEnvironment(ClientInfo &client) {
	
    Methods::lastFolder(client);
	
    setenv("REQUEST_METHOD", client.response["Method"].c_str(), 1);
    setenv("QUERY_STRING", client.response["Query"].c_str(), 1);
    setenv("CONTENT_TYPE", client.response["Content-Type"].c_str(), 1);
    setenv("CONTENT_LENGTH", client.response["Body-Length"].c_str(), 1);
    setenv("PATH_INFO", client.response["Path"].c_str(), 1);
	std::string pathTranslated = client.response["Root"] + client.response["Path"];
	setenv("PATH_TRANSLATED", pathTranslated.c_str(), 1);
    setenv("SCRIPT_NAME", client.response["Path"].c_str(), 1);
    setenv("SERVER_NAME", ConfigParser::servers[client.serverNbr].server_names[0].c_str(), 1);
    setenv("SERVER_PORT", client.server_port.c_str(), 1);
    setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
    setenv("SERVER_SOFTWARE", "ITUBOLA/2.0", 1);
    setenv("REMOTE_ADDR", client.client_ip.c_str(), 1);
    setenv("REMOTE_PORT", client.client_port.c_str(), 1);
	if (!ConfigParser::servers[client.serverNbr].password.empty())
		setenv("ADMIN_PASSWORD", ConfigParser::servers[client.serverNbr].password.c_str(), 1);
    if (client.response.find("Host") != client.response.end())
        setenv("HTTP_HOST", client.response["Host"].c_str(), 1);
    if (client.response.find("User-Agent") != client.response.end())
        setenv("HTTP_USER_AGENT", client.response["User-Agent"].c_str(), 1);
    if (client.response.find("Accept") != client.response.end())
        setenv("HTTP_ACCEPT", client.response["Accept"].c_str(), 1);
    if (client.response.find("Cookie") != client.response.end())
		setenv("HTTP_COOKIE", client.response["Cookie"].c_str(), 1);
	if (client.response.find("Referer") != client.response.end())
        setenv("HTTP_REFERER", client.response["Referer"].c_str(), 1);
	if (client.response.find("Upload_dir") != client.response.end())
        setenv("UPLOAD_DIR", client.response["Upload_dir"].c_str(), 1);
	if (client.response.find("Purpose") != client.response.end())
        setenv("HTTP_PURPOSE", client.response["Purpose"].c_str(), 1);
}

void Methods::deleteMethod(ClientInfo &client) {
	
	std::string folder = lastFolder(client);
	int check = locations(folder, client);
	if (check == 0) {
		Response::errorResponse(client);
		client.error = 0;
		return;
	}
	std::string fullPath;
	if (client.response.find("Root") != client.response.end()) {
		fullPath = client.response.at("Root") + "/cgi-bin/delete.cgi";
		client.response["fullPath"] = fullPath;
	}
	if (access(fullPath.c_str(), X_OK) == -1){
		client.response["statusCode"] = "403 Forbidden";
		client.error = 0;
		return;
	}
	client.cgi = true;
	ClientInfo cgi = client;
	cgi.type = CGI;
	cgi.myclient = &client;

	getCgi(cgi);
	Sockets::modifyEpoll(client.client_fd, EPOLL_CTL_MOD, EPOLLOUT);	
}


#pragma endregion

#pragma region data method
void Methods::bigFiles(ClientInfo &client) { 
	
	int pipefd[2];
	pipe(pipefd);
	fcntl(pipefd[0], F_SETFL, fcntl(pipefd[0], F_GETFL, 0) | O_NONBLOCK);
	fcntl(pipefd[1], F_SETFL, fcntl(pipefd[1], F_GETFL, 0) | O_NONBLOCK);

	client.response["Content-Length"] = logs.intToString(client.filesize);
	size_t lastDot = client.response.at("Path").find_last_of('.');
	std::string extension = client.response.at("Path").substr(lastDot + 1);
	if (mime_types.find(extension) != mime_types.end())
		client.response["Content-Type"] = mime_types[extension];
	else
		client.response["Content-Type"] = "Unknown";
	
	client.fileFD = open(client.response.at("Path").c_str(), O_RDONLY | O_NONBLOCK);
	if (client.fileFD == -1) {
		logs.errorLog("File at FD " + logs.intToString(client.fileFD) + "failed to open");
		client.response["statusCode"] = "500 Internal Server Error";
		client.error = 0;
		return;
	}
	
	client.finalResponse = Response::buildResponse(client);
	if (client.response.at("Method") == "HEAD") {
		client.bigfile = false;
		return;
	}
	client.cgiOutputFD = pipefd[1];
	client.cgiInputFD = pipefd[0];
	
	ClientInfo data = client;
	data.myclient = &client;
	data.type = DATA;

	ClientInfo client2 = client;
	client2.myclient = &client;
	client2.type = DATA;
	
	Sockets::modifyEpoll(client.cgiOutputFD, EPOLL_CTL_ADD, EPOLLOUT | EPOLLRDHUP);
	logs.accessLog("Add data client reading on FD " + logs.intToString(data.cgiOutputFD));
	Sockets::_clients[client.cgiOutputFD] = data;
	
	Sockets::modifyEpoll(client2.cgiInputFD, EPOLL_CTL_ADD, EPOLLIN | EPOLLRDHUP);
	logs.accessLog("Add data client writing on FD " + logs.intToString(client2.cgiInputFD));
	Sockets::_clients[client2.cgiInputFD] = client2;
	client.bigfile = false;
}

void Methods::readFileToPipe(ClientInfo &data) {

	if (!data.finalResponse.empty()) {
	
		size_t toSend = data.finalResponse.size();
		const char *responseData = data.finalResponse.c_str();
		ssize_t sent = send(data.myclient->client_fd, responseData, toSend, 0);
		if (sent == -1) { 
			logs.errorLog("Falied to send data to FD: " + logs.intToString(data.myclient->client_fd) + " Disconecting client");
			Sockets::removeClient(data.myclient->client_fd);
			Sockets::removeClient(data.cgiOutputFD);
				return;
		}
		if (sent >= 0)
			data.finalResponse.clear();
	}
    ssize_t bytes = splice(data.fileFD, NULL, data.cgiOutputFD, NULL, READ, SPLICE_F_MORE | SPLICE_F_MOVE);
  	if (bytes > 0) 
		data.bytesSent += bytes;
	
    if (bytes == 0 || data.bytesSent >= data.filesize) {
        close(data.fileFD);
        data.myclient->fileFD = -1;
		data.myclient->cgiOutputFD = -1;
		Sockets::removeClient(data.cgiOutputFD);
    } else if (bytes < 0) {
		logs.errorLog("Splice at file FD " + logs.intToString(data.fileFD) + " failed");
		data.myclient->response["statusCode"] = "500 Internal Server Error";
		data.myclient->error = 0;
		close(data.fileFD);
		data.myclient->cgiOutputFD = -1;
        Sockets::removeClient(data.cgiOutputFD);
	}
}
void Methods::pipeToClient(ClientInfo &client) {
   
    ssize_t bytes = splice(client.cgiInputFD, NULL, client.client_fd, NULL, READ, SPLICE_F_MORE | SPLICE_F_MOVE);
	if (bytes > 0)
		client.bytesSent += bytes;
	
	if (bytes == 0 || client.bytesSent >= client.filesize) {
		logs.accessLog("Splice Response to " + logs.intToString(client.client_fd) + " " + client.response.at("statusCode")
		+ " sent " + logs.intToString(client.bytesSent) + " bytes ");
		
		client.myclient->cgiInputFD = -1;
		Sockets::removeClient(client.cgiInputFD);
		
	} else if (client.myclient->fileFD == -1 && (bytes < 0 || client.error == 0)) {
			
		   logs.errorLog("Splice at file FD " + logs.intToString(client.fileFD) + " failed");
		   client.myclient->response["statusCode"] = "500 Internal Server Error";
		   client.myclient->error = 0;
		   client.myclient->cgiInputFD = -1;
		   Sockets::removeClient(client.cgiInputFD);
    }
}

#pragma endregion

void Methods::mimeTypes() {
	mime_types["html"]		= "text/html";
	mime_types["htm"]		= "text/html";
	mime_types["shtml"]		= "text/html";
	mime_types["css"]		= "text/css";
	mime_types["xml"]		= "text/xml";
	mime_types["gif"]		= "image/gif";
	mime_types["jpeg"]		= "image/jpeg";
	mime_types["jpg"]		= "image/jpeg";
	mime_types["js"]		= "application/javascript";
	mime_types["atom"]		= "application/atom+xml";
	mime_types["rss"]		= "application/rss+xml";
	mime_types["mml"]		= "text/mathml";
	mime_types["txt"]		= "text/plain";
	mime_types["ini"]		= "text/plain";
	mime_types["cfg"]		= "text/plain";
	mime_types["jad"]		= "text/vnd.sun.j2me.app-descriptor";
	mime_types["wml"]		= "text/vnd.wap.wml";
	mime_types["htc"]		= "text/x-component";
	mime_types["png"]		= "image/png";
	mime_types["tif"]		= "image/tiff";
	mime_types["tiff"]		= "image/tiff";
	mime_types["wbmp"]		= "image/vnd.wap.wbmp";
	mime_types["ico"]		= "image/x-icon";
	mime_types["jng"]		= "image/x-jng";
	mime_types["bmp"]		= "image/x-ms-bmp";
	mime_types["svg"]		= "image/svg+xml";
	mime_types["svgz"]		= "image/svg+xml";
	mime_types["webp"]		= "image/webp";
	mime_types["woff"]		= "application/font-woff";
	mime_types["jar"]		= "application/java-archive";
	mime_types["war"]		= "application/java-archive";
	mime_types["ear"]		= "application/java-archive";
	mime_types["json"]		= "application/json";
	mime_types["hqx"]		= "application/mac-binhex40";
	mime_types["doc"]		= "application/msword";
	mime_types["pdf"]		= "application/pdf";
	mime_types["ps"]		= "application/postscript";
	mime_types["eps"]		= "application/postscript";
	mime_types["ai"]		= "application/postscript";
	mime_types["rtf"]		= "application/rtf";
	mime_types["m3u8"]		= "application/vnd.apple.mpegurl";
	mime_types["xls"]		= "application/vnd.ms-excel";
	mime_types["eot"]		= "application/vnd.ms-fontobject";
	mime_types["ppt"]		= "application/vnd.ms-powerpoint";
	mime_types["wmlc"]		= "application/vnd.wap.wmlc";
	mime_types["kml"]		= "application/vnd.google-earth.kml+xml";
	mime_types["kmz"]		= "application/vnd.google-earth.kmz";
	mime_types["7z"]		= "application/x-7z-compressed";
	mime_types["cco"]		= "application/x-cocoa";
	mime_types["jardiff"]	= "application/x-java-archive-diff";
	mime_types["jnlp"]		= "application/x-java-jnlp-file";
	mime_types["run"]		= "application/x-makeself";
	mime_types["pl"]		= "application/x-perl";
	mime_types["pm"]		= "application/x-perl";
	mime_types["prc"]		= "application/x-pilot";
	mime_types["pdb"]		= "application/x-pilot";
	mime_types["rar"]		= "application/x-rar-compressed";
	mime_types["rpm"]		= "application/x-redhat-package-manager";
	mime_types["sea"]		= "application/x-sea";
	mime_types["swf"]		= "application/x-shockwave-flash";
	mime_types["sit"]		= "application/x-stuffit";
	mime_types["tcl"]		= "application/x-tcl";
	mime_types["tk"]		= "application/x-tcl";
	mime_types["der"]		= "application/x-x509-ca-cert";
	mime_types["pem"]		= "application/x-x509-ca-cert";
	mime_types["crt"]		= "application/x-x509-ca-cert";
	mime_types["xpi"]		= "application/x-xpinstall";
	mime_types["xhtml"]		= "application/xhtml+xml";
	mime_types["xspf"]		= "application/xspf+xml";
	mime_types["zip"]		= "application/zip";
	mime_types["bin"]		= "application/octet-stream";
	mime_types["exe"]		= "application/octet-stream";
	mime_types["dll"]		= "application/octet-stream";
	mime_types["deb"]		= "application/octet-stream";
	mime_types["dmg"]		= "application/octet-stream";
	mime_types["iso"]		= "application/octet-stream";
	mime_types["img"]		= "application/octet-stream";
	mime_types["msi"]		= "application/octet-stream";
	mime_types["msp"]		= "application/octet-stream";
	mime_types["msm"]		= "application/octet-stream";
	mime_types["docx"]		= "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
	mime_types["xlsx"]		= "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
	mime_types["pptx"]		= "application/vnd.openxmlformats-officedocument.presentationml.presentation";
	mime_types["mid"]		= "audio/midi";
	mime_types["midi"]		= "audio/midi";
	mime_types["kar"]		= "audio/midi";
	mime_types["mp3"]		= "audio/mpeg";
	mime_types["ogg"]		= "audio/ogg";
	mime_types["m4a"]		= "audio/x-m4a";
	mime_types["ra"]		= "audio/x-realaudio";
	mime_types["3gpp"]		= "video/3gpp";
	mime_types["3gp"]		= "video/3gpp";
	mime_types["ts"]		= "video/mp2t";
	mime_types["mp4"]		= "video/mp4";
	mime_types["mpeg"]		= "video/mpeg";
	mime_types["mpg"]		= "video/mpeg";
	mime_types["mov"]		= "video/quicktime";
	mime_types["webm"]		= "video/webm";
	mime_types["flv"]		= "video/x-flv";
	mime_types["m4v"]		= "video/x-m4v";
	mime_types["mng"]		= "video/x-mng";
	mime_types["asx"]		= "video/x-ms-asf";
	mime_types["asf"]		= "video/x-ms-asf";
	mime_types["wmv"]		= "video/x-ms-wmv";
	mime_types["avi"]		= "video/x-msvideo";
}


			