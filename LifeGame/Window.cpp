//
//  Window.cpp
//  LifeGame
//
//  Created by Максим Бакиров on 15.12.16.
//  Copyright © 2016 Arsonist (gmoximko@icloud.com). All rights reserved.
//

#if defined(__APPLE__)
#include <GLUT/GLUT.h>
//#include <OpenGL/gl.h>
//#include <OpenGL/glu.h>
#else
#include <GL/glut.h>
//#include <GL/gl.h>
//#include <GL/glu.h>
#endif

#include <cassert>
#include <iostream>
#include <string>
#include <stdlib.h>
#include "Window.hpp"
#include "GameField.hpp"

using namespace Geometry;

const int Window::KeyMinus = 45;
const int Window::KeyPlus = 61;
const int Window::KeyEscape = 27;
const int Window::KeySpace = 32;

Window::Window() :
    cameraMoveSensititity(1.f),
    unitAngleStep(10.0f),
    unitRadiusRatio(0.3f),
    cellSizeRatioMin(0.005f),
    cellSizeRatioMax(0.1f),
    cellSizeRatioStep(0.0f),
    cellSizeRatio(0.005f),
    cellSize(0.0f),
    deltaTime(1000 / 30),
    window(0),
    loadedUnitsTRS(Matrix3x3::Identity()),
    rightButtonPressed(false),
    leftButtonPressed(false),
    cellSelected(false),
    cameraScrolled(false),
    selectedCells(new std::vector<Vector>()) {}

Window::~Window() {
    if (window != 0) {
        glutDestroyWindow(window);
    }
}

Window &Window::Instance() {
    static Window window;
    return window;
}

void Window::MainLoop(int &argc, char **argv, const std::string &label, Vector size) {
    if (gameField == nullptr) throw std::invalid_argument("GameField does not exist!");
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(size.x, size.y);
    window = glutCreateWindow(label.c_str());
    glutReshapeFunc(Window::Reshape);
    glutDisplayFunc(Window::Display);
    glutMouseFunc(Window::MouseFunc);
    glutKeyboardFunc(Window::KeyboardFunc);
    glutSpecialFunc(Window::SpecialFunc);
    glutMotionFunc(Window::MotionFunc);
    glutPassiveMotionFunc(Window::PassiveMotionFunc);
    glutTimerFunc(deltaTime, Window::Update, 0);
    if (gameField->TurnTime() > 0) {
        glutTimerFunc(gameField->TurnTime(), Window::Update, 1);
    }
    RecalculateSize();
    CreateMenu();
    glutMainLoop();
}

void Window::Display() {
    Window &instance = Instance();
    glClear(GL_COLOR_BUFFER_BIT);
    instance.DrawGrid();
    instance.DrawCell();
    instance.DrawPoints();
    instance.DrawNumbers();
//    instance.DrawRect();
    glutSwapBuffers();
}

void Window::Reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    Instance().RecalculateSize();
}

void Window::MouseFunc(int button, int state, int x, int y) {
    Window &instance = Instance();
    if (button == GLUT_LEFT_BUTTON) {
        instance.LeftMouseHandle(Vector(x, y), state == GLUT_DOWN);
    } else if (button == GLUT_RIGHT_BUTTON) {
        instance.RightMouseHandle(Vector(x, y), state == GLUT_DOWN);
    }
}

void Window::KeyboardFunc(unsigned char key, int x, int y) {
    Instance().KeyboardHandle(key, Vector(x, y));
}

void Window::SpecialFunc(int key, int x, int y) {
    Instance().SpecialHandle(key, Vector(x, y));
}

void Window::MotionFunc(int x, int y) {
    Window &instance = Instance();
    instance.cellSelected = false;
    instance.loadedUnits = nullptr;
    instance.mousePosition = Vector(x, y);
    if (instance.leftButtonPressed) {
        instance.CameraScroll(Vector(x, y));
    }
}

void Window::PassiveMotionFunc(int x, int y) {
    Instance().mousePosition = Vector(x, y);
}

void Window::Update(int value) {
    const Window &instance = Instance();
    switch (value) {
    case 0:
        instance.gameField->Update();
        instance.Refresh();
        glutTimerFunc(instance.deltaTime, Window::Update, 0);
        break;
    case 1:
        instance.gameField->Turn();
        glutTimerFunc(instance.gameField->TurnTime(), Window::Update, 1);
        break;
    }
}

