#include "atom.h"
#include <math.h>

Atom InitAtom(ElementType type, Vector2 pos) {
    ElementData data = ElementTable[type];

    return (Atom){
        .position = pos,
        .velocity = { 0, 0 },
        .radius = data.radius,
        .color = data.cpkColor,
        .isDragging = false,
        .type = type,

        .currentBonds = 0,
        .maxBonds = data.valence,
        .affinity = data.electronegativity, // Use electronegativity as affinity
        .bondedIndices = { -1, -1, -1, -1 }
    };
}

void UpdateAtom(Atom *atom, float friction, Vector2 mousePos) {
    float dt = GetFrameTime();
    if (dt > 0.1f) dt = 0.1f; // Prevent huge leaps during lag
    
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && 
        CheckCollisionPointCircle(mousePos, atom->position, atom->radius)) {
        atom->isDragging = true;
    }

    if (atom->isDragging) {
        // Smooth out the dragging velocity
        atom->velocity.x = (mousePos.x - atom->position.x) / dt;
        atom->velocity.y = (mousePos.y - atom->position.y) / dt;
        atom->position = mousePos;
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) atom->isDragging = false;
    } else {
        // Limit total speed BEFORE applying movement
        float maxSpeed = 400.0f; 
        float speedSq = atom->velocity.x * atom->velocity.x + atom->velocity.y * atom->velocity.y;
        
        if (speedSq > maxSpeed * maxSpeed) {
            float scale = maxSpeed / sqrtf(speedSq);
            atom->velocity.x *= scale;
            atom->velocity.y *= scale;
        }

        atom->position.x += atom->velocity.x * dt;
        atom->position.y += atom->velocity.y * dt;
        
        // Higher friction (0.95 - 0.98) helps stabilize bonded atoms
        atom->velocity.x *= friction;
        atom->velocity.y *= friction;

        float worldLim = 15000.0f; 
        // Screen bounce
        if (atom->position.x < -worldLim) {
            atom->position.x = -worldLim;
            atom->velocity.x *= -1;
        } else if (atom->position.x > worldLim) {
            atom->position.x = worldLim;
            atom->velocity.x *= -1;
        }

        if (atom->position.y < -worldLim) {
            atom->position.y = -worldLim;
            atom->velocity.y *= -1;
        } else if (atom->position.y > worldLim) {
            atom->position.y = worldLim;
            atom->velocity.y *= -1;
        }
    }
}

void DrawAtom(Atom atom) {
    DrawCircleV(atom.position, atom.radius, atom.color);
    DrawCircleLines(atom.position.x, atom.position.y, atom.radius + 2, RAYWHITE);
}

// helper that turns positions into a hash index    
int getHash(Vector2 pos) {
    int ix = (int)floorf(pos.x / GRID_CELL_SIZE);
    int iy = (int)floorf(pos.y / GRID_CELL_SIZE);
    
    // A simple spatial hash function
    unsigned int hash = ((unsigned int)ix * 73856093) ^ ((unsigned int)iy * 19349663);
    return hash % HASH_SIZE;
}

