#include <GLFW/glfw3.h>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <algorithm>
#include <cstdio>
#include <string>
#include <ctime> // Added for time()

// ===== WORKAROUND FOR GCC 14 + MinGW =====
#define _GLIBCXX_USE_C99 1
#undef __STRICT_ANSI__  // Disable strict ANSI mode
extern "C" {
    void quick_exit(int status) { _Exit(status); }
    int at_quick_exit(void (*func)(void)) { return atexit(func); }
}
// =========================================
// Block types and colors
enum BlockType { AIR, GRASS, DIRT, STONE, WOOD, WATER, SAND };
const char* blockNames[] = {"Air", "Grass", "Dirt", "Stone", "Wood", "Water", "Sand"};

// Player state
struct Player {
    float x = 8.0f, y = 20.0f, z = 8.0f;  // Position
    float rx = 0.0f, ry = 0.0f;           // Rotation
    float dx = 0.0f, dy = 0.0f, dz = 0.0f; // Velocity
    bool onGround = false;
    int selectedSlot = 0;                  // Inventory selection
    float reach = 4.0f;                    // Block reach distance
} player;

// Inventory system
struct InventorySlot {
    BlockType type;
    int count;
};
std::vector<InventorySlot> inventory = {
    {GRASS, 10}, {DIRT, 5}, {STONE, 8}, {WOOD, 3}, {WATER, 4}, {SAND, 6}
};

// World data
const int WORLD_SIZE = 32;
const int WORLD_HEIGHT = 32;
BlockType world[WORLD_SIZE][WORLD_HEIGHT][WORLD_SIZE];

// Initialize world with terrain
void initWorld() {
    // Generate base terrain
    for (int x = 0; x < WORLD_SIZE; x++) {
        for (int z = 0; z < WORLD_SIZE; z++) {
            // Heightmap with hills and valleys
            float height = 5.0f + 3.0f * sin(x/5.0f) * cos(z/4.0f) + 2.0f * sin(z/3.0f);
            
            for (int y = 0; y < WORLD_HEIGHT; y++) {
                if (y < height) {
                    if (y == height - 1) {
                        world[x][y][z] = GRASS;
                    } else if (y > height - 4) {
                        world[x][y][z] = DIRT;
                    } else {
                        world[x][y][z] = STONE;
                    }
                } else if (y < 5) {
                    world[x][y][z] = WATER;
                } else {
                    world[x][y][z] = AIR;
                }
            }
            
            // Add some trees
            if (x > 5 && x < WORLD_SIZE-5 && z > 5 && z < WORLD_SIZE-5 && 
                rand() % 20 == 0 && height < WORLD_HEIGHT-6) {
                
                int treeHeight = 4 + rand() % 3;
                int treeX = x;
                int treeZ = z;
                int treeY = static_cast<int>(height);
                
                // Trunk
                for (int y = treeY; y < treeY + treeHeight; y++) {
                    if (y < WORLD_HEIGHT) world[treeX][y][treeZ] = WOOD;
                }
            }
            
            // Add sand near water
            if (height < 6 && rand() % 3 == 0) {
                world[x][static_cast<int>(height)][z] = SAND;
            }
        }
    }
}

// Get block color based on type
void getBlockColor(BlockType type, float& r, float& g, float& b) {
    switch (type) {
        case GRASS: r = 0.2f; g = 0.8f; b = 0.3f; break;
        case DIRT: r = 0.6f; g = 0.4f; b = 0.2f; break;
        case STONE: r = 0.5f; g = 0.5f; b = 0.5f; break;
        case WOOD: r = 0.5f; g = 0.35f; b = 0.15f; break;
        case WATER: r = 0.2f; g = 0.3f; b = 0.8f; break;
        case SAND: r = 0.9f; g = 0.8f; b = 0.5f; break;
        default: r = g = b = 0.0f;
    }
}

