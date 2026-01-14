/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbolano- <mbolano-@student.42malaga.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/19 07:33:42 by mbolano-          #+#    #+#             */
/*   Updated: 2025/06/11 15:03:15 by mbolano-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"

std::vector<ServerConfig> ConfigParser::servers;

// THESE ARE PRIVATE METHODS:

std::string ConfigParser::to_string(ssize_t value) const
{
    std::ostringstream oss;
    oss << value;
    return (oss.str());
}

// These are common methods for the SEVER BLOCK: (used by the methods to check the CONFIG FILE structure and values)

server_key   ConfigParser::stringToServerKey(const std::string &key) const
{
    if (key == "listen")
        return (LISTEN);
    else if (key == "server_name")
        return (NAME);
    else if (key == "password")
        return (PASSWORD);
    else if (key == "loginOn")
        return (LOGIN_ON);
    else if (key == "root")
        return (SERVER_ROOT);
    else if (key == "index")
        return (INDEX);
    else if (key == "error_page")
        return (ERROR);
    else if (key == "client_max_body_size")
        return (SIZE);
    else if (key == "location")
        return (LOCATION);
    else if (key == "}")
        return (CLOSE_SERVER_BRACKET);
    return (UNKNOWN_SERVER_KEY);
}

// ---------------------------------------------------------------------------------------------------------------

// These are common methods for the LOCATION BLOCK: (used by the methods to check the CONFIG FILE structure and values)


location_key    ConfigParser::stringToLocationKey(const std::string &key)   const
{
    if (key == "forbidden_methods")
        return (F_METHODS);
    else if (key == "root")
        return (LOCATION_ROOT);
    else if (key == "autoIndexOn")
        return (AUTO_INDEX);
    else if (key == "cgi_files")
        return (CGI_FILES);
    else if (key == "redirect")
        return (REDIRECT);
    else if (key == "try_files")
        return (TRY_FILES);
    else if (key == "}")
        return (CLOSE_LOCATION_BRACKET);
    return (UNKNOWN_LOCATION_KEY);
}

// ---------------------------------------------------------------------------------------------------------------

// 1st.- This is the method to check if the CONFIG FILE (cfg) path is valid:

bool    ConfigParser::isConfigFilePathValid(const std::string &configFilePath)    const
{
    std::ifstream file(configFilePath.c_str());
    if (!file.is_open())
        return (false);
    file.close();
    return (true);
}

// ---------------------------------------------------------------------------------------------------------------

// 2nd.- This is the method to read the CONFIG FILE (cfg) content and convert it to a string:

std::string					ConfigParser::configFileToString(const std::string &configFilePath)	const
{
    std::ifstream   file(configFilePath.c_str());

    if (!file.is_open())
        return ("");
    std::stringstream   buffer;
    buffer << file.rdbuf();
    file.close();
    std::string finalContent = buffer.str();
    // DELETE THE COMMENTS FROM THE CONFIG FILE:
    std::string::size_type pos = 0;
    while ((pos = finalContent.find("#", pos)) != std::string::npos)
    {
        std::string::size_type endPos = finalContent.find("\n", pos);
        if (endPos == std::string::npos)
            finalContent.erase(pos);
        else
            finalContent.erase(pos, endPos - pos);
    }
    const std::string charsToReplace = "\n\t\r\v\f\b\a\e";  // THESE ARE THE CHARACTERS TO REPLACE
    for (std::string::size_type i = 0; i < charsToReplace.size(); ++i)
    {
        std::replace(finalContent.begin(), finalContent.end(), charsToReplace[i], ' ');
    }
    
    const std::string specialChars = ";{}"; // THESE ARE THE CHARACTERS TO REPLACE BY A SPACE BEFORE AND AFTER
    for (std::string::size_type i = 0; i < specialChars.size(); ++i)
    {
        std::string toFind(1, specialChars[i]);
        std::string toReplace = " " + toFind + " ";
        std::string::size_type pos = 0;
        while ((pos = finalContent.find(toFind, pos)) != std::string::npos)
        {
            finalContent.replace(pos, 1, toReplace);
            pos += toReplace.length();
        }
    }
    return (finalContent);
}

// ---------------------------------------------------------------------------------------------------------------

// 3rd.- These are methods to check de CONFIG FILE (cfg) structure:

bool    ConfigParser::checkConfigFileStructure(const std::string &cfg)
{
    std::istringstream  mainStream(cfg);
    std::string         token;

    while (!(mainStream.eof()) && (mainStream >> token))
    {
        this->_wordCount++;
        this->_serverNumber++;
        // Verify that the first token is "server" and the next one is "{":
        if (token != "server")
        {
            this->_firstCheckError = "Error: The first token in CFG file must be 'server', but found: " + token;
            return (false);
        }
        if (!(mainStream >> token) || token != "{")
        {
            this->_wordCount++;
            this->_firstCheckError = "Error: The expected token after 'server' is '{', but found: " + token;
            return (false);
        }
        this->_wordCount++;
        if (!(this->parseServerBlock(mainStream)))
            return (false);
    }
    return (true);
}

//// These are the methods to check the SERVER BLOCK (serverStream) structure:

bool    ConfigParser::parseServerBlock(std::istringstream &serverStream)
{
    std::string token;

    while (serverStream >> token)
    {
        this->_wordCount++;
        server_key s_key = stringToServerKey(token);
        switch (s_key)
        {
            case LISTEN:
            case NAME:
            case ERROR:
            case SIZE:
            case INDEX:
            case PASSWORD:
            case LOGIN_ON:
            case SERVER_ROOT:
                if (!(this->parseServerDirective(serverStream)))
                    return (false);
                break ;
            case LOCATION:
                if (!(this->parseLocationBlock(serverStream)))
                    return (false);
                break ;
            case CLOSE_SERVER_BRACKET:
                return (true);
            default:
                this->_firstCheckError = "Error: Invalid key value: " + token + "; for a 'server' in configuration file (cfg): " + this->_configFilePath;
                return (false);
        }
    }
    this->_firstCheckError = "Error: The server block is not closed properly with '}' in configuration file (cfg): " + this->_configFilePath;
    return (false);
}

bool    ConfigParser::wordInServerKeyValues(const std::string &word) const
{
    if (word == "listen" || word == "loginOn" || word == "index" || word == "password" || word == "server_name" || word == "root" || word == "error_page" || word == "client_max_body_size" || word == "location")
        return (true);
    return (false);
}

bool    ConfigParser::parseServerDirective(std::istringstream &keyStream)
{
    std::string token;

    while (keyStream >> token && token != ";")
    {
        this->_wordCount++;
        if (this->wordInServerKeyValues(token) || token == "{" || token == "}")
        {
            this->_firstCheckError = "Error: Every server directive should be closed with ';'. Otherwise, it can't be parsed.";
            return (false);
        }
    }
    this->_wordCount++;
    if (token == ";")
        return (true);
    this->_firstCheckError = "Error: Every server directive should be closed with ';'. Otherwise, it can't be parsed.";
    return (false);
}

//// These are the methods to check the LOCATION BLOCK (locationStream) structure:

bool    ConfigParser::parseLocationBlock(std::istringstream &locationStream)
{
    std::string token;

    if (!(locationStream >> token))
    {
        this->_wordCount++;
        this->_firstCheckError = "Error: The location block should be defined with a path, but found nothing; in configuration file (cfg): " + this->_configFilePath;
        return (false);
    }
    this->_wordCount++;
    if (!(locationStream >> token) || token != "{")
    {
        this->_wordCount++;
        this->_firstCheckError = "Error: The expected token after 'location' is a path and a '{' after it, but found: " + token + "; in configuration file (cfg): " + this->_configFilePath;
        return (false);
    }
    this->_wordCount++;
    while (locationStream >> token)
    {
        this->_wordCount++;
        location_key l_key = stringToLocationKey(token);
        switch (l_key)
        {
            case F_METHODS:
            case AUTO_INDEX:
            case LOCATION_ROOT:
            case REDIRECT:
            case TRY_FILES:
            case CGI_FILES:
                if (!(this->parseLocationDirective(locationStream)))
                    return (false);
                break ;
            case CLOSE_LOCATION_BRACKET:
                return (true);
            default:
                this->_firstCheckError = "Error: Invalid key value: " + token + "; for a 'location' in configuration file (cfg): " + this->_configFilePath;
                return (false);
        }
    }
    this->_firstCheckError = "Error: The location block is not closed properly with '}' in configuration file (cfg): " + this->_configFilePath;
    return (false);
}

bool    ConfigParser::wordInLocationKeyValues(const std::string &word) const
{
    if (word == "forbidden_methods" || word == "root" || word == "autoIndexOn" || word == "cgi_files" || word == "redirect" || word == "try_files")
        return (true);
    return (false);
}

bool    ConfigParser::parseLocationDirective(std::istringstream &keyStream)
{
    std::string token;

    while (keyStream >> token && token != ";")
    {
        this->_wordCount++;
        if (this->wordInLocationKeyValues(token) || token == "{" || token == "}")
        {
            this->_firstCheckError = "Error: Every location directive should be closed with ';'. Otherwise, it can't be parsed.";
            return (false);
        }
    }
    this->_wordCount++;
    if (token == ";")
        return (true);
    this->_firstCheckError = "Error: Every location directive should be closed with ';'. Otherwise, it can't be parsed.";
    return (false);
}

// ---------------------------------------------------------------------------------------------------------------

// 4th.- These are the methods to check de CONFIG FILE (cfg) values:

void    ConfigParser::checkConfigFileValues(const std::string &cfg)
{
    std::istringstream  mainStream(cfg);
    std::string         token;
    
    while (!(mainStream.eof()) && (mainStream >> token))
    {
        if (token != "server")
        {
            // Obviusly, the first token must be "server", otherwise, it is an error... but we can ignore it for now
            // cause we have already checked the structure of the CONFIG FILE (cfg) in the method: checkConfigFileStructure().
            this->writeLog("Error: Invalid value for a 'server' key: " + token);
            this->isValidCFG = false;
            return ;
        }
        if (!(mainStream >> token) || token != "{")
        {
            // If the next token is not "{", it means that the server block is not valid... but we can ignore it for now
            // cause we have already checked the structure of the CONFIG FILE (cfg) in the method: checkConfigFileStructure() too.
            this->writeLog("Error: Invalid value for opening a new 'server' block '{': " + token);
            this->isValidCFG = false;
            return ;
        }
        this->parseServerValues(mainStream);
    }
}

//// 4.A.- These are the methods to check the SERVER BLOCK (serverStream) values:

void    ConfigParser::initServerBlock(ServerConfig &current_server)
{
    current_server.listen.clear();
    current_server.password.clear();
    current_server.loginOn = false;
    current_server.server_names.clear();
    current_server.root.clear();
    current_server.index.clear();
    current_server.error_pages.clear();
    current_server.client_max_body_size = -1;
    current_server.locations.clear();
}

void    ConfigParser::checkEmptyServerKeys(ServerConfig &current_server)
{
    if (current_server.listen.empty())
    {
        this->writeLog("Warning: The 'listen' key is empty in the server block.");
        std::cerr << "Warning: The 'listen' key is empty in the server block. Please, check the log for more info." << std::endl;
    }
    if (current_server.server_names.empty())
        this->writeLog("Info: The 'server_name' key is empty in the server block configuration.");
    if (current_server.root.empty())
        this->writeLog("Warning: The 'root' key is empty in the server block configuration.");
    if (current_server.index.empty())
        this->writeLog("Warning: The 'index' key is empty in the server block configuration.");
}

void    ConfigParser::parseServerValues(std::istringstream &serverStream)
{
    ServerConfig    current_server;
    std::string     token;
    // Initialize the server block with default values:
    this->initServerBlock(current_server);
    this->writeLog("-------- Parsing server block: " + this->to_string(this->_serverNumber++) + " --------");
    while (serverStream >> token)
    {
        server_key s_key = this->stringToServerKey(token);
        switch (s_key)
        {
            case LISTEN:
                this->handleListen(serverStream, current_server);
                break ;
            case NAME:
                this->handleServerName(serverStream, current_server);
                break ;
            case LOGIN_ON:
                this->handleLoginOn(serverStream, current_server);
                break ;
            case SERVER_ROOT:
                this->handleRoot(serverStream, current_server);
                break ;
            case INDEX:
                this->handleIndex(serverStream, current_server);
                break ;
            case ERROR:
                this->handleErrorPage(serverStream, current_server);
                break ;
            case SIZE:
                this->handleClientMaxBodySize(serverStream, current_server);
                break ;
            case PASSWORD:
                this->handlePassword(serverStream, current_server);
                break ;
            case LOCATION:
                this->handleLocation(serverStream, current_server);
                break ;
            case CLOSE_SERVER_BRACKET:
                // If the server block is closed, we can check if all the required keys are present. Otherwise, we can log an error:
                this->checkEmptyServerKeys(current_server);
                if (current_server.listen.empty())
                {
                    writeLog("Warning: This server block will not be added to current servers, because the 'listen' key is empty in the server block configuration file (cfg): " + this->_configFilePath);
                    return ;
                }
                this->servers.push_back(current_server);
                if (this->_writeLogEntries == 0)
                    this->writeLog("Info: This server block has been added successfully.");
                else
                    this->writeLog("Info: Although this server block has been added successfully, there are some warnings or errors in the configuration file (cfg): " + this->_configFilePath);
                return ;
            default:
                this->writeLog("Warning: Invalid key value: " + token + "; for a 'server' in configuration file (cfg): " + this->_configFilePath);
                while (serverStream >> token && token != ";")
                {
                    // Skip the rest of the line until the semicolon:
                }
        }
    }
}

////// 4.A.1.- These are the methods to manage the LISTEN (listen) key:

void    ConfigParser::handleListen(std::istringstream &ss, ServerConfig &current_server)
{
    std::string value;
    while (ss >> value)
    {
        if (value == ";")
            break ;
        if (isValidIPAndPort(value))
            this->addIPAndPortToServer(value, current_server);
        else
            this->writeLog("Error: Invalid value: " + value + "; for a 'listen' key in configuration file (cfg): " + this->_configFilePath);
    }
}

bool    ConfigParser::isValidIPAndPort(const std::string &value)    const
{
    size_t colonPos = value.find(":");
    if (colonPos == std::string::npos)
        return (isValidPort(value));
    std::string ip = value.substr(0, colonPos);
    std::string port = value.substr(colonPos + 1);
    return (this->isValidIP(ip) && this->isValidPort(port));
}

bool    ConfigParser::isValidPort(const std::string &value) const
{
    for (size_t i = 0; i < value.size(); ++i)
    {
        if (!std::isdigit(value[i]))
            return (false);
    }
    try
    {
        int port = std::atoi(value.c_str());
        if (port < 0 || port > 65535)
            return (false);
    }
    catch (const std::invalid_argument &e)
    {
        return (false); // No conversion could be performed.
    }
    catch (const std::out_of_range &e)
    {
        return (false); // The value is out of the range of representable values by an int.
    }
    return (true);
}

bool    ConfigParser::isValidIP(const std::string &value) const
{
    if (inet_addr(value.c_str()) == INADDR_NONE)
        return (false);
    return (true);
}

void    ConfigParser::addIPAndPortToServer(const std::string &value, ServerConfig &current_server)
{
    std::string ip = getIPValue(value);
    if (ip.empty()) // If there's no IP, it means that the IP default value is: 0.
    {
        // Check if there's any IP with 0 value already in the map:
        std::map<in_addr_t, std::vector<int> >::iterator it = current_server.listen.find(0);
        if (it != current_server.listen.end())
        {
            // If there's an IP already in the map, add the new PORT to the existing IP (0):
            it->second.push_back(std::atoi(value.c_str()));
        }
        else
        {
            // If there's no IP in the map, add the new IP and port:
            current_server.listen[0].push_back(std::atoi(value.c_str()));
        }
    }
    else if (!(ip.empty()))
    {
        // If the IP value is not empty, it means it comes with a port:
        size_t colonPos = value.find(":");
        std::string port = value.substr(colonPos + 1);
        // If the IP value is not empty, check if there's any IP with the same value already in the map:
        std::map<in_addr_t, std::vector<int> >::iterator it = current_server.listen.find(inet_addr(ip.c_str()));
        if (it != current_server.listen.end())
        {
            // If there's an IP already in the map, add the new PORT to the existing IP:
            it->second.push_back(std::atoi(port.c_str()));
        }
        else
        {
            // If there's no IP in the map, add the new IP and port:
            current_server.listen[inet_addr(ip.c_str())].push_back(std::atoi(port.c_str()));
        }
    }
}

std::string ConfigParser::getIPValue(const std::string &value) const
{
    size_t colonPos = value.find(":");
    if (colonPos == std::string::npos)
        return ("");
    return (value.substr(0, colonPos));
}

////// 4.A.2.- These are the methods to manage the NAME (server_name) key:

void    ConfigParser::handleServerName(std::istringstream &ss, ServerConfig &current_server)
{
    std::string value;
    while (ss >> value)
    {
        if (value == ";")
            break ;
        if (this->isValidServerName(value) || this->isValidIP(value))    
            current_server.server_names.push_back(value);
        else
            this->writeLog("Error: Invalid value: " + value + "; for a 'server_name' key in configuration file (cfg): " + this->_configFilePath);
    }
}

bool    ConfigParser::isValidServerName(const std::string &value)   const // This method checks the server name format compliance the RFC's: 1034 and 1123 (DNS NAMING RULES); and the RFC's: 7230 and 9110 (HTTP SEMANTICS).
{
    // Verify that the value is not empty.
    if (value.empty())
        return (false);
    // Verify that it does not contain more than 255 characters.
    if (value.size() > 255)
        return (false);
    // Verify that it does not start or end with a dot or a hyphen.
    if (value[0] == '.' || value[value.size() - 1] == '.' || value[0] == '-' || value[value.size() - 1] == '-')
        return (false);
    // Verify that it does not contain two consecutive dots or hyphens.
    if (value.find("..") != std::string::npos || value.find("--") != std::string::npos)
        return (false);
    // Verify that it does not contain any character other than alphanumeric, hyphen, or dot.
    for (size_t i = 0; i < value.size(); ++i)
    {
        if (!std::isalnum(value[i]) && value[i] != '-' && value[i] != '.')
            return (false);
    }
    // Verify that it does not contain a hyphen or a dot at the beginning or end of a segment.
    // Each segment is separated by a dot. For example: "www.-example.com" is invalid.
    std::istringstream ss(value);
    std::string segment;
    while (std::getline(ss, segment, '.'))
    {
        if (segment.empty() || segment[0] == '-' || segment[segment.size() - 1] == '-')
            return (false);
    }
    // Verify that every segment does not contain more than 63 characters.
    ss.clear();
    ss.str(value);
    while (std::getline(ss, segment, '.'))
    {
        if (segment.empty() || segment.size() > 63 || segment[0] == '-' || segment[segment.size() - 1] == '-')
            return false;
    }
    return (true);
}

////// 4.A.3.- These are the methods to manage the ROOT (root) key:

void    ConfigParser::handleRoot(std::istringstream &ss, ServerConfig &current_server)
{
    std::string value;
    while (ss >> value)
    {
        if (value == ";")
            break ;
        if (!(current_server.root.empty()))
        {
            this->writeLog("Error: The 'root' key value: " + value + "; is duplicated in the server block configuration file (cfg): " + this->_configFilePath);
        }
        if (isValidRoot(value))
        {
            if (value[value.size() - 1] != '/')
                current_server.root = value;
            else
                current_server.root = value.substr(0, value.size() - 1);
        }
        else
            this->writeLog("Error: Invalid value: " + value + "; for a 'root' key in the server block configuration file (cfg): " + this->_configFilePath);
    }
}

bool    ConfigParser::isValidRoot(const std::string &value)
{
    // Check that the path exists:
    struct stat buffer;
    if (stat(value.c_str(), &buffer) != 0)
    {
        this->writeLog("Error: The 'root' dir in the server block configuration (cfg) doesn't exist: " + value);
        return (false);
    }
    // Check that the path is not a "root" user directory:
    if (buffer.st_uid == 0)
    {
        this->writeLog("Error: Not enough access permissions to 'root' dir in server block configuration (cfg): " + value);
        return (false);
    }
    return (true);
}

////// 4.A.4.- These are the methods to manage the ERROR PAGE (error_page) key:

void    ConfigParser::handleErrorPage(std::istringstream &ss, ServerConfig &current_server)
{
    std::string value;

    while (ss >> value)
    {
        if (value == ";")
            break ;
        if (current_server.root.empty())
        {
            this->writeLog("Error: The 'root' key is empty/missing in the server block configuration (cfg): " + this->_configFilePath + "; so, the 'error_page' key/keys cannot be added.");
            while (ss >> value && value != ";")
            {
                // Skip the rest of the line until the semicolon:
            }
            return ;
        }
        std::string error_code = value;
        if (!(this->isValidErrorCode(error_code)))
            this->writeLog("Error: Invalid error code for a 'error_page' key in the server block configuration (cfg): " + error_code);
        if (!(ss >> value) || value == ";")
            this->writeLog("Error: Invalid value: " + value + "; for the 'error_page': " + error_code + "; in the server block configuration (cfg). It must be a path to a file or directory.");
        if (!(this->isValidErrorPage(value, current_server)))
            this->writeLog("Error: The 'error_page': " + value + "; is not valid. So, it will not be added to the server block configuration (cfg): " + this->_configFilePath);
        if (this->isValidErrorCode(error_code) && this->isValidErrorPage(value, current_server))
        {
            // Check that the error code is not duplicated. Otherwise, replace the value with the new one and log a warning in the log file:
            if (current_server.error_pages.find(error_code) != current_server.error_pages.end())
                this->writeLog("Warning: The current 'error_page' key value: " + current_server.error_pages[error_code] + "; will be replaced in the server block configuration by: " + value + "; cause it is duplicated in the configuration file (cfg): " + this->_configFilePath);
            else
                current_server.error_pages[error_code] = value;
        }
    }
}

/*

HTTP status codes are integers that indicate the outcome of an HTTP request.
These codes are defined in several RFCs, including: RFC 7231, RFC 7232, RFC 7233, RFC 7235, RFC 6585, and others.
HTTP status codes are divided into five classes, each represented by the first digit of the status code:

1. **1xx (Informational)**: Indicates that the request was received and processing continues.
2. **2xx (Successful)**: Indicates that the request was successfully received, understood, and accepted.
3. **3xx (Redirection)**: Indicates that further action must be taken to complete the request.
4. **4xx (Client Error)**: Indicates that the request contains incorrect syntax or cannot be fulfilled.
5. **5xx (Server Error)**: Indicates that the server failed to fulfill a seemingly valid request.

### Range of values ​​for HTTP status codes:

- **1xx**: 100 - 199
- **2xx**: 200 - 299
- **3xx**: 300 - 399
- **4xx**: 400 - 499
- **5xx**: 500 - 599

*/

