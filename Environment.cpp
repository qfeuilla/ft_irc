/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Environment.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: qfeuilla <qfeuilla@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2020/09/17 19:52:01 by qfeuilla          #+#    #+#             */
/*   Updated: 2020/09/17 22:42:02 by qfeuilla         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Environment.hpp"
#include <string>
#include <sys/select.h>
#include <sys/resource.h>
#include <iostream>

Environment::Environment() {
	struct rlimit	rlp;

	X(-1, getrlimit(RLIMIT_NOFILE, &rlp), "getrlimit");
	clients_num = rlp.rlim_cur;
	clients_fd = std::vector<Fd *>(clients_num);
	for (int i = 0; i < clients_num ; i++) {
		clients_fd[i] = new Fd();
	}
}

Environment::~Environment() { }

void Environment::init_fds() {
	int		i;
	
	i = 0;
	maxfd = 0;
	FD_ZERO(&fd_read);
	FD_ZERO(&fd_write);
	while (i < clients_num) {
		if (clients_fd[i]->type != FD_FREE) {
			FD_SET(i, &fd_read);
			if (std::string(clients_fd[i]->buf_write).length() > 0)
				FD_SET(i, &fd_write);
			maxfd = MAX(maxfd, i);
		}
		i++;
	}
}