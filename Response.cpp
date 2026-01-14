/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aiturria <aiturria@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/25 16:06:00 by aiturria          #+#    #+#             */
/*   Updated: 2025/06/10 14:55:49 by aiturria         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Response.hpp"

void Response::errorResponse(ClientInfo &client) {
	std::string html;
	std::string key = client.response["statusCode"].substr(0, 3);
	int nbr = client.serverNbr;
	std::map<std::string, std::string>::iterator it =  ConfigParser::servers[nbr].error_pages.find(key);
	if (it != ConfigParser::servers[nbr].error_pages.end()) {
		std::string path = ConfigParser::servers[nbr].root + it->second;
		std::ifstream file(path.c_str());
        if (file) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            html = buffer.str();
		}
	}
	else {
		html = errortemplate;
		
		std::string statusCode = client.response["statusCode"];
		char firstDigit = statusCode.at(0);
		char thirdDigit = statusCode.at(2);
		std::string rest = statusCode.substr(4);
		
		size_t pos;
		pos = html.find("{{left_number}}");
		if (pos != std::string::npos) {
			html.replace(pos, 15, std::string(1, firstDigit));
		}
		pos = html.find("{{right_number}}");
		if (pos != std::string::npos) {
			html.replace(pos, 16, std::string(1, thirdDigit));
		}
		pos = html.find("{{subtitle}}");
		if (pos != std::string::npos) {
			html.replace(pos, 12, rest);
		}
		
		std::string link = "<link rel=\"stylesheet\" href=\"houston.css\" />";
		pos = html.find(link);
		if (pos != std::string::npos)
			html.replace(pos, link.size(), "<style>" + errorstyles + "</style>");	
	}
	if (client.response.find("Method") != client.response.end() && client.response.at("Method") != "HEAD")
    	client.response["Body"] = html;
    client.response["Content-Type"] = "text/html";
    client.response["Content-Length"] = logs.intToString(html.size());
	client.response["X-Content-Type-Options"] = "nosniff";
	client.response["Cache-Control"] = "no-store";
}


std::string Response::buildResponse(ClientInfo &client) {
	std::string httpResponse;
	httpResponse += "HTTP/1.1 " + client.response["statusCode"] + "\r\n";
	if (client.response.find("Content-Type") != client.response.end())
		httpResponse += "Content-Type: " + client.response["Content-Type"] + "\r\n";
		
	if (client.response.find("Content-Length") != client.response.end())
		httpResponse += "Content-length: " + client.response["Content-Length"] + "\r\n";
	else {
		httpResponse += "Transfer-Encoding: chunked\r\n";
		client.cgiOutputFD = -5;	
	}
	httpResponse += "Server: ITUBOLA/2.0\r\n";
	httpResponse += "Date: " + logs.getCurrentTime(1) + " GMT\r\n";
	httpResponse += "Connection: " + client.response["Connection"] + "\r\n";
	if (client.response.find("Location") != client.response.end()) {
		httpResponse += "Location: " + client.response["Location"] + "\r\n\r\n";
		return httpResponse;
	}
	if (client.response.find("Set-Cookie") != client.response.end())
		httpResponse += "Set-Cookie: " + client.response["Set-Cookie"] + "\r\n";
	if (client.response.find("Allow") != client.response.end())
		httpResponse += "Allow: " + client.response["Allow"] + "\r\n";
	if (client.response.find("Access-Control-Allow-Origin") != client.response.end())
		httpResponse += "Access-Control-Allow-Origin: " + client.response["Access-Control-Allow-Origin"] + "\r\n";
	if (client.response.find("Access-Control-Allow-Methods") != client.response.end())
		httpResponse += "Access-Control-Allow-Methods: " + client.response["Access-Control-Allow-Methods"] + "\r\n";
	if (client.response.find("Access-Control-Allow-Headers") != client.response.end())
		httpResponse += "Access-Control-Allow-Headers: " + client.response["Access-Control-Allow-Headers"] + "\r\n";
	if (client.response.find("Access-Control-Max-Age") != client.response.end())
		httpResponse += "Access-Control-Max-Age: " + client.response["Access-Control-Max-Age"] + "\r\n";
	if (client.response.find("X-Content-Type-Options") != client.response.end())
		httpResponse += "X-Content-Type-Options: " + client.response["X-Content-Type-Options"] + "\r\n";
	if (client.response.find("Cache-Control") != client.response.end())
		httpResponse += "Cache-Control: " + client.response["Cache-Control"] + "\r\n";
	httpResponse += "\r\n";
	if (client.response.find("Body") != client.response.end())
		httpResponse += client.response["Body"] + "\r\n";
	
	/* std::string debugResponse = httpResponse;
	std::string visibleCRLF = "";
	for (size_t i = 0; i < debugResponse.length(); ++i) {
		if (debugResponse[i] == '\r') visibleCRLF += "\\r";
		else if (debugResponse[i] == '\n') visibleCRLF += "\\n";
		else visibleCRLF += debugResponse[i];
	}
	std::cerr << "HTTP response with visible line endings:\n" << visibleCRLF << std::endl; */
	//std::cerr << "http response: " << httpResponse << "\n";
	client.error = 1;
	return httpResponse;
}

