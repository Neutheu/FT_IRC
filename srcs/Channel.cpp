#include "Channel.hpp"
#include "Client.hpp"
#include "numerics.hpp"

Channel::Channel(const std::string &name): _name(name) {}

const std::string	&Channel::getName()
{
	return (_name);
}

const std::string	&Channel::getKey()
{
	return (_key);
}

bool	Channel::getInviteOnly()
{
	return _inviteOnly;
}

void	Channel::setInviteOnly()
{
	_inviteOnly = !_inviteOnly;
}

void	Channel::setKey(const std::string &key)
{
	_key = key;
}

std::string	Channel::generateUserList()
{
	std::string	userList;

	for (size_t i = 0; i < channelOperators.size(); ++i)
	{
		userList = userList + "@" + channelOperators[i]->getNickname();
		if (i != channelOperators.size() - 1)
			userList + " ";
	}
	for (size_t i = 0; i < channelClients.size(); ++i)
	{
		if (std::find(channelOperators.begin(), channelOperators.end(), channelClients[i]) ==  channelOperators.end())
			userList = userList + " " + channelClients[i]->getNickname();
	}
	return userList;
}

void	Channel::replySuccessfullJoin(Client *client)
{
	client->reply(JOIN(client->getNickname(), client->getUsername(), _name));
	if (!_channelTopic.empty())
	{
		client->reply(RPL_TOPIC(client->getNickname(), _name, _channelTopic));
		client->reply(RPL_TOPICWHOTIME(client->getNickname(), _name, _topicCreator, _topicCreationTime));
	}
	client->reply(RPL_NAMREPLY(client->getNickname(), _name, generateUserList()));
	client->reply(RPL_ENDOFNAMES(client->getNickname(), _name));
}

void	Channel::addChannelClient(Client *client)
{
	std::vector<Client*>::iterator it = std::find(channelClients.begin(), channelClients.end(), client);
	if (it != channelClients.end())
		return ;
	channelClients.push_back(client);
	replySuccessfullJoin(client);
}

void	Channel::addChannelOperator(Client *client)
{
	std::vector<Client*>::iterator it = std::find(channelOperators.begin(), channelOperators.end(), client);
	if (it != channelOperators.end())
		return ;
	channelOperators.push_back(client);
	std::cout << "Added channel Operator " << client->getNickname() << std::endl;
}

Client*		Channel::getChannelOperator(const std::string& nickname)
{
	for (std::vector<Client*>::iterator it = channelOperators.begin(); it != channelOperators.end(); ++it)
	{
		if ((*it)->getNickname() == nickname)
			return *it;
	}
	return NULL;
}

Client*		Channel::getChannelClient(const std::string& nickname)
{
	for (std::vector<Client*>::iterator it = channelClients.begin(); it != channelClients.end(); ++it)
	{
		if ((*it)->getNickname() == nickname)
			return *it;
	}
	return NULL;
}

std::string		Channel::getChannelTopic()
{
	return this->_channelTopic;
}

std::string		Channel::getTopicCreator()
{
	return this->_topicCreator;
}

std::string		Channel::getTopicCreationTime()
{
	return this->_topicCreationTime;
}

bool		Channel::getModeT()
{
	return this->_modeT;
}

void		Channel::setModeT()
{
	this->_modeT = !_modeT;
}

void		Channel::setChannelTopic(const std::string& topic)
{
	this->_channelTopic = topic;
}

void		Channel::setTopicCreator(const std::string& creator)
{
	this->_topicCreator = creator;
}

void		Channel::setTopicCreationTime(const std::string& creationTime)
{
	std::cout << "creationTime :" << creationTime << std::endl;
	this->_topicCreationTime = creationTime;
}

void		Channel::sendMessageToAllClients(const std::string& src)
{
	for (std::vector<Client*>::iterator it = channelClients.begin(); it != channelClients.end(); ++it)
	{
		if (src == "TOPIC")
			(*it)->reply(TOPIC((*it)->getNickname(), (*it)->getUsername(), this->getName(), this->getChannelTopic()));
	}
}
