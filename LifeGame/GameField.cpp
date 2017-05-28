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
#include "Presets.hpp"

using namespace Geometry;

GameField::GameField(std::shared_ptr<Presets> presets) : GameField(presets, Vector(), 0, -1) {}

GameField::GameField(std::shared_ptr<Presets> presets, Vector size, unsigned turnTime, int player) :
    presets(presets),
    player(player),
    exit(false),
    turnTime(turnTime),
    currentPreset(-1),
    size(size),
    units(new std::unordered_set<Unit>()) {}

void GameField::ClampVector(Vector &vec) const {
    vec.x %= size.x;
    vec.y %= size.y;
    if (vec.x < 0) vec.x = size.x + vec.x;
    if (vec.y < 0) vec.y = size.y + vec.y;
}

const std::bitset<32> oneOneOne(7);
void GameField::ProcessUnits() {
    std::unordered_map<Vector, std::bitset<32>> processCells;
    processCells.reserve(units->size() * 9);
    const size_t bucketCount = processCells.bucket_count();
    for (const auto &unit : *units) {
        ProcessUnit(unit, processCells);
    }
    assert(bucketCount == processCells.bucket_count());
    
    units->clear();
    for (const auto &cell : processCells) {
        const std::bitset<32> cellMask = cell.second;
        if (cellMask.none()) continue;
        uint32_t offset = 0;
        uint32_t maxNeighbours = 0;
        uint32_t self = 0;
        for (int i = 0; i < maxPlayers; i++) {
            const uint32_t shift = 4 * i;
            std::bitset<32> neighboursBit = cellMask & (oneOneOne << shift);
            neighboursBit >>= shift;
            uint32_t neighbours = static_cast<uint32_t>(neighboursBit.to_ulong());
            assert((neighboursBit >> 32).none());
            if (neighbours > maxNeighbours || (neighbours == maxNeighbours && Random::NextBool())) {
                maxNeighbours = neighbours;
                offset = i;
                self = cellMask[4 * (i + 1) - 1];
            }
        }
        assert(offset >= 0 && offset <= 7);
        assert(maxNeighbours >= 0 && maxNeighbours <= 7);
        if (maxNeighbours == 3 || (maxNeighbours == 2 && self != 0)) {
            units->emplace(offset, cell.first);
        }
    }
}

void GameField::ProcessUnit(const Unit &unit, std::unordered_map<Vector, std::bitset<32>> &processCells) {
    const uint32_t player = 4 * (unit.player + 1) - 1;
    const uint32_t offset = 4 * unit.player;
    
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            Vector pos = unit.position + Vector(x, y);
            ClampVector(pos);
            auto insertion = processCells.emplace(pos, 0);
            
            std::bitset<32> &cell = insertion.first->second;
            if (x == 0 && y == 0) {
                cell[player] = true;
            } else {
                std::bitset<32> neighbours = cell & (oneOneOne << offset);
                neighbours >>= offset;
                neighbours = std::bitset<32>(neighbours.to_ulong() + 1) & oneOneOne;
                assert(neighbours.to_ulong() >= 0 && neighbours.to_ulong() <= 7);
                cell &= ~(oneOneOne << offset);
                cell |= neighbours << offset;
            }
        }
    }
}

void GameField::AddPreset(const Matrix3x3 &matrix) {
    if (IsGameStopped()) return;
    const std::shared_ptr<std::vector<Vector>> loadedUnits = presets->Load(currentPreset);
    for (const auto &unit : *loadedUnits) {
        Vector vec = matrix * unit;
        ClampVector(vec);
        if (!CanInsert(vec)) return;
    }
    peer->AddPreset(matrix, currentPreset);
}

void GameField::AddPreset(const Matrix3x3 &matrix, int id, unsigned char preset) {
    std::shared_ptr<std::vector<Vector>> loadedUnits = presets->Load(preset);
    assert(loadedUnits != nullptr);
    for (int i = 0; i < loadedUnits->size(); i++) {
        Vector pos = matrix * loadedUnits->at(i);
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

void GameField::SavePreset(unsigned char preset, const std::shared_ptr<std::vector<Vector>> cells) {
    presets->Save(preset, cells);
}

const std::shared_ptr<std::vector<Vector>> GameField::LoadPreset(unsigned char preset) {
    currentPreset = preset;
    return presets->Load(preset);
}

bool GameField::IsGameStopped() const {
    return !peer->IsGameStarted() || peer->IsPause();
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
        std::exit(EXIT_SUCCESS);
    }
}

void GameField::Destroy() {
    if (exit) return;
    exit = true;
    presets->SaveOnDisk();
    peer->Destroy();
}
