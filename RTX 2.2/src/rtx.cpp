#include "rtx.h"

RTX::Material::Material(glm::vec3 color, float diffuse, float glass, float glassReflect, glm::vec4 uvInfo, bool emissive)
    : color(color), diffuse(diffuse), glass(glass), glassReflect(glassReflect), uvInfo(uvInfo), emissive(emissive) {};

RTX::Box::Box(glm::vec3 position, glm::vec3 scale, int material, std::string tag) : position(position), scale(scale), material(material), tag(tag) {}
RTX::Sphere::Sphere(glm::vec3 position, float radius, int material, std::string tag) : position(position), radius(radius), material(material), tag(tag) {}

RTX::Map::Map(
    int albedoTexture, int normalTexture, int skyboxTexture,
    std::vector<RTX::Material> materials, std::vector<RTX::Box> boxes, std::vector<RTX::Sphere> spheres
) :
    albedoTexture(albedoTexture), normalTexture(normalTexture), skyboxTexture(skyboxTexture),
    materials(materials), boxes(boxes), spheres(spheres)
{}

RTX::Map RTX::MapParser::parse(const char* location) {
    std::ifstream file(location);

    std::vector<RTX::Material> materials;
    std::vector<RTX::Box> boxes;
    std::vector<RTX::Sphere> spheres;

    int albedoTexture = 0, normalTexture = 0, skyboxTexture = 0;

    if (!file.is_open()) {
        throw std::runtime_error(std::string("Could not parse map: \"" + std::string(location) + "\""));
        return RTX::Map(albedoTexture, normalTexture, skyboxTexture, materials, boxes, spheres);
    }

    ReadMode readMode = INFO;

    while (file) {
        std::string line;
        getline(file, line);

        if (line.starts_with("Info")) readMode = INFO;
        else if (line.starts_with("Materials")) readMode = MATERIAL;
        else if (line.starts_with("Boxes")) readMode = BOX;
        else if (line.starts_with("Spheres")) readMode = SPHERE;
        else {
            if (line == "" || line.starts_with("//")) continue;

            std::stringstream lineStream(line);

            if (readMode == INFO) {
                albedoTexture = TT::Texture::loadFromFile(("res/textures/" + getNextSplit(lineStream, '/')).c_str(), GL_LINEAR);
                normalTexture = TT::Texture::loadFromFile(("res/textures/" + getNextSplit(lineStream, '/')).c_str(), GL_LINEAR);
                skyboxTexture = TT::Texture::loadFromFile(("res/textures/" + getNextSplit(lineStream, '/')).c_str(), GL_LINEAR);
            }
            else if (readMode == MATERIAL) {
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

                materials.push_back(RTX::Material(glm::vec3(red, green, blue), diffuse, glass, glassReflect, glm::vec4(uvX, uvY, uvWidth, uvHeight), emissive));
            }
            else if (readMode == BOX) {
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

                boxes.push_back(RTX::Box(glm::vec3(x, y, z), glm::vec3(width, height, length), material, tag.c_str()));
            }
            else {
                std::stringstream positionStream = getNextStreamSplit(lineStream, '/');

                float x = getNextSplit<float>(positionStream, ',');
                float y = getNextSplit<float>(positionStream, ',');
                float z = getNextSplit<float>(positionStream, ',');

                float radius = getNextSplit<float>(lineStream, '/');

                int material = getNextSplit<int>(lineStream, '/');
                std::string tag = getNextSplit(lineStream, '/');

                spheres.push_back(RTX::Sphere(glm::vec3(x, y, z), radius, material, tag.c_str()));
            }
        }
    }

    return RTX::Map(albedoTexture, normalTexture, skyboxTexture, materials, boxes, spheres);
}

template<typename T> T RTX::MapParser::parseString(std::string string) {
    T result;
    std::stringstream(string) >> result;

    return result;
}