// Draw a colored cube at position
void drawCube(float x, float y, float z, float r, float g, float b, bool outline = false) {
    glPushMatrix();
    glTranslatef(x, y, z);
    
    if (outline) {
        glLineWidth(2.0f);
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
        glVertex3f(-0.505f, -0.505f, -0.505f);
        glVertex3f(0.505f, -0.505f, -0.505f);
        glVertex3f(0.505f, 0.505f, -0.505f);
        glVertex3f(-0.505f, 0.505f, -0.505f);
        glEnd();
    }
    
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    
    // Front face
    glVertex3f(-0.5f, -0.5f, 0.5f);
    glVertex3f(0.5f, -0.5f, 0.5f);
    glVertex3f(0.5f, 0.5f, 0.5f);
    glVertex3f(-0.5f, 0.5f, 0.5f);
    
    // Back face
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(-0.5f, 0.5f, -0.5f);
    glVertex3f(0.5f, 0.5f, -0.5f);
    glVertex3f(0.5f, -0.5f, -0.5f);
    
    // Top face
    glVertex3f(-0.5f, 0.5f, -0.5f);
    glVertex3f(-0.5f, 0.5f, 0.5f);
    glVertex3f(0.5f, 0.5f, 0.5f);
    glVertex3f(0.5f, 0.5f, -0.5f);
    
    // Bottom face
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(0.5f, -0.5f, -0.5f);
    glVertex3f(0.5f, -0.5f, 0.5f);
    glVertex3f(-0.5f, -0.5f, 0.5f);
    
    // Left face
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(-0.5f, -0.5f, 0.5f);
    glVertex3f(-0.5f, 0.5f, 0.5f);
    glVertex3f(-0.5f, 0.5f, -0.5f);
    
    // Right face
    glVertex3f(0.5f, -0.5f, -0.5f);
    glVertex3f(0.5f, 0.5f, -0.5f);
    glVertex3f(0.5f, 0.5f, 0.5f);
    glVertex3f(0.5f, -0.5f, 0.5f);
    
    glEnd();
    glPopMatrix();
}

