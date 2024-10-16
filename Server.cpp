/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: nsouchal <nsouchal@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/11 12:56:29 by tnicolau          #+#    #+#             */
/*   Updated: 2024/10/16 16:16:40 by nsouchal         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

bool	Server::_signal = false;

Server::Server(int port, std::string password) : _port(port), _password(password), serverSocketFd(0)
{
	std::cout << "Password : " << _password << "\nPort : " << _port << std::endl;
	ServerProgram();
}

Server::~Server()
{
	for (size_t i = 0; i < clients.size(); ++i)
	{
		delete clients[i];
	}	
}

void	Server::SignalHandler(int signal)
{
	(void)signal;
	_signal = true;
}

void	Server::CloseFds()
{
	for (size_t i = 0; i < clients.size(); ++i)
	{
		std::cout << "Client " << clients[i]->getFd() << " disconnected" << std::endl;
		close(clients[i]->getFd());
	}
	close(serverSocketFd);
}

void	Server::ServerSocket()
{
	sockaddr_in serverAddress;
	pollfd	newPoll;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(_port);
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	int	optionval = 1;

	serverSocketFd = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocketFd == -1)
		throw (std::runtime_error("Failed to create socket"));

	if (setsockopt(serverSocketFd, SOL_SOCKET, SO_REUSEADDR, &optionval, sizeof(optionval)) == -1)
		throw(std::runtime_error("Failed to set option (SO_REUSEADDR) on socket"));

	if (fcntl(serverSocketFd, F_SETFL, O_NONBLOCK) == -1)
		throw(std::runtime_error("Failed to set option (O_NONBLOCK) on socket"));

	if (bind(serverSocketFd, (sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
		throw (std::runtime_error("Failed to bind socket"));

	if (listen(serverSocketFd, SOMAXCONN) == -1)
		throw(std::runtime_error("Failed to listen"));

	newPoll.fd = serverSocketFd;
	newPoll.events = POLLIN;
	newPoll.revents = 0;
	fds.push_back(newPoll);
}

void	Server::AcceptNewClient()
{
	Client	*client = new Client();
	sockaddr_in	clientAddress;
	pollfd	newPoll;
	socklen_t	len = sizeof(clientAddress);

	int	fd = accept(serverSocketFd, (sockaddr *)&clientAddress, &len);
	if (fd == -1)
		std::cerr << "Accept() failed" << std::endl;
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
	{
		std::cerr << "fcntl() failed" << std::endl;
		return ;
	}

	newPoll.fd = fd;
	newPoll.events = POLLIN;
	newPoll.revents = 0;

	client->setFd(fd);
	client->setIPaddress(inet_ntoa((clientAddress.sin_addr)));
	clients.push_back(client);
	fds.push_back(newPoll);

	std::cout << "Client " << fd << " connected !" << std::endl;
	//sendPing(fd);
}

void	Server::SendPing(int fd)
{
	std::string ping = "PING :serverping\r\n";
	send(fd, ping.c_str(), ping.length(), 0);
	time_t timePing = time(NULL);
	for (size_t i = 0; i < clients.size(); ++i)
	{
		if (clients[i]->getFd() == fd)
			clients[i]->setPing(timePing);
	}
}

void	Server::ReceiveData(int fd)
{
	char	buffer[1024];

	ssize_t bytes = recv(fd, buffer, sizeof(buffer) - 1 , 0); //-> receive the data

	if (bytes == 0)
	{ //-> check if the client disconnected
		std::cout << "Client " << fd << " disconnected" << std::endl;
		//ClearClients(fd); //-> clear the client
		close(fd); //-> close the client socket
	}
	else if (bytes < 0)
	{
		std::cout << "Client " << fd << " error made him disconnect" << std::endl;
		close(fd);
	}
	else
	{ //-> print the received data
		buffer[bytes] = '\0';
		parseMessage(buffer, fd);
		//send(fd, "001 tnicolau :Welcome to the choucroute Network, tnicolau!tnicolau@localhost\r\n", 79, 0);
		//here you can add your code to process the received data: parse, check, authenticate, handle the command, etc...
	}
}

void	Server::capabilityNegociation(const std::string& message)
{
	(void)message;
	// std::cout << "CAP !!!" << std::endl;
}

void	Server::password(const std::string& message)
{
	(void)message;
	// std::cout << "PASSWORD !!!" << std::endl;
}

void	Server::nickname(const std::string& message)
{
	(void)message;
	// std::cout << "NICKNAME !!!" << std::endl;
}

void	Server::user(const std::string& message)
{
	(void)message;
	// std::cout << "USER !!!" << std::endl;
}

Client	*Server::findClient(int fd)
{
	for (size_t i = 0; i < clients.size(); ++i)
	{
		if (clients[i]->getFd() == fd)
			return clients[i];
	}
	return NULL;
}

void	Server::parseMessage(char* message, int fd)
{
	std::cout << "Client " << fd << ", data: " << message << std::endl;
	std::istringstream	reader(message);
	std::string line, value;
	findClient(fd)->incrementMessagesSended();
	
	while (std::getline(reader, line))
	{
		if (line.size() > 1 && line[line.size() - 1] == '\r')
		{
			if (findClient(fd)->getRegistration())
			{
				//ici check des commandes autres que CAP PASS NICK USER
				checkCommand(line);
			}
			else if (findClient(fd)->getMessagesSended() == 1)
				checkCommand(line);
			else if (findClient(fd)->getMessagesSended() == 2)
			{
				checkCommand(line);
				//check si NICK PASS USER sont enregistres et update booleen
			}
			else if (!findClient(fd)->getRegistration() && findClient(fd)->getMessagesSended() < 5)
			{
				checkCommand(line);
				//check si NICK PASS USER sont enregistres et update booleen
				//si 4 passage et toujours false deconnection du client
			}	
		}
	}
}

void	Server::checkCommand(const std::string& message)
{
	std::string	command = message.substr(0, message.find(" "));

	void(Server::*function_ptr[])(const std::string&) = {&Server::capabilityNegociation, &Server::password, &Server::nickname, &Server::user};
	std::string commands[] = {"CAP", "PASS", "NICK", "USER"};
	bool	found = false;

	for (size_t i = 0; i < commands->length() + 1; i++)
	{
		if (commands[i] == command)
		{
			(this->*function_ptr[i])(message);
			found = true;
			break ;
		}
	}
	if (!found)
		std::cerr << "Command " << message << " does not exist, sorry" << std::endl;
	return ;
}

void	Server::ServerProgram()
{
	ServerSocket();
	while (!_signal)
	{
		//Attend qu'il se produise un evenement sur un des fds
		if((poll(&fds[0],fds.size(), -1) == -1) && !_signal)
			throw(std::runtime_error("poll() failed"));
		for (size_t i = 0; i < fds.size(); i++)
		{
			if (fds[i].revents & POLLIN)
			{
				if (fds[i].fd == serverSocketFd)
					AcceptNewClient();
				else
					ReceiveData(fds[i].fd);
			}
		}
	}
	CloseFds();
}
