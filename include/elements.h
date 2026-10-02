#ifndef ELEMENTS_H
#define ELEMENTS_H

#include "raylib.h"

typedef enum {
    ELEMENT_H = 0,
    ELEMENT_C,
    ELEMENT_O,
    ELEMENT_COUNT // Useful for loops later
} ElementType;

typedef struct {
    char name[16];
    Color cpkColor;
    float radius;
    ElementType type;
    int valence;
    float electronegativity; 
} ElementData;

extern const ElementData ElementTable[ELEMENT_COUNT];

#endif