bool    ConfigParser::isValidErrorCode(const std::string &error_code)   const
{
    // Check that the error code is a 3-digit integer between 100 and 599:
    if (error_code.size() != 3)
        return (false);
    for (size_t i = 0; i < error_code.size(); ++i)
    {
        if (!std::isdigit(error_code[i]))
            return (false);
    }
    size_t errorCode = std::atoi(error_code.c_str());
    if (errorCode < 100 || errorCode > 599)
        return (false);
    return (true);
}

bool    ConfigParser::isValidErrorPage(const std::string &value, ServerConfig &current_server) const
{
    // Check that the 'root' key is not empty:
    if (current_server.root.empty())
        return (false);
    // Check that the path exists:
    struct stat buffer;
    std::string path = current_server.root + value;
    if (stat(path.c_str(), &buffer) != 0)
        return (false);
    // Check that the path is not a "root" user directory:
    if (buffer.st_uid == 0)
        return (false);   
    return (true);
}

////// 4.A.5.- These are the methods to manage the INDEX (index) key:

void    ConfigParser::handleIndex(std::istringstream &ss, ServerConfig &current_server)
{
    std::string value;
    while (ss >> value)
    {
        if (value == ";")
            break ;
        // Check the value is not duplicated. Otherwise, add it to the index vector:
        if (std::find(current_server.index.begin(), current_server.index.end(), value) == current_server.index.end())
            current_server.index.push_back(value);
    }   
}

