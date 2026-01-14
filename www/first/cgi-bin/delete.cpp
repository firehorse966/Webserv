#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <algorithm>
#include <vector>
#include <fstream>

//c++ -Wall -std=c++98 -O2 -o delete.cgi delete.cpp

std::string urlDecode(const std::string& str) {
    std::string decoded;
    char ch;
    int hex;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            sscanf(str.substr(i + 1, 2).c_str(), "%x", &hex);
            ch = static_cast<char>(hex);
            decoded += ch;
            i += 2;
        } else if (str[i] == '+') {
            decoded += ' ';
        } else {
            decoded += str[i];
        }
    }
    return decoded;
}

void updateJsonIndex(const std::string& dirPath, const std::string& jsonPath) {
    DIR* dir = opendir(dirPath.c_str());
    if (!dir) return;

    std::vector<std::string> files;
    dirent* ent;
    while ((ent = readdir(dir))) {
        std::string name = ent->d_name;
        if (name == "." || name == "..") continue;

        if (name.size() >= 5 && (
            name  == "index.html" ||
            name.substr(name.size() - 5) == ".json")) {
            continue;
        }

        files.push_back(name);
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

int main() {
   
    const char* reqMethod = getenv("REQUEST_METHOD");
    if (!reqMethod || std::string(reqMethod) != "DELETE") {
        std::cout << "Status: 405 Method Not Allowed\r\n\r\n";
        std::cerr << "[DELETE CGI] Method not allowed\n";
        return 1;
    }

    const char* queryStr = getenv("QUERY_STRING");
    if (!queryStr) {
        std::cout << "Status: 400 Bad Request\r\n\r\nMissing query string.\n";
        std::cerr << "[DELETE CGI] Missing query string\n";
        return 1;
    }

    std::string query(queryStr);
    std::istringstream qs(query);
    std::string token;
    std::string type, target, password;

    while (std::getline(qs, token, '&')) {
        size_t pos = token.find('=');
        if (pos != std::string::npos) {
            std::string key = urlDecode(token.substr(0, pos));
            std::string val = urlDecode(token.substr(pos + 1));
            if (key == "type") type = val;
            else if (key == "target") target = val;
            else if (key == "password") password = val;
        }
    }

    if (type.empty() || target.empty()) {
        std::cout << "Status: 400 Bad Request\r\n\r\nMissing type or target parameter.\n";
        std::cerr << "[DELETE CGI] Missing type or target parameter\n";
        return 1;
    }

	bool passwordRequired = (type == "comment");

	const char* adminPasswordEnv = getenv("ADMIN_PASSWORD");
	if (!adminPasswordEnv) {
		passwordRequired = false;
		//std::cout << "Status: 500 Internal Server Error\r\n\r\nAdmin password not set.\n";
		//std::cerr << "[DELETE CGI] Admin password not set in environment variables.\n";
		//return 1;
	}
	if (passwordRequired) {

		std::string adminPassword(adminPasswordEnv);
		if (password != adminPassword) {
			std::cout << "Status: 403 Forbidden\r\n\r\nInvalid password.\n";
			std::cerr << "[DELETE CGI] Invalid password attempt: received '" << password << "'\n";
			return 1;
		}
	}


    std::string basePath;
	if (type == "comment") {
		basePath = "../comments/";
	} else if (type == "uploads") {
		const char* uploadDirEnv = getenv("UPLOAD_DIR");
		if (uploadDirEnv) {
			basePath = uploadDirEnv;
			if (!basePath.empty() && basePath[basePath.size() - 1] != '/') basePath += '/';
		} else {
			basePath = "../uploads/";
		}
	} else if (type == "image") {
		basePath = "../images/";
	} else if (type == "files") {
		basePath = "../files/";
	} else {
		std::cout << "Status: 400 Bad Request\r\n\r\nInvalid type parameter.\n";
		std::cerr << "[DELETE CGI] Invalid type parameter: " << type << "\n";
		return 1;
	}

    std::string fullPath = basePath + target;

    if (std::remove(fullPath.c_str()) == 0) {

		if (type == "uploads") {
            updateJsonIndex("../uploads", "../resources/uploads.json");
        }

        if (type == "comment") {
            updateJsonIndex("../comments", "../comments/index.json");
        }

        std::cout << "Status: 200 OK\r\nCache-Control: no-store, no-cache, must-revalidate\r\nPragma: no-cache\r\n\r\nDeleted " << target << ".\n";
    } else {
        std::cout << "Status: 404 Not Found\r\n\r\nFile " << target << " not found or could not be deleted.\n";
        std::cerr << "[DELETE CGI] Failed to delete: " << fullPath << "\n";
    }
    std::cout << std::flush;
    return 0;
}
