//
//  Peer.hpp
//  LifeGame
//
//  Created by Максим Бакиров on 16.02.17.
//  Copyright © 2017 Arsonist (gmoximko@icloud.com). All rights reserved.
//

#ifndef Peer_hpp
#define Peer_hpp

#include <string>
#include <queue>
#include <unordered_map>
#include "Geometry.h"
#include "Messenger.hpp"
#include "Command.hpp"

class Peer : public Messaging::Messenger {
    typedef std::shared_ptr<Messaging::Command> CommandPtr;
    typedef std::queue<CommandPtr> CommandsQueue;
    typedef std::shared_ptr<CommandsQueue> CommandsQueuePtr;
    
    uint32_t seed;
    int readyPlayers;
    int playersCount;
    bool pause;
    mutable bool pauseOnLastTurn;
    const int futureTurns;
    
    Messaging::ConnectionPtr masterPeer;
    std::unordered_map<int, CommandsQueuePtr> players;
    std::unordered_map<Messaging::ConnectionPtr, int> ids;
    
    std::vector<CommandPtr> commands;
    CommandsQueuePtr selfCommands;
    std::shared_ptr<class GameField> gameField;
    std::vector<Geometry::Vector> addedUnits;
    
public:
    explicit Peer(std::shared_ptr<GameField> gameField, const std::string &address);
    explicit Peer(std::shared_ptr<GameField> gameField, int players);
    virtual ~Peer() override {}
    
    void Init();
    void Turn();
    void Pause();
    void AddUnit(const Geometry::Vector vector);
    void AddPreset(const Geometry::Matrix3x3 &matrix, int preset);
    bool IsPause() const;
    bool IsGameStarted() const { return playersCount == readyPlayers; }
    
protected:
    virtual void OnMessageRecv(const Messaging::ConnectionPtr connection) override;
    virtual void OnMessageSend(const Messaging::ConnectionPtr connection) override;
    virtual void OnNewConnection(const Messaging::ConnectionPtr connection) override;
    virtual void OnCloseConnection(const Messaging::ConnectionPtr connection) override;
    virtual void OnDestroy() override;
    
private:
    explicit Peer(std::shared_ptr<GameField> gameField, int readyPlayers, int playersCount);
    bool IsMaster() const { return masterPeer == nullptr; }
    
    struct Message;
    
    void AddPlayer(int id, const Messaging::ConnectionPtr connection);
    void AcceptNewPlayer(const Messaging::ConnectionPtr connection);
    void BroadcastMessage(Message &message);
    void ConnectNewPlayer(const std::string &listenerAddress, int id);
    void CheckReadyForGame();
    void ApplyCommand(CommandsQueuePtr queue);
    void StartGame();
    void PrepareCommands();
    void SetSeed(uint32_t seed);
    uint64_t CalculateChecksum() const;
    bool CheckSync();
    
    
    
    #ifndef MESSAGE
    #define MESSAGE(newMsg, msgType) struct newMsg##Message : public Message {               \
        virtual ~newMsg##Message() override {}                                               \
        virtual Msg Type() override { return msgType; }                                      \
    private:                                                                                 \
        virtual void OnWrite(Peer *peer, const Messaging::ConnectionPtr connection) override;\
        virtual void OnRead(Peer *peer, const Messaging::ConnectionPtr connection) override; \
    };
    #endif
    
    struct Message {
        enum class Msg {
            NewPlayer,
            AcceptPlayer,
            ConnectPlayer,
            ReadyForGame,
            Command,
            Pause
        };
        
        static std::shared_ptr<Message> Parse(Network::InputMemoryStream &stream);
        
        virtual ~Message() = 0;
        virtual Msg Type() = 0;
        
        void Write(Peer *peer, const Messaging::ConnectionPtr connection);
        void Read(Peer *peer, const Messaging::ConnectionPtr connection);
        
    private:
        virtual void OnWrite(Peer *peer, const Messaging::ConnectionPtr connection) = 0;
        virtual void OnRead(Peer *peer, const Messaging::ConnectionPtr connection) = 0;
    };
    
    struct NewPlayerMessage : public Message {
    private:
        int id;
        std::string listenerAddress;
        
    public:
        NewPlayerMessage() {}
        NewPlayerMessage(int id, std::string listenerAddress) : id(id), listenerAddress(listenerAddress) {}
        
        virtual ~NewPlayerMessage() override {}
        virtual Msg Type() override { return Msg::NewPlayer; }
        
    private:
        virtual void OnWrite(Peer *peer, const Messaging::ConnectionPtr connection) override;
        virtual void OnRead(Peer *peer, const Messaging::ConnectionPtr connection) override;
        void ReadAddress(Network::InputMemoryStream &stream, std::string &address);
        void WriteAddress(Network::OutputMemoryStream &stream, const std::string &address);
    };
    
    MESSAGE(AcceptPlayer, Msg::AcceptPlayer)
    MESSAGE(ConnectPlayer, Msg::ConnectPlayer)
    MESSAGE(ReadyForGame, Msg::ReadyForGame)
    MESSAGE(Command, Msg::Command)
    MESSAGE(Pause, Msg::Pause)
};

#endif /* Peer_hpp */
