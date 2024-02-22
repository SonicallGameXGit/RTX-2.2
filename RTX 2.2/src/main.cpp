#include <vector>
#include <unordered_map>
#include "engine/graphics.h"
#include "engine/math.h"
#include "engine/input.h"

namespace RTX {
    struct Material {
        const glm::vec3 color;

        const float diffuse;
        const float glass;
        const float glassReflectivity;

        const bool emissive;

        Material(glm::vec3 color, float diffuse, float glass, float glassReflectivity, bool emissive) : color(color), diffuse(diffuse), glass(glass), glassReflectivity(glassReflectivity), emissive(emissive) {};
    };

    struct Box {
        glm::vec3 position;
        glm::vec3 scale;

        int materialId;

        const char* tag;

        Box(glm::vec3 position, glm::vec3 scale, int materialId) : position(position), scale(scale), materialId(materialId) {}
    };
    struct Sphere {
        glm::vec3 position;
        float scale;

        int materialId;

        Sphere(glm::vec3 position, float scale, int materialId) : position(position), scale(scale), materialId(materialId) {}
    };

    class Player {
    public:
        float walkSpeed = 6.0f, rotateSpeed = 0.09f, jumpHeight = 1.5f, eyeHeight = 0.2f;
        glm::vec3 position, rotation, scale, velocity;

        bool flyMode = false;

        Player(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale) {
            this->position = glm::vec3(position);
            this->rotation = glm::vec3(rotation);
            this->scale = glm::vec3(scale);

            velocity = glm::vec3();
        }

        void update(Time time, float gravity, std::vector<Box> boxes, std::vector<Sphere> spheres) {
            velocity.x = 0.0f;
            velocity.z = 0.0f;

            if (!flyMode) velocity.y -= gravity * time.getDelta();
            else velocity.y = 0.0f;

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
            if (!flyMode) {
                if (RTX::Keyboard::isPressed(GLFW_KEY_SPACE) && onGround) {
                    velocity.y = jumpHeight;
                    onGround = false;
                }
            }
            else {
                if (RTX::Keyboard::isPressed(GLFW_KEY_SPACE))
                    velocity.y += 1.0f;
                if (RTX::Keyboard::isPressed(GLFW_KEY_LEFT_SHIFT))
                    velocity.y -= 1.0f;
            }

            float horizontalLength = glm::length(glm::vec2(velocity.x, velocity.z));
            if (horizontalLength > 0.0f) {
                velocity.x /= horizontalLength;
                velocity.z /= horizontalLength;
            }

            position.x += velocity.x * walkSpeed * time.getDelta();
            if (checkCollision(boxes, spheres)) {
                position.x -= velocity.x * walkSpeed * time.getDelta();
                velocity.x = 0.0f;
            }

            position.y += velocity.y * walkSpeed * time.getDelta();
            if (checkCollision(boxes, spheres)) {
                position.y -= velocity.y * walkSpeed * time.getDelta();

                if (velocity.y <= 0.0f) onGround = true;
                velocity.y = 0.0f;
            }

            position.z += velocity.z * walkSpeed * time.getDelta();
            if (checkCollision(boxes, spheres)) {
                position.z -= velocity.z * walkSpeed * time.getDelta();
                velocity.z = 0.0f;
            }

            rotation.x -= RTX::Mouse::getVelocity().y * rotateSpeed;
            rotation.x = fmax(fmin(rotation.x, 90.0f), -90.0f);

            rotation.y += RTX::Mouse::getVelocity().x * rotateSpeed;
            rotation.y -= floor(rotation.y / 360.0f) * 360.0f;
        }

        glm::vec3 getEyePosition() {
            return glm::vec3(position.x + scale.x / 2.0f, position.y + scale.y - eyeHeight, position.z + scale.z / 2.0f);
        }
    private:
        bool onGround = false;
        bool checkCollision(std::vector<Box> boxes, std::vector<Sphere> spheres) const {
            for (Box& box : boxes)
                if (position.x + scale.x >= box.position.x && position.x <= box.position.x + box.scale.x)
                    if (position.y + scale.y >= box.position.y && position.y <= box.position.y + box.scale.y)
                        if (position.z + scale.z >= box.position.z && position.z <= box.position.z + box.scale.z)
                            return true;
            for (Sphere& sphere : spheres) {
                glm::vec3 nearest(glm::max(glm::min(sphere.position.x, position.x + scale.x), position.x), glm::max(glm::min(sphere.position.y, position.y + scale.y), position.y), glm::max(glm::min(sphere.position.z, position.z + scale.z), position.z));
                float length = glm::length(glm::vec3(sphere.position.x - nearest.x, sphere.position.y - nearest.y, sphere.position.z - nearest.z));

                if (length * length < sphere.scale * sphere.scale)
                    return true;
            }

            return false;
        }
    };