void ResolveCollisionsGrid(Atom *atoms, int count) {
    int gridHeads[HASH_SIZE];   // Stores the index of the first atom in this cell
    int nextAtoms[count];       // Stores the index of the "next" atom in the chain
    for (int i = 0; i < HASH_SIZE; i++) gridHeads[i] = -1; //reset grid

    // Build the spatial hash grid
    for (int i = 0; i < count; i++) {
        int hash = getHash(atoms[i].position);
        nextAtoms[i] = gridHeads[hash]; // Link to previous head
        gridHeads[hash] = i;            // Set this atom as the new head
    }

    // 3. Resolve collisions
    for (int i = 0; i < count; i++) {
        // Optimization: Skip sleeping atoms
        float speedSq = (atoms[i].velocity.x * atoms[i].velocity.x) + (atoms[i].velocity.y * atoms[i].velocity.y);
        bool active = (speedSq > 0.01f || atoms[i].isDragging);
        if (!active){
            for (int b = 0; b < atoms[i].currentBonds; b++) {
                if (atoms[atoms[i].bondedIndices[b]].isDragging) {
                    active = true;
                    break;
                }
            }
        }

        // Check the 9 neighboring cells (including its own)
        for (int ox = -1; ox <= 1; ox++) {
            for (int oy = -1; oy <= 1; oy++) {
                Vector2 neighborPos = { 
                    atoms[i].position.x + (ox * GRID_CELL_SIZE), 
                    atoms[i].position.y + (oy * GRID_CELL_SIZE) 
                };
                int hash = getHash(neighborPos);

                // Iterate through every atom in this neighbor cell
                int j = gridHeads[hash];
                while (j != -1) {
                    if (i != j) { // Don't collide with self
                        // Check if atoms are already bonded
                        bool directlyBonded = false;
                        for (int b = 0; b < atoms[i].currentBonds; b++) {
                            if (atoms[i].bondedIndices[b] == j) {
                                directlyBonded = true;
                                break;
                            }
                        }
                        
                        if (directlyBonded) {
                            j = nextAtoms[j];
                            continue;
                        }

                        // --- Standard Collision Logic ---
                        float dx = atoms[j].position.x - atoms[i].position.x;
                        float dy = atoms[j].position.y - atoms[i].position.y;
                        float distSq = dx*dx + dy*dy;
                        float minDist = atoms[i].radius + atoms[j].radius;

                        if (distSq < minDist * minDist) {
                            float distance = sqrtf(distSq);
                            float overlap = minDist - distance;
                            Vector2 normal = { dx / distance, dy / distance };

                            // Static Resolution
                            if (atoms[i].isDragging) {
                                // Atom i is the boss; move j entirely
                                atoms[j].position.x += normal.x * overlap;
                                atoms[j].position.y += normal.y * overlap;
                            } 
                            else if (atoms[j].isDragging) {
                                // Atom j is the boss; move i entirely
                                atoms[i].position.x -= normal.x * overlap;
                                atoms[i].position.y -= normal.y * overlap;
                            } 
                            else {
                                // Neither are being dragged; split the difference 50/50
                                atoms[i].position.x -= normal.x * overlap * 0.5f;
                                atoms[i].position.y -= normal.y * overlap * 0.5f;
                                atoms[j].position.x += normal.x * overlap * 0.5f;
                                atoms[j].position.y += normal.y * overlap * 0.5f;
                            }

                            // Dynamic Resolution
                            Vector2 relVel = { atoms[j].velocity.x - atoms[i].velocity.x, atoms[j].velocity.y - atoms[i].velocity.y };
                            float velAlongNormal = relVel.x * normal.x + relVel.y * normal.y;

                            if (velAlongNormal < 0) {
                                float j_impulse = -(1.5f) * velAlongNormal / 2.0f;
                                atoms[i].velocity.x -= j_impulse * normal.x;
                                atoms[i].velocity.y -= j_impulse * normal.y;
                                atoms[j].velocity.x += j_impulse * normal.x;
                                atoms[j].velocity.y += j_impulse * normal.y;
                            }
                        }
                    }
                    j = nextAtoms[j]; // Move to the next atom in this cell
                }
            }
        }
    }
}

void ResolveReactions(Atom *atoms, int count, int *gridHeads, int *nextAtoms) {
    float reactionRange = 60.0f; // The "snapping" distance

    for (int i = 0; i < count; i++) {
        // Does this atom have an open slot
        if (atoms[i].currentBonds >= atoms[i].maxBonds) continue;

        int hash = getHash(atoms[i].position);

        // Check 9 neighboring cells
        for (int ox = -1; ox <= 1; ox++) {
            for (int oy = -1; oy <= 1; oy++) {
                Vector2 neighborPos = { 
                    atoms[i].position.x + (ox * GRID_CELL_SIZE), 
                    atoms[i].position.y + (oy * GRID_CELL_SIZE) 
                };
                int neighborHash = getHash(neighborPos);

                int j = gridHeads[neighborHash];
                while (j != -1) {
                    // Check both i and j capacity inside the while loop
                    if (i != j && 
                        atoms[i].currentBonds < atoms[i].maxBonds && 
                        atoms[j].currentBonds < atoms[j].maxBonds) 
                    {
                        float dx = atoms[j].position.x - atoms[i].position.x;
                        float dy = atoms[j].position.y - atoms[i].position.y;
                        float distSq = dx*dx + dy*dy;

                        if (distSq < reactionRange * reactionRange) {
                            // Verify they arent already bonded
                            bool alreadyBonded = false;
                            for (int b = 0; b < atoms[i].currentBonds; b++) {
                                if (atoms[i].bondedIndices[b] == j) { alreadyBonded = true; break; }
                            }

                            if (!alreadyBonded) {
                                atoms[i].bondedIndices[atoms[i].currentBonds++] = j;
                                atoms[j].bondedIndices[atoms[j].currentBonds++] = i;
                            }
                        }
                    }
                    j = nextAtoms[j];
                }
            }
        }
        next_atom:;
    }
}

