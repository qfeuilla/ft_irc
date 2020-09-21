/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: qfeuilla <qfeuilla@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/09/17 18:37:26 by qfeuilla          #+#    #+#             */
/*   Updated: 2020/09/17 19:05:16 by qfeuilla         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include <stdlib.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <fcntl.h>
#include <string.h>

Server::~Server() {
	std::cout << "server destructed" << std::endl;
}

Server::Server() {
	type = FD_SERVER;
	ev = new Environment();
	ev->password = new std::string("");
}

Server::Server(int ac, char **av) {
	type = FD_SERVER;
	ev = new Environment();
	ev->password = new std::string("");
	load_options(ac, av);
}

void		Server::load_options(int ac, char **av) {
	if (ac != 3) {
		std::cerr << "Usage: " << av[0] <<" <port> <password>" << std::endl;
		exit(EXIT_FAILURE);
	}
	port = std::atoi(av[1]);
	delete ev->password;
	ev->password = new std::string(av[2]);
}

void		Server::create() {
	struct sockaddr_in	sin;
	struct protoent		*pe;
	int					opt = 1;
	
	pe = reinterpret_cast<struct protoent*>(Xv(NULL, getprotobyname("tcp"), "getprotobyname"));
	sock = X(-1, socket(PF_INET, SOCK_STREAM, pe->p_proto), "socket");
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
		perror("socket opt change failed\n");
		exit (EXIT_FAILURE);
	}
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr("10.0.2.15");
	sin.sin_port = htons(port);
	X(-1, bind(sock, (struct sockaddr*)&sin, sizeof(sin)), "bind");
	X(-1, listen(sock, 42), "listen");
	ev->clients_fd[sock] = this;
}

void		Server::accept_srv() {
	int					cs;
	struct sockaddr_in	csin;
	socklen_t			csin_len;

	csin_len = sizeof(csin);
	cs = X(-1, accept(sock, (struct sockaddr*)&csin, &csin_len), "accept");
	std::cout << "New client fd:" << cs << " from " 
		<< inet_ntoa(csin.sin_addr) << ":"
		<< ntohs(csin.sin_port) << std::endl;

	delete ev->clients_fd[cs];
	ev->clients_fd[cs] = new Client(ev, cs);
}

void		Server::read_func() {
	accept_srv();
}

void		Server::write_func() { 
}

void	Server::init_fd() {
	ev->init_fds();
}

void		Server::serv_select() {
	action = select(ev->maxfd + 1, &(ev->fd_read), &(ev->fd_write), NULL, NULL);
}

void		Server::do_actions() {
	int	i;

	i = 0;
	while ((i < ev->clients_num) && (action > 0))
	{
		if (FD_ISSET(i, &ev->fd_read))
			ev->clients_fd[i]->read_func();
		if (FD_ISSET(i, &ev->fd_write))
			ev->clients_fd[i]->write_func();
		if (FD_ISSET(i, &ev->fd_read) ||
			FD_ISSET(i, &ev->fd_write))
			action--;
		i++;
	}
}