void Window::CreateMenu() {
    int subMenu = glutCreateMenu(Window::Menu);
    const std::string label("Preset ");
    for (char key = '1'; key <= '5'; key++) {
        glutAddMenuEntry((label + key).c_str(), key);
    }
    glutCreateMenu(Window::Menu);
    glutAddSubMenu("Presets", subMenu);
    glutAddMenuEntry("Exit", 0);
    glutMenuStatusFunc(Window::MenuStatus);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void Window::Menu(int value) {
    Instance().MenuHandle(value);
}

void Window::MenuStatus(int status, int x, int y) {
    std::cout << status << std::endl;
    Window &instance = Instance();
    
    bool inUse = status == GLUT_MENU_IN_USE;
    instance.cellSelected = inUse;
    if (inUse) {
        Vector pos(x, y);
        if (instance.loadedUnits == nullptr) {
            instance.rightButtonPressedPos = pos;
            instance.mousePosition = pos;
        }
        instance.RightMouseHandle(pos, inUse);
    } else {
        instance.rightButtonPressed = false;
    }
}

void Window::DrawGrid() {
    if (cellSizeRatio <= cellSizeRatioMin) return;
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_LINES);
    for (float x = cellSize; x < windowSize.x; x += cellSize) {
        glVertex2f(x, 0.f);
        glVertex2f(x, windowSize.y);
    }
    for (float y = cellSize; y < windowSize.y; y += cellSize) {
        glVertex2f(0.f, y);
        glVertex2f(windowSize.x, y);
    }
    glEnd();
}

void Window::DrawPoints() {
    for (const auto &unit : *gameField->GetUnits()) {
        Vector pos(unit.position + cellOffset);
        gameField->ClampVector(pos);
        switch (unit.player) {
            case 0: glColor3f(1.0f, 0.0f, 0.0f); break;
            case 1: glColor3f(0.0f, 1.0f, 0.0f); break;
            case 2: glColor3f(0.0f, 0.0f, 1.0f); break;
            case 3: glColor3f(1.0f, 1.0f, 0.0f); break;
            case 4: glColor3f(1.0f, 0.0f, 1.0f); break;
            case 5: glColor3f(0.0f, 1.0f, 1.0f); break;
            case 6: glColor3f(1.0f, 1.0f, 1.0f); break;
            default:glColor3f(0.5f, 0.5f, 0.5f); break;
        }
        DrawPoint(pos);
    }
    if (loadedUnits == nullptr) return;
    glColor3f(1.0f, 1.0f, 0.5f);
    for (const auto &unit : *loadedUnits) {
        Vector pos = Matrix3x3::Translation(cellOffset) * loadedUnitsTRS * unit;
        gameField->ClampVector(pos);
        DrawPoint(pos);
    }
}

void Window::DrawPoint(Vector pos) {
    const float radius = 0.5f * cellSize;
    const float unitRadius = unitRadiusRatio * cellSize;
    const float x = pos.x * cellSize + radius;
    const float y = pos.y * cellSize + radius;
    glBegin(GL_POLYGON);
    for (float i = 0.f; i < 360.f; i += unitAngleStep) {
        const float radians = i * DegToRad;
        const float pointX = x + cosf(radians) * unitRadius;
        const float pointY = y + sinf(radians) * unitRadius;
        glVertex2f(pointX, pointY);
    }
    glEnd();
}

void Window::RecalculateSize() {
    windowSize = Vector(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
    const float sideRatio = windowSize.x > windowSize.y ? windowSize.y : windowSize.x;
    cellSize = sideRatio * cellSizeRatio;
}

void Window::DrawNumbers() {
    if (cellSizeRatio <= cellSizeRatioMin) return;
    glColor3f(1.0f, 1.0f, 1.0f);
    const float halfSize = cellSize * 0.5f;
    const Vector &fieldSize = gameField->GetSize();
    for (int x = 0; x < windowSize.x / cellSize; x++) {
        glRasterPos2f(x * cellSize + halfSize, 0.f);
        const int number = (x - cellOffset.x) % fieldSize.x;
        DrawNumber(number >= 0 ? number : fieldSize.x + number);
    }
    for (int y = 0; y < windowSize.y / cellSize; y++) {
        glRasterPos2f(0.f, y * cellSize + halfSize);
        const int number = (y - cellOffset.y) % fieldSize.y;
        DrawNumber(number >= 0 ? number : fieldSize.y + number);
    }
}

void Window::DrawNumber(int number) {
    const std::string &string = std::to_string(number);
    for (int i = 0; i < string.size(); i++) {
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_10, string[i]);
    }
}