void ResolveBonds(Atom *atoms, int count) {
    float restDistance = 60.0f;
    float stiffness = 0.9f; 
    float angularStiffness = 0.0085f; 
    int iterations = 12; 
    float dt = GetFrameTime(); // Get delta time to sync velocity

    for (int iter = 0; iter < iterations; iter++) {
        for (int i = 0; i < count; i++) {
            
            // STABLE DISTANCE CONSTRAINT
            for (int b = 0; b < atoms[i].currentBonds; b++) {
                int j = atoms[i].bondedIndices[b];
                if (j < i) continue; 

                Vector2 delta = { atoms[j].position.x - atoms[i].position.x, atoms[j].position.y - atoms[i].position.y };
                float dist = sqrtf(delta.x * delta.x + delta.y * delta.y);
                if (dist < 0.1f) continue; 

                float diff = (dist - restDistance) / dist;
                Vector2 move = { delta.x * diff * 0.5f * stiffness, delta.y * diff * 0.5f * stiffness };

                // Update Positions
                atoms[i].position.x += move.x;
                atoms[i].position.y += move.y;
                atoms[j].position.x -= move.x;
                atoms[j].position.y -= move.y;

                // SYNC VELOCITY- Adjust velocity to match the position correction
                if (dt > 0) {
                    atoms[i].velocity.x += move.x / dt;
                    atoms[i].velocity.y += move.y / dt;
                    atoms[j].velocity.x -= move.x / dt;
                    atoms[j].velocity.y -= move.y / dt;
                }
            }

            // STABLE ANGULAR (VSEPR) CONSTRAINT
            if (atoms[i].currentBonds > 1) {
                float targetSpacing = (2.0f * PI) / atoms[i].currentBonds;
                
                float angles[MAX_BONDS];
                int indices[MAX_BONDS];
                for (int b = 0; b < atoms[i].currentBonds; b++) {
                    int id = atoms[i].bondedIndices[b];
                    Vector2 dir = { atoms[id].position.x - atoms[i].position.x, atoms[id].position.y - atoms[i].position.y };
                    angles[b] = atan2f(dir.y, dir.x);
                    indices[b] = id;
                }

                // Bubble Sort angles
                for (int m = 0; m < atoms[i].currentBonds - 1; m++) {
                    for (int n = 0; n < atoms[i].currentBonds - m - 1; n++) {
                        if (angles[n] > angles[n+1]) {
                            float tA = angles[n]; angles[n] = angles[n+1]; angles[n+1] = tA;
                            int tI = indices[n]; indices[n] = indices[n+1]; indices[n+1] = tI;
                        }
                    }
                }

                for (int b = 0; b < atoms[i].currentBonds; b++) {
                    int next = (b + 1) % atoms[i].currentBonds;
                    float currentGap = angles[next] - angles[b];
                    while (currentGap <= 0) currentGap += 2.0f * PI;
                    
                    float error = currentGap - targetSpacing;
                    if (error > PI) error -= 2.0f * PI;
                    if (error < -PI) error += 2.0f * PI;
                    
                    float nudge = error * angularStiffness;
                    int idA = indices[b];
                    int idB = indices[next];

                    Vector2 dirA = { atoms[idA].position.x - atoms[i].position.x, atoms[idA].position.y - atoms[i].position.y };
                    Vector2 dirB = { atoms[idB].position.x - atoms[i].position.x, atoms[idB].position.y - atoms[i].position.y };

                    float lenA = sqrtf(dirA.x * dirA.x + dirA.y * dirA.y);
                    float lenB = sqrtf(dirB.x * dirB.x + dirB.y * dirB.y);

                    if (lenA > 0.1f && lenB > 0.1f) {
                        Vector2 perpA = { -dirA.y / lenA, dirA.x / lenA };
                        Vector2 perpB = { -dirB.y / lenB, dirB.x / lenB };

                        Vector2 moveA = { perpA.x * nudge * restDistance * 0.1f, perpA.y * nudge * restDistance * 0.1f };
                        Vector2 moveB = { perpB.x * -nudge * restDistance * 0.1f, perpB.y * -nudge * restDistance * 0.1f };
                        Vector2 moveCenter = { (moveA.x + moveB.x) * -0.5f, (moveA.y + moveB.y) * -0.5f };

                        // Update Positions
                        atoms[idA].position.x += moveA.x;
                        atoms[idA].position.y += moveA.y;
                        atoms[idB].position.x += moveB.x;
                        atoms[idB].position.y += moveB.y;
                        atoms[i].position.x += moveCenter.x;
                        atoms[i].position.y += moveCenter.y;

                        //  Correct the velocity vectors for all three involved atoms
                        if (dt > 0) {
                            atoms[idA].velocity.x += moveA.x / dt;
                            atoms[idA].velocity.y += moveA.y / dt;
                            atoms[idB].velocity.x += moveB.x / dt;
                            atoms[idB].velocity.y += moveB.y / dt;
                            atoms[i].velocity.x += moveCenter.x / dt;
                            atoms[i].velocity.y += moveCenter.y / dt;
                        }
                    }
                }
            }
        }
    }
}

void DrawBonds(Atom *atoms, int count) {
    for (int i = 0; i < count; i++) {
        for (int b = 0; b < atoms[i].currentBonds; b++) {
            int j = atoms[i].bondedIndices[b];
            
            // Only draw each bond once (when i < j) to save performance
            if (j > i) {
                // Draw a thick line between the two atom positions
                DrawLineEx(atoms[i].position, atoms[j].position, 6.0f, Fade(RAYWHITE, 0.6f));
            }
        }
    }
}