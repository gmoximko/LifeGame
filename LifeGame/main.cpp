//
//  main.cpp
//  LifeGame
//
//  Created by Максим Бакиров on 10.12.16.
//  Copyright © 2016 Arsonist (gmoximko@icloud.com). All rights reserved.
//

#include <cstdlib>
#include <string>
#include <sstream>
#include <iostream>
#include "Window.hpp"
#include "GameField.hpp"
#include "Peer.hpp"

struct {
    Geometry::Vector field = Geometry::Vector(1000, 1000);
    Geometry::Vector window = Geometry::Vector(800, 600);
	std::string address;
    std::string presetPath = "rle";
    std::string label = "LifeGame";
    bool master = true;
    unsigned turnTime = 100;
    int players = 1;
    int distanceToEnemy = 4;
} args;

void Parse(int argc, char **argv);

int main(int argc, char **argv) {
    Parse(argc, argv);
    std::shared_ptr<Peer> peer;
    std::shared_ptr<GameField> gameField;
	
    if (args.master) {
        gameField = std::make_shared<GameField>(args.presetPath, args.field, args.turnTime, 0, args.distanceToEnemy);
        peer = std::make_shared<Peer>(gameField, args.players);
        args.address = peer->Address();
    } else {
        gameField = std::make_shared<GameField>(args.presetPath);
        peer = std::make_shared<Peer>(gameField, args.address);
    }
    peer->Init();
    Window &instance = Window::Instance();
    instance.Init(gameField);
	args.label += std::string(args.master ? " Master " : " Slave ") + args.address;
    instance.MainLoop(argc, argv, args.label, args.window);
    return 0;
}

void Parse(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (std::strcmp("field", argv[i]) == 0) {
            args.field.x = std::atoi(argv[++i]);
            args.field.y = std::atoi(argv[++i]);
        }
        if (std::strcmp("window", argv[i]) == 0) {
            args.window.x = std::atoi(argv[++i]);
            args.window.y = std::atoi(argv[++i]);
        }
        if (std::strcmp("server", argv[i]) == 0) {
			args.address = std::string(argv[++i]);
            args.master = false;
        }
        if (std::strcmp("presets", argv[i]) == 0) {
            args.presetPath = argv[++i];
        }
        if (std::strcmp("turn", argv[i]) == 0) {
            unsigned turnTime = std::atoi(argv[++i]);
            if (turnTime > 10) {
                turnTime = 10;
            }
            args.turnTime = turnTime > 0 ? 1000 / turnTime : 0;
        }
        if (std::strcmp("players", argv[i]) == 0) {
            int players = std::atoi(argv[++i]);
            if (players > 0) {
                if (players > GameField::maxPlayers) {
                    players = GameField::maxPlayers;
                }
                args.players = players;
            }
        }
        if (std::strcmp("distanceToEnemy", argv[i]) == 0) {
            args.distanceToEnemy = std::atoi(argv[++i]);
        }
    }
}
