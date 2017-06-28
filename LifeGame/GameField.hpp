//
//  GameField.hpp
//  LifeGame
//
//  Created by Максим Бакиров on 17.12.16.
//  Copyright © 2016 Arsonist (gmoximko@icloud.com). All rights reserved.
//

#ifndef GameField_hpp
#define GameField_hpp

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include "Geometry.h"

struct Unit {
    int player;
    Geometry::Vector position;
    
    Unit(int player, Geometry::Vector position) : player(player), position(position) {}
    
    friend bool operator == (const Unit &lhs, const Unit &rhs) {
        return lhs.position == rhs.position;
    }
};

template <>
struct std::hash<Unit> {
    size_t operator () (const Unit &unit) const {
        return std::hash<Geometry::Vector>()(unit.position);
    }
};

class GameField {
    static const int distanceToEnemy = 4;
    class Peer *peer;
    std::shared_ptr<class Presets> presets;
    std::shared_ptr<std::unordered_set<Unit>> units;
    Geometry::Vector size;
    int player;
    bool exit;
    bool playerWinsCell;
    unsigned turnTime;
    unsigned char currentPreset;
    
public:
    explicit GameField(const std::string &presetsPath);
    explicit GameField(const std::string &presetsPath, Geometry::Vector size, unsigned turnTime, int player);
    
    static const int maxPlayers = 8;
    
    int Player() const { return player; }
    unsigned TurnTime() const { return turnTime; }
    Geometry::Vector GetSize() const { return size; }
    bool IsInitialized() const { return player >= 0 && size.x > 0 && size.y > 0; }
    
    void SetPeer(Peer *peer) { this->peer = peer; }
    void SetTurnTime(unsigned turnTime) { this->turnTime = turnTime; }
    void SetSize(Geometry::Vector size) { this->size = size; }
    void SetPlayer(int player) { this->player = player; }
    
    const std::shared_ptr<std::unordered_set<Unit>> GetUnits() const { return units; }
    void ClampVector(Geometry::Vector &vec) const;
    void AddPreset(const Geometry::Matrix3x3 &matrix);
    void AddPreset(const Geometry::Matrix3x3 &matrix, int id, unsigned char preset);
    void AddUnit(Geometry::Vector unit);
    bool AddUnit(Geometry::Vector unit, int id);
    
    void SavePreset(unsigned char preset, const std::shared_ptr<std::vector<Geometry::Vector>> cells);
    const std::shared_ptr<std::vector<Geometry::Vector>> LoadPreset(unsigned char preset);
    
    void Turn();
    void Pause();
    void Update();
    void ProcessUnits();
    void Destroy();
    
private:
    void ProcessUnit(const Unit &unit, std::unordered_map<Geometry::Vector, uint32_t> &processCells);
    bool IsGameStopped() const;
    bool PlayerWonCell(int neighbours, int player);
    bool CanInsert(const Geometry::Vector &unit) const;
};

#endif /* GameField_hpp */