////// 4.A.6.- These are the methods to manage the SIZE (client_max_body_size) key:

void    ConfigParser::handleClientMaxBodySize(std::istringstream &ss, ServerConfig &current_server)
{
    std::string value;

    while (ss >> value)
    {
        if (value == ";")
            break ;
        if (!(this->isValidBodySize(value)))
            this->writeLog("Error: Invalid value: " + value + "; for a 'client_max_body_size' key in the server block configuration (cfg): " + this->_configFilePath);
        else
        {
            // Check that the key is not duplicated. Otherwise, replace the value with the new one and log a warning in the log file:
            if (current_server.client_max_body_size >= 0)
                this->writeLog("Warning: The current 'client_max_body_size' key value: " + this->to_string(current_server.client_max_body_size) + "; will be replaced in the server block configuration by: " + this->to_string(this->getBodySizeBytes(value)) + "; cause it is duplicated in the configuration file (cfg): " + this->_configFilePath);
            current_server.client_max_body_size = this->getBodySizeBytes(value);
        }
    }
}

bool    ConfigParser::isValidBodySize(const std::string &value) const
{
    // Check that the value is not empty:
    if (value.empty())
        return (false);
    if (value.find("M") != std::string::npos || value.find("K") != std::string::npos || value.find("G") != std::string::npos)
    {
        std::string unit = value.substr(value.size() - 1);
        if (unit != "M" && unit != "K" && unit != "G")
            return (false);
        std::string number = value.substr(0, value.size() - 1);
        if (!(this->isPositiveInteger(number)))
            return (false);
    }
    else
    {
        if (!(this->isPositiveInteger(value)))
            return (false);
    }
    return (true);
}

