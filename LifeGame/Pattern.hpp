//
//  Pattern.hpp
//  LifeGame
//
//  Created by Максим Бакиров on 28.06.17.
//  Copyright © 2017 Arsonist (gmoximko@icloud.com). All rights reserved.
//

#ifndef Pattern_hpp
#define Pattern_hpp

#include <string>
#include <vector>
#include <memory>
#include "Geometry.h"

namespace Resources {
    
    class Pattern {
        std::string name;
        std::vector<Geometry::Vector> units;
        
    public:
        static std::unique_ptr<Pattern> FromRLE(std::istream &stream, const std::string &name);
        
        const std::string &Name() const { return name; }
        const std::vector<Geometry::Vector> &Units() const { return units; }
        const size_t Size() const { return units.size(); }
        
    private:
        explicit Pattern() {}
        bool FillUnits(std::istream &rle, const Geometry::Vector size);
    };
    
}

#endif /* Pattern_hpp */
