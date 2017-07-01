//
//  Window.hpp
//  LifeGame
//
//  Created by Максим Бакиров on 15.12.16.
//  Copyright © 2016 Arsonist (gmoximko@icloud.com). All rights reserved.
//

#ifndef Window_hpp
#define Window_hpp

#include <string>
#include <memory>
#include <vector>
#include "Geometry.h"

class Window {
    const static int KeyMinus;
    const static int KeyPlus;
    const static int KeyEscape;
    const static int KeySpace;
    
    mutable std::shared_ptr<std::vector<Geometry::Vector>> selectedCells;
    Geometry::Vector rightButtonPressedPos;
    Geometry::Vector leftButtonPressedPos;
    Geometry::Vector cellOffset;
    Geometry::Vector windowSize;
    Geometry::Vector mousePosition;
    Geometry::Matrix3x3 loadedUnitsTRS;
    
    int window;
    bool cellSelected;
    bool cameraScrolled;
    bool rightButtonPressed;
    bool leftButtonPressed;
    float cellSizeRatio;
    float cellSize;
    const float cameraMoveSensititity;
    const float unitAngleStep;
    const float unitRadiusRatio;
    const float cellSizeRatioMin;
    const float cellSizeRatioMax;
    const float cellSizeRatioStep;
    const unsigned deltaTime;
    
    std::shared_ptr<class GameField> gameField;
    const std::vector<Geometry::Vector> *loadedUnits;
    std::vector<std::pair<std::string, std::size_t>> patterns;
    
public:
    static Window &Instance();
    
    void MainLoop(int &argc, char **argv, const std::string &label, Geometry::Vector size);
    void Init(std::shared_ptr<class GameField> gameField);
    void Refresh() const;
    Geometry::Vector GetCellUnderMouse() const;
    const std::shared_ptr<std::vector<Geometry::Vector>> GetSelectedCells() const;
    
private:
    static void Reshape(int w, int h);
    static void Display();
    static void MouseFunc(int button, int state, int x, int y);
    static void KeyboardFunc(unsigned char key, int x, int y);
    static void SpecialFunc(int key, int x, int y);
    static void MotionFunc(int x, int y);
    static void PassiveMotionFunc(int x, int y);
    static void Update(int value);
    static void CreateMenu();
    static void Menu(int value);
    static void MenuStatus(int status, int x, int y);
    
    void DrawGrid();
    void DrawPoints();
    void DrawPoint(Geometry::Vector pos);
    void RecalculateSize();
    void DrawNumbers();
    void DrawNumber(int number);
    void DrawRect();
    void DrawCell();
    
    void Zoom(float zoom);
    void LeftMouseHandle(Geometry::Vector mousePos, bool pressed);
    void RightMouseHandle(Geometry::Vector mousePos, bool pressed);
    void KeyboardHandle(unsigned char key, Geometry::Vector mousePos);
    void SpecialHandle(int key, Geometry::Vector mousePos);
    void NumbersHandle(int key, Geometry::Vector mousePos);
    void CameraScroll(Geometry::Vector pos);
    void MenuHandle(int value);
    Geometry::Vector ScreenToCell(int x, int y) const;
    Geometry::Vector ScreenToCell(Geometry::Vector vec) const;
    Geometry::Vector CellToScreen(int x, int y) const;
    Geometry::Vector CellToScreen(Geometry::Vector vec) const;
    void CalulateSelectedCells() const;
    
    explicit Window();
    ~Window();
    Window(const Window &other) = delete;
    Window &operator = (const Window &other) = delete;
};

#endif /* Window_hpp */