bool    ConfigParser::isPositiveInteger(const std::string &value)   const
{
    for (size_t i = 0; i < value.size(); ++i)
    {
        if (!std::isdigit(value[i]))
            return (false);
    }
    try
    {
        int number = std::atoi(value.c_str());
        if (number < 0)
            return (false);
    }
    catch (const std::invalid_argument &exception)
    {
        return (false); // No conversion could be performed.
    }
    catch (const std::out_of_range &exception)
    {
        return (false); // The value is out of the range of representable values by an int.
    }
    return (true);
}

ssize_t  ConfigParser::getBodySizeBytes(const std::string &value)   const
{
    ssize_t size = 0;

    if (value.find("K") != std::string::npos)
        size = std::atoi(value.substr(0, value.size() - 1).c_str()) * 1024;
    else if (value.find("M") != std::string::npos)
        size = std::atoi(value.substr(0, value.size() - 1).c_str()) * 1024 * 1024;
    else if (value.find("G") != std::string::npos)
        size = std::atoi(value.substr(0, value.size() - 1).c_str()) * 1024 * 1024 * 1024;
    else
        size = std::atoi(value.c_str());
    return (size);
}

////// 4.A.7.- These are the methods to manage the PASSWORD (password) key:

void    ConfigParser::handlePassword(std::istringstream &ss, ServerConfig &current_server)
{
    std::string value;
    while (ss >> value)
    {
        if (value == ";")
            break ;
        if (current_server.password.empty())
            current_server.password = value;
        else
            this->writeLog("Error: The 'password' key is duplicated in the server block configuration (cfg): " + value);
    }
}

