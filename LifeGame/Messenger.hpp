//
//  Messenger.hpp
//  LifeGame
//
//  Created by Максим Бакиров on 30.01.17.
//  Copyright © 2017 Arsonist (gmoximko@icloud.com). All rights reserved.
//

#ifndef Messenger_hpp
#define Messenger_hpp

#include <vector>
#include <unordered_map>
#include "Geometry.h"
#include "Network.h"

class Connection {
    bool canRead;
    bool canWrite;
    uint32_t recvData;
    uint32_t allRecvData;
    uint32_t sendData;
    
public:
    Network::TCPSocketPtr socket;
    Network::InputMemoryStream input;
    Network::OutputMemoryStream output;
    
    explicit Connection(Network::TCPSocketPtr socket);
    
    bool CanRead() const { return canRead; }
    bool CanWrite() const { return canWrite; }
    
    int Recv();
    int Send();
};

typedef std::shared_ptr<Connection> ConnectionPtr;

class Messenger {
    std::vector<Network::TCPSocketPtr> recvList;
    std::vector<Network::TCPSocketPtr> sendList;
    std::vector<Network::TCPSocketPtr> exceptList;
    Network::TCPSocketPtr listener;
    
protected:
    std::vector<ConnectionPtr> connections;
    std::shared_ptr<Network::SocketAddress> address;
    
public:
    virtual ~Messenger() = 0;
    virtual void Init(class GameField *gameField) = 0;
    virtual void AddUnit(Geometry::Vector vec) {}
    virtual void Turn() {}
    void Destroy();
    void Update(bool block = false);
    std::shared_ptr<Network::SocketAddress> Address() { return address; }
    
protected:
    virtual void OnMessageRecv(const ConnectionPtr connection) {}
    virtual void OnMessageSend(const ConnectionPtr connection) {}
    virtual void OnNewConnection(const ConnectionPtr connection) {}
    virtual void OnCloseConnection(const ConnectionPtr connection) {}
    virtual void OnDestroy() {}
    void Listen();
    void AddConnection(const ConnectionPtr connection);
    void Send(const ConnectionPtr connection);
    
private:
    void Read(const std::vector<Network::TCPSocketPtr> &outRead);
    void Write(const std::vector<Network::TCPSocketPtr> &outWrite);
    void Except(const std::vector<Network::TCPSocketPtr> &outExcept);
    void NewConnection(const std::vector<Network::TCPSocketPtr> &outRead);
    void CloseConnection(const ConnectionPtr connection);
    void Remove(Network::TCPSocketPtr socket, std::vector<Network::TCPSocketPtr> &from);
};

class Client : public Messenger {
    ConnectionPtr server;
    std::vector<Geometry::Vector> addedUnits;
    class GameField *gameField;
    
public:
    explicit Client(std::shared_ptr<Network::SocketAddress> address);
    virtual ~Client() override {}
    
    virtual void Turn() override;
    virtual void AddUnit(const Geometry::Vector vector) override;
    virtual void Init(class GameField *gameField) override;
    
protected:
    virtual void OnMessageRecv(const ConnectionPtr connection) override;
    virtual void OnMessageSend(const ConnectionPtr connection) override;
    virtual void OnDestroy() override;
};

class Server : public Messenger {
    const int maxPlayers;
    std::unordered_map<int, std::vector<Geometry::Vector>> allUnits;
    std::unordered_map<int, bool> playerTurns;
    std::unordered_map<ConnectionPtr, int> ids;
    Geometry::Vector fieldSize;
    
public:
    explicit Server(Geometry::Vector fieldSize);
    virtual ~Server() override {}
    
    virtual void Turn() override { Log::Error("Trying to turn on server!"); }
    virtual void AddUnit(const Geometry::Vector vector) override { Log::Error("Trying to add unit on server!"); }
    virtual void Init(class GameField *gameField) override;
    
protected:
    virtual void OnMessageRecv(const ConnectionPtr connection) override;
    virtual void OnMessageSend(const ConnectionPtr connection) override;
    virtual void OnNewConnection(const ConnectionPtr connection) override;
    virtual void OnCloseConnection(const ConnectionPtr connection) override;
    
private:
    void Process();
    void AddPlayer(const ConnectionPtr connection);
};

#endif /* Messenger_hpp */