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
        const float glassReflect;

        const glm::vec4 uvInfo;

        const bool emissive;

        Material(glm::vec3 color, float diffuse, float glass, float glassReflect, glm::vec4 uvInfo, bool emissive) : color(color), diffuse(diffuse), glass(glass), glassReflect(glassReflect), uvInfo(uvInfo), emissive(emissive) {};
    };

    struct Box {
        glm::vec3 position;
        glm::vec3 scale;

        int material;
        std::string tag;

        Box(glm::vec3 position, glm::vec3 scale, int material, std::string tag) : position(position), scale(scale), material(material), tag(tag) {}
    };
    struct Sphere {
        glm::vec3 position;
        float radius;

        int material;
        std::string tag;

        Sphere(glm::vec3 position, float radius, int material, std::string tag) : position(position), radius(radius), material(material), tag(tag) {}
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
            startPosition = glm::vec3(position);
        }

        void respawn() {
            velocity = glm::vec3();
            position = glm::vec3(startPosition);
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
                    rawOnGround = false;
                    onGround = false;
                }
            } else {
                if (RTX::Keyboard::isPressed(GLFW_KEY_SPACE))
                    velocity.y += 1.0f;
                if (RTX::Keyboard::isPressed(GLFW_KEY_LEFT_SHIFT))
                    velocity.y -= 1.0f;
            }

            if (rawOnGround) {
                onGround = true;
                onGroundResetDelayTimer = 0.0f;
            } else {
                if (onGroundResetDelayTimer >= onGroundResetDelay) onGround = false;
                else onGroundResetDelayTimer += time.getDelta();
            }

            float horizontalLength = glm::length(glm::vec2(velocity.x, velocity.z));
            if (horizontalLength > 0.0f) {
                velocity.x /= horizontalLength;
                velocity.z /= horizontalLength;
            }

            std::vector<std::string> collidedTags;
            
            rawOnGround = false;

            position.x += velocity.x * walkSpeed * time.getDelta();
            collidedTags.push_back(checkCollision(boxes, spheres));

            if (collidedTags[collidedTags.size() - 1] != "") {
                position.x -= velocity.x * walkSpeed * time.getDelta();
                velocity.x = 0.0f;
            }

            position.y += velocity.y * walkSpeed * time.getDelta();
            collidedTags.push_back(checkCollision(boxes, spheres));

            if (collidedTags[collidedTags.size() - 1] != "") {
                position.y -= velocity.y * walkSpeed * time.getDelta();

                if (velocity.y <= 0.0f) rawOnGround = true;
                velocity.y = std::find(collidedTags.begin(), collidedTags.end(), "jump_pad") != collidedTags.end() ? 7.0f : 0.0f;
            }

            position.z += velocity.z * walkSpeed * time.getDelta();
            collidedTags.push_back(checkCollision(boxes, spheres));
            
            if (collidedTags[collidedTags.size() - 1] != "") {
                position.z -= velocity.z * walkSpeed * time.getDelta();
                velocity.z = 0.0f;
            }

            if (std::find(collidedTags.begin(), collidedTags.end(), "laser") != collidedTags.end())
                respawn();

            rotation.x -= RTX::Mouse::getVelocity().y * rotateSpeed;
            rotation.x = fmax(fmin(rotation.x, 89.99f), -89.99f);

            rotation.y += RTX::Mouse::getVelocity().x * rotateSpeed;
            rotation.y -= floor(rotation.y / 360.0f) * 360.0f;
        }

        glm::vec3 getEyePosition() {
            return glm::vec3(position.x + scale.x / 2.0f, position.y + scale.y - eyeHeight, position.z + scale.z / 2.0f);
        }
    private:
        const float onGroundResetDelay = 0.3f;

        bool rawOnGround = false, onGround = false;
        float onGroundResetDelayTimer = 0.0f;

        glm::vec3 startPosition;

        std::string checkCollision(std::vector<Box> boxes, std::vector<Sphere> spheres) const {
            for (Box& box : boxes)
                if (position.x + scale.x >= box.position.x && position.x <= box.position.x + box.scale.x)
                    if (position.y + scale.y >= box.position.y && position.y <= box.position.y + box.scale.y)
                        if (position.z + scale.z >= box.position.z && position.z <= box.position.z + box.scale.z)
                            return box.tag;
            for (Sphere& sphere : spheres) {
                glm::vec3 nearest(glm::max(glm::min(sphere.position.x, position.x + scale.x), position.x), glm::max(glm::min(sphere.position.y, position.y + scale.y), position.y), glm::max(glm::min(sphere.position.z, position.z + scale.z), position.z));
                float length = glm::length(glm::vec3(sphere.position.x - nearest.x, sphere.position.y - nearest.y, sphere.position.z - nearest.z));

                if (length * length < sphere.radius * sphere.radius) return sphere.tag;
            }

            return "";
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
                    if (line == "") continue;

                    std::stringstream lineStream(line);
                    if(readMode == MATERIAL) {
                        std::stringstream vectorStream = getNextStreamSplit(lineStream, '/');

                        float red = getNextSplit<float>(vectorStream, ',');
                        float green = getNextSplit<float>(vectorStream, ',');
                        float blue = getNextSplit<float>(vectorStream, ',');

                        float diffuse = getNextSplit<float>(lineStream, '/');
                        float glass = getNextSplit<float>(lineStream, '/');
                        float glassReflect = getNextSplit<float>(lineStream, '/');

                        vectorStream = getNextStreamSplit(lineStream, '/');

                        float uvX = getNextSplit<float>(vectorStream, ',');
                        float uvY = getNextSplit<float>(vectorStream, ',');
                        float uvWidth = getNextSplit<float>(vectorStream, ',');
                        float uvHeight = getNextSplit<float>(vectorStream, ',');

                        bool emissive = getNextSplit(lineStream, '/') == "true";

                        materials.push_back(Material(glm::vec3(red, green, blue), diffuse, glass, glassReflect, glm::vec4(uvX, uvY, uvWidth, uvHeight), emissive));
                    } else if (readMode == BOX) {
                        std::stringstream positionStream = getNextStreamSplit(lineStream, '/');
                        std::stringstream scaleStream = getNextStreamSplit(lineStream, '/');

                        float x = getNextSplit<float>(positionStream, ',');
                        float y = getNextSplit<float>(positionStream, ',');
                        float z = getNextSplit<float>(positionStream, ',');

                        float width = getNextSplit<float>(scaleStream, ',');
                        float height = getNextSplit<float>(scaleStream, ',');
                        float length = getNextSplit<float>(scaleStream, ',');

                        int material = getNextSplit<int>(lineStream, '/');
                        std::string tag = getNextSplit(lineStream, '/');

                        boxes.push_back(Box(glm::vec3(x, y, z), glm::vec3(width, height, length), material, tag.c_str()));
                    } else {
                        std::stringstream positionStream = getNextStreamSplit(lineStream, '/');

                        float x = getNextSplit<float>(positionStream, ',');
                        float y = getNextSplit<float>(positionStream, ',');
                        float z = getNextSplit<float>(positionStream, ',');

                        float radius = getNextSplit<float>(lineStream, '/');

                        int material = getNextSplit<int>(lineStream, '/');
                        std::string tag = getNextSplit(lineStream, '/');
                        
                        spheres.push_back(Sphere(glm::vec3(x, y, z), radius, material, tag.c_str()));
                    }
                }
            }

            return Map(materials, boxes, spheres);
        }
        private:
            enum ReadMode {
                MATERIAL, BOX, SPHERE
            };

            template<typename T> static T parseString(std::string string) {
                T result;
                std::stringstream(string) >> result;

                return result;
            }

            static std::string getNextSplit(std::stringstream& line, char splitter) {
                std::string result;
                std::getline(line, result, splitter);

                return result;
            }
            static std::stringstream getNextStreamSplit(std::stringstream& line, char splitter) {
                std::string split = getNextSplit(line, splitter);
                return std::stringstream(split);
            }

            template<typename T> static T getNextSplit(std::stringstream& line, char splitter) {
                return parseString<T>(getNextSplit(line, splitter));
            }
    };

    struct DofRay {
        glm::vec3 position;
        glm::vec3 direction;

        float getDistanceOrDefault(Sphere sphere, float _default) {
            glm::vec3 delta = position - sphere.position;

            float b = glm::dot(delta, direction);
            float h = b * b - (dot(delta, delta) - sphere.radius * sphere.radius);
            if (h < 0.0f) return _default;

            return -b - sqrt(h);
        }
        float getDistanceOrDefault(Box box, float _default) {
            glm::vec3 delta = position - box.position - box.scale / 2.0f;

            glm::vec3 m = 1.0f / direction;
            glm::vec3 n = m * delta;
            glm::vec3 k = abs(m) * box.scale / 2.0f;
            glm::vec3 t1 = -n - k;
            glm::vec3 t2 = -n + k;

            float tN = glm::max(glm::max(t1.x, t1.y), t1.z);
            float tF = glm::min(glm::min(t2.x, t2.y), t2.z);

            if (tN > tF || tF < 0.0f) return _default;
            return tN;
        }
    };
}