// Draw text using GLFW (simple implementation)
void drawText(GLFWwindow* window, const char* text, float x, float y, float r, float g, float b) {
    // Switch to orthographic projection
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    glOrtho(0, width, height, 0, -1, 1);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    
    // Simple text rendering - one character at a time
    for (const char* c = text; *c != '\0'; c++) {
        // This is a simple placeholder - real implementation would use a texture atlas
        // For now we'll just draw a small quad for each character
        glBegin(GL_QUADS);
        glVertex2f(0, 0);
        glVertex2f(8, 0);
        glVertex2f(8, 12);
        glVertex2f(0, 12);
        glEnd();
        glTranslatef(8, 0, 0);
    }
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// Draw inventory UI
void drawInventory(GLFWwindow* window) {
    // Switch to orthographic projection
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    glOrtho(0, width, height, 0, -1, 1);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // Inventory background
    glColor4f(0.1f, 0.1f, 0.1f, 0.7f);
    glBegin(GL_QUADS);
    glVertex2f(width/2 - 200, height - 80);
    glVertex2f(width/2 + 200, height - 80);
    glVertex2f(width/2 + 200, height - 20);
    glVertex2f(width/2 - 200, height - 20);
    glEnd();
    
    // Slot positions
    const float slotSize = 50.0f;
    const float startX = width/2 - (inventory.size() * slotSize)/2;
    const float yPos = height - 70;
    
    for (int i = 0; i < inventory.size(); i++) {
        // Slot background
        if (i == player.selectedSlot) {
            glColor3f(0.4f, 0.4f, 0.8f); // Highlight selected
        } else {
            glColor3f(0.2f, 0.2f, 0.2f);
        }
        
        glBegin(GL_QUADS);
        glVertex2f(startX + i*slotSize, yPos);
        glVertex2f(startX + i*slotSize + slotSize, yPos);
        glVertex2f(startX + i*slotSize + slotSize, yPos + slotSize);
        glVertex2f(startX + i*slotSize, yPos + slotSize);
        glEnd();
        
        // Block preview
        if (inventory[i].type != AIR) {
            float r, g, b;
            getBlockColor(inventory[i].type, r, g, b);
            glColor3f(r, g, b);
            glBegin(GL_QUADS);
            glVertex2f(startX + i*slotSize + 10, yPos + 10);
            glVertex2f(startX + i*slotSize + 40, yPos + 10);
            glVertex2f(startX + i*slotSize + 40, yPos + 40);
            glVertex2f(startX + i*slotSize + 10, yPos + 40);
            glEnd();
            
            // Block count
            char countText[10];
            snprintf(countText, sizeof(countText), "%d", inventory[i].count);
            drawText(window, countText, startX + i*slotSize + 35, yPos + 45, 1.0f, 1.0f, 1.0f);
        }
    }
    
    // Selected block info
    char slotInfo[50];
    snprintf(slotInfo, sizeof(slotInfo), "Selected: %s (%d)", 
            blockNames[inventory[player.selectedSlot].type],
            inventory[player.selectedSlot].count);
    drawText(window, slotInfo, 10, 30, 1.0f, 1.0f, 1.0f);
    
    // Position info
    char posText[50];
    snprintf(posText, sizeof(posText), "Position: %.1f, %.1f, %.1f", 
            player.x, player.y, player.z);
    drawText(window, posText, 10, 10, 1.0f, 1.0f, 1.0f);
    
    // Controls info
    drawText(window, "WASD: Move | Space: Jump | Mouse: Look | LMB: Break | RMB: Place | 1-6: Select | ESC: Quit", 
            width/2 - 300, 10, 0.8f, 0.8f, 0.8f);
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// Handle keyboard input
void handleInput(GLFWwindow* window) {
    // Movement
    float speed = 0.1f;
    float moveX = 0.0f, moveZ = 0.0f;
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        moveZ -= speed;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        moveZ += speed;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        moveX -= speed;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        moveX += speed;
    }
    
    // Apply rotation to movement vector
    float rad = player.ry * (3.14159265f / 180.0f);
    player.dx = moveX * cos(rad) - moveZ * sin(rad);
    player.dz = moveX * sin(rad) + moveZ * cos(rad);
    
    // Jumping
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && player.onGround) {
        player.dy = 0.15f;
        player.onGround = false;
    }
    
    // Gravity
    player.dy -= 0.01f;
    if (player.dy < -0.3f) player.dy = -0.3f;
    
    // Apply movement
    player.x += player.dx;
    player.y += player.dy;
    player.z += player.dz;
    
    // Simple collision with ground
    if (player.y < 1.0f) {
        player.y = 1.0f;
        player.dy = 0.0f;
        player.onGround = true;
    }
    
    // Collision with blocks
    int playerX = static_cast<int>(player.x);
    int playerY = static_cast<int>(player.y);
    int playerZ = static_cast<int>(player.z);
    
    if (playerX >= 0 && playerX < WORLD_SIZE && 
        playerY >= 0 && playerY < WORLD_HEIGHT && 
        playerZ >= 0 && playerZ < WORLD_SIZE) {
        
        if (world[playerX][playerY][playerZ] != AIR) {
            player.y = playerY + 1.0f;
            player.dy = 0.0f;
            player.onGround = true;
        }
    }
    
    // Inventory selection
    static bool keyDebounce = false;
    for (int i = GLFW_KEY_1; i <= GLFW_KEY_6; i++) {
        if (glfwGetKey(window, i) == GLFW_PRESS && !keyDebounce) {
            int slot = i - GLFW_KEY_1;
            if (slot < inventory.size()) {
                player.selectedSlot = slot;
            }
            keyDebounce = true;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_RELEASE &&
        glfwGetKey(window, GLFW_KEY_2) == GLFW_RELEASE &&
        glfwGetKey(window, GLFW_KEY_3) == GLFW_RELEASE &&
        glfwGetKey(window, GLFW_KEY_4) == GLFW_RELEASE &&
        glfwGetKey(window, GLFW_KEY_5) == GLFW_RELEASE &&
        glfwGetKey(window, GLFW_KEY_6) == GLFW_RELEASE) {
        keyDebounce = false;
    }
}

// Handle block interaction
void handleBlockInteraction(GLFWwindow* window) {
    // Raycast from player position in look direction
    float step = 0.1f;
    float distance = 0.0f;
    int lastX = -1, lastY = -1, lastZ = -1;
    
    // Convert rotation to radians
    float yawRad = player.ry * (3.14159265f / 180.0f);
    float pitchRad = player.rx * (3.14159265f / 180.0f);
    
    // Direction vector
    float dx = -sin(yawRad) * cos(pitchRad);
    float dy = -sin(pitchRad);
    float dz = -cos(yawRad) * cos(pitchRad);
    
    while (distance < player.reach) {
        int checkX = static_cast<int>(player.x + dx * distance);
        int checkY = static_cast<int>(player.y + dy * distance);
        int checkZ = static_cast<int>(player.z + dz * distance);
        
        // Check bounds
        if (checkX < 0 || checkX >= WORLD_SIZE || 
            checkY < 0 || checkY >= WORLD_HEIGHT || 
            checkZ < 0 || checkZ >= WORLD_SIZE) {
            break;
        }
        
        if (world[checkX][checkY][checkZ] != AIR) {
            // Left mouse button - break block
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                BlockType brokenBlock = world[checkX][checkY][checkZ];
                world[checkX][checkY][checkZ] = AIR;
                
                // Add block to inventory
                for (auto& slot : inventory) {
                    if (slot.type == brokenBlock) {
                        slot.count++;
                        break;
                    }
                }
            }
            // Right mouse button - place block
            else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
                if (lastX >= 0 && lastY >= 0 && lastZ >= 0 && inventory[player.selectedSlot].count > 0) {
                    if (world[lastX][lastY][lastZ] == AIR) {
                        world[lastX][lastY][lastZ] = inventory[player.selectedSlot].type;
                        inventory[player.selectedSlot].count--;
                    }
                }
            }
            break;
        }
        
        lastX = checkX;
        lastY = checkY;
        lastZ = checkZ;
        
        distance += step;
    }
}

