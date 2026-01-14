/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Methods.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aiturria <aiturria@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/03/13 09:14:42 by aiturria          #+#    #+#             */
/*   Updated: 2025/06/10 14:41:06 by aiturria         ###   ########.fr       */
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
# include <sys/stat.h>
# include <unistd.h>
# include <fcntl.h>
# include <dirent.h>
# include "Logs.hpp"
# include "Sockets.hpp"
# include "Response.hpp"
# include "Requests.hpp"
# include "ConfigParser.hpp"

class Methods {
	
	public:
		
		static std::map<std::string, std::string> mime_types;
		static void decideMethod(ClientInfo &client);
		static void getMethod(ClientInfo &client);
		static std::string lastFolder(ClientInfo &client);
		static int locations(std::string folder, ClientInfo &client);
		static int findResource(ClientInfo &client);
		static std::string getFullPath(ClientInfo &client, int check);
		static void optionsMethod(ClientInfo &client);
		static int handlecgi(ClientInfo &client);
		static int getCgi(ClientInfo &cgi);
		static int postCgi(ClientInfo &cgi);
		static void setCgiEnvironment(ClientInfo &client);
		static void writeToCgi(ClientInfo &cgi);
		static void readFromCgi(ClientInfo &cgi);
		static void mimeTypes(void);
		static void bigFiles(ClientInfo &client);
		static void readFileToPipe(ClientInfo &client);
		static void pipeToClient(ClientInfo &client);
		static void deleteMethod(ClientInfo &client);
		static void autoindexOn(ClientInfo &client, std::string &dirPath);
		
};