void Window::DrawRect() {
    if (!rightButtonPressed || rightButtonPressedPos == mousePosition) {
        if (!selectedCells->empty()) selectedCells->clear();
        return;
    }
    const Vector min = rightButtonPressedPos;
    const Vector max = mousePosition;
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(min.x, windowSize.y - min.y);
    glVertex2f(min.x, windowSize.y - max.y);
    glVertex2f(max.x, windowSize.y - max.y);
    glVertex2f(max.x, windowSize.y - min.y);
    glEnd();
}

void Window::DrawCell() {
    if (!cellSelected || loadedUnits != nullptr) return;
    Vector cell = ScreenToCell(rightButtonPressedPos) + cellOffset;
    gameField->ClampVector(cell);
    const float cellX = cell.x * cellSize;
    const float cellY = cell.y * cellSize;
    glColor3f(1.f, 1.f, 0.5f);
    glBegin(GL_POLYGON);
    glVertex2f(cellX, cellY);
    glVertex2f(cellX, cellY + cellSize);
    glVertex2f(cellX + cellSize, cellY + cellSize);
    glVertex2f(cellX + cellSize, cellY);
    glEnd();
}

void Window::Zoom(float zoom) {
    const Vector oldCell = GetCellUnderMouse();
    cellSizeRatio += zoom;
    if (cellSizeRatio < cellSizeRatioMin) cellSizeRatio = cellSizeRatioMin;
    if (cellSizeRatio > cellSizeRatioMax) cellSizeRatio = cellSizeRatioMax;
    RecalculateSize();
    const Vector newCell = GetCellUnderMouse();
    cellOffset += newCell - oldCell;
}

void Window::LeftMouseHandle(Vector mousePos, bool pressed) {
    if (!pressed) {
        if (cameraScrolled) {
            cameraScrolled = false;
        } else if (loadedUnits != nullptr) {
            gameField->AddPreset(loadedUnitsTRS);
            loadedUnits = nullptr;
            cellSelected = false;
        } else {
            const Vector cell = ScreenToCell(mousePos);
            gameField->AddUnit(cell);
        }
    }
    if (!leftButtonPressed && pressed) {
        leftButtonPressed = true;
        leftButtonPressedPos = Vector(mousePos.x, windowSize.y - mousePos.y) - cellOffset * cellSize;
    } else if (leftButtonPressed && !pressed) {
        leftButtonPressed = false;
    }
}

void Window::RightMouseHandle(Vector mousePos, bool pressed) {
    if (!pressed) {
        if (rightButtonPressedPos == mousePosition) {
            cellSelected = true;
        }
    }
    if (!rightButtonPressed && pressed) {
        rightButtonPressed = true;
        if (loadedUnits == nullptr) {
            loadedUnitsTRS = Matrix3x3::Translation(ScreenToCell(mousePos));
        } else {
            loadedUnitsTRS = Matrix3x3::Translation(ScreenToCell(mousePos) - ScreenToCell(rightButtonPressedPos)) * loadedUnitsTRS;
        }
        rightButtonPressedPos = mousePos;
    } else if (rightButtonPressed && !pressed) {
        rightButtonPressed = false;
        CalulateSelectedCells();
    }
}

void Window::KeyboardHandle(unsigned char key, Vector mousePos) {
    switch (key) {
        case KeyEscape:
            gameField->Destroy();
            break;
        case KeySpace:
            if (gameField->TurnTime() == 0) {
                gameField->Turn();
            } else {
                gameField->Pause();
            }
            break;
        case KeyMinus:
            Zoom(-cellSizeRatioStep);
            break;
        case KeyPlus:
            Zoom(+cellSizeRatioStep);
            break;
        default:
            cellSelected = false;
            loadedUnits = nullptr;
            break;
    }
}

void Window::SpecialHandle(int key, Vector mousePos) {
    if (loadedUnits == nullptr) return;
    switch (key) {
        case GLUT_KEY_LEFT:  loadedUnitsTRS = loadedUnitsTRS * Matrix3x3::Rotation(90.f);       break;
        case GLUT_KEY_RIGHT: loadedUnitsTRS = loadedUnitsTRS * Matrix3x3::Rotation(-90.f);      break;
        case GLUT_KEY_UP:    loadedUnitsTRS = loadedUnitsTRS * Matrix3x3::Scale(Vector(1, -1)); break;
        case GLUT_KEY_DOWN:  loadedUnitsTRS = loadedUnitsTRS * Matrix3x3::Scale(Vector(-1, 1)); break;
    }
}