    struct Map {
        std::vector<Material> materials;
        std::vector<Box> boxes;
        std::vector<Sphere> spheres;

        Map(std::vector<Material> materials, std::vector<Box> boxes, std::vector<Sphere> spheres) : materials(materials), boxes(boxes), spheres(spheres) {}
    };
    class MapParser {
    public:
        static Map parse(const char* location) {
            std::ifstream file(location);
            
            std::vector<Material> materials;
            std::vector<Box> boxes;
            std::vector<Sphere> spheres;

            if (!file.is_open()) {
                throw std::runtime_error(std::string("Could not parse map: \"" + std::string(location) + "\""));
                return Map(materials, boxes, spheres);
            }

            ReadMode readMode = MATERIAL;

            while (file) {
                std::string line;
                getline(file, line);

                if (line.starts_with("Materials")) readMode = MATERIAL;
                else if (line.starts_with("Boxes")) readMode = BOX;
                else if (line.starts_with("Spheres")) readMode = SPHERE;
                else {
                    std::stringstream lineStream(line);
                    if(readMode == MATERIAL) {
                        // Split line to color, diffuse, glass, glassReflectivity and emissive by '/'                        
                        std::string color;
                        std::string diffuse;
                        std::string glass;
                        std::string glassReflectivity;
                        std::string emissive;

                        std::getline(lineStream, color, '/');
                        std::getline(lineStream, diffuse, '/');
                        std::getline(lineStream, glass, '/');
                        std::getline(lineStream, glassReflectivity, '/');
                        std::getline(lineStream, emissive, '/');

                        // Get red, green and blue channels from color param by ','
                        std::stringstream colorStream(color);
                        std::string colorParam;
                        std::stringstream colorParamStream;

                        float red, green, blue;

                        std::getline(colorStream, colorParam, ',');
                        colorParamStream = std::stringstream(colorParam);
                        colorParamStream >> red;

                        std::getline(colorStream, colorParam, ',');
                        colorParamStream = std::stringstream(colorParam);
                        colorParamStream >> green;

                        std::getline(colorStream, colorParam, ',');
                        colorParamStream = std::stringstream(colorParam);
                        colorParamStream >> blue;

                        // Get diffuse from diffuse param
                        float diffuseFloat;
                        std::stringstream(diffuse) >> diffuseFloat;

                        // Get diffuse from glass param
                        float glassFloat;
                        std::stringstream(glass) >> glassFloat;

                        // Get diffuse from glassReflectivity param
                        float glassReflectivityFloat;
                        std::stringstream(glassReflectivity) >> glassReflectivityFloat;

                        // Get diffuse from emissive param
                        bool emissiveBoolean = emissive == "true";

                        materials.push_back(Material(glm::vec3(red, green, blue), diffuseFloat, glassFloat, glassReflectivityFloat, emissiveBoolean));
                    }
                    else if (readMode == BOX) {
                        // Split line to position, scale, materialId by '/'                        
                        std::string position;
                        std::string scale;
                        std::string materialId;

                        std::getline(lineStream, position, '/');
                        std::getline(lineStream, scale, '/');
                        std::getline(lineStream, materialId, '/');

                        // Get x, y and z coords from position param by ','
                        std::stringstream vectorStream(position);
                        std::string vectorParam;
                        std::stringstream vectorParamStream;

                        float x, y, z;

                        std::getline(vectorStream, vectorParam, ',');
                        vectorParamStream = std::stringstream(vectorParam);
                        vectorParamStream >> x;

                        std::getline(vectorStream, vectorParam, ',');
                        vectorParamStream = std::stringstream(vectorParam);
                        vectorParamStream >> y;

                        std::getline(vectorStream, vectorParam, ',');
                        vectorParamStream = std::stringstream(vectorParam);
                        vectorParamStream >> z;

                        vectorStream = std::stringstream(scale);
                        float width, height, length;

                        std::getline(vectorStream, vectorParam, ',');
                        vectorParamStream = std::stringstream(vectorParam);
                        vectorParamStream >> width;

                        std::getline(vectorStream, vectorParam, ',');
                        vectorParamStream = std::stringstream(vectorParam);
                        vectorParamStream >> height;

                        std::getline(vectorStream, vectorParam, ',');
                        vectorParamStream = std::stringstream(vectorParam);
                        vectorParamStream >> length;

                        int materialIdInt;
                        std::stringstream(materialId) >> materialIdInt;

                        boxes.push_back(Box(glm::vec3(x, y, z), glm::vec3(width, height, length), materialIdInt));
                    }
                    else {
                        // Split line to position, scale, materialId by '/'                        
                        std::string position;
                        std::string scale;
                        std::string materialId;

                        std::getline(lineStream, position, '/');
                        std::getline(lineStream, scale, '/');
                        std::getline(lineStream, materialId, '/');

                        // Get x, y and z coords from position param by ','
                        std::stringstream vectorStream(position);
                        std::string vectorParam;
                        std::stringstream vectorParamStream;

                        float x, y, z;

                        std::getline(vectorStream, vectorParam, ',');
                        vectorParamStream = std::stringstream(vectorParam);
                        vectorParamStream >> x;

                        std::getline(vectorStream, vectorParam, ',');
                        vectorParamStream = std::stringstream(vectorParam);
                        vectorParamStream >> y;

                        std::getline(vectorStream, vectorParam, ',');
                        vectorParamStream = std::stringstream(vectorParam);
                        vectorParamStream >> z;

                        float scaleFloat;
                        std::stringstream(scale) >> scaleFloat;

                        int materialIdInt;
                        std::stringstream(materialId) >> materialIdInt;

                        spheres.push_back(Sphere(glm::vec3(x, y, z), scaleFloat, materialIdInt));
                    }
                }
            }

            return Map(materials, boxes, spheres);
        }

