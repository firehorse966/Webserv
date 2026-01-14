/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aiturria <aiturria@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/25 16:00:02 by aiturria          #+#    #+#             */
/*   Updated: 2025/06/10 14:56:28 by aiturria         ###   ########.fr       */
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
# include "Logs.hpp"
# include "Sockets.hpp"
# include "ConfigParser.hpp"

# define CHUNK 4096
# define READ 4096
# define MAX_SIZE 1024 * 1024

extern std::string errortemplate;
extern std::string errorstyles;

class Response {
	public:
		
		static void errorResponse(ClientInfo &client);
		static std::string buildResponse(ClientInfo &client);
		static void sendData(ClientInfo &client);
		static void sendChunkedData(ClientInfo &client);
		
};