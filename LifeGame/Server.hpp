//
//  Server.hpp
//  LifeGame
//
//  Created by Максим Бакиров on 10.01.17.
//  Copyright © 2017 Arsonist (gmoximko@icloud.com). All rights reserved.
//

#ifndef Server_hpp
#define Server_hpp

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include "Vector.hpp"
#include "Network.h"

class Server {
    std::vector<std::shared_ptr<Network::TCPSocket>> players;
    std::shared_ptr<Network::TCPSocket> listen;
    Geometry::Vector fieldSize;
    Network::SocketAddress address;
    int playersTurns;
    
public:
    explicit Server(Geometry::Vector fieldSize);
    
    Network::SocketAddress &Address() { return address; }
    
private:
    void Update();
};

#endif /* Server_hpp */