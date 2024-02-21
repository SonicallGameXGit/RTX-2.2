#include "engine/graphics.h"
#include "engine/math.h"
#include "engine/input.h"

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

    RTX::ShaderProgram raytraceProgram;
    raytraceProgram.addShader(RTX::Shader("res/shaders/raytrace.vert", GL_VERTEX_SHADER));
    raytraceProgram.addShader(RTX::Shader("res/shaders/raytrace.frag", GL_FRAGMENT_SHADER));
    raytraceProgram.compile();

    RTX::ShaderProgram screenProgram;
    screenProgram.addShader(RTX::Shader("res/shaders/screen.vert", GL_VERTEX_SHADER));
    screenProgram.addShader(RTX::Shader("res/shaders/screen.frag", GL_FRAGMENT_SHADER));
    screenProgram.compile();

    RTX::FrameBuffer frameBuffer = RTX::FrameBuffer((int)RTX::Window::getSize().x, (int)RTX::Window::getSize().y);
    RTX::FrameBuffer lastFrameBuffer = RTX::FrameBuffer((int)RTX::Window::getSize().x, (int)RTX::Window::getSize().y);

    int skyboxTexture = RTX::Texture::loadFromFile("res/textures/skybox.png", GL_LINEAR);

    RTX::Player player(glm::vec3(), glm::vec3(), glm::vec3(0.4f, 1.76f, 0.4f));

    float fpsUpdateTime = 0.0f;
    int fps = 0;

    RTX::Mouse::initialize();
    RTX::Mouse::setGrabbed(true);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float sunDirection[3] = { 0.3f, 0.4f, 0.3f };
    float shaderUpdateTime = 0.0f;

    int renderFrame = 0;
    int frame = 0;
    int mouseGrabFrame = 0;

    float loopTime = 0.0f;

    glm::vec2 lastWindowSize = RTX::Window::getSize();

    RTX::Time time;
    while (RTX::Window::isRunning()) {
        RTX::Window::update();
        RTX::Mouse::update();

        frame++;

        loopTime += time.getDelta();
        if (loopTime >= 10.0f) loopTime = 0.0f;

        if (RTX::Keyboard::isPressed(GLFW_KEY_ESCAPE)) {
            if (!mouseGrabFrame) {
                RTX::Mouse::setGrabbed(!RTX::Mouse::isGrabbed());
                mouseGrabFrame = frame;
            }
        } else {
            mouseGrabFrame = 0;
        }

        glm::vec3 lastPlayerPos = glm::vec3(player.position);
        glm::vec3 lastPlayerAngle = glm::vec3(player.rotation);

        if(RTX::Mouse::isGrabbed()) player.update(time);
        time.update();

        shaderUpdateTime += time.getDelta();

        fpsUpdateTime += time.getDelta();
        fps++;
        
        if (fpsUpdateTime >= 1.0f) {
            RTX::Window::setTitle(std::string("SUPER 3D YOPTA! Fps: " + std::to_string(fps)).c_str());

            fpsUpdateTime = 0.0f;
            fps = 0;
        }

        renderFrame++;
        bool renderToLast = renderFrame % 2;

        if (renderToLast) lastFrameBuffer.load();
        else frameBuffer.load();

        raytraceProgram.load();
        raytraceProgram.setUniform("playerPosition", player.position);
        raytraceProgram.setUniform("playerRotation", player.rotation);
        raytraceProgram.setUniform("sunDirection", glm::vec3(sunDirection[0], sunDirection[1], sunDirection[2]));
        raytraceProgram.setUniform("screenResolution", RTX::Window::getSize());
        raytraceProgram.setUniform("time", loopTime);
        raytraceProgram.setUniform("denoiseFactor", 1.0f / (float) renderFrame);
        raytraceProgram.setUniform("lastFrameSampler", 0);
        raytraceProgram.setUniform("skyboxSampler", 1);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, renderToLast ? frameBuffer.getTexture() : lastFrameBuffer.getTexture());

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, skyboxTexture);

        glBegin(GL_QUADS);
        glVertex2i(-1, -1);
        glVertex2i(1, -1);
        glVertex2i(1, 1);
        glVertex2i(-1, 1);
        glEnd();

        RTX::FrameBuffer::unload();

        screenProgram.load();
        screenProgram.setUniform("screenResolution", RTX::Window::getSize());

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, renderToLast ? lastFrameBuffer.getTexture() : frameBuffer.getTexture());

        glBegin(GL_QUADS);
        glVertex2i(-1, -1);
        glVertex2i(1, -1);
        glVertex2i(1, 1);
        glVertex2i(-1, 1);
        glEnd();

        RTX::ShaderProgram::unload();
        glBindTexture(GL_TEXTURE_2D, 0);

        RTX::Window::beginImGui();

        ImGui::Begin("Shader Editor");

        glm::vec3 lastSunDirection = glm::vec3(sunDirection[0], sunDirection[1], sunDirection[2]);

        ImGui::DragFloat3("Sun Diretion", sunDirection, 0.005f, -1.0f, 1.0f);
        if (shaderUpdateTime >= 5.0f) {
            shaderUpdateTime = 0.0f;

            raytraceProgram.clear();
            raytraceProgram = RTX::ShaderProgram();
            raytraceProgram.addShader(RTX::Shader("res/shaders/raytrace.vert", GL_VERTEX_SHADER));
            raytraceProgram.addShader(RTX::Shader("res/shaders/raytrace.frag", GL_FRAGMENT_SHADER));
            raytraceProgram.compile();

            screenProgram.clear();
            screenProgram = RTX::ShaderProgram();
            screenProgram.addShader(RTX::Shader("res/shaders/screen.vert", GL_VERTEX_SHADER));
            screenProgram.addShader(RTX::Shader("res/shaders/screen.frag", GL_FRAGMENT_SHADER));
            screenProgram.compile();
        }

        ImGui::End();

        RTX::Window::endImGui();

        if (lastPlayerPos != player.position || lastPlayerAngle != player.rotation || lastSunDirection.x != sunDirection[0] || lastSunDirection.y != sunDirection[1] || lastSunDirection.z != sunDirection[2]) {
            renderFrame = 0;
        }

        glm::vec2 windowSize = RTX::Window::getSize();
        if (lastWindowSize != windowSize) {
            lastWindowSize = glm::vec2(windowSize);

            frameBuffer.clear();
            lastFrameBuffer.clear();

            frameBuffer = RTX::FrameBuffer((int)windowSize.x, (int)windowSize.y);
            lastFrameBuffer = RTX::FrameBuffer((int)windowSize.x, (int)windowSize.y);
        }
    }

    raytraceProgram.clear();
    screenProgram.clear();

    frameBuffer.clear();
    lastFrameBuffer.clear();

    RTX::Window::clearImGui();
    RTX::Window::close();

    return 0;
}