std::string RTX::MapParser::getNextSplit(std::stringstream& line, char splitter) {
    std::string result;
    std::getline(line, result, splitter);

    return result;
}
std::stringstream RTX::MapParser::getNextStreamSplit(std::stringstream& line, char splitter) {
    std::string split = getNextSplit(line, splitter);
    return std::stringstream(split);
}

template<typename T> T RTX::MapParser::getNextSplit(std::stringstream& line, char splitter) {
    return parseString<T>(getNextSplit(line, splitter));
}

float RTX::World::gravity = 25.0f;
float* RTX::World::sunDirection = new float[3];

RTX::Map* RTX::World::map = NULL;

void RTX::World::initialize(const char* mapName, float gravity, glm::vec3 sunDirection) {
    map = new Map(MapParser::parse((std::string("res/maps/") + mapName + ".rtmap").c_str()));

    World::gravity = gravity;
    World::sunDirection[0] = sunDirection.x;
    World::sunDirection[1] = sunDirection.y;
    World::sunDirection[2] = sunDirection.z;
}
void RTX::World::clear() {
    TT::Texture::clear(map->albedoTexture);
    TT::Texture::clear(map->normalTexture);
    TT::Texture::clear(map->skyboxTexture);

    delete map;
    delete[] sunDirection;
}

const float RTX::Player::onGroundResetDelay = 0.3f;
const float RTX::Player::stepSpeed = 0.5f;

RTX::Player::Player(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale) : stepSoundSource(TT::SoundSource()), stepSounds(new ALuint[3]), blyaSound(TT::AudioSystem::loadFromFile("res/sounds/blya.ogg")) {
    walkSpeed = 6.0f;
    rotateSpeed = 0.09f;
    jumpHeight = 8.0f;
    eyeHeight = 0.2f;
    cinematicSharpness = 2.0f;

    stepTimer = 0.0f;
    onGroundResetDelayTimer = 0.0f;

    flyMode = false;
    cinematicMode = false;
    
    this->position = glm::vec3(position);
    this->rotation = glm::vec3(rotation);
    this->scale = glm::vec3(scale);

    rawRotation = glm::vec3(rotation);

    velocity = glm::vec3();
    startPosition = glm::vec3(position);

    stepSounds[0] = TT::AudioSystem::loadFromFile("res/sounds/player/step0.ogg");
    stepSounds[1] = TT::AudioSystem::loadFromFile("res/sounds/player/step1.ogg");
    stepSounds[2] = TT::AudioSystem::loadFromFile("res/sounds/player/step2.ogg");
}

