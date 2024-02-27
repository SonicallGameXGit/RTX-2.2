#include <windows.h>
#include <fstream>
#include <vector>
#include <unordered_map>
#include "engine/graphics.h"
#include "engine/math.h"
#include "engine/input.h"
#include "rtx.h"

int main() {
    if (!TT::Window::create(1920, 1080, "SUPER 3D YOPTA!", true, false)) {
        std::cerr << "Could not create a window...\n";
        return 1;
    }
    
    TT::AudioSystem::initialize();

    TT::Window::initializeImGui(TT_IMGUI_THEME_LIGHT);

    RTX::World::initialize("obby", 25.0f, glm::vec3(-1.0f, 1.0f, -0.175f));
    RTX::Camera::initialize(0.05f, 12.0f, 90.0f);
    RTX::Denoiser::initialize(TT::Window::getSize());

    RTX::Player player(glm::vec3(-1.5f, 5.0f, -1.5f), glm::vec3(), glm::vec3(0.4f, 1.76f, 0.4f));

    float fpsUpdateTime = 0.0f;
    int fps = 0;

    TT::Mouse::initialize();
    TT::Mouse::setGrabbed(true);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ALuint musicSound = TT::AudioSystem::loadFromFile("res/sounds/music.ogg");
    TT::SoundSource musicSoundSource(musicSound);
    musicSoundSource.play(0.5f, 1.0f, true);

    int frame = 0;
    int mouseGrabFrame = 0;

    glm::vec2 lastWindowSize = TT::Window::getSize();

    TT::Time time;
    while (TT::Window::isRunning()) {
        TT::Window::update();
        TT::Mouse::update();

        RTX::Denoiser::render(time, player);

        frame++;
        if (TT::Keyboard::isPressed(GLFW_KEY_ESCAPE)) {
            if (!mouseGrabFrame) {
                TT::Mouse::setGrabbed(!TT::Mouse::isGrabbed());
                mouseGrabFrame = frame;
            }
        } else mouseGrabFrame = 0;

        if (player.position.y <= -50.0f) player.respawn();

        glm::vec3 lastPlayerPos = glm::vec3(player.position);
        glm::vec3 lastPlayerAngle = glm::vec3(player.rotation);

        if(TT::Mouse::isGrabbed()) player.update(time);

        time.update();

        fpsUpdateTime += time.getDelta();
        fps++;
        
        if (fpsUpdateTime >= 1.0f) {
            TT::Window::setTitle(std::string("SUPER 3D YOPTA! Fps: " + std::to_string(fps)).c_str());

            fpsUpdateTime = 0.0f;
            fps = 0;
        }

        TT::Window::beginImGui();

        if (!TT::Mouse::isGrabbed())
            RTX::DebugHud::render(player);

        TT::Window::endImGui();

        if (lastPlayerPos != player.position || lastPlayerAngle != player.rotation) RTX::Denoiser::reset();

        glm::vec2 windowSize = TT::Window::getSize();
        if (lastWindowSize != windowSize) {
            lastWindowSize = glm::vec2(windowSize);

            if (RTX::DebugHud::getFrameScaleMode()) windowSize *= RTX::DebugHud::getFrameScale();
            else windowSize /= RTX::DebugHud::getFrameScale();

            RTX::Denoiser::resize(windowSize);
        }
    }

    RTX::World::clear();
    RTX::Denoiser::clear();

    player.clear();

    musicSoundSource.clear();

    TT::AudioSystem::clear(musicSound);
    TT::AudioSystem::clear();

    TT::Window::clearImGui();
    TT::Window::close();

    return 0;
}