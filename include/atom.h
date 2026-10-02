#ifndef ATOM_H
#define ATOM_H

#define GRID_CELL_SIZE 120
#define HASH_SIZE 1024
#define MAX_BONDS 4

#include "raylib.h"
#include "elements.h"

typedef struct Atom {
    Vector2 position;
    Vector2 velocity;
    float radius;
    Color color;
    bool isDragging;
    ElementType type;

    // Bonding information
    int currentBonds;
    int maxBonds;
    int bondedIndices[MAX_BONDS]; // Stores indices of atoms in the main array
    float affinity;               // Higher affinity = stronger desire to bond
} Atom;

Atom InitAtom(ElementType type, Vector2 pos);
void UpdateAtom(Atom *atom, float friction, Vector2 mousePos);
void DrawAtom(Atom atom);
void ResolveCollisionsGrid(Atom *atoms, int count);
int getHash(Vector2 pos);
void ResolveReactions(Atom *atoms, int count, int *gridHeads, int *nextAtoms);
void ResolveBonds(Atom *atoms, int count);
void DrawBonds(Atom *atoms, int count);

#endif