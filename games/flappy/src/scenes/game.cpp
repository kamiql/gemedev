#include <algorithm>
#include <cmath>
#include <random>
#include <string>
#include <vector>
#include <iostream>

#include "gemedev/Application.hpp"
#include "scenes.hpp"
#include "../logic/HighScore.hpp"

namespace flappy::game {
    static gd::Entity background;
    static gd::Entity player;

    std::int64_t score = 0;

    struct PipePiece {
        gd::Entity entity;
        bool upper;
        int segment;
    };

    struct PipeSlot {
        float worldX = 0.0f;
        float gapCenterY = 0.0f;
        bool scored = false;
        std::vector<PipePiece> pieces;
    };

    void setup(gd::Application &app, gd::Scene &scene) {
        score = 0;
        scene.setPaused(true);

        const gd::TextureHandle backgroundTexture =
                app.assets().texture(asset("textures/background.png"));

        const gd::TextureHandle pipeTexture =
                app.assets().texture(asset("textures/pipe/pipe.png"));

        const gd::TextureHandle pipeDownTexture =
                app.assets().texture(asset("textures/pipe/pipe_down.png"));

        const gd::TextureHandle pipeUpTexture =
                app.assets().texture(asset("textures/pipe/pipe_up.png"));

        const gd::FontHandle font =
                app.assets().font(asset("textures/font/Silkscreen.ttf"));

        background = scene.createEntity("background");
        scene.add(background, gd::Transform{
                      .kind = gd::TransformKind::Absolute,
                      .position = {0.0f, 0.0f}
                  });

        gd::Panorama panorama;
        panorama.texture = backgroundTexture;
        panorama.parallax = 1.0f;
        panorama.drivesCamera = true;
        panorama.velocity = {250.0f, 0.0f};
        panorama.fit = gd::PanoramaFit::FitHeight;
        scene.add(background, panorama);

        auto playerAnimation =
                scene.assets().animation(asset("textures/player"));

        player = scene.createEntity("player");
        scene.add(player, gd::Transform{
                      .kind = gd::TransformKind::Relative,
                      .position = {0.0f, 0.0f},
                      .scale = {0.075f, 0.075f}
                  });
        scene.add(player, gd::Sprite{});
        scene.add(player, gd::Animation{
                      playerAnimation,
                      "idle",
                      0
                  });

        constexpr int minPipeDistance = 256;
        constexpr int maxPipeDistance = 1024;
        constexpr float pipeSpriteSize = 96.0f;
        constexpr float gapHeight = 220.0f;
        constexpr float gapEdgeMargin = 24.0f;

        const auto windowWidth = static_cast<float>(app.width());
        const auto windowHeight = static_cast<float>(app.height());

        const float minGapCenter =
                std::min(gapHeight * 0.5f + gapEdgeMargin, windowHeight * 0.5f);

        const float maxGapCenter = std::max(
            minGapCenter,
            windowHeight - gapHeight * 0.5f - gapEdgeMargin
        );

        const int pipeSegmentsPerSide =
                static_cast<int>(std::ceil(windowHeight / pipeSpriteSize)) + 2;

        const auto pipePoolSize = (app.width() + 2 * maxPipeDistance) / minPipeDistance + 2;

        scene.setOverlay(
            [font](gd::Canvas canvas) {
                canvas.text(
                    {
                        0.0f, 0.0f
                    },
                    std::to_string(score),
                    font
                );
            }
        );

        scene.setUpdateCallback(
            [
                velocityY = 0.0f,
                &app,
                pipeTexture,
                pipeDownTexture,
                pipeUpTexture,
                pipeDistance = std::uniform_int_distribution{
                    minPipeDistance,
                    maxPipeDistance
                },
                gapCenterDistribution =
                std::uniform_real_distribution{
                    minGapCenter,
                    maxGapCenter
                },
                rng = std::mt19937{std::random_device{}()},
                nextPipeX =
                scene.camera().x + static_cast<float>(app.width()),
                pipeSlots = std::vector<PipeSlot>{},
                nextPipeId = 0,
                pipePoolSize,
                pipeSegmentsPerSide,
                windowWidth,
                windowHeight
            ](
        gd::Scene &currentScene,
        const gd::Input &input,
        const float delta
    ) mutable {
                constexpr float gravityAcceleration = 600.0f;
                constexpr float jumpVelocity = -250.0f;
                constexpr float terminalVelocity = 400.0f;

                auto restart = [&]() {
                    highScore = std::max(highScore, score);
                    std::cout << std::max(highScore, score);
                    currentScene.setPaused(true);
                    currentScene.clear();
                    setup(app, currentScene);
                    app.scenes().activate("menu");
                };

                if (input.mousePressed(gd::MouseButton::Left) ||
                    input.keyPressed(gd::Key::Space)) {
                    velocityY = jumpVelocity;
                }

                velocityY = std::min(
                    velocityY + gravityAcceleration * delta,
                    terminalVelocity
                );

                currentScene.add(player, gd::Motion{
                                     {.x = 0.0f, .y = -velocityY / 250.0f}
                                 });

                gd::Transform *playerTransform =
                        currentScene.transform(player);

                if (!playerTransform) {
                    return;
                }

                const gd::Vec2 playerNdc = playerTransform->position;
                const float playerCenterX =
                        (playerNdc.x + 1.0f) * 0.5f * windowWidth;
                const float playerCenterY =
                        (1.0f - playerNdc.y) * 0.5f * windowHeight;

                const float playerWidth =
                        playerTransform->scale.x * windowWidth;
                const float playerHeight =
                        playerTransform->scale.y * windowHeight;

                const float playerLeft =
                        playerCenterX - playerWidth * 0.5f;
                const float playerRight =
                        playerCenterX + playerWidth * 0.5f;
                const float playerTop =
                        playerCenterY - playerHeight * 0.5f;
                const float playerBottom =
                        playerCenterY + playerHeight * 0.5f;

                if (playerTop <= 0.0f || playerBottom >= windowHeight) {
                    restart();
                    return;
                }

                if (gd::Animation *animation =
                        currentScene.animation(player)) {
                    if (velocityY > 0.0f) {
                        animation->clip = "fall";
                    } else if (velocityY < 0.0f) {
                        animation->clip = "jump";
                    }
                }

                const gd::Vec2 camera = currentScene.camera();

                auto placePipePieces = [&](PipeSlot &pipe) {
                    const float gapTop =
                            pipe.gapCenterY - gapHeight * 0.5f;
                    const float gapBottom =
                            pipe.gapCenterY + gapHeight * 0.5f;

                    for (PipePiece &piece: pipe.pieces) {
                        gd::Transform *transform =
                                currentScene.transform(piece.entity);

                        if (!transform) {
                            continue;
                        }

                        const float y = piece.upper
                                            ? gapTop -
                                              (static_cast<float>(piece.segment) + 1.0f) *
                                              pipeSpriteSize
                                            : gapBottom +
                                              static_cast<float>(piece.segment) *
                                              pipeSpriteSize;

                        transform->position = {pipe.worldX, y};
                    }
                };

                auto createPipe = [&]() {
                    PipeSlot pipe;
                    pipe.worldX = nextPipeX;
                    pipe.gapCenterY =
                            currentScene.camera().y +
                            gapCenterDistribution(rng);

                    for (int side = 0; side < 2; ++side) {
                        const bool upper = side == 0;

                        for (int segment = 0;
                             segment < pipeSegmentsPerSide;
                             ++segment) {
                            const gd::TextureHandle texture =
                                    segment == 0
                                        ? (upper
                                               ? pipeDownTexture
                                               : pipeUpTexture)
                                        : pipeTexture;

                            const gd::Entity entity =
                                    currentScene.createEntity(
                                        "pipe_" + std::to_string(nextPipeId++)
                                    );

                            currentScene.add(entity, gd::Transform{
                                                 .kind = gd::TransformKind::Absolute,
                                                 .position = {pipe.worldX, 0.0f},
                                                 .scale = {1.0f, 1.0f}
                                             });

                            currentScene.add(entity, gd::Sprite{
                                                 .size = [](const gd::Vec2) {
                                                     return gd::Vec2{
                                                         pipeSpriteSize,
                                                         pipeSpriteSize
                                                     };
                                                 },
                                                 .texture = texture
                                             });

                            pipe.pieces.push_back({
                                entity,
                                upper,
                                segment
                            });
                        }
                    }

                    placePipePieces(pipe);
                    return pipe;
                };

                while (pipeSlots.size() < pipePoolSize) {
                    nextPipeX += static_cast<float>(pipeDistance(rng));
                    pipeSlots.push_back(createPipe());
                }

                bool collision = false;

                for (PipeSlot &pipe: pipeSlots) {
                    if (pipe.worldX + pipeSpriteSize < camera.x) {
                        nextPipeX += static_cast<float>(pipeDistance(rng));
                        pipe.worldX = nextPipeX;
                        pipe.gapCenterY =
                                camera.y + gapCenterDistribution(rng);
                        pipe.scored = false;
                        placePipePieces(pipe);
                    }

                    const float pipeLeft = pipe.worldX - camera.x;
                    const float pipeRight = pipeLeft + pipeSpriteSize;

                    if (!pipe.scored && pipeRight < playerLeft) {
                        ++score;
                        pipe.scored = true;
                    }

                    if (playerRight <= pipeLeft ||
                        playerLeft >= pipeRight) {
                        continue;
                    }

                    const float gapCenterScreen =
                            pipe.gapCenterY - camera.y;
                    const float gapTop =
                            gapCenterScreen - gapHeight * 0.5f;
                    const float gapBottom =
                            gapCenterScreen + gapHeight * 0.5f;

                    if (playerTop <= gapTop ||
                        playerBottom >= gapBottom) {
                        collision = true;
                        break;
                    }
                }

                if (collision) {
                    restart();
                }
            }
        );
    }
}
