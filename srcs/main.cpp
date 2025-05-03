#include "shoot_3d.hpp"

void setup_camera(Camera3D& camera) {
    camera.position = Vector3{ 0.0f, 1.0f, 4.0f };
    camera.target = Vector3{ 0.0f, 0.0f, 0.0f };
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    camera.fovy = 70.0f;
    camera.projection = CAMERA_PERSPECTIVE;
}

entt::entity create_player(entt::registry& registry) {
    entt::entity player = registry.create();
    registry.emplace<Position>(player, Vector3{ 0, 0, 0 });
    registry.emplace<Velocity>(player, Vector3{ 0, 0, 0 });
    registry.emplace<Rotation>(player, Vector3{ 0, 0, 0 });
    registry.emplace<Body>(player, 1.0f, BLUE);
    registry.emplace<HP>(player, 100, 100);
    registry.emplace<Damage>(player, 10);
    registry.emplace<Player>(player, PLAYER_SPEED, SHOOT_COOLDOWN, 0.0f);
    return player;
}

void spawn_initial_enemies(entt::registry& registry) {
    for (int i = 0; i < ENEMY_COUNT; i++) {
        float x = GetRandomValue(-ARENA_SIZE + 5, ARENA_SIZE - 5);
        float z = GetRandomValue(-ARENA_SIZE + 5, ARENA_SIZE - 5);
        spawn_enemy(registry, Vector3{ x, 0, z });
    }
}

void reset_game(entt::registry& registry) {
    registry.clear();
    create_player(registry);
    spawn_initial_enemies(registry);
}

