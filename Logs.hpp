/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logs.hpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aiturria <aiturria@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/02/20 14:30:35 by aiturria          #+#    #+#             */
/*   Updated: 2025/06/11 16:07:35 by aiturria         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

# include <iostream>
# include <sys/stat.h>
# include <fstream>
# include <string>
# include <sstream>
# include <cstdlib>
# include <map>
# include <ctime>
# include <utility>
# include <iomanip>

class Logs {
	private:
		std::ofstream _access_log;
		std::ofstream _error_log;
	
	public:
		Logs(void);
		~Logs(void);
		void errorLog(const std::string &error);
		void accessLog(const std::string &access);
		std::string getCurrentTime(int date);
		std::string intToString(int value);

};

