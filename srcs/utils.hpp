#ifndef MYUTILS_HPP
#define MYUTILS_HPP

#include <string>
#include <memory>
#include <list>
#include "defines.hpp"
#include <vector>

namespace utils
{
	bool	strMatch(const std::string &filter, const std::string &str);
	bool	strMatchToLower(const std::string &filter, const std::string &str);
	bool	strCmp(const std::string &str1, const std::string &str2);
	std::string	ircLowerCase(const std::string &str);
	std::string	ircUpperCase(const std::string &str);

	int isalpha( int ch );
	int isdigit( int ch );
	int isalnum( int ch );

	void* memset( void* dest, int ch, std::size_t count );

	std::string	delFirstWord(const std::string &str);
	
	std::string	strJoin(const std::vector<std::string> &vec, char sep);
	std::string	getNickFromMask(const std::string &fullmask);
	bool		strIsNum(const std::string &str);
}

#endif