#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <fstream>
#include <ctime>

//g++ -Wall -std=c++98 -O2 -o login.cgi login.cpp

#define USERS_FILE "../resources/users.txt"
#define SESSION_FILE "../resources/sessions.txt"

std::string generateSessionToken() {
    std::stringstream ss;
    std::srand(std::time(NULL));
    for (int i = 0; i < 16; ++i) {
        int r = std::rand() % 36;
        ss << (r < 10 ? ('0' + r) : ('a' + r - 10));
    }
    return ss.str();
}

void printHeaders(size_t length, const std::string& status) {
    std::cout << status << "\r\n";
    std::cout << "Content-Type: text/html\r\n";
    std::cout << "Content-Length: " << length << "\r\n\r\n";
}


int main() {

	char* Query = getenv("QUERY_STRING");
	if (!Query) {
    	printHeaders(0, "Status: 400 Bad Request");
    	return 1;
	}
	std::string type(Query);
	size_t pos = type.find("=");
	if (pos != std::string::npos)
		type = type.substr(pos + 1);


	const char* len = getenv("CONTENT_LENGTH");
    int length = len ? std::atoi(len) : 0;
	std::string data;

    if (length > 0) {
        data.resize(length);
        std::cin.read(&data[0], length);
    }

	if (length <= 0) {
        std::string msg = "<p>Missing or invalid Content-Length</p>\n";
        printHeaders(msg.size(), "Status: 411 Length Required");
		std::cerr << "[LOGIN CGI] Missing or invalid Content-Length\n";
        std::cout << msg;
        return 1;
    }

	std::string username, password;
    std::string::size_type u_start = data.find("username=");
    std::string::size_type p_start = data.find("password=");
    if (u_start != std::string::npos && p_start != std::string::npos) {
        username = data.substr(u_start + 9, p_start - u_start - 10);
        password = data.substr(p_start + 9);
    }

	std::fstream file(USERS_FILE, std::ios::in | std::ios::app);
	if (!file.is_open()) {
		if (errno == ENOENT) {
			file.open(USERS_FILE, std::ios::out);
			file.close();
			file.open(USERS_FILE, std::ios::in | std::ios::app);
	    }
		else {
			std::string msg = "<p>Could not open user database.</p>\n";
			printHeaders(msg.size(), "Status: 500 Internal Server Error");
			std::cerr << "[LOGIN CGI] Failed to read user file\n";
			std::cout << msg;
			return 1;
		}
	}

	std::string line;
	bool valid = false;
	bool user = false;
	while (std::getline(file, line)) {
		size_t sep = line.find(':');
		if (sep != std::string::npos) {
			std::string stored_user = line.substr(0, sep);
			std::string stored_pass = line.substr(sep + 1);
			if (stored_user == username) {
				user = true;
				if (stored_pass == password) {
					valid = true;
					break;
				}
			}
		}
	}

	if (type == "login") {
		
		std::string msg;
		if (valid) {
			std::string token = generateSessionToken();

			std::ofstream sessionFile(SESSION_FILE, std::ios::app);
			sessionFile << token << ":" << username << ":" << std::time(NULL) << "\n";
			sessionFile.close();

			std::ostringstream headers;
			headers << "Status: 302 Found\r\n";
			headers << "Set-Cookie: session=" << token << "; Path=/; HttpOnly; Max-Age=3600\r\n";
			headers << "Location: /index.html\r\n"; 
			headers << "Content-Type: text/html\r\n";
			headers << "Content-Length: " << std::string("<p>Login successful!</p>\n").size() << "\r\n\r\n";

			std::cout << headers.str();
			std::cout << "<p>Login successful!</p>\n";

		} else {
			msg = "<p>Invalid username or password.</p>\n";
			printHeaders(msg.size(), "Status: 401 Unauthorized");
			std::cout << msg;
		}
	}
	
	else if (type == "register") {
		std::string msg;
		if (user) {
			msg = "<p>user already exist</p>\n";
			printHeaders(msg.size(), "Status: 200 OK");
		}
		else {
			file.clear(); 

			file << username << ":" << password << "\n";
			msg = "<p>Registered successfully!</p>\n";
        	printHeaders(msg.size(), "Status: 201 Created");
		}
		std::cout << msg;
	}
	file.close();
	return 0;
}