////// 4.A.8.- These are the methods to manage the LOGIN_ON (loginOn) key:

void    ConfigParser::handleLoginOn(std::istringstream &ss, ServerConfig &current_server)
{
    std::string value;
    while (ss >> value)
    {
        if (value == ";")
            break ;
        if (value == "on" || value == "off")
        {
            if (value == "on")
                current_server.loginOn = true;
            else
                current_server.loginOn = false;
        }
        else
            this->writeLog("Error: Invalid value for a 'loginOn' key in the server block configuration (cfg): " + value);
    }
}

// ---------------------------------------------------------------------------------------------------------------

//// 4.B.- These are the methods to manage the LOCATION BLOCK (current_server.locations) values:

void    ConfigParser::handleLocation(std::istringstream &ss, ServerConfig &current_server)
{
    std::string path;
    path = this->handleLocationPath(ss);
    if (path.empty())
        return ;
    // Initialize the location block with default values:
    if (current_server.locations.find(path) == current_server.locations.end())
    {
        this->initLocationBlock(current_server.locations[path]);
        current_server.locations[path].path = path;
    }
    else
        this->writeLog("Error: The 'location' path: " + path + "; is duplicated and it'll be overwrited in this server block.");
    std::string token;
    ss >> token;
    while (ss >> token)
    {
        location_key l_key = this->stringToLocationKey(token);
        switch (l_key)
        {
            case F_METHODS:
                this->handleForbiddenMethods(ss, current_server.locations[path]);
                break ;
            case LOCATION_ROOT:
                this->handleLocationRoot(ss, current_server.locations[path]);
                break ;
            case AUTO_INDEX:
                this->handleUploadEnabled(ss, current_server.locations[path]);
                break ;
            case CGI_FILES:
                this->handleCgiFiles(ss, current_server.locations[path]);
                break ;
            case REDIRECT:
                this->handleRedirect(ss, current_server.locations[path]);
                break ;
            case TRY_FILES:
                this->handleTryFiles(ss, current_server.locations[path]);
                break ;
            case CLOSE_LOCATION_BRACKET:
                return ;
            default:
                this->writeLog("Error: Invalid value for a 'location' key in the server block configuration (cfg): " + token);
                while (ss >> token && token != ";")
                {
                    // Skip the rest of the line until the semicolon:
                }
        }
    }
}

void    ConfigParser::initLocationBlock(LocationConfig &current_location)
{
    current_location.root = "";
    current_location.autoIndexOn = false;
    current_location.cgi_files.clear();
    current_location.forbiddenMethods.clear();
    current_location.redirect = "";
    current_location.try_files.clear();
}

////// 4.B.1.- These are the methods to manage the LOCATION PATH (path) key:

std::string    ConfigParser::handleLocationPath(std::istringstream &ss)
{
    // Check that the location path is valid:
    std::string path;
    if (!(ss >> path) || path == "{")
    {
        this->writeLog("Error: The 'location' path is missing/empty in the server block configuration.");
        return ("");
    }
    if (!(this->isValidLocationPath(path)))
    {
        this->writeLog("Error: Invalid value for a 'location' key in this server block configuration.");
        while (ss >> path && path != "}")
        {
            // Skip the rest of the line until the semicolon:
        }
        return ("");
    }
    if (path.size() > 1 && path[path.size() - 1] == '/')
        path = path.substr(0, path.size() - 1);
    return (path);
}

bool    ConfigParser::isValidLocationPath(const std::string &path) const
{
    // Check that the value is not empty.
    if (path.empty())
        return (false);
    // Check that it starts with a slash.
    if (path[0] != '/')
        return (false);
    // Check that it does not contain consecutive slashes.
    if (path.find("//") != std::string::npos)
        return (false);
    // Check that it does not contain any disallowed characters.
    const std::string invalidChars = "*?|<>\"'&$#@!%()[]{};:~`";
    for (size_t i = 0; i < path.size(); ++i)
    {
        // If a disallowed character is found, return false.
        if (invalidChars.find(path[i]) != std::string::npos)
            return (false);
        // Reject control characters (non-printable).
        if (std::iscntrl(static_cast<unsigned char>(path[i])))
            return (false);
    }
    // Split the path by '/' and check each segment.
    std::istringstream iss(path);
    std::string segment;
    // The first read is always empty since the path starts with a slash.
    std::getline(iss, segment, '/');
    // Validate each segment of the path.
    while (std::getline(iss, segment, '/'))
    {
        // Avoid empty segments (although the consecutive slash check already handles this).
        if (segment.empty())
            return (false);
        // Reject segments equal to "." or ".." to prevent relative path traversal.
        if (segment == "." || segment == "..")
            return (false);
        // Check if the length of each segment is less than or equal to 255 characters:
        if (segment.size() > 255)
            return (false);
    }
    return (true);
}

////// 4.B.2.- These are the methods to manage the FORBIDDEN METHODS (F_METHODS) (forbidden_methods) key:

