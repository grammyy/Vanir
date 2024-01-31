#include <SDL.h>
#include <SDL_opengl.h>
#include "render.h"

void drawLine(float x1, float y1, float x2, float y2) {
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //glEnable(GL_LINE_SMOOTH);
    //glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glColor4f(0.0f, 0.0f, 0.0f, 1.0f); // Black with full alpha

    glBegin(GL_LINES);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glEnd();

    //glDisable(GL_LINE_SMOOTH);
    //glDisable(GL_BLEND);
}