void Window::NumbersHandle(unsigned char key, Vector mousePos) {
    const auto cells = GetSelectedCells();
    if (!cells->empty()) {
        gameField->SavePreset(key, cells);
        return;
    }
    const auto newLoadedUnits = gameField->LoadPreset(key);
    if (loadedUnits != newLoadedUnits) {
        loadedUnits = newLoadedUnits;
    } else {
        loadedUnits = nullptr;
        cellSelected = false;
    }
}

void Window::CameraScroll(Vector pos) {
    const float sensitivity = cameraMoveSensititity;
    const Vector &fieldSize = gameField->GetSize();
    Vector newOffset = Vector(pos.x, windowSize.y - pos.y) - leftButtonPressedPos;
    cameraScrolled = Vector::Dot(newOffset, newOffset) >= cellSize * cellSize;
    newOffset *= sensitivity;
    newOffset /= cellSize;
    newOffset.x %= fieldSize.x;
    newOffset.y %= fieldSize.y;
    cellOffset = newOffset;
}

void Window::MenuHandle(int value) {
    switch (value) {
        case 0:
            gameField->Destroy();
            break;
        default:
            if (value > '0' && value <= '5') {
                NumbersHandle(value, mousePosition);
            }
            break;
    }
}

Vector Window::ScreenToCell(int x, int y) const {
    Vector result(x / cellSize, (windowSize.y - y) / cellSize);
    result -= cellOffset;
    gameField->ClampVector(result);
    return result;
}

Vector Window::ScreenToCell(Vector vec) const {
    return ScreenToCell(vec.x, vec.y);
}

Vector Window::CellToScreen(int x, int y) const {
    Vector result(x, y);
    result += cellOffset;
    gameField->ClampVector(result);
    result *= cellSize;
    result += Vector(cellSize, cellSize) / 2;
    result.y = windowSize.y - result.y;
    return result;
}

Vector Window::CellToScreen(Vector vec) const {
    return CellToScreen(vec.x, vec.y);
}

void Window::CalulateSelectedCells() const {
    selectedCells->clear();
    Vector min = mousePosition;
    Vector max = rightButtonPressedPos;
    if (min.x > max.x) {
        const int tempX = min.x;
        min.x = max.x;
        max.x = tempX;
    }
    if (min.y > max.y) {
        const int tempY = min.y;
        min.y = max.y;
        max.y = tempY;
    }
    const Rect screentRect(min, max - min);
    Vector cellMin = Vector::one * std::numeric_limits<int>::max();
    Vector cellMax = Vector::one * std::numeric_limits<int>::min();
    for (const auto &unit : *gameField->GetUnits()) {
        Vector screenUnit = CellToScreen(unit.position);
        if (screentRect.Contains(screenUnit)) {
            cellMin.x = screenUnit.x < cellMin.x ? screenUnit.x : cellMin.x;
            cellMin.y = screenUnit.y < cellMin.y ? screenUnit.y : cellMin.y;
            cellMax.x = screenUnit.x > cellMax.x ? screenUnit.x : cellMax.x;
            cellMax.y = screenUnit.y > cellMax.y ? screenUnit.y : cellMax.y;
            selectedCells->push_back(screenUnit);
        }
    }
    const Rect rect(cellMin, cellMax - cellMin);
    const Vector center = rect.Center();
    std::vector<Vector> &vec = *selectedCells.get();
    for (int i = 0; i < vec.size(); i++) {
        vec[i] = vec[i] - center;
        float cellX = vec[i].x / cellSize;
        float cellY = vec[i].y / cellSize;
        vec[i] = Vector(ceilf(cellX + 0.5f), -ceilf(cellY + 0.5f));
    }
}

const std::shared_ptr<std::vector<Vector>> Window::GetSelectedCells() const {
    if (rightButtonPressed) {
        CalulateSelectedCells();
    }
    return selectedCells;
}

void Window::Init(std::shared_ptr<GameField> gameField) {
    this->gameField = gameField;
}

void Window::Refresh() const {
    glutPostRedisplay();
}

Vector Window::GetCellUnderMouse() const {
    return ScreenToCell(mousePosition.x, mousePosition.y);
}
