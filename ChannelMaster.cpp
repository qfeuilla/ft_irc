#include "ChannelMaster.hpp"
ChannelMaster::ChannelMaster(const std::string &srvName): _srv_name(srvName) {
	_channels = new _channel_list();
	_user_channels = new _user_chan_map();
}
ChannelMaster::~ChannelMaster() {
	_channel_list::iterator	current = _channels->begin();
	_channel_list::iterator	end = _channels->end();
	Channel					*chan;

	while (current != end) {
		chan = *current;
		++current;
		delete chan;
	}
	delete _channels;
	delete _user_channels;
}
bool	ChannelMaster::_testChannelName(Client *client, const std::string &channelName)
{
	bool badName = false;
	std::string	ms;

	if (channelName.at(0) != '#' && channelName.at(0) != '&' && channelName.at(0) != '+' && channelName.at(0) != '!')
		badName = true;
	if (channelName.length() > 50)
		badName = true;
	if (channelName.find(" ") != std::string::npos)
		badName = true;
	if (channelName.find(",") != std::string::npos)
		badName = true;
	if (channelName.find("\7") != std::string::npos)
		badName = true;
	if (badName) {
		(void)client;
		return (true);
	}
	return (false); // channelName is well formatted
}

std::vector<std::string>	splitComma(std::string const &str)
{
	std::vector<std::string>	ret;
	size_t startPos;
	size_t endPos = 0;
	std::string		chunk;

	while ((startPos = str.find_first_not_of(',', endPos)) != std::string::npos)
	{
		endPos = str.find(',', startPos);
		chunk = str.substr(startPos, endPos - startPos);
		ret.push_back(chunk);
	}
	return (ret);
}


bool	ChannelMaster::join(Client *client, const std::vector<std::string> &args, std::list<Channel*> *usrChans)
{
	std::map<std::string, std::string>	name_pass_map;
	std::map<std::string, std::string>::iterator	current;
	std::map<std::string, std::string>::iterator	end;
	std::vector<std::string>	names;
	std::vector<std::string>	passwds;
	bool	ret = true;

	(*_user_channels)[client->nick] = usrChans;
	names = splitComma(args[0]);
	if (args.size() > 1)
		passwds = splitComma(args[1]);
	for (size_t i = 0; i < names.size(); ++i) {
		name_pass_map[names[i]] = "";
	}
	for (size_t i = 0; i < passwds.size(); ++i) {
		name_pass_map[names[i]] = passwds[i];
	}
	current = name_pass_map.begin();
	end = name_pass_map.end();
	while (current != end) {
		ret &= _join_channel(client, (*current).first, (*current).second);
		++current;
	}
	return (ret);
}

bool	ChannelMaster::_join_channel(Client *client, const std::string &channelName, const std::string &passwd)
{
	Channel	*chan = _chan_exists(channelName);
	if (!chan) {
		if (_testChannelName(client, channelName))
			return (false);
		Channel	*nchan = new Channel(channelName, client, _srv_name);
		_channels->push_back(nchan);
		(*_user_channels)[client->nick]->push_back(nchan);
		return (true);
	}
	if (chan->join(client, passwd)) {
		(*_user_channels)[client->nick]->push_back(chan);
		return (true);
	}
	return (false);
}

bool	ChannelMaster::_leave_channel(Client *client, const std::string &channelName, const std::string &reason)
{
	Channel	*chan = _chan_exists(channelName);
	std::string		ms;

	if (!chan) {
		ms = reply_formating(client->servername.c_str(), ERR_NOSUCHCHANNEL, std::vector<std::string>({channelName}), client->nick.c_str());
		return (!custom_send(ms, client));
	}
	if (chan->leave(client, reason)) {
		(*_user_channels)[client->nick]->remove(chan); // remove this chan from this user channels list 
		if (!chan->isEmpty())
			return (true);
		_channels->remove(chan); // channel is empty -> we delete it
		delete chan;
		return (true);
	}
	return (false);
}

