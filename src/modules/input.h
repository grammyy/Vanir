#ifndef VANIR_INPUT_H
#define VANIR_INPUT_H

#include <GLFW/glfw3.h>

int inputInit(lua_State* L);

#define keyBitmask 0x80

int getKey(lua_State *L);

/* ↓ glfw key callback; call glfwSetKeyCallback(win, cbKey) for each new window ↓ */
void cbKey(GLFWwindow *win, int key, int scancode, int action, int mods);

/* ↓ glfw mouse button callback; call glfwSetMouseButtonCallback(win, cbMouseButton) for each new window ↓ */
void cbMouseButton(GLFWwindow *win, int button, int action, int mods);

extern struct hook keyPress;
extern struct hook keyRelease;
extern struct hook inputPressed;
extern struct hook inputReleased;

#endif