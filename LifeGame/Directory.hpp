//
//  Directory.hpp
//  LifeGame
//
//  Created by Максим Бакиров on 28.06.17.
//  Copyright © 2017 Arsonist (gmoximko@icloud.com). All rights reserved.
//

#ifndef Directory_hpp
#define Directory_hpp

#include <string>
#include <dirent.h>

namespace Resources {
    
    class Directory {
        DIR *dir;
        const std::string name;
        
    public:
        explicit Directory(const std::string &dir);
        ~Directory();
        
        const std::string &Name() const { return name; }
        std::string Read();
        void Reset();
    };

}
    
#endif /* Directory_hpp */
