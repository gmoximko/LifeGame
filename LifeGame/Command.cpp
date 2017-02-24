//
//  Command.cpp
//  LifeGame
//
//  Created by Максим Бакиров on 24.02.17.
//  Copyright © 2017 Arsonist (gmoximko@icloud.com). All rights reserved.
//

#include "Peer.hpp"
#include "Log.hpp"
#include "Command.hpp"

using namespace Network;
using namespace Geometry;

Command::~Command() {}

std::shared_ptr<Command> Command::Parse(InputMemoryStream &stream) {
    int32_t cmd;
    stream.Read(cmd, stream.Size());
    std::shared_ptr<Command> result;
    switch (static_cast<Cmd>(cmd)) {
        case Cmd::AddUnits:
            result = std::make_shared<AddUnitsCommand>();
            break;
        default:
            return nullptr;
    }
    result->Read(stream);
    return result;
}

void Command::Read(InputMemoryStream &stream) {
    int32_t cmd;
    stream >> cmd;
    if (static_cast<Cmd>(cmd) != Type()) {
        Log::Error("Commands types are not the same!");
    }
    OnRead(stream);
}

void Command::Write(OutputMemoryStream &stream) {
    stream << static_cast<int32_t>(Type());
    OnWrite(stream);
}

void AddUnitsCommand::Apply(Peer *peer) {
    for (int i = 0; i < units.size(); i++) {
        peer->AddUnit(units[i], id);
    }
}

void AddUnitsCommand::OnRead(Network::InputMemoryStream &stream) {
    int32_t id, size;
    stream >> id >> size;
    for (int i = 0; i < size; i++) {
        int32_t x, y;
        stream >> x >> y;
        units.push_back(Vector(static_cast<int>(x), static_cast<int>(y)));
    }
    this->id = static_cast<int>(id);
}

void AddUnitsCommand::OnWrite(Network::OutputMemoryStream &stream) {
    stream << static_cast<int32_t>(id) << static_cast<int32_t>(units.size());
    for (int i = 0; i < units.size(); i++) {
        stream << static_cast<int32_t>(units[i].x) << static_cast<int32_t>(units[i].y);
    }
}