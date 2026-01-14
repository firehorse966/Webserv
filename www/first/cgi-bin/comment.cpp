#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>

// c++ -Wall -std=c++98 -O2 -o comment.cgi comment.cpp

void updateJsonIndex(const std::string& dirPath, const std::string& jsonPath) {
    DIR* dir = opendir(dirPath.c_str());
    if (!dir) return;

    std::vector<std::string> files;
    dirent* ent;
    while ((ent = readdir(dir))) {
        std::string name = ent->d_name;
        if (name != "." && name != ".." && name.find(".html") != std::string::npos) {
            files.push_back(name);
        }
    }
    closedir(dir);

    std::sort(files.begin(), files.end());

    std::ofstream out(jsonPath.c_str());
    if (!out) return;

    out << "[\n";
    for (size_t i = 0; i < files.size(); ++i) {
        out << "  \"" << files[i] << "\"";
        if (i + 1 < files.size()) out << ",";
        out << "\n";
    }
    out << "]\n";
}


std::string urlDecode(const std::string& str) {
    std::string out;
    int hex;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            sscanf(str.substr(i + 1, 2).c_str(), "%x", &hex);
            out += static_cast<char>(hex);
            i += 2;
        } else if (str[i] == '+') {
            out += ' ';
        } else {
            out += str[i];
        }
    }
    return out;
}

void printHeadersWithLength(size_t length, const std::string& status) {
    std::cout << status << "\r\n";
    std::cout << "Content-Type: text/html\r\n";
    std::cout << "Content-Length: " << length << "\r\n\r\n";
}

void handlePost() {
    const char* len = getenv("CONTENT_LENGTH");
    int contentLength = len ? std::atoi(len) : 0;

    if (contentLength <= 0) {
        std::string msg = "<p>Missing or invalid Content-Length</p>\n";
        printHeadersWithLength(msg.size(), "Status: 411 Length Required");
		std::cerr << "[COMMENT CGI] Missing or invalid Content-Length\n";
        std::cout << msg;
        return;
    }

    std::string data(contentLength, '\0');
    std::cin.read(&data[0], contentLength);

    std::string author, message;
    std::istringstream ss(data);
    std::string token;
    while (std::getline(ss, token, '&')) {
        size_t eq = token.find('=');
        if (eq != std::string::npos) {
            std::string key = urlDecode(token.substr(0, eq));
            std::string val = urlDecode(token.substr(eq + 1));
            if (key == "author") author = val;
            else if (key == "message") message = val;
        }
    }
    if (author.empty() || message.empty()) {
        std::string msg = "<p>Missing author or message</p>\n";
        printHeadersWithLength(msg.size(), "Status: 400 Bad Request");
		std::cerr << "[COMMENT CGI] Missing author or message\n";
        std::cout << msg;
        return;
    }

    struct stat st;
    if (stat("../comments", &st) != 0) {
        if (mkdir("../comments", 0777) != 0) {
            std::string msg = "<p>Failed to create comment directory</p>\n";
            printHeadersWithLength(msg.size(), "Status: 500 Internal Server Error");
			std::cerr << "[COMMENT CGI] Failed to create comment directory\n";
            std::cout << msg;
            return;
        }
    }

    std::ostringstream filename;
    filename << "../comments/comment_" << std::time(0) << "_" << getpid() << ".html";
    std::ofstream out(filename.str().c_str());
    if (!out) {
        std::string msg = "<p>Failed to write comment</p>\n";
        printHeadersWithLength(msg.size(), "Status: 500 Internal Server Error");
		std::cerr << "[COMMENT CGI] Failed to write comment\n";
        std::cout << msg;
        return;
    }

    out << "<div class=\"comment\">\n";
    out << "<strong>" << author << "</strong><br>\n";
    out << "<p>" << message << "</p>\n";
    out << "</div>\n";
    out.close();

    updateJsonIndex("../comments", "../comments/index.json");

    std::string msg = "{\"status\":\"ok\"}";
	printHeadersWithLength(msg.size(), "Status: 200 OK");
	std::cout << msg;

}

void handleGet() {
    std::string query = getenv("QUERY_STRING") ? getenv("QUERY_STRING") : "";

    std::ostringstream html;
    html << "<html><body>\n";
    html << "<h1>Leave a comment</h1>\n";
    html << "<form method=\"POST\" action=\"/cgi-bin/comment.cgi\">\n";
    html << "Author: <input name=\"author\"><br>\n";
    html << "Message:<br><textarea name=\"message\"></textarea><br>\n";
    html << "<input type=\"submit\" value=\"Post\">\n";
    html << "</form>\n";
    html << "<hr><h2>Comments</h2>\n";

    // Handle ?mode=comments query
    if (query == "mode=comments") {
        DIR* dir = opendir("../comments");
        if (!dir) {
            html << "<p>No comments found.</p>\n";
        } else {
            std::vector<std::string> files;
            dirent* ent;
            while ((ent = readdir(dir))) {
                std::string name = ent->d_name;
                if (name != "." && name != "..") {
                    files.push_back(name);
                }
            }
            closedir(dir);
            std::sort(files.begin(), files.end());

            for (size_t i = 0; i < files.size(); ++i) {
				std::string filepath = "../comments/" + files[i];
				std::ifstream in(filepath.c_str());
				if (in) {
					html << in.rdbuf();
				}
			}
        }
    } else {
        html << "<p>Invalid or missing query. To view comments, append ?mode=comments to the URL.</p>\n";
    }

    html << "</body></html>\n";
    printHeadersWithLength(html.str().size(), "Status: 200 OK");
    std::cout << html.str();
}

int main() {
    const char* method = getenv("REQUEST_METHOD");
    std::string req = method ? method : "";

    if (req == "POST") {
        handlePost();
    } else if (req == "GET") {
        handleGet();
    } else {
        std::string msg = "<p>Unsupported method</p>\n";
        printHeadersWithLength(msg.size(), "Status: 405 Method Not Allowed");
		std::cerr << "[COMMENT CGI] Unsupported method\n";
        std::cout << msg;
    }

    std::cout << std::flush;
    return 0;
}