bool	ChannelMaster::leave(Client *client, const std::vector<std::string> &args)
{
	bool	ret = true;
	std::vector<std::string>	names = splitComma(args[0]);
	std::string		reason = args.size() > 1 ? Channel::parseArg(1, args) : std::string();
	for (size_t i = 0; i < names.size(); ++i) {
		ret &= _leave_channel(client, names[i], reason);
	}
	return (ret);
}

bool	ChannelMaster::getChanModes(Client *client, const std::vector<std::string> &args)
{
	Channel		*chan;
	std::string	ms;
	std::string	modes;

	chan = _chan_exists(args[0]);
	if (!chan)
		return (false);
	modes = chan->getModes();
	std::vector<std::string>	params({chan->getName(), modes, ""});
	ms = reply_formating(client->servername.c_str(), RPL_CHANNELMODEIS, params, client->nick.c_str());
	return (custom_send(ms, client));
}

bool	ChannelMaster::mode(Client *client, const std::vector<std::string> &args)
{
	Channel					*chan;
	std::string				operations = args[1]; // looks like "+o" 
	bool					append;

	chan = _chan_exists(args[0]);
	
	if (!(operations[0] == '+' || operations[0] == '-'))
		return (false);
	append = operations[0] == '+';
	if (!chan)
		return (false);
	for (size_t i = 1; i < operations.size(); ++i) {
		switch (operations[i])
		{
		case 'o':
			if (args.size() < 3)
				return (false);
			chan->mode_o(append, client, args[2]);
			break;
		case 'v':
			if (args.size() < 3)
				return (false);
			chan->mode_v(append, client, args[2]);
			break;
		case 'p':
			chan->mode_p(append, client);
			break;
		case 's':
			chan->mode_s(append, client);
			break;
		case 'i':
			chan->mode_i(append, client);
			break;
		case 't':
			chan->mode_t(append, client);
			break;
		case 'm':
			chan->mode_m(append, client);
			break;
		case 'n':
			chan->mode_n(append, client);
		case 'q':
			chan->mode_q(append, client);
			break;
		case 'l':
			if (args.size() == 2) {
				if (!append)
					chan->mode_l(append, client, -1);
			} else
				chan->mode_l(append, client, std::stoi(args[2]));
			break;
		case 'k':
			if (args.size() == 2) {
				if (!append) 
					chan->mode_k(append, client, "");
			} else
				chan->mode_k(append, client, args[2]);
			break;
		default:
			break;
		}
	}
	return (true);
}

Channel		*ChannelMaster::_chan_exists(const std::string &channelName)
{
	_channel_list::iterator	current = _channels->begin();
	_channel_list::iterator	end = _channels->end();

	while (current != end) {
		if (utils::strCmp(channelName, (*current)->getName()))
			return (*current);
		++current;
	}
	return (nullptr);
}

bool	ChannelMaster::kick(Client *client, const std::vector<std::string> &args)
{
	std::string	ms;
	Channel		*channel = _chan_exists(args[0]);
	std::string	guyToKick = args[1];
	std::string	reason = args.size() > 2 ? Channel::parseArg(2, args) : std::string();
	if (!channel) {
		ms = reply_formating(client->servername.c_str(), ERR_NOSUCHCHANNEL, std::vector<std::string>({args[0]}), client->nick.c_str());
		return (!custom_send(ms, client));
	}
	if (channel->kick(client, guyToKick, reason)) {
		(*_user_channels)[client->nick]->remove(channel);
		return (true);
	}
	return (false);
}