void    ConfigParser::handleForbiddenMethods(std::istringstream &ss, LocationConfig &current_location)
{
    std::string value;
    while (ss >> value)
    {
        if (value == ";")
            break ;
        if (this->isValidMethod(value))
        {
            if (std::find(current_location.forbiddenMethods.begin(), current_location.forbiddenMethods.end(), value) == current_location.forbiddenMethods.end())
                current_location.forbiddenMethods.push_back(value);
            else
                this->writeLog("Warning: The method: " + value + "; is already in the 'forbidden_methods' list in this location block: " + current_location.path);
        }
        else
            this->writeLog("Error: Invalid value: " + value + "; for a 'forbidden_methods' key in this location block: " + current_location.path);
    }
}

bool    ConfigParser::isValidMethod(const std::string &value) const
{
    if (value == "GET" || value == "POST" || value == "DELETE" || value == "PUT" || value == "PATCH" || value == "OPTIONS" || value == "HEAD")
        return (true);
    return (false);
}

////// 4.B.3.- These are the methods to manage the ROOT (root) (location_root) key:

void    ConfigParser::handleLocationRoot(std::istringstream &ss, LocationConfig &current_location)
{
    std::string value;
    while (ss >> value)
    {
        if (value == ";")
            break ;
        if (this->isValidLocationRoot(value))
        {
            // Check that the root key is not duplicated. Otherwise, replace the value with the new one and log a warning in the log file:
            if (!(current_location.root.empty()))
                this->writeLog("Warning: The current 'location_root' key value: " + current_location.root + "; will be replaced in the location block configuration by: " + value);
            // If the root value is valid, set it:
            if (value[value.size() - 1] != '/')
                current_location.root = value;
            else
                current_location.root = value.substr(0, value.size() - 1);
        }
        else
            this->writeLog("Error: Invalid value: " + value + "; for the 'location_root' in the current location block: " + current_location.path);
    }
}

bool    ConfigParser::isValidLocationRoot(const std::string &value) const
{
    // Check that the path exists:
    struct stat buffer;
    if (stat(value.c_str(), &buffer) != 0)
        return (false);
    // Check that the path is not a "root" user directory:
    if (buffer.st_uid == 0)
        return (false);
    return (true);
}

////// 4.B.4.- These are the methods to manage the AUTO_INDEX (autoIndexOn) key:

void    ConfigParser::handleUploadEnabled(std::istringstream &ss, LocationConfig &current_location)
{
    std::string value;
    std::string token;
    while (ss >> token)
    {
        if (token == ";")
        {
            // If the value is empty, set the default value to false:
            if (value == "")
                this->writeLog("Warning: The 'autoIndexOn' key is missing in the location block: " + current_location.path + "; its current value is: " + (current_location.autoIndexOn ? "true" : "false"));
            return ;
        }
        if (token == "true" && value == "")
        {
            // If the value is "true", set the autoIndexOn to true:
            current_location.autoIndexOn = true;
            value = "true";
            while (ss >> token && token != ";")
            {
                // Skip the rest of the line until the semicolon:
            }
            return ;
        }
        else if (token == "false" && value == "")
        {
            // If the value is "false", set the autoIndexOn to false:
            current_location.autoIndexOn = false;
            value = "false";
            while (ss >> token && token != ";")
            {
                // Skip the rest of the line until the semicolon:
            }
            return ;
        }
        else
            this->writeLog("Error: Invalid value: " + token + "; for a 'autoIndexOn' key in the location block: " + current_location.path);
    }
}

////// 4.B.5.- These are the methods to manage the CGI FILES (cgi) key:

void    ConfigParser::handleCgiFiles(std::istringstream &ss, LocationConfig &current_location)
{
    std::string value;
    while (ss >> value)
    {
        if (value == ";")
        {
            if (current_location.cgi_files.empty())
                this->writeLog("Warning: The 'cgi_files' key is missing in the location block: " + current_location.path + "; its current value is empty.");
            break ;
        }
        if (this->isValidCgiFile(value))
        {
            if (this->isNotInCgiFile(value, current_location))
               current_location.cgi_files.push_back(value);
            else
                this->writeLog("Warning: The current 'cgi_file' key value: " + value + "; is duplicated in the location block:" + current_location.path + "; it will not be added again.");
        }
        else
            this->writeLog("Error: Invalid value for a 'cgi_file' key in the location block: " + current_location.path);
    }
}

bool    ConfigParser::isValidCgiFile(const std::string &value)  const
{
    // Check that the value is not empty.
    if (value.empty())
        return (false);
    // Check that it does not contain any disallowed characters.
    const std::string invalidChars = "*?|<>\"'&$#@!%()[]{};:~`";
    for (size_t i = 0; i < value.size(); ++i)
    {
        // If a disallowed character is found, return false.
        if (invalidChars.find(value[i]) != std::string::npos)
            return (false);
        // Reject control characters (non-printable).
        if (std::iscntrl(static_cast<unsigned char>(value[i])))
            return (false);
    }
    return (true);
}

bool    ConfigParser::isNotInCgiFile(const std::string &value, LocationConfig &current_location) const
{
    // Check that the CGI file is not duplicated:
    if (std::find(current_location.cgi_files.begin(), current_location.cgi_files.end(), value) == current_location.cgi_files.end())
        return (true);
    return (false);
}

// 4.B.6.- These are the methods to manage the REDIRECT (redirect) key:

void    ConfigParser::handleRedirect(std::istringstream &ss, LocationConfig &current_location)
{
    std::string value;
    while (ss >> value)
    {
        if (value == ";")
        {
            if (current_location.redirect.empty())
                this->writeLog("Error: The 'redirect' key is missing in the location block: " + current_location.path);
            return ;
        }
        if (!(this->isValidRedirect(value)))
        {
            this->writeLog("Error: Invalid value for a 'redirect' key in the server block configuration (cfg): " + value);
            while (ss >> value && value != ";")
            {
                // Skip the rest of the line until the semicolon:
            }
            return ;
        }
        // Check that the redirect key is not duplicated:
        if (!(current_location.redirect.empty()))
            this->writeLog("Warning: The current 'reirect' key value: " + current_location.redirect + "; will be replaced in the location block configuration by: " + value + "; cause it is duplicated in the configuration file (cfg): " + this->_configFilePath);
        // If the redirect value is valid, set it:
        if (value[value.size() - 1] == '/')
            value = value.substr(0, value.size() - 1); // Remove the trailing slash if it exists.
        current_location.redirect = value;
    }
}