void Response::sendData(ClientInfo &client) {
	
	if (client.error == 0) {
		errorResponse(client);
		client.finalResponse = buildResponse(client);
	}
	client.last_activity = time(NULL);
	size_t responseSize = client.finalResponse.size();
	if (client.finalResponse.empty() && client.cgi == false) {
		Sockets::modifyEpoll(client.client_fd, EPOLL_CTL_MOD, EPOLLIN | EPOLLRDHUP);
		return;
	}
	else if (client.finalResponse.empty() && client.cgi == true)
		return;
    const char *responseData = client.finalResponse.c_str();
	size_t toSend = responseSize > CHUNK ? CHUNK : responseSize;
	ssize_t sent = send(client.client_fd, responseData, toSend, 0);
    if (sent == -1) { 
        logs.errorLog("Falied to send data to FD: " + logs.intToString(client.client_fd) + " Disconecting client");
        Sockets::removeClient(client.client_fd);
            return;
    }
	if (sent == 0) {
		logs.errorLog("Send returned 0 on FD: " + logs.intToString(client.client_fd) + "Disconnecting client");
		Sockets::removeClient(client.client_fd);
		return;
	}

	client.finalResponse = client.finalResponse.substr(sent);
	client.bytesSent += sent;
	if (client.cgi == true && client.bytesSent < client.filesize)
		return;
	if (client.finalResponse.empty()) {
		logs.accessLog("Response to " + logs.intToString(client.client_fd) + " " + client.response.at("statusCode")
		+ " sent " + logs.intToString(client.bytesSent) + " bytes ");
		if (client.cgiOutputFD > 0 && client.mycgi != 0)
			Sockets::removeClient(client.cgiOutputFD);
		if (client.response.find("Connection") != client.response.end() &&
			client.response["Connection"] == "close") {
			logs.accessLog("Connection closed on FD " + logs.intToString(client.client_fd));
			Sockets::removeClient(client.client_fd);
		} else {
			Sockets::modifyEpoll(client.client_fd, EPOLL_CTL_MOD, EPOLLIN | EPOLLRDHUP);
		}
		client.bytesSent = 0;
		client.error = 1;
		client.cgi = false;
		client.mycgi = 0;
		client.cgiInputFD = -1;
		client.cgiOutputFD = -1;
		client.filesize = 0;
		client.request.clear();
	}	
}