// Set perspective projection
void setPerspective(GLFWwindow* window) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    float aspect = static_cast<float>(width) / height;
    float fov = 65.0f;
    float near = 0.1f;
    float far = 100.0f;
    
    float f = 1.0f / tan(fov * 0.5f * 3.14159f / 180.0f);
    float range = near - far;
    
    float matrix[16] = {
        f / aspect, 0, 0, 0,
        0, f, 0, 0,
        0, 0, (far + near) / range, -1,
        0, 0, (2 * far * near) / range, 0
    };
    
    glMultMatrixf(matrix);
    glMatrixMode(GL_MODELVIEW);
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        return -1;
    }
    
    // Create window
    GLFWwindow* window = glfwCreateWindow(1024, 768, "BussCraft - Enhanced Edition", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    
    // Initialize world
    srand(static_cast<unsigned int>(time(NULL)));
    initWorld();
    
    // Basic OpenGL setup
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    // Hide cursor and capture it
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // Set initial cursor position
    glfwSetCursorPos(window, 512, 384);
    
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Update rotation based on mouse
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        player.ry += static_cast<float>(mx - 512) * 0.1f;
        player.rx += static_cast<float>(my - 384) * 0.1f;
        
        // Clamp vertical rotation
        if (player.rx > 89.0f) player.rx = 89.0f;
        if (player.rx < -89.0f) player.rx = -89.0f;
        
        // Reset cursor position
        glfwSetCursorPos(window, 512, 384);
        
        // Handle input
        handleInput(window);
        
        // Handle block interaction
        handleBlockInteraction(window);
        
        // Clear screen
        glClearColor(0.53f, 0.81f, 0.98f, 1.0f); // Sky blue
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Set perspective
        setPerspective(window);
        
        // Set camera position
        glLoadIdentity();
        glRotatef(player.rx, 1, 0, 0);
        glRotatef(player.ry, 0, 1, 0);
        glTranslatef(-player.x, -player.y, -player.z);
        
        // Render world
        for (int x = 0; x < WORLD_SIZE; x++) {
            for (int y = 0; y < WORLD_HEIGHT; y++) {
                for (int z = 0; z < WORLD_SIZE; z++) {
                    if (world[x][y][z] != AIR) {
                        float r, g, b;
                        getBlockColor(world[x][y][z], r, g, b);
                        drawCube(x, y, z, r, g, b);
                    }
                }
            }
        }
        
        // Render inventory and UI
        drawInventory(window);
        
        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
        
        // Exit on ESC
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }
    
    glfwTerminate();
    return 0;
}