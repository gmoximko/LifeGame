//
//  GameField.cpp
//  LifeGame
//
//  Created by Максим Бакиров on 17.12.16.
//  Copyright © 2016 Arsonist (gmoximko@icloud.com). All rights reserved.
//

#include <cstdlib>
#include <cassert>
#include "GameField.hpp"
#include "Peer.hpp"

using namespace Geometry;

GameField::GameField(const std::string &presetsPath) : GameField(presetsPath, Vector(), 0, -1, 0) {}

GameField::GameField(const std::string &presetsPath, Vector size, unsigned turnTime, int player, int distanceToEnemy) :
    presets(new Resources::Presets(presetsPath)),
    player(player),
    exit(false),
    playerWinsCell(false),
    turnTime(turnTime),
    currentPreset(-1),
    size(size),
    distanceToEnemy(distanceToEnemy),
    units(new std::unordered_set<Unit>()) {}

void GameField::ClampVector(Vector &vec) const {
    vec.x %= size.x;
    vec.y %= size.y;
    if (vec.x < 0) vec.x = size.x + vec.x;
    if (vec.y < 0) vec.y = size.y + vec.y;
}

void GameField::ProcessUnits() {
    std::unordered_map<Vector, uint32_t> processCells;
    processCells.reserve(units->size() * 9);
    const size_t bucketCount = processCells.bucket_count();
    for (const auto &unit : *units) {
        ProcessUnit(unit, processCells);
    }
    assert(bucketCount == processCells.bucket_count());
    
    units->clear();
    const uint32_t oneOneOne = 7;
    for (const auto &cell : processCells) {
        const uint32_t cellMask = cell.second;
        if (cellMask == 0) continue;
        uint32_t offset = 0;
        uint32_t maxNeighbours = 0;
        uint32_t self = 0;
        bool occupy = false;
        for (int i = 0; i < maxPlayers; i++) {
            const uint32_t shift = 4 * i;
            uint32_t neighbours = cellMask & (oneOneOne << shift);
            neighbours >>= shift;
            if (neighbours > maxNeighbours) {
                occupy = true;
                maxNeighbours = neighbours;
                offset = i;
                self = cellMask & (static_cast<uint32_t>(1) << (4 * (i + 1) - 1));
            } else if (neighbours == maxNeighbours) {
                occupy = false;
            }
        }
        if (!occupy) continue;
        assert(offset >= 0 && offset <= 7);
        assert(maxNeighbours >= 0 && maxNeighbours <= 7);
        if (maxNeighbours == 3 || (maxNeighbours == 2 && self != 0)) {
            units->emplace(offset, cell.first);
        }
    }
}

void GameField::ProcessUnit(const Unit &unit, std::unordered_map<Vector, uint32_t> &processCells) {
    const uint32_t player = static_cast<uint32_t>(1) << (4 * (unit.player + 1) - 1);
    const uint32_t offset = 4 * unit.player;
    const uint32_t oneOneOne = 7;
    
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            Vector pos = unit.position + Vector(x, y);
            ClampVector(pos);
            auto insertion = processCells.emplace(pos, 0);
            
            if (x == 0 && y == 0) {
                insertion.first->second |= player;
            } else {
                uint32_t &cell = insertion.first->second;
                uint32_t neighbours = cell & (oneOneOne << offset);
                neighbours >>= offset;
                neighbours = (neighbours + 1) & oneOneOne;
                assert(neighbours >= 0 && neighbours <= 7);
                cell &= ~(oneOneOne << offset);
                cell |= neighbours << offset;
            }
        }
    }
}

void GameField::AddPreset(const Matrix3x3 &matrix) {
    if (IsGameStopped()) return;
    const std::vector<Vector> loadedUnits = presets->GetUnits(currentPreset);
    for (const auto &unit : loadedUnits) {
        Vector vec = matrix * unit;
        ClampVector(vec);
        if (!CanInsert(vec)) return;
    }
    peer->AddPreset(matrix, currentPreset);
}

void GameField::AddPreset(const Matrix3x3 &matrix, int id, int preset) {
    const std::vector<Vector> &loadedUnits = presets->GetUnits(preset);
    for (int i = 0; i < loadedUnits.size(); i++) {
        Vector pos = matrix * loadedUnits.at(i);
        ClampVector(pos);
        AddUnit(pos, id);
    }
}

void GameField::AddUnit(Vector unit) {
    if (IsGameStopped()) return;
    const Unit key(player, unit);
    if (units->find(key) == units->end() && CanInsert(unit)) {
        peer->AddUnit(unit);
    }
}

bool GameField::AddUnit(Vector unit, int id) {
    return units->emplace(id, unit).second;
}

bool GameField::CanInsert(const Vector &unit) const {
    const int halfDistance = distanceToEnemy;
    for (int x = -halfDistance; x <= halfDistance; x++) {
        for (int y = -halfDistance; y <= halfDistance; y++) {
            Vector vec = unit + Vector(x, y);
            ClampVector(vec);
            const auto search = units->find(Unit(player, vec));
            if (search != units->end() && search->player != player) return false;
        }
    }
    return true;
}

void GameField::ConfiguratePatterns(std::vector<std::pair<std::string, std::size_t>> &patterns) const {
    const std::size_t size = presets->Count();
    for (std::size_t i = 0; i < size; i++) {
        patterns.emplace_back(presets->GetName(i), presets->GetSize(i));
    }
}

void GameField::SavePreset(unsigned char preset, const std::shared_ptr<std::vector<Vector>> cells) {
    Log::Error("GameField::SavePreset deprecated!");
}

const std::vector<Vector> &GameField::LoadPreset(int preset) {
    currentPreset = preset;
    return presets->GetUnits(preset);
}

bool GameField::IsGameStopped() const {
    return !peer->IsGameStarted() || peer->IsPause();
}

bool GameField::PlayerWonCell(int neighbours, int player) {
    const bool even = neighbours % 2 == 0;
    const bool playerEven = player % 2 == 0;
    playerWinsCell = !playerWinsCell;
    return playerWinsCell ? even == playerEven : even != playerEven;
}

void GameField::Turn() {
    if (IsGameStopped()) return;
    peer->Turn();
}

void GameField::Pause() {
    peer->Pause();
}

void GameField::Update() {
    peer->Update();
    if (peer->Destroyed()) {
		peer->Cleanup();
        presets.reset();
        std::exit(EXIT_SUCCESS);
    }
}

void GameField::Destroy() {
    if (exit) return;
    exit = true;
    peer->Destroy();
}
