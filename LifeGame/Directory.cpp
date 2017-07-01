//
//  Directory.cpp
//  LifeGame
//
//  Created by Максим Бакиров on 28.06.17.
//  Copyright © 2017 Arsonist (gmoximko@icloud.com). All rights reserved.
//

#include <cassert>
#include <errno.h>
#include "Utils.hpp"
#include "Directory.hpp"

namespace Resources {
    
    Directory::Directory(const std::string &path) : dir(opendir(path.c_str())), name(path) {
        if (dir == nullptr) {
            Log::Warning(path, "does not exist!");
        }
    }

    Directory::~Directory() {
        if (dir != nullptr) {
            closedir(dir);
        }
    }

    std::string Directory::Read() {
        std::string result;
        if (dir == nullptr) return result;
        
        errno = 0;
        dirent *info = readdir(dir);
        if (errno != 0) {
            Log::Error("Directory::Read failed!");
        }
        if (info == nullptr) {
            Reset();
        } else {
            result = info->d_name;
        }
        return result;
    }

    void Directory::Reset() {
        rewinddir(dir);
    }

}