bool    ConfigParser::isValidRedirect(const std::string &value) const
{
    // Check if the value is a valid URL or a valid path:
    if (value.empty())
        return (false);
    if (value[0] == '/')
    {
        // If it starts with a slash, it is a valid path:
        return (true);
    }
    else if (value.find("http://") == 0 || value.find("https://") == 0)
    {
        // If it starts with "http://" or "https://", it is a valid URL:
        return (true);
    }
    // If it does not start with a slash or "http://" or "https://", it is not a valid redirect:
    return (false);
    
}

// 4.B.7.- These are the methods to manage the TRY_FILES (try_files) key:

void    ConfigParser::handleTryFiles(std::istringstream &ss, LocationConfig &current_location)
{
    std::string value;
    while (ss >> value)
    {
        if (value == ";")
            return ;
        if (this->isValidTryFiles(value))
        {
            if (this->isNotInTryFiles(value, current_location))
                current_location.try_files.push_back(value);
            else
                this->writeLog("Warning: The current 'try_file' key value: " + value + "; is duplicated in the location block: " + current_location.path + "; it will not be added again.");
        }
        else
            this->writeLog("Error: Invalid value for a 'try_file' key: " + value + "; in the location block: " + current_location.path + "; it will not be added.");
    }
}

bool    ConfigParser::isValidTryFiles(const std::string &value) const
{
    // Check that the value is not empty.
    if (value.empty())
        return (false);
    // Check that it does not contain any disallowed characters.
    const std::string invalidChars = "*?|<>\"'&$#@!%()[]{};:~`";
    for (size_t i = 0; i < value.size(); ++i)
    {
        // If a disallowed character is found, return false.
        if (invalidChars.find(value[i]) != std::string::npos)
            return (false);
        // Reject control characters (non-printable).
        if (std::iscntrl(static_cast<unsigned char>(value[i])))
            return (false);
    }
    return (true);
}

bool    ConfigParser::isNotInTryFiles(const std::string &value, LocationConfig &current_location) const
{
    // Check that the CGI file is not duplicated:
    if (std::find(current_location.cgi_files.begin(), current_location.cgi_files.end(), value) == current_location.cgi_files.end())
        return (true);
    return (false);
}

// ---------------------------------------------------------------------------------------------------------------

// THESE ARE METHODS TO MANAGE THE LOG FILE:

bool    ConfigParser::isLogFilePathValid(const std::string &logPathName) const
{
    struct stat buffer;
    if (stat(logPathName.c_str(), &buffer) != 0)
    {
        if (mkdir(logPathName.c_str(), 0755) != 0)
            return (false);
        // std::cout << "Log directory created: " << logPathName << std::endl;
        return (true);
    }
    if (!S_ISDIR(buffer.st_mode))
        return (false);
    if (access(logPathName.c_str(), W_OK) != 0)
        return (false);
    return (true);
}

bool    ConfigParser::isLogFileWritable(const std::string &logFileName) const
{
    // 1st.- Check if the file doesn't exists, I have to create it:
    std::ifstream file(logFileName.c_str());
    if (!file)
    {
        std::ofstream newFile(logFileName.c_str());
        if (!newFile)
            return (false);
        newFile.close();
        return (true);
    }
    // 2nd.- If the file exists, check if it is writable:
    if (access(logFileName.c_str(), W_OK) != 0)
        return (false);
    return (true);
}

void    ConfigParser::writeLog(const std::string &message)
{
    std::ofstream logFile((this->_logPathName + this->_logFileName).c_str(), std::ios::app);
    if (logFile.is_open())
    {
        // Get the current time:
        std::time_t now = std::time(0);
        std::tm *tm = std::localtime(&now);
        // Format the time:
        std::ostringstream oss;
        oss << "[" << 1900 + tm->tm_year << "-";
        if (tm->tm_mon < 10)
            oss << "0"; // Add leading zero if month is less than 10
        oss << 1 + tm->tm_mon << "-";
        if (tm->tm_mday < 10)
            oss << "0"; // Add leading zero if day is less than 10
        oss << tm->tm_mday << " - ";
        if (tm->tm_hour < 10)
            oss << "0"; // Add leading zero if hour is less than 10
        oss << tm->tm_hour << ":";
        if (tm->tm_min < 10)
            oss << "0"; // Add leading zero if minute is less than 10
        oss << tm->tm_min << ":";
        if (tm->tm_sec < 10)
            oss << "0"; // Add leading zero if second is less than 10
        oss << tm->tm_sec << "] - ";
        // Write the time and message to the log file:
        logFile << oss.str() << message << std::endl;
        logFile.close();
    }
    else
        std::cerr << "Error: Unable to open log file." << std::endl;
    // Increment the log entries counter if the "message" variable contents: "-------- Parsing server block: ":
    if (message.find("Error: ") != std::string::npos || message.find("Warning: ") != std::string::npos)
        this->_writeLogEntries++;
}

std::string ConfigParser::getWordCount(const std::string &configFileSingleString)
{
    std::istringstream iss(configFileSingleString);
    std::string word;
    std::string result;
    ssize_t count = 0;
    while (iss >> word && count < this->_wordCount)
    {
        count++;
        // Get the last 5 words before the error:
        if (count > this->_wordCount - 5 && !word.empty())
        {
            if (!result.empty())
                result += " "; // Add a space before the word if result is not empty.
            result += word; // Append the word to the result.
        }
    }
    return (result);
}

// THESE ARE PUBLIC METHODS:

ConfigParser::ConfigParser(const std::string &configFilePath) : _configFilePath(configFilePath), _logPathName("./logs/"), _logFileName("config.log"), _writeLogEntries(0), _serverNumber(0), _wordCount(0), isValidCFG(true)
{
    try
    {
        if (!(this->servers.empty()))
            this->servers.clear();
        // Check if the log directory exists, is writable and the log file is inside it:
        if (!this->isLogFilePathValid(_logPathName))
            throw (ConfigParser::ConfigParserException("Error: IMPOSSIBLE ACCESS TO CFG-LOG DIRECTORY. Please, check the permissions."));
        if (!this->isLogFileWritable(_logPathName + _logFileName))
            throw (ConfigParser::ConfigParserException("Error: IMPOSSIBLE ACCESS TO CFG-LOG FILE. Please, check the permissions."));
        if (!this->isConfigFilePathValid(configFilePath))
            throw (ConfigParser::ConfigParserException("Error: IMPOSSIBLE ACCESS TO CFG FILE. Please, check the permissions."));
        this->_configFileSingleString = this->configFileToString(configFilePath);
        if (!(this-> checkConfigFileStructure(this->_configFileSingleString)))
        {
            this->writeLog(this->_firstCheckError + " Please, check the syntax in the server block number: " + this->to_string(this->_serverNumber) + "; close to the word number: " + this->to_string(this->_wordCount) + "; or: '" + this->getWordCount(this->_configFileSingleString) + "'.");
            throw (ConfigParser::ConfigParserException("Error: THE CFG FILE SYNTAX IS NOT VALID. Please, check the config.log file for more details."));
        }
        this->_serverNumber = 1;
        this->checkConfigFileValues(this->_configFileSingleString);
        if (this->isValidCFG && this->_writeLogEntries == 0)
            std::cout << "Config file successfully parsed." << std::endl;
        else if (this->isValidCFG && this->_writeLogEntries > 0)
            std::cout << "Config file parsed with errors. Please, check the 'config.log' file." << std::endl;
        else if (!this->isValidCFG)
            std::cerr << "Config file parsed with errors/warnings. Please, check the 'config.log' file." << std::endl;
    }
    catch (const ConfigParser::ConfigParserException &exception)
    {
        std::cerr << exception.what() << std::endl;
        this->isValidCFG = false;
    }
}

