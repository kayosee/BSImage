/*
 * Cross-platform compatibility superclass for sockets
 *
 * Copyright (C) 2019 Simon D. Levy
 *
 * MIT License
 */

#pragma once

#include "SocketCompat.hpp"

class TcpSocket : public Socket {

    protected:

        char _host[200];
        char _port[10];

        SOCKET _conn;

        struct addrinfo * _addressInfo;

        bool _connected;

        TcpSocket(const char * host, const short port)
        {
            sprintf_s(_host, "%s", host);
            sprintf_s(_port, "%d", port);

            // No connection yet
            _sock = INVALID_SOCKET;
            _connected = false;
            *_message = 0;

            // Initialize Winsock, returning on failure
            if (!initWinsock()) return;

            // Set up client address info
            struct addrinfo hints = {0};
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;

            // Resolve the server address and port, returning on failure
            _addressInfo = NULL;
            int iResult = getaddrinfo(_host, _port, &hints, &_addressInfo);
            if ( iResult != 0 ) {
                sprintf_s(_message, "getaddrinfo() failed with error: %d", iResult);
                cleanup();
                return;
            }

            // Create a SOCKET for connecting to server, returning on failure
            _sock = socket(_addressInfo->ai_family, _addressInfo->ai_socktype, _addressInfo->ai_protocol);
            if (_sock == INVALID_SOCKET) {
                sprintf_s(_message, "socket() failed");
                cleanup();
                return;
            }
        }

    public:

        bool sendData(void *buf, size_t len)
        {
            return (size_t)send(_conn, (const char *)buf, len, 0) == len;
        }


        size_t receiveData(void *buf, size_t len)
        {
            return (size_t)recv(_conn, (char *)buf, len, 0);
        }

        bool receiveLine(void* buf, size_t max)
        {
            int start = 0;
            int ret = 0;
            do {
                ret = recv(_conn, ((char*)buf + start), 2, 0);
                start += ret;
                if (start >= max)
                    return false;

            } while (ret > 0 && strcmp((char*)buf + start - 2, "\r\n") != 0);
            
            return start > 0;
        }

        bool isConnected()
        {
            return _connected;
        }

};
