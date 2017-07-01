//
//  Presets.cpp
//  LifeGame
//
//  Created by Максим Бакиров on 19.12.16.
//  Copyright © 2016 Arsonist (gmoximko@icloud.com). All rights reserved.
//

#include <fstream>
#include <cassert>
#include <string>
#include <iostream>
#include <regex>
#include "Directory.hpp"
#include "Pattern.hpp"
#include "Presets.hpp"

using namespace Geometry;

namespace Resources {

    Presets::Presets(const std::string &path) {
        std::regex regex("^([\\w\\-. ]+).rle$");
        Directory dir(path);
        
        std::string name = dir.Read();
        while (!name.empty()) {
            std::smatch match;
            name = dir.Read();
            if (!std::regex_match(name, match, regex)) continue;
            
            std::fstream file(dir.Name() + "/" + name, std::ios::in);
            if (!file.is_open()) continue;
            
            auto pattern = Pattern::FromRLE(file, name);
            if (pattern != nullptr) {
                patterns.emplace_back(std::move(pattern));
            }
        }
    }
    
    Presets::~Presets() = default;
    
    const std::string & Presets::GetName(std::size_t index) const {
        return patterns.at(index)->Name();
    }

    const std::vector<Vector> &Presets::GetUnits(std::size_t index) const {
        return patterns.at(index)->Units();
    }
    
    std::size_t Presets::GetSize(std::size_t index) const {
        return patterns.at(index)->Size();
    }

}