void Response::sendChunkedData(ClientInfo &client) {
	
	if (client.error == 0) {
		errorResponse(client);
		client.finalResponse = buildResponse(client);
		client.chunked = false;
		return;
	}
	client.last_activity = time(NULL);
	if (client.finalResponse.empty() && client.cgi == false) {
		Sockets::modifyEpoll(client.client_fd, EPOLL_CTL_MOD, EPOLLIN | EPOLLRDHUP);
        return;
    }
	size_t responseSize = client.finalResponse.size();
    size_t ChunkSize = responseSize > CHUNK ? CHUNK : responseSize;
		
    std::stringstream chunkSizeStream;
    chunkSizeStream << std::hex << ChunkSize;
    std::string chunkHeader = chunkSizeStream.str();        
    std::string chunkedResponse = chunkHeader + client.finalResponse.substr(0, ChunkSize - chunkHeader.size()) + "\r\n";
            
	ssize_t sent = send(client.client_fd, chunkedResponse.c_str(), chunkedResponse.size(), 0);
    if (sent == -1) { 
        logs.errorLog("Falied to send data to FD: " + logs.intToString(client.client_fd) + " Disconnecting client");
        Sockets::removeClient(client.client_fd);
		return; 
	}
	 if (sent == 0) {
		logs.errorLog("Send returned 0 on FD: " + logs.intToString(client.client_fd) + "Disconnecting client");
		Sockets::removeClient(client.client_fd);
		return;
	}
	
	client.finalResponse = client.finalResponse.substr(sent - chunkHeader.size());
	client.bytesSent += sent - chunkHeader.size();
	
	if (client.finalResponse.empty() && (client.cgi == false || (client.bytesSent >= client.filesize))) {
		std::string endChunk = "0\r\n\r\n";
		send(client.client_fd, endChunk.c_str(),endChunk.size() , 0);
		
		logs.accessLog("Chunked response to " + logs.intToString(client.client_fd) + " " + client.response.at("statusCode")
		+ " sent " + logs.intToString(client.bytesSent + chunkHeader.size()) + " bytes ");
		if (client.cgiOutputFD > 0)
			Sockets::removeClient(client.cgiOutputFD);
		if (client.response.find("Connection") != client.response.end() && client.response["Connection"] == "close") {
			
			logs.accessLog("Connection closed on FD " + logs.intToString(client.client_fd));
			Sockets::removeClient(client.client_fd);
		} else {
			Sockets::modifyEpoll(client.client_fd, EPOLL_CTL_MOD, EPOLLIN | EPOLLRDHUP);
			
			client.bytesSent = 0;
			client.error = 1;
			client.cgi = false;
			client.mycgi = 0;
			client.cgiInputFD = -1;
			client.cgiOutputFD = -1;
			client.filesize = 0;
			client.request.clear();
		}
	
	}
	  
}
        

std::string errortemplate =
"	<!DOCTYPE html>\n"
"<html lang=\"en\">\n\n"

"<head>\n"
"  <meta charset=\"UTF-8\" />\n"
"  <title>An Error Has Occurred</title>\n"
"  <link rel=\"stylesheet\" href=\"houston.css\" />\n"
"</head>\n\n"

"<body>\n"
"  <div class=\"container\">\n"
"    <div class=\"main\">\n"
"      <span class=\"number left\">{{left_number}}</span>\n"
"      <img src=\"/error_pages/astronaut.png\" alt=\"Astronaut\" class=\"astronaut\" />\n"
"      <span class=\"number right\">{{right_number}}</span>\n"
"    </div>\n"
"    <h1 class=\"subTitle\">\n"
"      {{subtitle}}\n"
"    </h1>\n"
"    <h1 class=\"MainTitle\">\n"
"      Houston, we have a problem.\n"
"    </h1>\n"
"  </div>\n"
"</body>\n\n"

"</html>\n";

std::string errorstyles =
"body {\n"
"    font-family: Arial, sans-serif;\n"
"    background-color: #f0f0f0;\n"
"    color: #333;\n"
"    text-align: center;\n"
"    margin: 0;\n"
"    padding: 0;\n"
"}\n"
"\n"
".container {\n"
"    display: flex;\n"
"    flex-direction: column;\n"
"    align-items: center;\n"
"    justify-content: center;\n"
"    height: 100vh;\n"
"}\n"
"\n"
".main {\n"
"    position: relative;\n"
"    display: flex;\n"
"    align-items: center;\n"
"    justify-content: center;\n"
"    margin-bottom: 20px;\n"
"}\n"
"\n"
".astronaut {\n"
"    width: 500px;\n"
"    height: 400px;\n"
"    margin: 0 0px;\n"
"}\n"
"\n"
".number {\n"
"    position: absolute;\n"
"    font-size: 400px;\n"
"    font-weight: bold;\n"
"    color: rgba(0, 0, 0, 0.6);\n"
"}\n"
"\n"
".left {\n"
"    left: 10%;\n"
"    top: 30%;\n"
"    transform: translate(-50%, -50%);\n"
"}\n"
"\n"
".right {\n"
"    right: 10%;\n"
"    top: 30%;\n"
"    transform: translate(50%, -50%);\n"
"}\n"
"\n"
".MainTitle {\n"
"    font-size: 36px;\n"
"    font-weight: bold;\n"
"}\n"
"\n"
".subTitle {\n"
"    font-size: 24px;\n"
"    font-weight: bold;\n"
"}\n";





