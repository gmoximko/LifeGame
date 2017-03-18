//
//  Messenger.cpp
//  LifeGame
//
//  Created by Максим Бакиров on 12.02.17.
//  Copyright © 2017 Arsonist (gmoximko@icloud.com). All rights reserved.
//

#include <cassert>
#include <string>
#include <algorithm>
#include "Messenger.hpp"

#if defined(_WIN32)
#include "Log.hpp"
#endif

using namespace Network;

namespace Messaging {
    
	Messenger::Messenger() : destroyed(false) {
#if defined(_WIN32)
		WSADATA data;
		int result = WSAStartup(MAKEWORD(2, 2), &data);
		if (result != 0) {
			Log::Error("WSAStartup failed!");
		}
#endif
	}

    Messenger::~Messenger() {
		if (!destroyed) {
			Destroy();
            Close();
		}
	}

    void Messenger::Destroy() {
        OnDestroy();
        Remove(listener, recvList);
        listener.reset();
        
        sendList.clear();
        exceptList.clear();
        
        if (connections.size() > 0) {
            for (auto &connection : connections) {
                connection->Close();
            }
        } else {
            Close();
        }
    }

    void Messenger::Update(bool block) {
        std::vector<TCPSocketPtr> outRead;
        std::vector<TCPSocketPtr> outWrite;
        std::vector<TCPSocketPtr> outExcept;
        SocketSelector::Select(&recvList, &outRead, &sendList, &outWrite, &exceptList, &outExcept, block);
        Read(outRead);
        Write(outWrite);
        Except(outExcept);
    }

    void Messenger::Read(const std::vector<TCPSocketPtr> &outRead) {
        NewConnection(outRead);
        for (TCPSocketPtr socket : outRead) {
            auto iter = std::find_if(connections.begin(), connections.end(), [socket](ConnectionPtr c){ return socket == c->socket; });
            if (iter == connections.end()) continue;
            ConnectionPtr connection = *iter;
            if (connection->Recv() == 0) {
                CloseConnection(connection, true);
            } else if (connection->CanRead()) {
                OnMessageRecv(connection);
                connection->Clear();
            }
        }
    }

    void Messenger::Write(const std::vector<TCPSocketPtr> &outWrite) {
        for (TCPSocketPtr socket : outWrite) {
            auto iter = std::find_if(connections.begin(), connections.end(), [socket](ConnectionPtr c){ return socket == c->socket; });
            if (iter == connections.end()) continue;
            ConnectionPtr connection = *iter;
            connection->Send();
            if (connection->CanWrite()) {
                Remove(connection->socket, sendList);
                OnMessageSend(connection);
            }
        }
    }

    void Messenger::Except(const std::vector<TCPSocketPtr> &outExcept) {}

    void Messenger::NewConnection(const std::vector<TCPSocketPtr> &outRead) {
        if (listener == nullptr || std::find(outRead.begin(), outRead.end(), listener) == outRead.end()) return;
        TCPSocketPtr socket = listener->Accept(*address);
        ConnectionPtr connection = std::make_shared<Connection>(socket);
        OnNewConnection(connection);
    }

    void Messenger::CloseConnection(const ConnectionPtr connection, bool callback) {
        if (callback) {
            OnCloseConnection(connection);
        }
        Remove(connection->socket, recvList);
        Remove(connection->socket, sendList);
        Remove(connection->socket, exceptList);
        auto iter = std::find(connections.begin(), connections.end(), connection);
        if (iter != connections.end()) {
            connections.erase(iter);
        }
        if (connections.size() == 0 && listener == nullptr) {
            Close();
        }
    }
    
    void Messenger::Remove(TCPSocketPtr socket, std::vector<TCPSocketPtr> &from) {
        auto iter = std::find(from.begin(), from.end(), socket);
        if (iter != from.end()) {
            from.erase(iter);
        }
    }

    void Messenger::Close() {
#if defined(_WIN32)
        WSACleanup();
#endif
        assert(recvList.size() == 0);
        assert(sendList.size() == 0);
        assert(exceptList.size() == 0);
        assert(connections.size() == 0);
        destroyed = true;
    }
    
    void Messenger::Listen(std::shared_ptr<Network::SocketAddress> address) {
        std::shared_ptr<SocketAddress> addr = address != nullptr ? address : SocketAddress::CreateIPv4("localhost");
        listener = TCPSocket::Create();
        listener->Bind(*addr);
        listener->Listen();
        listener->Addr(*addr);
        recvList.push_back(listener);
    }

    void Messenger::AddConnection(const ConnectionPtr connection) {
        connections.push_back(connection);
        recvList.push_back(connection->socket);
    }

    void Messenger::Send(const ConnectionPtr connection) {
        assert(connection->CanWrite());
        sendList.push_back(connection->socket);
    }

    void Messenger::CloseConnection(const ConnectionPtr connection) {
        CloseConnection(connection, false);
    }

    std::string Messenger::ListenerAddress() {
        if (listener == nullptr) {
            throw std::runtime_error("Listener is nullptr!");
        }
        SocketAddress address;
        listener->Addr(address);
        return address.ToString();
    }

}