ConfigParser::~ConfigParser(void)
{
}

std::vector<ServerConfig>    ConfigParser::getServers() const
{
    return (this->servers);
}

void    ConfigParser::printConfig(const std::vector<ServerConfig>	&servers) const
{
    std::cout << "Config File Path: " << this->_configFilePath << std::endl;
    // std::cout << "Config File Content: " << this->_configFileSingleString << std::endl;
    if (servers.empty())
        std::cout << "No SERVER found in the config file." << std::endl;
    else
    {
        std::cout << "This is the configuration file content: " << std::endl;
        std::cout << this->_configFileSingleString << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Number of servers: " << servers.size() << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        for (size_t i = 0; i < servers.size(); ++i)
        {
            std::cout << "Server " << i + 1 << ":" << std::endl;
            std::cout << "----------------------------------------" << std::endl;
            // Print the LISTEN directive:
            if (servers[i].listen.empty())
                std::cout << "No LISTEN directive found.";
            else
            {
                std::cout << "Listen: ";
                for (std::map<in_addr_t, std::vector<int> >::const_iterator it = servers[i].listen.begin(); it != servers[i].listen.end(); ++it)
                {
                    // Print the IP address:
                    std::cout << it->first << ":";
                    // Print the ports assigned to the IP address:
                    for (std::vector<int>::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
                        std::cout << *it2 << " ";
                }
            }
            std::cout << std::endl;
            // Print the SERVER_NAME directive:
            if (servers[i].server_names.empty())
                std::cout << "No SERVER_NAME directive found.";
            else
            {
                std::cout << "Server Name: ";
                for (std::vector<std::string>::const_iterator it = servers[i].server_names.begin(); it != servers[i].server_names.end(); ++it)
                    std::cout << *it << " ";
            }
            std::cout << std::endl;
            // Print the PASSWORD directive:
            if (servers[i].password.empty())
                std::cout << "No PASSWORD directive found.";
            else
                std::cout << "Password: " << servers[i].password;
            std::cout << std::endl;
            // Print the LOGIN_ON directive:
            if (servers[i].loginOn == false)
                std::cout << "Login On: false";
            else
                std::cout << "Login On: true";
            std::cout << std::endl;
            // Print the ROOT directive:
            if (servers[i].root.empty())
                std::cout << "No ROOT directive found.";
            else
                std::cout << "Root: " << servers[i].root;
            std::cout << std::endl;
            // Print the CLIENT_MAX_BODY_SIZE directive:
            if (servers[i].client_max_body_size == -1)
                std::cout << "No CLIENT_MAX_BODY_SIZE directive found.";
            else
                std::cout << "Client Max Body Size: " << servers[i].client_max_body_size;
            std::cout << std::endl;
            // Print the ERROR_PAGES directive:
            if (servers[i].error_pages.empty())
                std::cout << "No ERROR_PAGES directive found." << std::endl;
            else
            {
                std::cout << "Error Pages: " << std::endl;
                for (std::map<std::string, std::string>::const_iterator it = servers[i].error_pages.begin(); it != servers[i].error_pages.end(); ++it)
                    std::cout << "\t" << it->first << ": " << it->second << std::endl;
            }
            // Print the INDEX directive:
            if (servers[i].index.empty())
                std::cout << "No INDEX directive found.";
            else
            {
                std::cout << "Index: ";
                for (std::vector<std::string>::const_iterator it = servers[i].index.begin(); it != servers[i].index.end(); ++it)
                    std::cout << *it << " ";
            }
            std::cout << std::endl;
            // Print the LOCATIONS directive:
            if (servers[i].locations.empty())
                std::cout << "No LOCATIONS directive found." << std::endl;
            else
            {
                for (std::map<std::string, LocationConfig>::const_iterator it = servers[i].locations.begin(); it != servers[i].locations.end(); ++it)
                {
                    std::cout << "-----------------------------------------" << std::endl;
                    std::cout << "Location Path: " << it->first << std::endl;
                    if (!(it->second.root.empty()))
                        std::cout << "Location Root: " << it->second.root << std::endl;
                    if (it->second.autoIndexOn == false)
                        std::cout << "autoIndexOn: false" << std::endl;
                    else
                        std::cout << "autoIndexOn: true" << std::endl;;
                    if (!(it->second.cgi_files.empty()))
                    {
                        std::cout << "Cgi Files: ";
                        for (std::vector<std::string>::const_iterator it2 = it->second.cgi_files.begin(); it2 != it->second.cgi_files.end(); ++it2)
                            std::cout << *it2 << " ";
                        std::cout << std::endl;
                    }
                    if (!(it->second.forbiddenMethods.empty()))
                    {
                        std::cout << "Forbidden Methods: ";
                        for (std::vector<std::string>::const_iterator it2 = it->second.forbiddenMethods.begin(); it2 != it->second.forbiddenMethods.end(); ++it2)
                            std::cout << *it2 << " ";
                        std::cout << std::endl;
                    }
                    if (!(it->second.redirect.empty()))
                        std::cout << "Redirect: " << it->second.redirect << std::endl;
                    if (!(it->second.try_files.empty()))
                    {
                        std::cout << "Try Files: ";
                        for (std::vector<std::string>::const_iterator it2 = it->second.try_files.begin(); it2 != it->second.try_files.end(); ++it2)
                            std::cout << *it2 << " ";
                        std::cout << std::endl;
                    }
                    std::cout << "-----------------------------------------" << std::endl;
                }
            }
            std::cout << "----------------------------------------" << std::endl;
        }
    }
}
