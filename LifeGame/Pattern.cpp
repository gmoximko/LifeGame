//
//  Pattern.cpp
//  LifeGame
//
//  Created by Максим Бакиров on 28.06.17.
//  Copyright © 2017 Arsonist (gmoximko@icloud.com). All rights reserved.
//

#include <istream>
#include <cassert>
#include <regex>
#include "Pattern.hpp"

using namespace Geometry;

namespace Resources {
    
    std::unique_ptr<Pattern> Pattern::FromRLE(std::istream &stream, const std::string &name) {
        std::unique_ptr<Pattern> result(new Pattern);
        std::regex rname("#[Nn] +([\\w\\-. ]+)");
        std::regex rsize("x *= *(\\d+).+y *= *(\\d+).*");
        Geometry::Vector size;
        
        std::string buffer;
        while (std::getline(stream, buffer)) {
            std::size_t r = buffer.find('\r');
            if (r != std::string::npos) {
                buffer.erase(r);
            }
            std::smatch match;
            if (std::regex_match(buffer, match, rname)) {
                assert(result->name.empty());
                assert(match.size() > 0);
                result->name = match.str(1);
            } else if (std::regex_match(buffer, match, rsize)) {
                assert(match.size() > 1);
                int x = std::atoi(match.str(1).c_str());
                int y = std::atoi(match.str(2).c_str());
                size = Vector(x, y);
                break;
            }
        }
        if (!result->FillUnits(stream, size)) {
            result.reset();
        } else if (result->name.empty()) {
            result->name = name;
        }
        return result;
    }
    
    bool Pattern::FillUnits(std::istream &rle, const Vector size) {
        if (size == Vector::zero) return false;
        std::regex regex("[\\dboxy$]+!?");
        Vector pos;
        
        std::string buffer;
        int digit = 0;
        
        while (std::getline(rle, buffer)) {
            std::size_t r = buffer.find('\r');
            if (r != std::string::npos) {
                buffer.erase(r);
            }
            std::smatch match;
            if (!std::regex_match(buffer, match, regex)) continue;
            const std::string &str = match.str();
            for (int i = 0; i < str.size(); i++) {
                const char c = str[i];
                if (c == '!') {
                    return true;
                } else if (std::isdigit(c)) {
                    digit *= 10;
                    digit += c - '0';
                } else {
                    digit = (digit > 0) ? digit : 1;
                    if (c == '$') {
                        pos.x = 0;
                        pos.y += digit;
                    } else if (std::tolower(c) == 'b') {
                        pos.x += digit;
                    } else /*if (std::tolower(c) == 'o')*/ {
                        for (int x = 0; x < digit; x++, pos.x++) {
                            Vector unit(pos.x, size.y - 1 - pos.y);
                            units.push_back(unit - size / 2);
                        }
                    }
                    digit = 0;
                }
                assert(pos.x >= 0 && pos.x <= size.x);
                assert(pos.y >= 0 && pos.y <= size.y);
            }
        }
        return false;
    }
    
}