void RTX::Player::respawn() {
    velocity = glm::vec3();
    position = glm::vec3(startPosition);
}
void RTX::Player::update(TT::Time time) {
    velocity.x = 0.0f;
    velocity.z = 0.0f;

    if (!flyMode) velocity.y -= World::gravity * time.getDelta();
    else velocity.y = 0.0f;

    if (TT::Keyboard::isPressed(GLFW_KEY_W)) {
        velocity.x += sin(glm::radians(rotation.y));
        velocity.z += cos(glm::radians(rotation.y));
    }
    if (TT::Keyboard::isPressed(GLFW_KEY_S)) {
        velocity.x += sin(glm::radians(rotation.y + 180.0f));
        velocity.z += cos(glm::radians(rotation.y + 180.0f));
    }
    if (TT::Keyboard::isPressed(GLFW_KEY_D)) {
        velocity.x += sin(glm::radians(rotation.y + 90.0f));
        velocity.z += cos(glm::radians(rotation.y + 90.0f));
    }
    if (TT::Keyboard::isPressed(GLFW_KEY_A)) {
        velocity.x += sin(glm::radians(rotation.y - 90.0f));
        velocity.z += cos(glm::radians(rotation.y - 90.0f));
    }
    if (!flyMode) {
        if (TT::Keyboard::isPressed(GLFW_KEY_SPACE) && onGround) {
            velocity.y = jumpHeight;
            rawOnGround = false;
            onGround = false;
        }
    }
    else {
        if (TT::Keyboard::isPressed(GLFW_KEY_SPACE)) velocity.y += 1.0f;
        if (TT::Keyboard::isPressed(GLFW_KEY_LEFT_SHIFT))velocity.y -= 1.0f;
    }

    if (rawOnGround) {
        onGround = true;
        onGroundResetDelayTimer = 0.0f;
    }
    else {
        if (onGroundResetDelayTimer >= onGroundResetDelay) onGround = false;
        else onGroundResetDelayTimer += time.getDelta();
    }

    float horizontalLength = glm::length(glm::vec2(velocity.x, velocity.z));
    if (horizontalLength > 0.01f) {
        velocity.x /= horizontalLength;
        velocity.z /= horizontalLength;

        if (onGround) {
            if (stepTimer == 0.0f)
                stepSoundSource.play(stepSounds[rand() % 3], 1.5f, 1.0f, false);

            stepTimer += stepSpeed * time.getDelta();
            if (stepTimer >= 1.0f / walkSpeed) stepTimer = 0.0f;
        }
        else stepTimer = 0.0f;

        Renderer::resetDenoiser();
    }

    std::vector<std::string> collidedTags;

    rawOnGround = false;

    position.x += velocity.x * walkSpeed * time.getDelta();
    collidedTags.push_back(checkCollision(World::map->boxes, World::map->spheres));

    if (collidedTags[collidedTags.size() - 1] != "") {
        position.x -= velocity.x * walkSpeed * time.getDelta();
        velocity.x = 0.0f;
    }

    position.y += velocity.y * (flyMode ? walkSpeed : 1.0f) * time.getDelta();
    collidedTags.push_back(checkCollision(World::map->boxes, World::map->spheres));

    if (collidedTags[collidedTags.size() - 1] != "") {
        position.y -= velocity.y * (flyMode ? walkSpeed : 1.0f) * time.getDelta();

        if (velocity.y <= 0.0f) {
            bool onJumpPad = std::find(collidedTags.begin(), collidedTags.end(), "jump_pad") != collidedTags.end();
            velocity.y = onJumpPad ? 50.0f : 0.0f;

            if (!onJumpPad) rawOnGround = true;
            else blyaSoundSource.play(blyaSound, 0.8f, 1.0f, false);
        }
    }

    position.z += velocity.z * walkSpeed * time.getDelta();
    collidedTags.push_back(checkCollision(World::map->boxes, World::map->spheres));

    if (collidedTags[collidedTags.size() - 1] != "") {
        position.z -= velocity.z * walkSpeed * time.getDelta();
        velocity.z = 0.0f;
    }

    if (std::find(collidedTags.begin(), collidedTags.end(), "laser") != collidedTags.end())
        respawn();

    if (abs(velocity.y) > 0.1f) Renderer::resetDenoiser();

    glm::vec3 lastRotation = glm::vec3(rotation);
    if (!cinematicMode) {
        rotation.x -= TT::Mouse::getVelocity().y * rotateSpeed;
        rotation.x = fmax(fmin(rotation.x, 89.99f), -89.99f);

        rotation.y += TT::Mouse::getVelocity().x * rotateSpeed;
        rotation.y -= floor(rotation.y / 360.0f) * 360.0f;
    }
    else {
        rawRotation.x -= TT::Mouse::getVelocity().y * rotateSpeed;
        rawRotation.x = fmax(fmin(rawRotation.x, 89.99f), -89.99f);
        rawRotation.y += TT::Mouse::getVelocity().x * rotateSpeed;

        rotation.x += (cinematicSharpness * time.getDelta()) * (rawRotation.x - rotation.x);
        rotation.y += (cinematicSharpness * time.getDelta()) * (rawRotation.y - rotation.y);
        rotation.z += (cinematicSharpness * time.getDelta()) * (rawRotation.z - rotation.z);
    }

    if(abs(lastRotation.x - rotation.x) > 0.01f || abs(lastRotation.y - rotation.y) > 0.01f || abs(lastRotation.z - rotation.z) > 0.01f)
        Renderer::resetDenoiser();
}
void RTX::Player::clear() {
    TT::AudioSystem::clear(stepSounds[0]);
    TT::AudioSystem::clear(stepSounds[1]);
    TT::AudioSystem::clear(stepSounds[2]);

    stepSoundSource.stop();
    stepSoundSource.clear();

    delete[] stepSounds;
}

