//
//  Peer.cpp
//  LifeGame
//
//  Created by Максим Бакиров on 16.02.17.
//  Copyright © 2017 Arsonist (gmoximko@icloud.com). All rights reserved.
//

#include <cassert>
#include "Utils.hpp"
#include "GameField.hpp"
#include "Peer.hpp"

using namespace Messaging;
using namespace Network;
using namespace Geometry;

Peer::Peer(std::shared_ptr<GameField> gameField, int readyPlayers, int playersCount) :
    gameField(gameField),
    readyPlayers(readyPlayers),
    playersCount(playersCount),
    futureTurns(3),
    seed(0),
    pauseOnLastTurn(false),
    pause(false),
    selfCommands(new CommandsQueue()) {
    gameField->SetPeer(this);
}

Peer::Peer(std::shared_ptr<GameField> gameField, const std::string &address) :
    Peer(gameField, 0, 0) {
	this->masterPeer = std::make_shared<Connection>(TCPSocket::Create());
    this->address = SocketAddress::CreateIPv4(address);
}

Peer::Peer(std::shared_ptr<GameField> gameField, int players) :
    Peer(gameField, 1, players) {
    address = std::make_shared<SocketAddress>();
    Listen(address);
    SetSeed(Random::NextUInt());
    if (IsGameStarted()) {
        assert(players == 1);
        StartGame();
    }
}

void Peer::Init() {
    if (IsMaster()) return;
    masterPeer->socket->Connect(*address);
    AddConnection(masterPeer);
    Listen();
    NewPlayerMessage().Write(this, masterPeer);
    Send(masterPeer);
    do {
        Update(true);
    } while (!masterPeer->CanWrite());
    do {
        Update(true);
    } while (!masterPeer->CanRead());
    assert(gameField->IsInitialized());
}

void Peer::Turn() {
    if (CheckSync()) {
        for (auto player : players) {
            ApplyCommand(player.second);
        }
        ApplyCommand(selfCommands);
        gameField->ProcessUnits();
        PrepareCommands();
    } else {
        Log::Warning("Game instances are out of sync!");
        gameField->Destroy();
    }
}

void Peer::Pause() {
    pause = !pause;
    PauseMessage msg;
    BroadcastMessage(msg);
}

void Peer::AddUnit(const Vector vector) {
    addedUnits.push_back(vector);
}

void Peer::AddPreset(const Geometry::Matrix3x3 &matrix, int preset) {
    CommandPtr command = std::make_shared<AddPresetCommand>(matrix, preset, gameField->Player());
    commands.push_back(command);
}

bool Peer::IsPause() const {
    if (pause) return true;
    typedef const std::unordered_map<int, CommandsQueuePtr>::value_type &value;
    const auto emptyQueue = std::find_if(players.begin(), players.end(), [](value v){ return v.second->size() == 0; });
    const bool pause = emptyQueue != players.end();
    
    if (pause && !pauseOnLastTurn) {
        Log::Warning("Peer", gameField->Player(), "has not recieved command from", emptyQueue->first);
    } else if (!pause && pauseOnLastTurn) {
        Log::Warning("Peer", gameField->Player(), "has recievd all commands!");
    }
    pauseOnLastTurn = pause;
    return pause;
}

void Peer::OnMessageRecv(const ConnectionPtr connection) {
    for (int i = 0; i < connection->RecvMessages(); i++) {
        std::shared_ptr<Message> msg = Message::Parse(connection->input);
        if (msg == nullptr) {
            CloseConnection(connection);
            break;
        } else {
            msg->Read(this, connection);
        }
    }
    connection->input.Clear();
}

void Peer::OnMessageSend(const ConnectionPtr connection) {
    connection->output.Clear();
}

void Peer::OnNewConnection(const ConnectionPtr connection) {
    if (players.size() >= playersCount - 1) return;
    AddConnection(connection);
}

