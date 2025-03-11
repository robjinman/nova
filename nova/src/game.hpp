#pragma once

#include "player.hpp"
#include <memory>

enum class KeyboardKey
{
  A = 'A', B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
  Space = 32,
  Escape = 256,
  Enter = 257,
  F1 = 290,
  F2 = 291,
  F3 = 292,
  F4 = 293,
  F5 = 294,
  F6 = 295,
  F7 = 296,
  F8 = 297,
  F9 = 298,
  F10 = 299,
  F11 = 300,
  F12 = 301
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
