//
//  GameField.hpp
//  LifeGame
//
//  Created by Максим Бакиров on 17.12.16.
//  Copyright © 2016 Arsonist (gmoximko@icloud.com). All rights reserved.
//

#ifndef GameField_hpp
#define GameField_hpp

#include <unordered_set>
#include <vector>
#include <memory>
#include "Presets.hpp"
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
    class Peer *peer;
    std::unique_ptr<Resources::Presets> presets;
    std::shared_ptr<std::unordered_set<Unit>> units;
    Geometry::Vector size;
    int player;
    int distanceToEnemy;
    bool exit;
    bool playerWinsCell;
    unsigned turnTime;
    int currentPreset;
    
public:
    static const int maxPlayers = 8;
    
    explicit GameField(const std::string &presetsPath);
    explicit GameField(const std::string &presetsPath, Geometry::Vector size, unsigned turnTime, int player, int distanceToEnemy);
    
    int Player() const { return player; }
    unsigned TurnTime() const { return turnTime; }
    Geometry::Vector GetSize() const { return size; }
    int DistanceToEnemy() const { return distanceToEnemy; }
    bool IsInitialized() const { return player >= 0 && size.x > 0 && size.y > 0; }
    
    void SetPeer(Peer *peer) { this->peer = peer; }
    void SetTurnTime(unsigned turnTime) { this->turnTime = turnTime; }
    void SetSize(Geometry::Vector size) { this->size = size; }
    void SetPlayer(int player) { this->player = player; }
    void SetDistanceToEnemy(int distanceToEnemy) { this->distanceToEnemy = distanceToEnemy; }
    
    const std::shared_ptr<std::unordered_set<Unit>> GetUnits() const { return units; }
    void ClampVector(Geometry::Vector &vec) const;
    void AddPreset(const Geometry::Matrix3x3 &matrix);
    void AddPreset(const Geometry::Matrix3x3 &matrix, int id, int preset);
    void AddUnit(Geometry::Vector unit);
    bool AddUnit(Geometry::Vector unit, int id);
    
    void ConfiguratePatterns(std::vector<std::pair<std::string, std::size_t>> &patterns) const;
    void SavePreset(unsigned char preset, const std::shared_ptr<std::vector<Geometry::Vector>> cells);
    const std::vector<Geometry::Vector> &LoadPreset(int preset);
    
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