int main() {
    if (!RTX::Window::create(1920, 1080, "SUPER 3D YOPTA!", true, false)) {
        std::cerr << "Could not create a window...\n";
        return 1;
    }

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
    int albedoTexture = RTX::Texture::loadFromFile("res/textures/albedo.png", GL_LINEAR);

    RTX::Map mapa = RTX::MapParser::parse("res/maps/obby.rtmap");

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

    float focusDistance = 12.0f;
    float dofBlurSize = 0.05f;
    float fov = 90.0f;

    float rawDynamicFocusDistance = 0.0f;
    float dynamicFocusDistance = 0.0f;

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

        if (player.position.y <= -50.0f) player.respawn();

        glm::vec3 lastPlayerPos = glm::vec3(player.position);
        glm::vec3 lastPlayerAngle = glm::vec3(player.rotation);

        if(RTX::Mouse::isGrabbed()) player.update(time, gravity, mapa.boxes, mapa.spheres);

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
        raytraceProgram.setUniform("albedoSampler", 2);
        raytraceProgram.setUniform("dofBlurSize", dofBlurSize);
        raytraceProgram.setUniform("fov", fov);

        rawDynamicFocusDistance = focusDistance;
        for (int i = 0; i < mapa.boxes.size(); i++) {
            RTX::Box box = mapa.boxes[i];
            RTX::Material material = mapa.materials[box.material];

            std::string uniformId = "boxes[" + std::to_string(i) + ']';

            raytraceProgram.setUniform((uniformId + ".position").c_str(), box.position);
            raytraceProgram.setUniform((uniformId + ".size").c_str(), box.scale);
            raytraceProgram.setUniform((uniformId + ".material.color").c_str(), material.color);
            raytraceProgram.setUniform((uniformId + ".material.diffuse").c_str(), material.diffuse);
            raytraceProgram.setUniform((uniformId + ".material.glass").c_str(), material.glass);
            raytraceProgram.setUniform((uniformId + ".material.glassReflectivity").c_str(), material.glassReflect);
            raytraceProgram.setUniform((uniformId + ".material.uvInfo").c_str(), material.uvInfo);
            raytraceProgram.setUniform((uniformId + ".material.emissive").c_str(), material.emissive ? 1 : 0);
        }
        for (int i = 0; i < mapa.spheres.size(); i++) {
            RTX::Sphere sphere = mapa.spheres[i];
            RTX::Material material = mapa.materials[sphere.material];

            std::string uniformId = "spheres[" + std::to_string(i) + ']';

            raytraceProgram.setUniform((uniformId + ".position").c_str(), sphere.position);
            raytraceProgram.setUniform((uniformId + ".radius").c_str(), sphere.radius);
            raytraceProgram.setUniform((uniformId + ".material.color").c_str(), material.color);
            raytraceProgram.setUniform((uniformId + ".material.diffuse").c_str(), material.diffuse);
            raytraceProgram.setUniform((uniformId + ".material.glass").c_str(), material.glass);
            raytraceProgram.setUniform((uniformId + ".material.glassReflect").c_str(), material.glassReflect);
            raytraceProgram.setUniform((uniformId + ".material.uvInfo").c_str(), material.uvInfo);
            raytraceProgram.setUniform((uniformId + ".material.emissive").c_str(), material.emissive ? 1 : 0);
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, renderToLast ? frameBuffer.getTexture() : lastFrameBuffer.getTexture());

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, skyboxTexture);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, albedoTexture);

        glBegin(GL_QUADS);
        glVertex2i(-1, -1);
        glVertex2i(1, -1);
        glVertex2i(1, 1);
        glVertex2i(-1, 1);
        glEnd();

        RTX::FrameBuffer::unload();

        screenProgram.load();
        screenProgram.setUniform("screenResolution", RTX::Window::getSize());
        screenProgram.setUniform("firstFrame", renderFrame == 1);

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
            ImGui::DragFloat("Walk Speed", &player.walkSpeed, 0.005f, 0.005f, 20.0f);
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