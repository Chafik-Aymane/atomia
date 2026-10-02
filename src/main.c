#include "raylib.h"
#include "atom.h"

#define MAX_ATOMS 10

int main(void) {
    const int width = 1600;
    const int height = 900;
    Vector2 resolution = { (float)width, (float)height };
    InitWindow(width, height, "ATOMIA");
    
    Atom atoms[MAX_ATOMS] = { 0 };

    atoms[0] = InitAtom(ELEMENT_C, (Vector2){ 0, 0 });
    atoms[1] = InitAtom(ELEMENT_H, (Vector2){ 70, 30 });
    atoms[2] = InitAtom(ELEMENT_H, (Vector2){ 140, 60 });
    atoms[3] = InitAtom(ELEMENT_H, (Vector2){ 210, 90 });
    atoms[4] = InitAtom(ELEMENT_O, (Vector2){ 280, 120 });
    atoms[5] = InitAtom(ELEMENT_H, (Vector2){ 350, 150 });
    atoms[6] = InitAtom(ELEMENT_O, (Vector2){ 420, 180 });
    atoms[7] = InitAtom(ELEMENT_C, (Vector2){ 490, 210 });
    atoms[8] = InitAtom(ELEMENT_C, (Vector2){ 560, 240 });
    atoms[9] = InitAtom(ELEMENT_C, (Vector2){ 630, 270 });

    int activeAtoms = 10;

    Shader waterShader = LoadShader(0, "resources/water.fs");
    Shader atomShader = LoadShader(0, "resources/atomShader.fs");
    int atomCenterLoc = GetShaderLocation(atomShader, "center");
    int atomRadiusLoc = GetShaderLocation(atomShader, "radius");
    int lightPosLoc = GetShaderLocation(atomShader, "lightPos");
    int timeLoc = GetShaderLocation(waterShader, "time");
    float seconds = 0.0f;
    int zoomLoc = GetShaderLocation(waterShader, "cameraZoom");
    int targetLoc = GetShaderLocation(waterShader, "cameraTarget");
    int camOffLoc = GetShaderLocation(waterShader, "cameraOffset");
    int resLoc = GetShaderLocation(waterShader, "screenRes");


    Camera2D camera = { 0 };
    camera.target = (Vector2){ 0, 0 };
    camera.offset = (Vector2){ width/2.0f, height/2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;
    Vector2 shaderOffset = { camera.offset.x, height - camera.offset.y };

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        // 1. Update
        seconds += GetFrameTime() + 0.008;
        shaderOffset = (Vector2){ camera.offset.x, (float)height - camera.offset.y };
        Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
        Vector2 lightSource = { (float)width / 2.0f, (float)height };

        for (int i = 0; i < activeAtoms; i++) {
            UpdateAtom(&atoms[i], 0.98f, mouseWorldPos);
        }

        int gridHeads[HASH_SIZE];
        int nextAtoms[MAX_ATOMS]; 
        for (int i = 0; i < HASH_SIZE; i++) gridHeads[i] = -1;

        for (int i = 0; i < activeAtoms; i++) {
            int hash = getHash(atoms[i].position);
            nextAtoms[i] = gridHeads[hash];
            gridHeads[hash] = i;
        }
        ResolveReactions(atoms, activeAtoms, gridHeads, nextAtoms);
        ResolveBonds(atoms, activeAtoms);
        ResolveCollisionsGrid(atoms, activeAtoms);

        SetShaderValue(waterShader, resLoc, &resolution, SHADER_UNIFORM_VEC2);
        SetShaderValue(waterShader, camOffLoc, &shaderOffset, SHADER_UNIFORM_VEC2);
        SetShaderValue(waterShader, timeLoc, &seconds, SHADER_UNIFORM_FLOAT);
        SetShaderValue(waterShader, zoomLoc, &camera.zoom, SHADER_UNIFORM_FLOAT);
        SetShaderValue(waterShader, targetLoc, &camera.target, SHADER_UNIFORM_VEC2);

        BeginDrawing(); // DRAWING 
                        
            float wheel = GetMouseWheelMove();

            if(wheel != 0){
                // Get the world point that      under the mouse
                Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
                // Set the offset to where the mouse is so we zoom "into" the cursor
                camera.offset = GetMousePosition();
                camera.target = mouseWorldPos;
                float zoomIncrement = 0.125f;
                camera.zoom += (wheel * zoomIncrement * camera.zoom);
                if (camera.zoom < 0.15f) camera.zoom = 0.15f;
                if (camera.zoom > 1.85f) camera.zoom = 1.85f; // Limit zoom in
            }

            BeginShaderMode(waterShader); // SHADER
                DrawRectangle(0, 0, width, height, WHITE);
                DrawFPS(10, 10);
            EndShaderMode();

            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
                Vector2 delta = GetMouseDelta();
                // We scale the movement by 1/zoom so panning feels consistent 
                // whether you are zoomed in or out.
                delta.x *= -1.0f / camera.zoom;
                delta.y *= -1.0f / camera.zoom;

                camera.target.x += delta.x;
                camera.target.y += delta.y;
            }
            
            // 2. Draw
            BeginMode2D(camera); // CAMERA

                DrawBonds(atoms, activeAtoms);
                for (int i = 0; i < activeAtoms; i++) {
                    // Update uniforms specifically for THIS atom
                    Vector2 screenPos = GetWorldToScreen2D(atoms[i].position, camera);
                    Vector2 shaderPos = { screenPos.x, (float)height - screenPos.y };
                    float renderedRadius = atoms[i].radius * camera.zoom;

                    BeginShaderMode(atomShader); // ATOM SHADER 
                        SetShaderValue(atomShader, atomCenterLoc, &shaderPos, SHADER_UNIFORM_VEC2);
                        SetShaderValue(atomShader, atomRadiusLoc, &renderedRadius, SHADER_UNIFORM_FLOAT);
                        SetShaderValue(atomShader, lightPosLoc, &lightSource, SHADER_UNIFORM_VEC2);

                        DrawAtom(atoms[i]);
                    EndShaderMode(); 
                }
            EndMode2D();
        EndDrawing();
    }

    UnloadShader(waterShader);
    CloseWindow();
    return 0;
}