void Peer::OnCloseConnection(const ConnectionPtr connection) {
    if (ids.size() == 0 || !IsGameStarted()) {
        gameField->Destroy();
        return;
    }
    const int id = ids[connection];
    ids.erase(connection);
    players.erase(id);
    
    if (connection == masterPeer) {
        typedef const std::unordered_map<ConnectionPtr, int>::value_type &value;
        auto min = std::min_element(ids.begin(), ids.end(), [](value v1, value v2){ return v1.second < v2.second; });
        if (min == ids.end() || min->second == gameField->Player()) {
            masterPeer.reset();
        } else {
            masterPeer = min->first;
        }
    }
    Log::Warning("Peer", id, "closed connection. Master is", IsMaster() ? gameField->Player() : ids[masterPeer]);
}

void Peer::OnDestroy() {
    players.clear();
    ids.clear();
    if (!IsMaster()) {
        masterPeer.reset();
    }
}

void Peer::AddPlayer(int id, const ConnectionPtr connection) {
    players.emplace(id, std::make_shared<CommandsQueue>());
    ids.emplace(connection, id);
}

void Peer::AcceptNewPlayer(const ConnectionPtr connection) {
    AcceptPlayerMessage().Write(this, connection);
    Send(connection);
}

void Peer::BroadcastMessage(Message &message) {
    for (const auto &it : ids) {
        message.Write(this, it.first);
        Send(it.first);
    }
}

void Peer::ConnectNewPlayer(const std::string &listenerAddress, int id) {
    auto address = SocketAddress::CreateIPv4(listenerAddress);
    Log::Warning("Peer", gameField->Player(), "connects to other peer", id, *address);
    TCPSocketPtr socket = TCPSocket::Create();
    socket->Connect(*address);
    ConnectionPtr newPlayer = std::make_shared<Connection>(socket);
    AddConnection(newPlayer);
    AddPlayer(id, newPlayer);
    ConnectPlayerMessage().Write(this, newPlayer);
    Send(newPlayer);
}

void Peer::CheckReadyForGame() {
    if (IsGameStarted()) return;
    if (players.size() == playersCount - 1) {
        ReadyForGameMessage().Write(this, masterPeer);
        Send(masterPeer);
    }
}

void Peer::ApplyCommand(CommandsQueuePtr queue) {
    assert(queue->size() > 0);
    queue->front()->Apply(gameField.get());
    queue->pop();
}

void Peer::StartGame() {
    for (auto player : players) {
        assert(player.second->empty());
        for (int i = 0; i < futureTurns; i++) {
            player.second->push(std::make_shared<EmptyCommand>());
        }
    }
    assert(selfCommands->empty());
    for (int i = 0; i < futureTurns; i++) {
        selfCommands->push(std::make_shared<EmptyCommand>());
    }
    assert(!IsPause());
    Log::Warning("Game started!");
}

void Peer::PrepareCommands() {
    if (addedUnits.size() > 0) {
        commands.emplace_back(std::make_shared<AddUnitsCommand>(gameField->Player(), std::move(addedUnits)));
    }
    const int32_t random = Random::Next();
    const uint64_t checksum = CalculateChecksum();
    CommandPtr command = std::make_shared<ComplexCommand>(random, checksum, std::move(commands));
    selfCommands->push(command);
    CommandMessage msg;
    BroadcastMessage(msg);
}

void Peer::SetSeed(uint32_t seed) {
    this->seed = seed;
    Random::Seed(seed);
}

uint64_t Peer::CalculateChecksum() const {
    uint64_t result = 0;
    for (const auto &unit : *gameField->GetUnits()) {
		uint32_t x = static_cast<uint32_t>(unit.position.x);
		uint32_t y = static_cast<uint32_t>(unit.position.y);
		uint32_t size = static_cast<uint32_t>(gameField->GetSize().y);
		uint32_t player = static_cast<uint32_t>(unit.player + 1);
		result += x * size + player + y * player;
    }
    return result;
}

bool Peer::CheckSync() {
	const CommandPtr command = selfCommands->front();
    for (const auto &it : players) {
		const CommandPtr playerCommand = it.second->front();
        if (command->TurnStep() != playerCommand->TurnStep() 
			|| command->Checksum() != playerCommand->Checksum()) {
            return false;
        }
    }
    return true;
}



Peer::Message::~Message() {}

