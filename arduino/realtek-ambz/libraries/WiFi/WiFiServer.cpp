#include "WiFiServer.h"
#include "WiFiPriv.h"

WiFiServer::WiFiServer(uint16_t port, uint8_t maxClients)
	: _sock(-1), _sockAccepted(-1), _port(port), _maxClients(maxClients), _active(false), _noDelay(false) {}

WiFiServer::operator bool() {
	return _active;
}

bool WiFiServer::begin(uint16_t port, bool reuseAddr) {
	if (_active)
		return true;
	if (port)
		_port = port;

	_sock = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_sock < 0)
		return false;

	int enable = reuseAddr;
	lwip_setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

	struct sockaddr_in addr;
	addr.sin_family		 = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port		 = htons(_port);

	if (lwip_bind(_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		return false;

	if (lwip_listen(_sock, _maxClients) < 0)
		return false;

	lwip_fcntl(_sock, F_SETFL, O_NONBLOCK);
	_active		  = true;
	_noDelay	  = false;
	_sockAccepted = -1;
}

void WiFiServer::end() {
	if (_sock == -1)
		return;
	lwip_close(_sock);
	_sock	= -1;
	_active = -1;
}

WiFiClient WiFiServer::accept() {
	if (!_active)
		return WiFiClient();

	int sock;
	if (_sockAccepted) {
		sock		  = _sockAccepted;
		_sockAccepted = -1;
	} else {
		struct sockaddr_in addr;
		socklen_t len = sizeof(addr);
		sock		  = lwip_accept(_sock, (struct sockaddr *)&addr, &len);
	}

	if (sock >= 0) {
		int enable = 1;
		if (lwip_setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable)) == ERR_OK) {
			enable = _noDelay;
			if (lwip_setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable)) == ERR_OK) {
				return WiFiClient(sock);
			}
		}
	}

	return WiFiClient();
}

int WiFiServer::setTimeout(uint32_t seconds) {
	struct timeval tv;
	tv.tv_sec  = seconds;
	tv.tv_usec = 0;
	if (lwip_setsockopt(_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
		return -1;
	return lwip_setsockopt(_sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
}

void WiFiServer::setNoDelay(bool noDelay) {
	_noDelay = noDelay;
}

bool WiFiServer::getNoDelay() {
	return _noDelay;
}

bool WiFiServer::hasClient() {
	if (_sockAccepted >= 0) {
		return true;
	}
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	_sockAccepted = lwip_accept(_sock, (struct sockaddr *)&addr, &len);
	if (_sockAccepted >= 0) {
		return true;
	}
	return false;
}
