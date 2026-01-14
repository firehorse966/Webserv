/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mbolano- <mbolano-@student.42malaga.com>   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/19 06:57:02 by mbolano-          #+#    #+#             */
/*   Updated: 2025/06/10 07:27:48 by mbolano-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <iostream>
#include <ctime>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <exception>
#include <map>
#include <string>
#include <arpa/inet.h>
#include <sys/stat.h>


// All these methods are authorized except those ones that specifically appear in 
// any location block as forbidden_methods.

typedef enum {
	GET,
	POST,
	DELETE,
	PUT
} server_method;

typedef enum {
	LISTEN,
	NAME,
	PASSWORD,
	LOGIN_ON,
	SERVER_ROOT,
	INDEX,
	ERROR,
	SIZE,
	LOCATION,
	CLOSE_SERVER_BRACKET,
	UNKNOWN_SERVER_KEY
}	server_key;

typedef enum {
	F_METHODS,
	AUTO_INDEX,
	CGI_FILES,
	LOCATION_ROOT,
	REDIRECT,
	TRY_FILES,
	CLOSE_LOCATION_BRACKET,
	UNKNOWN_LOCATION_KEY
}	location_key;

struct	LocationConfig
{
	std::string								path;
	std::string								root;
	std::vector<std::string>				forbiddenMethods;
	bool									autoIndexOn;
	std::vector<std::string>				cgi_files;
	std::string								redirect;
	std::vector<std::string>				try_files;
};

struct	ServerConfig
{
	std::map<in_addr_t, std::vector<int> >	listen;
	std::string								password;
	bool									loginOn;
	std::vector<std::string>				server_names;
	std::string								root;
	std::vector<std::string>				index;
	std::map<std::string, std::string>		error_pages;
	ssize_t									client_max_body_size;
	std::map<std::string, LocationConfig>	locations;
};

class	ConfigParser
{
	private:
		std::string							_configFilePath;
		std::string							_configFileSingleString;
		std::string							_logPathName;
		std::string							_logFileName;
		std::string							_firstCheckError;
		size_t								_writeLogEntries;
		size_t								_serverNumber;
		ssize_t								_wordCount;

		// These are common methods:
		std::string							to_string(ssize_t value)													const;
		std::string							getWordCount(const std::string &configFileSingleString);

		// These are methods to manage the LOG FILE:
		bool 								isLogFilePathValid(const std::string &logFilePath)							const;
		bool								isLogFileWritable(const std::string &logFilePath)							const;
		void								writeLog(const std::string &message);

		// These are common methods for the SEVER BLOCK:
		server_key							stringToServerKey(const std::string &key)									const;

		// These are common methods for the LOCATION BLOCK:
		location_key						stringToLocationKey(const std::string &key)									const;

		// 1st.- This is the method to check if the CONFIG FILE (cfg) path is valid:
		bool								isConfigFilePathValid(const std::string &configFilePath)    				const;

		// 2nd.- This is the method to read the CONFIG FILE (cfg) content and convert it to a string:
		std::string							configFileToString(const std::string &configFilePath)						const;

		// 3rd.- These are the methods to check the CONFIG FILE (cfg) structure:
		bool								checkConfigFileStructure(const std::string &cfg);

		//// These are the methods to check the SERVER BLOCK (serverStream) structure:
		bool								parseServerBlock(std::istringstream &serverStream);
		bool								wordInServerKeyValues(const std::string &word)								const;
		bool								parseServerDirective(std::istringstream &keyStream);

		//// These are the methods to check the LOCATION BLOCK (locationStream) structure:
		bool								parseLocationBlock(std::istringstream &serverStream);
		bool								wordInLocationKeyValues(const std::string &word)							const;
		bool								parseLocationDirective(std::istringstream &keyStream);

		// 4th.- These are methods to check de CONFIG FILE (cfg) values:
		void								checkConfigFileValues(const std::string &cfg);

		//// 4.A.- These are the methods to check the SERVER BLOCK (serverStream) values:
		void								parseServerValues(std::istringstream &serverStream);
		void								initServerBlock(ServerConfig &current_server);
		void 								checkEmptyServerKeys(ServerConfig &current_server);

		////// 4.A.1.- These are the methods to manage the LISTEN (listen) key:
		void								handleListen(std::istringstream &ss, ServerConfig &current_server);
		bool								isValidIPAndPort(const std::string &value)									const;
		bool								isValidPort(const std::string &value)										const;
		bool								isValidIP(const std::string &value)											const;
		void								addIPAndPortToServer(const std::string &value, ServerConfig &current_server);
		std::string							getIPValue(const std::string &value)										const;

		////// 4.A.2.- These are the methods to manage the NAME (server_name) key:
		void								handleServerName(std::istringstream &ss, ServerConfig &current_server);
		bool								isValidServerName(const std::string &value)									const;

		////// 4.A.3.- These are the methods to manage the ROOT (root) key:
		void								handleRoot(std::istringstream &ss, ServerConfig &current_server);
		bool								isValidRoot(const std::string &value);
		
		////// 4.A.4.- These are the methods to manage the ERROR PAGE (error_page) key:
		void								handleErrorPage(std::istringstream &ss, ServerConfig &current_server);
		bool								isValidErrorCode(const std::string &error_code)   							const;
		bool    							isValidErrorPage(const std::string &value, ServerConfig &current_server)	const;

		////// 4.A.5.- These are the methods to manage the INDEX (index) key:
		void								handleIndex(std::istringstream &ss, ServerConfig &current_server);
		
		////// 4.A.6.- These are the methods to manage the SIZE (client_max_body_size) key:
		void								handleClientMaxBodySize(std::istringstream &ss, ServerConfig &current_server);
		bool								isValidBodySize(const std::string &value)									const;
		bool								isPositiveInteger(const std::string &value)									const;
		ssize_t								getBodySizeBytes(const std::string &value)									const;
		////// 4.A.7.- These are the methods to manage the PASSWORD (password) key:
		void								handlePassword(std::istringstream &ss, ServerConfig &current_server);
		////// 4.A.8.- These are the methods to manage the LOGIN ON (loginOn) key:
		void								handleLoginOn(std::istringstream &ss, ServerConfig &current_server);

		//// 4.B.- These are the methods to manage the LOCATION BLOCK (locationStream) values:
		void								handleLocation(std::istringstream &ss, ServerConfig &current_server);
		void 								initLocationBlock(LocationConfig &current_location);
		
		////// 4.B.1.- These are the methods to manage the PATH (path) key:
		std::string							handleLocationPath(std::istringstream &ss);
		bool								isValidLocationPath(const std::string &path)								const;
		
		////// 4.B.2.- These are the methods to manage the FORBIDDEN METHODS (forbidden_methods) key:
		void								handleForbiddenMethods(std::istringstream &ss, LocationConfig &current_location);
		bool								isValidMethod(const std::string &value)										const;
		
		////// 4.B.3.- These are the methods to manage the LOCATION ROOT (root) key:
		void								handleLocationRoot(std::istringstream &ss, LocationConfig &current_location);
		bool								isValidLocationRoot(const std::string &value)								const;
		
		////// 4.B.4.- These are the methods to manage the UPLOAD ENABLE (upload_enabled) key:
		void								handleUploadEnabled(std::istringstream &ss, LocationConfig &current_location);

		////// 4.B.5.- These are the methods to manage the CGI FILES (cgi_files) key:
		void								handleCgiFiles(std::istringstream &ss, LocationConfig &current_location);
		bool								isValidCgiFile(const std::string &value)									const;
		bool								isNotInCgiFile(const std::string &value, LocationConfig &current_location) const;
		////// 4.B.6.- These are the methods to manage the REDIRECT (redirect) key:
		void								handleRedirect(std::istringstream &ss, LocationConfig &current_location);
		bool								isValidRedirect(const std::string &value)									const;
		////// 4.B.7.- These are the methods to manage the TRY FILES (try_files) key:
		void								handleTryFiles(std::istringstream &ss, LocationConfig &current_location);
		bool								isValidTryFiles(const std::string &value)									const;
		bool								isNotInTryFiles(const std::string &value, LocationConfig &current_location) const;

	public:
		static std::vector<ServerConfig>	servers;
		bool								isValidCFG;
											ConfigParser(const std::string &configFile);
											~ConfigParser(void);
		std::vector<ServerConfig>			getServers()																	const;							
		void								printConfig(const std::vector<ServerConfig>	&servers)							const;

	class ConfigParserException: public std::exception
	{
		private:
			std::string						_error;
		public:
											ConfigParserException(const std::string &error) : _error(error) {}
											~ConfigParserException(void)														throw() {}
			virtual const char				*what(void)																	const	throw()
			{
				return (this->_error.c_str());
			}
	};
};