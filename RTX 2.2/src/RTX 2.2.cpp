#include "engine/graphics.h"
#include "engine/math.h"
#include "engine/input.h"
#include <sys/types.h>
#include <sys/stat.h>

namespace RTX {
    class Player {
    public:
        const float walkSpeed = 7.0f, rotateSpeed = 0.09f;
        glm::vec3 position, rotation, scale, velocity;

        Player(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale) {
            this->position = glm::vec3(position);
            this->rotation = glm::vec3(rotation);
            this->scale = glm::vec3(scale);

            velocity = glm::vec3();
        }

        void update(Time time) {
            velocity = glm::vec3();

            if (RTX::Keyboard::isPressed(GLFW_KEY_W)) {
                velocity.x += sin(glm::radians(rotation.y));
                velocity.z += cos(glm::radians(rotation.y));
            }
            if (RTX::Keyboard::isPressed(GLFW_KEY_S)) {
                velocity.x += sin(glm::radians(rotation.y + 180.0f));
                velocity.z += cos(glm::radians(rotation.y + 180.0f));
            }
            if (RTX::Keyboard::isPressed(GLFW_KEY_D)) {
                velocity.x += sin(glm::radians(rotation.y + 90.0f));
                velocity.z += cos(glm::radians(rotation.y + 90.0f));
            }
            if (RTX::Keyboard::isPressed(GLFW_KEY_A)) {
                velocity.x += sin(glm::radians(rotation.y - 90.0f));
                velocity.z += cos(glm::radians(rotation.y - 90.0f));
            }
            if (RTX::Keyboard::isPressed(GLFW_KEY_SPACE)) velocity.y += 1.0f;
            if (RTX::Keyboard::isPressed(GLFW_KEY_LEFT_SHIFT)) velocity.y -= 1.0f;

            float horizontalLength = glm::length(glm::vec2(velocity.x, velocity.z));
            if (horizontalLength > 0.0f) {
                velocity.x /= horizontalLength;
                velocity.z /= horizontalLength;
            }

            position += velocity * walkSpeed * time.getDelta();

            rotation.x -= RTX::Mouse::getVelocity().y * rotateSpeed;
            rotation.x = fmax(fmin(rotation.x, 90.0f), -90.0f);

            rotation.y += RTX::Mouse::getVelocity().x * rotateSpeed;
            rotation.y -= floor(rotation.y / 360.0f) * 360.0f;
        }
    };
}

int main() {
    if (!RTX::Window::create(1920, 1080, "SUPER 3D YOPTA!", true, false))
        return 1;

    RTX::Window::initializeImGui(RTX_IMGUI_THEME_CLASSIC);

    RTX::ShaderProgram shaderProgram;
    shaderProgram.addShader(RTX::Shader("res/shaders/raytrace.vert", GL_VERTEX_SHADER));
    shaderProgram.addShader(RTX::Shader("res/shaders/raytrace.frag", GL_FRAGMENT_SHADER));
    shaderProgram.compile();

    RTX::Player player(glm::vec3(), glm::vec3(), glm::vec3(0.4f, 1.76f, 0.4f));

    float fpsUpdateTime = 0.0f;
    int fps = 0;

    RTX::Mouse::initialize();
    RTX::Mouse::setGrabbed(true);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float sunDirection[3] = { 0.3f, 0.4f, 0.3f };
    float shaderUpdateTime = 0.0f;

    int frame = 0;
    int mouseGrabFrame = 0;

    RTX::Time time;
    while (RTX::Window::isRunning()) {
        RTX::Window::update();
        RTX::Mouse::update();

        frame++;

        if (RTX::Keyboard::isPressed(GLFW_KEY_ESCAPE)) {
            if (!mouseGrabFrame) {
                RTX::Mouse::setGrabbed(!RTX::Mouse::isGrabbed());
                mouseGrabFrame = frame;
            }
        } else {
            mouseGrabFrame = 0;
        }

        if(RTX::Mouse::isGrabbed()) player.update(time);
        time.update();

        shaderUpdateTime += time.getDelta();

        fpsUpdateTime += time.getDelta();
        fps++;
        
        if (fpsUpdateTime >= 1.0f) {
            std::cout << fps << '\n';

            fpsUpdateTime = 0.0f;
            fps = 0;
        }

        shaderProgram.load();
        shaderProgram.setUniform("playerPosition", player.position);
        shaderProgram.setUniform("playerRotation", player.rotation);
        shaderProgram.setUniform("sunDirection", glm::vec3(sunDirection[0], sunDirection[1], sunDirection[2]));
        shaderProgram.setUniform("aspect", RTX::Window::getSize().x / RTX::Window::getSize().y);
        shaderProgram.setUniform("time", time.getTime());

        glBegin(GL_QUADS);
        glVertex2i(-1, -1);
        glVertex2i(1, -1);
        glVertex2i(1, 1);
        glVertex2i(-1, 1);
        glEnd();

        RTX::ShaderProgram::unload();

        RTX::Window::beginImGui();

        ImGui::Begin("Shader Editor");

        ImGui::DragFloat3("Sun Diretion", sunDirection, 0.005f, -1.0f, 1.0f);
        if (shaderUpdateTime >= 5.0f) {
            shaderUpdateTime = 0.0f;

            shaderProgram.clear();
            shaderProgram = RTX::ShaderProgram();
            shaderProgram.addShader(RTX::Shader("res/shaders/raytrace.vert", GL_VERTEX_SHADER));
            shaderProgram.addShader(RTX::Shader("res/shaders/raytrace.frag", GL_FRAGMENT_SHADER));
            shaderProgram.compile();
        }

        ImGui::End();

        RTX::Window::endImGui();
    }

    shaderProgram.clear();

    RTX::Window::clearImGui();
    RTX::Window::close();

    return 0;
}