        enum ReadMode {
            MATERIAL, BOX, SPHERE
        };
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

    RTX::Map mapa = RTX::MapParser::parse("res/maps/obby.rtmap");

    /*std::vector<RTX::Material> materials;
    materials.push_back(RTX::Material(glm::vec3(0.9f), 0.9f, 0.0f, 0.0f, false));
    materials.push_back(RTX::Material(glm::vec3(0.9f, 0.1f, 0.2f), 0.9f, 0.0f, 0.0f, false));
    materials.push_back(RTX::Material(glm::vec3(0.2f, 0.9f, 0.2f), 0.9f, 0.0f, 0.0f, false));
    materials.push_back(RTX::Material(glm::vec3(0.2f, 0.5f, 1.0f), 0.01f, 0.15f, 0.4f, false));
    materials.push_back(RTX::Material(glm::vec3(1.0f, 0.2f, 0.4f), 0.0f, 0.01f, 0.2f, false));

    std::vector<RTX::Box> boxes;
    boxes.push_back(RTX::Box(glm::vec3(-10.0f, -1.0f, -10.0f), glm::vec3(20.0f, 1.0f, 20.0f), 0));
    boxes.push_back(RTX::Box(glm::vec3(-10.0f, 0.0f, -10.0f), glm::vec3(20.0f, 10.0f, 1.0f), 1));
    boxes.push_back(RTX::Box(glm::vec3(-10.0f, 0.0f, -10.0f), glm::vec3(1.0f, 10.0f, 20.0f), 2));

    std::vector<RTX::Sphere> spheres;
    spheres.push_back(RTX::Sphere(glm::vec3(4.0f, 1.0f, 4.0f), 1.0f, 3));
    spheres.push_back(RTX::Sphere(glm::vec3(2.0f, 1.0f, 1.0f), 0.5f, 4));*/

    RTX::Player player(glm::vec3(-1.5f, 5.0f, -1.5f), glm::vec3(), glm::vec3(0.4f, 1.76f, 0.4f));

    float fpsUpdateTime = 0.0f;
    int fps = 0;

    RTX::Mouse::initialize();
    RTX::Mouse::setGrabbed(true);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float sunDirection[3] = { -1.0f, 1.0f, -0.175f };
    float shaderUpdateTime = 0.0f;

    int renderFrame = 0;
    int frame = 0;
    int mouseGrabFrame = 0;

    float loopTime = 0.0f;
    float gravity = 5.0f;

    float focusDistance = 5.0f;
    float dofBlurSize = 0.05f;
    float fov = 90.0f;

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

        if (player.position.y <= -50.0f) {
            player.position = glm::vec3(0.0f, 5.0f, 0.0f);
            player.velocity.y = 0.0f;
        }

        glm::vec3 lastPlayerPos = glm::vec3(player.position);
        glm::vec3 lastPlayerAngle = glm::vec3(player.rotation);

        if(RTX::Mouse::isGrabbed()) player.update(time, gravity, mapa.boxes, mapa.spheres);

