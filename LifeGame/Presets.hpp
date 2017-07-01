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
#include <string>
#include <memory>
#include "Geometry.h"

namespace Resources {

    class Presets {
        std::vector<std::unique_ptr<class Pattern>> patterns;
        
    public:
        explicit Presets(const std::string &path);
        ~Presets();
        
        std::size_t Count() const { return patterns.size(); }
        const std::string &GetName(std::size_t index) const;
        const std::vector<Geometry::Vector> &GetUnits(std::size_t index) const;
        std::size_t GetSize(std::size_t index) const;
    };

}
#endif /* Presets_hpp */
