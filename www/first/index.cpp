#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <cstdio>

// c++ -Wall -std=c++98 -O2 -o index.cgi index.cpp

#define SESSION_FILE "./resources/sessions.txt"
#define SESSION_TIMEOUT 3600
#define CONTINUE_SESSION true

std::string getSessionToken() {
    const char* cookie = getenv("HTTP_COOKIE");
    if (!cookie) return "";

    std::string cookies(cookie);
    size_t pos = cookies.find("session=");
    if (pos == std::string::npos) return "";

    size_t start = pos + 8;
    size_t end = cookies.find(";", start);

    if (end == std::string::npos)
        end = cookies.find("\r", start);
    
    return cookies.substr(start, end - start);
}


bool isSessionValid(const std::string& token) {
    std::ifstream sessions(SESSION_FILE);
    std::ofstream temp("./resources/sessions.tmp");
    std::string line;
    std::time_t now = std::time(NULL);
    bool valid = false;

    while (std::getline(sessions, line)) {
		size_t p1 = line.find(':');
        size_t p2 = line.find(':', p1 + 1);
		
        if (p1 == std::string::npos || p2 == std::string::npos) continue;
		
        std::string tkn = line.substr(0, p1);
        std::string user = line.substr(p1 + 1, p2 - p1 - 1);
        std::string timestamp_str = line.substr(p2 + 1);
		std::time_t ts = static_cast<std::time_t>(std::strtol(timestamp_str.c_str(), NULL, 10));
		
		//std::cerr << "token1: [" << token << "] size=" << token.size() << "\n";
		//std::cerr << "token2: [" << tkn << "] size=" << tkn.size() << "\n";

        if (now - ts < SESSION_TIMEOUT) {
			if (tkn == token) {
				valid = true;
                if (CONTINUE_SESSION) {
					ts = now;
                }
            }
            temp << tkn << ":" << user << ":" << ts << "\n";
        }
    }
	
    sessions.close();
    temp.close();
	
    std::remove(SESSION_FILE);
    std::rename("./resources/sessions.tmp", SESSION_FILE);
    return valid;
}

void redirectToLogin() {
    
    std::cout << "Status: 302 Found\r\n";
    std::cout << "Location: /login.html\r\n";
    std::cout << "Content-Type: text/html\r\n";
    std::cout << "Content-Length: 0\r\n\r\n";
}

int main() {

	/* std::string purpose = getenv("HTTP_PURPOSE") ? getenv("HTTP_PURPOSE") : "";
	if (purpose == "prefetch") {
		std::cout << "Status: 204 No Content\r\n\r\n";
    	return 0;
	} */
    std::string token = getSessionToken();
    if (!isSessionValid(token)) {
        redirectToLogin();
        return 0;
    }

    std::string msg =
    "<html><head><meta http-equiv=\"refresh\" content=\"2;url=/index.html\"></head>"
    "<body><h1>Welcome, you are logged in!</h1></body></html>";
	std::cout << "Status: 302 Found\r\n";
	std::cout << "Content-Type: text/html\r\n";
	std::cout << "Content-Length: " << msg.size() << "\r\n\r\n";
	std::cout << msg;

    return 0;
}
