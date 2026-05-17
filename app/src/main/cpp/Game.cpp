#include "Game.h"
#include "AndroidOut.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

Game::Game(struct android_app* app)
    : app_(app), currentLevel(1), playerX(2.0f), playerY(2.0f),
      playerDir(0.0f), playerHealth(100.0f), currentWeapon(Weapon::PISTOL),
      shootTimer(0.0f), animTimer(0.0f), walkTimer(0.0f), moving(false), firing(false) {
    loadLevel(1);
}

void Game::loadLevel(int level) {
    currentLevel = level;
    entities.clear();
    playerHealth = 100.0f;
    firing = false;
    animTimer = 0.0f;
    walkTimer = 0.0f;
    playerX = 2.0f; playerY = 2.0f; playerDir = 0.0f;

    if (level == 1) {
        currentWeapon = Weapon::PISTOL;
        map = {"##########","#........#","#........#","#........#","#........#","##########"};
        // 5 Inimigos espalhados
        for(int i=0; i<5; i++) {
            entities.push_back({4.0f + (float)i * 1.1f, 2.0f + (float)(i % 3) * 1.0f, 40.0f, true, false, true});
        }
    } else if (level == 2) {
        currentWeapon = Weapon::SHOTGUN;
        map = {"##############","#............#","#............#","#............#","#............#","##############"};
        // 10 Inimigos espalhados
        for(int i=0; i<10; i++) {
            float ex = 3.5f + (float)(i % 5) * 1.5f;
            float ey = 1.5f + (float)(i / 5) * 2.0f;
            entities.push_back({ex, ey, 50.0f, true, false, true});
        }
    } else if (level == 3) {
        currentWeapon = Weapon::BAZOOKA;
        map = {"################","#..............#","#..............#","#......B.......#","#..............#","#..............#","################"};
        playerX = 2.0f; playerY = 3.0f;
        entities.push_back({12.0f, 3.0f, 2000.0f, true, true, true}); // BOSS final
    }
}

void Game::update(float deltaTime) {
    if (shootTimer > 0) shootTimer -= deltaTime;

    if (firing) {
        animTimer += deltaTime;
        float duration = (currentWeapon == Weapon::BAZOOKA) ? 1.5f : 0.4f;
        if (animTimer >= duration) { firing = false; animTimer = 0; }
    }

    if (moving) walkTimer += deltaTime;
    else walkTimer *= 0.5f;

    moving = false;

    bool anyEnemyAlive = false;
    for (auto& e : entities) {
        if (e.active && e.isEnemy) {
            anyEnemyAlive = true;
            float dx = playerX - e.x, dy = playerY - e.y, dist = std::sqrt(dx*dx + dy*dy);
            if (dist > 0.8f) {
                float speed = (e.isBoss) ? 0.7f : 1.1f;
                float moveX = (dx / dist) * speed * deltaTime;
                float moveY = (dy / dist) * speed * deltaTime;

                // Colisão simples para os inimigos (não atravessam as paredes)
                if (map[(int)(e.y + moveY)][(int)(e.x + moveX)] != '#') {
                    e.x += moveX;
                    e.y += moveY;
                }
            } else {
                takeDamage(20.0f * deltaTime);
            }
        }
    }

    // Só passa de fase se não houver inimigos ativos
    if (!anyEnemyAlive) nextLevel();
}

void Game::takeDamage(float amount) {
    playerHealth -= amount;
    static float damageAcc = 0;
    damageAcc += amount;
    if (damageAcc >= 10.0f) {
        vibrate("vibrateLight");
        damageAcc = 0;
    }
    if (playerHealth <= 0) {
        vibrate("vibrateHeavy");
        loadLevel(currentLevel); // Reinicia a fase atual ao morrer
    }
}

void Game::nextLevel() {
    vibrate("playWin");
    vibrate("vibrateLevelComplete");
    if (currentLevel < 3) loadLevel(currentLevel + 1);
    else loadLevel(1); // Reinicia o jogo após o Boss
}

void Game::handleInput(float moveForward, float rotate) {
    if (std::abs(moveForward) > 0.01f) moving = true;
    playerDir += rotate;
    float nextX = playerX + std::cos(playerDir) * moveForward;
    float nextY = playerY + std::sin(playerDir) * moveForward;

    if (nextY >= 0 && nextY < (float)map.size() && nextX >= 0 && nextX < (float)map[0].size()) {
        if (map[(int)nextY][(int)nextX] != '#') { playerX = nextX; playerY = nextY; }
    }
}

void Game::shoot() {
    if (shootTimer <= 0) {
        firing = true; animTimer = 0;
        const char* sound = "playPistol";
        float dmg = 25.0f; shootTimer = 0.4f;
        if (currentWeapon == Weapon::SHOTGUN) { sound = "playShotgun"; dmg = 60.0f; shootTimer = 0.6f; }
        else if (currentWeapon == Weapon::BAZOOKA) { sound = "playBazooka"; dmg = 300.0f; shootTimer = 1.2f; }
        vibrate(sound);

        for (auto& e : entities) {
            if (e.active) {
                float dx = e.x - playerX, dy = e.y - playerY, dist = std::sqrt(dx*dx + dy*dy);
                float angle = std::atan2(dy, dx);
                float diff = std::fmod(angle - playerDir + M_PI, 2.0f * M_PI) - M_PI;
                // Hitbox baseada no ângulo e distância
                if (std::abs(diff) < 0.45f && dist < 12.0f) {
                    e.health -= dmg;
                    if (e.health <= 0) e.active = false;
                }
            }
        }
    }
}

void Game::vibrate(const char* methodName) {
    JNIEnv* env;
    if (app_->activity->vm->AttachCurrentThread(&env, nullptr) == JNI_OK) {
        jclass clazz = env->GetObjectClass(app_->activity->javaGameActivity);
        jmethodID method = env->GetMethodID(clazz, methodName, "()V");
        if (method) env->CallVoidMethod(app_->activity->javaGameActivity, method);
    }
}
