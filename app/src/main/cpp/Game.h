#ifndef GAME_H
#define GAME_H

#include <vector>
#include <string>
#include <memory>
#include <jni.h>
#include <android/native_window.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>

enum class Weapon { NONE, PISTOL, SHOTGUN, BAZOOKA };

struct Entity {
    float x, y;
    float health;
    bool isEnemy;
    bool isBoss;
    bool active;
};

class Game {
public:
    Game(struct android_app* app);
    void update(float deltaTime);
    void handleInput(float moveForward, float rotate);
    void shoot();

    int getCurrentLevel() const { return currentLevel; }
    float getPlayerX() const { return playerX; }
    float getPlayerY() const { return playerY; }
    float getPlayerDir() const { return playerDir; }
    Weapon getWeapon() const { return currentWeapon; }
    float getPlayerHealth() const { return playerHealth; }

    const std::vector<std::string>& getMap() const { return map; }
    const std::vector<Entity>& getEntities() const { return entities; }

    bool isMoving() const { return moving; }
    bool isFiring() const { return firing; }
    float getAnimTimer() const { return animTimer; }
    float getWalkTimer() const { return walkTimer; }

private:
    void loadLevel(int level);
    void nextLevel();
    void vibrate(const char* methodName);
    void takeDamage(float amount);

    struct android_app* app_;
    int currentLevel;
    float playerX, playerY;
    float playerDir;
    float playerHealth;
    Weapon currentWeapon;

    std::vector<std::string> map;
    std::vector<Entity> entities;

    float shootTimer;
    float animTimer;
    float walkTimer;
    bool moving;
    bool firing;
};

#endif
