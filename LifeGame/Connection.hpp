//
//  Connection.hpp
//  LifeGame
//
//  Created by Максим Бакиров on 12.02.17.
//  Copyright © 2017 Arsonist (gmoximko@icloud.com). All rights reserved.
//

#ifndef Connection_hpp
#define Connection_hpp

#include "TCPSocket.hpp"
#include "MemoryStream.hpp"

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

#endif /* Connection_hpp */