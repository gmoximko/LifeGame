//
//  Presets.hpp
//  LifeGame
//
//  Created by Максим Бакиров on 19.12.16.
//  Copyright © 2016 Arsonist (gmoximko@icloud.com). All rights reserved.
//

#ifndef Presets_hpp
#define Presets_hpp

#include <unordered_map>
#include <vector>
#include "Vector.hpp"

class Presets {
    std::unordered_map<unsigned char, std::vector<Vector>> presets;
    const char *const path;
    
public:
    explicit Presets(const char *path);
    void SaveOnDisk();
    void Save(unsigned char preset, const std::vector<Vector> *units);
    const std::vector<Vector> *Load(unsigned char preset) const;
};

#endif /* Presets_hpp */