int main() {
    InitWindow(1280, 720, "3D Space Shooter");
    SetTargetFPS(60);
    DisableCursor();

    Camera3D camera;
    setup_camera(camera);

    entt::registry registry;
    create_player(registry);
    spawn_initial_enemies(registry);

    // Load sunlight shader with appropriate fallbacks
    // Attempt to load custom shader, fall back to default if there's an issue
    Shader sunlightShader = {};
    
    // First try to load the custom shader
    sunlightShader = LoadShader("shaders/sunlight.vs", "shaders/sunlight.fs");
    
    // Check if shader loaded successfully (non-zero ID)
    if (sunlightShader.id == 0) {
        // If custom shader failed, use the default shader (NULL, NULL loads default)
        TraceLog(LOG_WARNING, "Custom shader failed to load. Using default shader.");
        sunlightShader = LoadShader(NULL, NULL);
    }
    
    // Set up shader locations for uniform variables - these might not exist in default shader
    int lightDirLoc = GetShaderLocation(sunlightShader, "lightDirection");
    int lightColorLoc = GetShaderLocation(sunlightShader, "lightColor");
    int ambientColorLoc = GetShaderLocation(sunlightShader, "ambientColor");
    int objectColorLoc = GetShaderLocation(sunlightShader, "objectColor");
    
    // Set up lighting parameters
    Vector3 lightDir = Vector3Normalize(Vector3{ -1.0f, -1.0f, -0.5f });
    Vector3 lightColor = { 1.0f, 0.0f, 0.0f };
    Vector3 ambientColor = { 0.0f, 0.0f, 1.0f }; // Increased ambient for better visibility
    
    // Only set shader values if the location is valid (>= 0)
    if (lightDirLoc >= 0) SetShaderValue(sunlightShader, lightDirLoc, &lightDir, SHADER_UNIFORM_VEC3);
    else std::cout << "light dir missing\n";
	if (lightColorLoc >= 0) SetShaderValue(sunlightShader, lightColorLoc, &lightColor, SHADER_UNIFORM_VEC3);
    else std::cout << "light color missing\n";
    if (ambientColorLoc >= 0) SetShaderValue(sunlightShader, ambientColorLoc, &ambientColor, SHADER_UNIFORM_VEC3);
    else std::cout << "Error3\n";
    
    // Debug trace
    TraceLog(LOG_INFO, "Shader loaded. Object color location: %d", objectColorLoc);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
    
        // Update systems
        ecs_system::player_update(registry, dt);
        ecs_system::enemy_update(registry, dt);
        ecs_system::entity_movement(registry, dt);
        ecs_system::entity_collision(registry);
        ecs_system::entity_lifetime(registry, dt);
        ecs_system::entity_cleanup(registry);
        ecs_system::enemy_respawn(registry);
    
        // Update camera based on player position
        auto playerView = registry.view<Player, Position, Rotation>();
        for (auto entity : playerView) {
            Position& pos = playerView.get<Position>(entity);
            Rotation& rot = playerView.get<Rotation>(entity);
            Vector3 forward = GetForwardVector(rot);
            camera.position = Vector3Add(pos.value, Vector3Scale(forward, -4.0f));
            camera.position.y += 3.0f;
            camera.target = Vector3Add(pos.value, Vector3Scale(forward, 10.0f));
        }
    
        BeginDrawing();
        ClearBackground(BLACK);
    
        // Begin 3D mode
        BeginMode3D(camera);
        
        // Draw the grid without shader
        DrawGrid(ARENA_SIZE * 2, 10);
        
        // Draw 3D objects - first pass without shader to ensure visibility
        auto renderView = registry.view<Position, Body>();
        
        // FIRST PASS: Draw all objects with default lighting to ensure they're visible
        for (auto entity : renderView) {
            Position& pos = renderView.get<Position>(entity);
            Body& body = renderView.get<Body>(entity);
            
            // Draw the entity's sphere with default rendering
            DrawSphere(pos.value, body.radius, body.color);
            
            // Draw direction indicator lines
            if (registry.all_of<Rotation>(entity)) {
                Rotation& rot = registry.get<Rotation>(entity);
                Vector3 forward = GetForwardVector(rot);
                Vector3 lineEnd = Vector3Add(pos.value, Vector3Scale(forward, body.radius * 2));
                DrawLine3D(pos.value, lineEnd, WHITE);
            }
        }
        
        // OPTIONAL SECOND PASS: Try with custom shader if you want lighting effects
        // Comment this block out if objects still don't appear
        if (false) { // Change to true to enable shader pass
            BeginShaderMode(sunlightShader);
            
            for (auto entity : renderView) {
                Position& pos = renderView.get<Position>(entity);
                Body& body = renderView.get<Body>(entity);
                
                // Convert color to floating point (0.0-1.0)
                float colorVec[4] = { 
                    body.color.r / 255.0f, 
                    body.color.g / 255.0f, 
                    body.color.b / 255.0f, 
                    body.color.a / 255.0f 
                };
                
                // Only set object color if the location exists
                if (objectColorLoc >= 0) {
                    SetShaderValue(sunlightShader, objectColorLoc, colorVec, SHADER_UNIFORM_VEC4);
                }
                
                // Draw the entity's sphere with shader
                DrawSphere(pos.value, body.radius, body.color);
            }
            
            EndShaderMode();
        }
        
        // Draw health bars after ending shader mode
        auto healthView = registry.view<Position, Body, HP>();
        for (auto entity : healthView) {
            Position& pos = healthView.get<Position>(entity);
            Body& body = healthView.get<Body>(entity);
            HP& hp = healthView.get<HP>(entity);
            
            Vector2 screen = GetWorldToScreen(Vector3Add(pos.value, { 0, body.radius * 2, 0 }), camera);
            float w = 40, h = 4, pct = (float)hp.value / hp.maxValue;
            DrawRectangle(screen.x - w / 2, screen.y, w, h, GRAY);
            DrawRectangle(screen.x - w / 2, screen.y, w * pct, h, GREEN);
        }
        
        // End 3D mode
        EndMode3D();
    
        // Draw 2D overlays
        DrawFPS(10, 10);
    
        auto playerHealthView = registry.view<Player, HP>();
        if (playerHealthView.begin() != playerHealthView.end()) {
            HP& hp = playerHealthView.get<HP>(playerHealthView.front());
            DrawText(TextFormat("HP: %d/%d", hp.value, hp.maxValue), 10, 40, 20, WHITE);
            DrawText("WASD to move, Arrows to turn, LMB/Space to shoot", 10, 70, 20, WHITE);
        } else {
            const char* msg = "GAME OVER - PRESS R TO RESTART";
            int w = MeasureText(msg, 40);
            DrawText(msg, GetScreenWidth() / 2 - w / 2, GetScreenHeight() / 2, 40, RED);
            if (IsKeyPressed(KEY_R))
                reset_game(registry);
        }
    
        EndDrawing();
    }

    UnloadShader(sunlightShader);
    CloseWindow();
    return 0;
}