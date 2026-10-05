# Лабораторные работы по компьютерной графике

**Отчёты:** [docs/314_Александров_ЛабX.pdf]

## Лабораторная работа №1

**Вариант 2:** правильный октаэдр.

### Условие

Программа должна работать в реальном времени с возможностью динамической смены
проекции и трансформаций объектов. Все объекты должны корректно отрисовываться
с учётом проекции и иметь возможность взаимодействия с пользователем через ImGui.

### Требования

- C++20 компилятор (GCC 10+, Clang 10+, MSVC 2019+)
- CMake 3.21+
- Vulkan SDK 1.4+
- GLFW, glm, vk-bootstrap, VMA, ImGui — скачиваются автоматически через CMake `FetchContent`

```bash

## Сборка

### Linux (GCC/Clang)

cmake --preset debug
cmake --build build-debug --parallel

### Windows (Visual Studio 2019+)

cmake --preset msvc-debug
cmake --build build-debug --parallel

### Windows (MinGW)

cmake --preset mingw-debug
cmake --build build-debug --parallel

## Запуск

### Linux

    ./build-debug/vulkan-starter-app