glm::vec3 RTX::Player::getEyePosition() {
    return glm::vec3(position.x + scale.x / 2.0f, position.y + scale.y - eyeHeight, position.z + scale.z / 2.0f);
}

std::string RTX::Player::checkCollision(std::vector<Box> boxes, std::vector<Sphere> spheres) const {
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

float RTX::Camera::dofBlurSize = 0.05f;
float RTX::Camera::dofFocusDistance = 12.0f;
float RTX::Camera::fov = 90.0f;

void RTX::Camera::initialize(float dofBlurSize, float dofFocusDistance, float fov) {
    Camera::dofBlurSize = dofBlurSize;
    Camera::dofFocusDistance = dofFocusDistance;
    Camera::fov = fov;
}

TT::ShaderProgram* RTX::Renderer::raytraceProgram = NULL;
TT::ShaderProgram* RTX::Renderer::screenProgram = NULL;

TT::FrameBuffer* RTX::Renderer::firstFrameBuffer = NULL;
TT::FrameBuffer* RTX::Renderer::secondFrameBuffer = NULL;

int RTX::Renderer::denoiserStep = 0;

void RTX::Renderer::initialize(glm::uvec2 size) {
    resize(size);
    reloadShaders();
}
void RTX::Renderer::resize(glm::uvec2 size) {
    clearFrameBuffers();

    firstFrameBuffer = new TT::FrameBuffer(size.x, size.y);
    secondFrameBuffer = new TT::FrameBuffer(size.x, size.y);
}
void RTX::Renderer::reloadShaders() {
    clearShaders();

    if (raytraceProgram) delete raytraceProgram;
    if (screenProgram) delete screenProgram;

    raytraceProgram = new TT::ShaderProgram();
    raytraceProgram->addShader(TT::Shader("res/shaders/raytrace.vert", GL_VERTEX_SHADER));
    raytraceProgram->addShader(TT::Shader("res/shaders/raytrace.frag", GL_FRAGMENT_SHADER));
    raytraceProgram->compile();

    screenProgram = new TT::ShaderProgram();
    screenProgram->addShader(TT::Shader("res/shaders/screen.vert", GL_VERTEX_SHADER));
    screenProgram->addShader(TT::Shader("res/shaders/screen.frag", GL_FRAGMENT_SHADER));
    screenProgram->compile();
}
void RTX::Renderer::resetDenoiser() {
    denoiserStep = 1;
}

void RTX::Renderer::render(TT::Time time, Player player) {
    bool denoiserSwapState = denoiserStep % 2 == 0;

    TT::FrameBuffer* renderFrameBuffer = denoiserSwapState ? firstFrameBuffer : secondFrameBuffer;
    TT::FrameBuffer* backFrameBuffer = denoiserSwapState ? secondFrameBuffer : firstFrameBuffer;

    renderFrameBuffer->load();

    raytraceProgram->load();
    raytraceProgram->setUniform("playerPosition", player.getEyePosition());
    raytraceProgram->setUniform("playerRotation", player.rotation);
    raytraceProgram->setUniform("sunDirection", glm::vec3(World::sunDirection[0], World::sunDirection[1], World::sunDirection[2]));
    raytraceProgram->setUniform("screenResolution", TT::Window::getSize());
    raytraceProgram->setUniform("random", time.getTime());
    raytraceProgram->setUniform("backFrameFactor", 1.0f / denoiserStep);
    raytraceProgram->setUniform("backFrameSampler", 0);
    raytraceProgram->setUniform("skyboxSampler", 1);
    raytraceProgram->setUniform("albedoSampler", 2);
    raytraceProgram->setUniform("normalSampler", 3);
    raytraceProgram->setUniform("dofFocusDistance", Camera::dofFocusDistance);
    raytraceProgram->setUniform("dofBlurSize", Camera::dofBlurSize);
    raytraceProgram->setUniform("fov", Camera::fov);

    for (int i = 0; i < World::map->boxes.size(); i++) {
        Box box = World::map->boxes[i];
        Material material = World::map->materials[box.material];

        std::string uniformId = "boxes[" + std::to_string(i) + ']';

        raytraceProgram->setUniform((uniformId + ".position").c_str(), box.position);
        raytraceProgram->setUniform((uniformId + ".size").c_str(), box.scale);
        raytraceProgram->setUniform((uniformId + ".material.color").c_str(), material.color);
        raytraceProgram->setUniform((uniformId + ".material.diffuse").c_str(), material.diffuse);
        raytraceProgram->setUniform((uniformId + ".material.glass").c_str(), material.glass);
        raytraceProgram->setUniform((uniformId + ".material.glassReflectivity").c_str(), material.glassReflect);
        raytraceProgram->setUniform((uniformId + ".material.uvInfo").c_str(), material.uvInfo);
        raytraceProgram->setUniform((uniformId + ".material.emissive").c_str(), material.emissive ? 1 : 0);
    }
    for (int i = 0; i < World::map->spheres.size(); i++) {
        Sphere sphere = World::map->spheres[i];
        Material material = World::map->materials[sphere.material];

        std::string uniformId = "spheres[" + std::to_string(i) + ']';

        raytraceProgram->setUniform((uniformId + ".position").c_str(), sphere.position);
        raytraceProgram->setUniform((uniformId + ".radius").c_str(), sphere.radius);
        raytraceProgram->setUniform((uniformId + ".material.color").c_str(), material.color);
        raytraceProgram->setUniform((uniformId + ".material.diffuse").c_str(), material.diffuse);
        raytraceProgram->setUniform((uniformId + ".material.glass").c_str(), material.glass);
        raytraceProgram->setUniform((uniformId + ".material.glassReflect").c_str(), material.glassReflect);
        raytraceProgram->setUniform((uniformId + ".material.uvInfo").c_str(), material.uvInfo);
        raytraceProgram->setUniform((uniformId + ".material.emissive").c_str(), material.emissive ? 1 : 0);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, backFrameBuffer->getTexture());

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, World::map->skyboxTexture);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, World::map->albedoTexture);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, World::map->normalTexture);

    glBegin(GL_QUADS);
    glVertex2i(-1, -1);
    glVertex2i(1, -1);
    glVertex2i(1, 1);
    glVertex2i(-1, 1);
    glEnd();

    TT::FrameBuffer::unload();

    screenProgram->load();
    screenProgram->setUniform("screenResolution", TT::Window::getSize());
    screenProgram->setUniform("textureResolution", glm::vec2(renderFrameBuffer->getWidth(), renderFrameBuffer->getHeight()));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderFrameBuffer->getTexture());

    glBegin(GL_QUADS);
    glVertex2i(-1, -1);
    glVertex2i(1, -1);
    glVertex2i(1, 1);
    glVertex2i(-1, 1);
    glEnd();

    TT::ShaderProgram::unload();
    TT::Texture::unload();

    glBindTexture(GL_TEXTURE_2D, 0);

    denoiserStep++;
}
void RTX::Renderer::clear() {
    clearShaders();
}
void RTX::Renderer::clearShaders() {
    if(raytraceProgram) raytraceProgram->clear();
    if(screenProgram) screenProgram->clear();
}
void RTX::Renderer::clearFrameBuffers() {
    if (firstFrameBuffer) {
        firstFrameBuffer->clear();
        delete firstFrameBuffer;
    }
    if (secondFrameBuffer) {
        secondFrameBuffer->clear();
        delete secondFrameBuffer;
    }
}

