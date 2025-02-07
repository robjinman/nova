#pragma once

#include "player.hpp"
#include <memory>

enum class KeyboardKey
{
  A = 'A', B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
  Space = 32,
  Escape = 256,
  Enter = 257
};

class Game
{
  public:
    virtual void onKeyDown(KeyboardKey key) = 0;
    virtual void onKeyUp(KeyboardKey key) = 0;
    virtual void onMouseMove(const Vec2f& delta) = 0;
    virtual void update() = 0;

    virtual ~Game() {}
};

using GamePtr = std::unique_ptr<Game>;

class CollisionSystem;
class Logger;

GamePtr createGame(PlayerPtr player, CollisionSystem& collisionSystem, Logger& logger);