std::shared_ptr<Peer::Message> Peer::Message::Parse(InputMemoryStream &stream) {
    int32_t type;
    stream.Read(type, stream.Size() + sizeof(uint32_t));
    switch (static_cast<Msg>(type)) {
        case Msg::NewPlayer:     return std::make_shared<NewPlayerMessage>();
        case Msg::AcceptPlayer:  return std::make_shared<AcceptPlayerMessage>();
        case Msg::ConnectPlayer: return std::make_shared<ConnectPlayerMessage>();
        case Msg::ReadyForGame:  return std::make_shared<ReadyForGameMessage>();
        case Msg::Command:       return std::make_shared<CommandMessage>();
        case Msg::Pause:         return std::make_shared<PauseMessage>();
        default:
            Log::Warning("Unknown message has been received!");
            return nullptr;
    }
}

void Peer::Message::Read(Peer *peer, const ConnectionPtr connection) {
    uint32_t msgSize;
    int32_t type;
    connection->input >> msgSize >> type;
    if (static_cast<Msg>(type) != Type()) {
        Log::Error("Messages types are not the same!");
    }
//    Log::Warning("Peer", peer->gameField->Player(), "reads message of type", type);
    OnRead(peer, connection);
}

void Peer::Message::Write(Peer *peer, const ConnectionPtr connection) {
//    Log::Warning("Peer", peer->gameField->Player(), "writes message of type", static_cast<int32_t>(Type()));
    uint32_t msgSize = 0;
    uint32_t oldSize = connection->output.Size();
    connection->output << msgSize << static_cast<int32_t>(Type());
    OnWrite(peer, connection);
    msgSize = connection->output.Size() - oldSize;
    connection->output.Write(msgSize, oldSize);
}

void Peer::NewPlayerMessage::OnRead(Peer *peer, const ConnectionPtr connection) {
    if (peer->IsMaster()) {
        SocketAddress address;
        connection->socket->Addr(address, true);
        std::string remoteAddress = address.ToString();
        std::string listenerAddress;
        ReadAddress(connection->input, listenerAddress);
        std::string host;
        std::string port;
        host = remoteAddress.substr(0, remoteAddress.find_last_of(':'));
        port = listenerAddress.substr(listenerAddress.find_last_of(':'));
        std::string listenerRemoteAddress = host + port;
        int id = static_cast<int>(peer->players.size() + 1);
        NewPlayerMessage msg(id, listenerRemoteAddress);
        peer->BroadcastMessage(msg);
        peer->AddPlayer(id, connection);
        peer->AcceptNewPlayer(connection);
    } else {
        int32_t id;
        std::string listenerAddress;
        ReadAddress(connection->input, listenerAddress);
        connection->input >> id;
        assert(id >= 0);
        peer->ConnectNewPlayer(listenerAddress, static_cast<int>(id));
    }
}

void Peer::NewPlayerMessage::OnWrite(Peer *peer, const ConnectionPtr connection) {
    if (peer->IsMaster()) {
        WriteAddress(connection->output, listenerAddress);
        connection->output << static_cast<int32_t>(id);
    } else {
        WriteAddress(connection->output, peer->ListenerAddress());
    }
}

void Peer::NewPlayerMessage::ReadAddress(InputMemoryStream &stream, std::string &address) {
    uint32_t size;
    stream >> size;
    address.resize(size);
    for (int i = 0; i < size; i++) {
        uint8_t symbol;
        stream >> symbol;
        address[i] = static_cast<char>(symbol);
    }
}

void Peer::NewPlayerMessage::WriteAddress(OutputMemoryStream &stream, const std::string &address) {
    stream << static_cast<uint32_t>(address.size());
    for (int i = 0; i < address.size(); i++) {
        stream << static_cast<uint8_t>(address[i]);
    }
}