TT::FrameBuffer* RTX::Renderer::getFirstFrameBuffer() {
    return firstFrameBuffer;
}
TT::FrameBuffer* RTX::Renderer::getSecondFrameBuffer() {
    return secondFrameBuffer;
}

bool RTX::DebugHud::frameScaleMode = false;
int RTX::DebugHud::frameScale = 1;

void RTX::DebugHud::initialize() {
    frameScaleMode = false;
    frameScale = 1;
}
void RTX::DebugHud::render(Player& player) {
    ImGui::Begin("Settings");

    ImGui::Text("Player");
    ImGui::DragFloat("Walk Speed", &player.walkSpeed, 0.005f, 0.005f, 20.0f);
    ImGui::DragFloat("Rotate Speed", &player.rotateSpeed, 0.005f, 0.005f, 0.2f);
    ImGui::DragFloat("Jump Height", &player.jumpHeight, 0.005f, 0.005f, 20.0f);
    
    ImGui::Checkbox("Fly Mode", &player.flyMode);

    ImGui::Separator();
    ImGui::Text("Cinematic");
    ImGui::Checkbox("Cinematic Mode", &player.cinematicMode);
    ImGui::DragFloat("Cinematic Sharpness", &player.cinematicSharpness, 0.005f, 0.005f, 100.0f);

    ImGui::Separator();
    ImGui::Text("World");

    if (ImGui::DragFloat3("Sun Direction", World::sunDirection, 0.005f, -1.0f, 1.0f))
        Renderer::resetDenoiser();
    ImGui::DragFloat("Gravity", &World::gravity, 0.005f, 0.005f, 20.0f);

    ImGui::Separator();
    ImGui::Text("Camera");
    if (ImGui::DragFloat("Eye Height", &player.eyeHeight, 0.005f, 0.1f, 0.9f))
        Renderer::resetDenoiser();

    ImGui::Spacing();

    if (ImGui::DragFloat("DoF Focus Distance", &Camera::dofFocusDistance, 0.005f, 0.1f, 100.0f))
        Renderer::resetDenoiser();
    if (ImGui::DragFloat("DoF Blur Size", &Camera::dofBlurSize, 0.0005f, 0.0f, 0.9f))
        Renderer::resetDenoiser();
    if (ImGui::DragFloat("Fov", &Camera::fov, 1.0f, 20.0f, 179.0f))
        Renderer::resetDenoiser();

    ImGui::Separator();
    ImGui::Text("Graphics");

    if (ImGui::ArrowButton("frameScaleMode", frameScaleMode ? ImGuiDir_Up : ImGuiDir_Down)) {
        frameScaleMode = !frameScaleMode;
        frameScale = 1;

        Renderer::resetDenoiser();
    }

    ImGui::SameLine();
    if (ImGui::InputInt(frameScaleMode ? "Upscale" : "Downscale", &frameScale)) {
        frameScale = glm::clamp(frameScale, 1, 50);

        glm::vec2 size = TT::Window::getSize();
        if (frameScaleMode) size *= frameScale;
        else size /= frameScale;

        Renderer::resize(glm::ivec2((int)size.x, (int)size.y));
        Renderer::resetDenoiser();
    }

    if (ImGui::Button("Reload Shaders")) Renderer::reloadShaders();

    ImGui::End();
}

bool RTX::DebugHud::getFrameScaleMode() {
    return frameScaleMode;
}
int RTX::DebugHud::getFrameScale() {
    return frameScale;
}