bool	ChannelMaster::broadcastMsg(Client *client, const std::string &chanName, const std::vector<std::string> &args, bool sendErrors)
{
	std::string	ms;
	Channel		*channel = _chan_exists(chanName);
	std::string	msgToSend = Channel::parseArg(1, args);

	if (!channel) {
		ms = reply_formating(client->servername.c_str(), ERR_NOSUCHCHANNEL, {chanName}, client->nick.c_str());
		if (sendErrors)
			custom_send(ms, client);
		return (false);
	}
	if (channel->getModeN() && !channel->isInChan(client->nick)) {
		ms = reply_formating(client->servername.c_str(), ERR_CANNOTSENDTOCHAN, {chanName}, client->nick.c_str());
		if (sendErrors)
			custom_send(ms, client);
		return (false);
	}
	if (channel->msgErrors(client, sendErrors))
		return (false);
	ms = ":" + client->nick + "!" + client->username + "@" + client->servername;
	ms += sendErrors ? " PRIVMSG " : " NOTICE ";
	ms += channel->getName() + " :" + msgToSend;
	ms += CRLF;
	return (channel->broadcastMsg(client, ms));
}

bool	ChannelMaster::topic(Client *client, const std::vector<std::string> &args)
{
	std::string	ms;
	std::string	newTopic;
	std::string	topicIs;

	Channel		*channel = _chan_exists(args[0]);

	if (!channel) {
		ms = reply_formating(client->servername.c_str(), ERR_NOSUCHCHANNEL, std::vector<std::string>({args[0]}), client->nick.c_str());
		return (!custom_send(ms, client));
	}
	if (args.size() == 1) {
		topicIs = channel->getTopic();
		if (topicIs == "") {
			ms = reply_formating(client->servername.c_str(), RPL_NOTOPIC, std::vector<std::string>({channel->getName()}), client->nick.c_str());
			return (custom_send(ms, client));
		}
		ms = reply_formating(client->servername.c_str(), RPL_TOPIC, std::vector<std::string>({channel->getName(), topicIs}), client->nick.c_str());
		return (custom_send(ms, client));
	}
	newTopic = Channel::parseArg(1, args);
	return (channel->setTopic(client, newTopic));
}

bool	ChannelMaster::invite(Client *client, const std::vector<std::string> &args)
{
	std::string	ms;
	std::string	userName = args[0];
	Channel		*channel = _chan_exists(args[1]);

	if (!channel) {
		ms = reply_formating(client->servername.c_str(), ERR_NOSUCHCHANNEL, std::vector<std::string>({args[1]}), client->nick.c_str());
		return (!custom_send(ms, client));
	}
	return (channel->invite(client, userName));
}

void	ChannelMaster::setSrvName(const std::string &srvName)
{
	_srv_name = std::string(srvName);
}

// USER mayo 0 * :realname (pour se connecter avec nc)
bool	ChannelMaster::list(Client *client, const std::vector<std::string> &args)
{
	_channel_list::iterator	current = _channels->begin();
	_channel_list::iterator	end = _channels->end();
	std::string	chanName;
	std::string	numUsersVisible;
	std::string	topic;

	// :chatjunkies.org 321 adwonno Channel :Users Name
	std::string	ms;
	ms = ":" + client->servername + " 321 " + client->nick + " Channel :Users Name" + CRLF;
	custom_send(ms, client);

	// :chatjunkies.org 322 adwonno #linuxdojo 10 :[+nt]
	// :chatjunkies.org 322 adwonno #trax 7 :[+nt] #trax museum : bring back the only demo art
	while (current != end) {
		chanName = (*current)->getName();
		numUsersVisible = (*current)->getUsersNum();
		topic = "[" + (*current)->getModes() + "]";
		if ((*current)->getTopic() != "") {
			topic += " " + (*current)->getTopic();
		}
		std::vector<std::string> params({chanName, numUsersVisible, topic});
		ms = reply_formating(client->servername.c_str(), RPL_LIST, params, client->nick.c_str());
		if (args.size() < 1 || utils::strMatch(args[0], chanName))
			custom_send(ms, client);
		++current;
	}

	// :chatjunkies.org 323 adwonno :End of channel list
	ms = ":" + client->servername + " 323 " + client->nick + " :End of channel list" + CRLF;
	custom_send(ms, client);
	return (true);
}

size_t	ChannelMaster::size() const
{
	_channel_list::const_iterator	current = _channels->begin();
	_channel_list::const_iterator	end = _channels->end();
	size_t	size = 0;

	while (current != end) {
		++current;
		++size;
	}
	return (size);
}
