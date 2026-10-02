#include "elements.h"

const ElementData ElementTable[ELEMENT_COUNT] = {
    // Name,       Color,                Radius,  Type,     Valence, Electronegativity
    { "Hydrogen", { 255, 255, 255, 255 }, 12.0f, ELEMENT_H, 1,       2.20f },
    { "Carbon",   { 30, 30, 30, 255 },    20.0f, ELEMENT_C, 4,       2.55f },
    { "Oxygen",   { 255, 0, 0, 255 },     18.0f, ELEMENT_O, 2,       3.44f }
};