        //world.update(lastPlayerPos, lastPlayerAngle, player);

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
        raytraceProgram.setUniform("playerPosition", player.getEyePosition());
        raytraceProgram.setUniform("playerRotation", player.rotation);
        raytraceProgram.setUniform("sunDirection", glm::vec3(sunDirection[0], sunDirection[1], sunDirection[2]));
        raytraceProgram.setUniform("screenResolution", RTX::Window::getSize());
        raytraceProgram.setUniform("time", loopTime);
        raytraceProgram.setUniform("denoiseFactor", 1.0f / (float) renderFrame);
        raytraceProgram.setUniform("lastFrameSampler", 0);
        raytraceProgram.setUniform("skyboxSampler", 1);
        raytraceProgram.setUniform("dofFocusDistance", focusDistance);
        raytraceProgram.setUniform("dofBlurSize", dofBlurSize);
        raytraceProgram.setUniform("fov", fov);

        for (int i = 0; i < mapa.boxes.size(); i++) {
            RTX::Box box = mapa.boxes[i];
            RTX::Material material = mapa.materials[box.materialId];

            std::string uniformId = "boxes[" + std::to_string(i) + ']';

            raytraceProgram.setUniform((uniformId + ".position").c_str(), box.position);
            raytraceProgram.setUniform((uniformId + ".size").c_str(), box.scale);
            raytraceProgram.setUniform((uniformId + ".material.color").c_str(), material.color);
            raytraceProgram.setUniform((uniformId + ".material.diffuse").c_str(), material.diffuse);
            raytraceProgram.setUniform((uniformId + ".material.glass").c_str(), material.glass);
            raytraceProgram.setUniform((uniformId + ".material.glassReflectivity").c_str(), material.glassReflectivity);
            raytraceProgram.setUniform((uniformId + ".material.emissive").c_str(), material.emissive ? 1 : 0);
        }
        for (int i = 0; i < mapa.spheres.size(); i++) {
            RTX::Sphere sphere = mapa.spheres[i];
            RTX::Material material = mapa.materials[sphere.materialId];

            std::string uniformId = "spheres[" + std::to_string(i) + ']';

            raytraceProgram.setUniform((uniformId + ".position").c_str(), sphere.position);
            raytraceProgram.setUniform((uniformId + ".radius").c_str(), sphere.scale);
            raytraceProgram.setUniform((uniformId + ".material.color").c_str(), material.color);
            raytraceProgram.setUniform((uniformId + ".material.diffuse").c_str(), material.diffuse);
            raytraceProgram.setUniform((uniformId + ".material.glass").c_str(), material.glass);
            raytraceProgram.setUniform((uniformId + ".material.glassReflectivity").c_str(), material.glassReflectivity);
            raytraceProgram.setUniform((uniformId + ".material.emissive").c_str(), material.emissive ? 1 : 0);
        }

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

        float lastPlayerEyeHeight = player.eyeHeight;
        float lastFocusDistance = focusDistance;
        float lastDofBlurSize = dofBlurSize;
        float lastFov = fov;

        glm::vec3 lastSunDirection = glm::vec3(sunDirection[0], sunDirection[1], sunDirection[2]);
        
        if (!RTX::Mouse::isGrabbed()) {
            ImGui::Begin("Shader Editor");

            ImGui::Text("Player");
            ImGui::DragFloat("Walk Speed", &player.walkSpeed, 0.005f, 0.005f, 2.0f);
            ImGui::DragFloat("Rotate Speed", &player.rotateSpeed, 0.005f, 0.005f, 0.2f);
            ImGui::DragFloat("Jump Height", &player.jumpHeight, 0.005f, 0.005f, 20.0f);
            ImGui::Checkbox("Fly Mode", &player.flyMode);

            ImGui::Separator();
            ImGui::Text("World");

            ImGui::DragFloat3("Sun Direction", sunDirection, 0.005f, -1.0f, 1.0f);

            ImGui::DragFloat("Gravity", &gravity, 0.005f, 0.005f, 20.0f);

            ImGui::Separator();
            ImGui::Text("Camera");
            ImGui::DragFloat("Eye Height", &player.eyeHeight, 0.005f, 0.1f, 0.9f);

            ImGui::Spacing();

            ImGui::DragFloat("DoF Focus Distance", &focusDistance, 0.005f, 0.1f, 100.0f);
            ImGui::DragFloat("DoF Blur Size", &dofBlurSize, 0.0005f, 0.0f, 0.9f);
            ImGui::DragFloat("Fov", &fov, 1.0f, 20.0f, 179.0f);

            ImGui::End();
        }

        RTX::Window::endImGui();

        if (lastPlayerPos != player.position || lastPlayerAngle != player.rotation || lastSunDirection.x != sunDirection[0] || lastSunDirection.y != sunDirection[1] || lastSunDirection.z != sunDirection[2])
            renderFrame = 0;
        if (lastPlayerEyeHeight != player.eyeHeight || lastFocusDistance != focusDistance || lastDofBlurSize != dofBlurSize || lastFov != fov)
            renderFrame = 0;

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