void Peer::AcceptPlayerMessage::OnRead(Peer *peer, const ConnectionPtr connection) {
    int32_t playersCount, x, y, id, masterId, distance;
    uint32_t turnTime, seed;
    connection->input >> playersCount >> x >> y >> id >> masterId >> distance >> turnTime >> seed;
    peer->playersCount = static_cast<int>(playersCount);
    
    peer->gameField->SetSize(Vector(static_cast<int>(x), static_cast<int>(y)));
    peer->gameField->SetPlayer(static_cast<int>(id));
    peer->gameField->SetDistanceToEnemy(static_cast<int>(distance));
    peer->gameField->SetTurnTime(static_cast<unsigned>(turnTime));
    peer->AddPlayer(static_cast<int>(masterId), connection);
    peer->CheckReadyForGame();
    peer->SetSeed(seed);
    assert(peer->gameField->Player() >= 0 && peer->gameField->Player() < peer->playersCount);
}

void Peer::AcceptPlayerMessage::OnWrite(Peer *peer, const ConnectionPtr connection) {
    connection->output
    << static_cast<int32_t>(peer->playersCount)
    << static_cast<int32_t>(peer->gameField->GetSize().x)
    << static_cast<int32_t>(peer->gameField->GetSize().y)
    << static_cast<int32_t>(peer->players.size())
    << static_cast<int32_t>(peer->gameField->Player())
    << static_cast<int32_t>(peer->gameField->DistanceToEnemy())
    << static_cast<uint32_t>(peer->gameField->TurnTime())
    << peer->seed;
}

void Peer::ConnectPlayerMessage::OnRead(Peer *peer, const ConnectionPtr connection) {
    int32_t id;
    connection->input >> id;
    peer->AddPlayer(static_cast<int>(id), connection);
    peer->CheckReadyForGame();
}

void Peer::ConnectPlayerMessage::OnWrite(Peer *peer, const ConnectionPtr connection) {
    connection->output << static_cast<int32_t>(peer->gameField->Player());
    peer->CheckReadyForGame();
}

void Peer::ReadyForGameMessage::OnRead(Peer *peer, const ConnectionPtr connection) {
    int32_t playersSize, playersCount, readyPlayers;
    connection->input >> playersSize >> playersCount >> readyPlayers;
    assert(playersCount == peer->playersCount);
    assert(playersSize == peer->players.size());
    
    if (peer->IsMaster()) {
        assert(readyPlayers == 0);
        if (++peer->readyPlayers == peer->playersCount) {
            ReadyForGameMessage msg;
            peer->BroadcastMessage(msg);
            peer->StartGame();
        }
    } else {
        assert(peer->readyPlayers == 0);
        peer->readyPlayers = readyPlayers;
        peer->StartGame();
        assert(peer->playersCount == peer->readyPlayers);
    }
}

void Peer::ReadyForGameMessage::OnWrite(Peer *peer, const ConnectionPtr connection) {
    assert(peer->playersCount == peer->players.size() + 1);
    connection->output
    << static_cast<int32_t>(peer->players.size())
    << static_cast<int32_t>(peer->playersCount)
    << static_cast<int32_t>(peer->readyPlayers);
    
    if (!peer->IsMaster()) {
        assert(connection == peer->masterPeer);
    }
}

void Peer::CommandMessage::OnRead(Peer *peer, const Messaging::ConnectionPtr connection) {
    int32_t id;
    connection->input >> id;
    CommandPtr command = Command::Parse(connection->input);
    if (command != nullptr) {
        const int key = static_cast<int>(id);
        auto player = peer->players.find(key);
        if (player != peer->players.end()) {
//            Log::Warning("Command recv", id, command->TurnStep());
            player->second->push(command);
        }
    }
}

void Peer::CommandMessage::OnWrite(Peer *peer, const Messaging::ConnectionPtr connection) {
    connection->output << static_cast<int32_t>(peer->gameField->Player());
    peer->selfCommands->back()->Write(connection->output);
//    Log::Warning("Command send", peer->gameField->Player(), peer->selfCommands->back()->TurnStep());
}

void Peer::PauseMessage::OnRead(Peer *peer, const Messaging::ConnectionPtr connection) {
    bool pause;
    connection->input >> pause;
    peer->pause = pause;
}

void Peer::PauseMessage::OnWrite(Peer *peer, const Messaging::ConnectionPtr connection) {
    connection->output << peer->pause;
}
