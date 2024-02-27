#pragma once
#include "engine/graphics.h"
#include "engine/input.h"
#include "engine/math.h"
#include "engine/audio.h"

namespace RTX {
    struct Material {
        const glm::vec3 color;

        const float diffuse;
        const float glass;
        const float glassReflect;

        const glm::vec4 uvInfo;

        const bool emissive;

        Material(glm::vec3 color, float diffuse, float glass, float glassReflect, glm::vec4 uvInfo, bool emissive);
    };

    struct Box {
        glm::vec3 position;
        glm::vec3 scale;

        int material;
        std::string tag;

        Box(glm::vec3 position, glm::vec3 scale, int material, std::string tag);
    };
    struct Sphere {
        glm::vec3 position;
        float radius;

        int material;
        std::string tag;

        Sphere(glm::vec3 position, float radius, int material, std::string tag);
    };

    struct Map {
        std::vector<Material> materials;
        std::vector<Box> boxes;
        std::vector<Sphere> spheres;

        int albedoTexture, normalTexture, skyboxTexture;

        Map(
            int albedoTexture, int normalTexture, int skyboxTexture,
            std::vector<Material> materials, std::vector<Box> boxes, std::vector<Sphere> spheres
        );
    };
    class MapParser {
    public:
        static Map parse(const char* location);
    private:
        enum ReadMode {
            INFO, MATERIAL, BOX, SPHERE
        };

        template<typename T> static T parseString(std::string string);

        static std::string getNextSplit(std::stringstream& line, char splitter);
        static std::stringstream getNextStreamSplit(std::stringstream& line, char splitter);

        template<typename T> static T getNextSplit(std::stringstream& line, char splitter);
    };
    struct World {
        static float gravity;
        static float* sunDirection;

        static Map* map;

        static void initialize(const char* mapName, float gravity, glm::vec3 sunDirection);
        static void clear();
    };

    class Player {
    public:
        float walkSpeed, rotateSpeed, jumpHeight, eyeHeight;
        glm::vec3 position, rotation, scale, velocity;

        bool flyMode;

        Player(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale);

        void respawn();
        void update(TT::Time time);
        void clear();

        glm::vec3 getEyePosition();
    private:
        const static float onGroundResetDelay, stepSpeed;

        ALuint* const stepSounds;
        const ALuint blyaSound;

        TT::SoundSource blyaSoundSource;
        TT::SoundSource stepSoundSource;

        bool rawOnGround = false, onGround = false;
        float onGroundResetDelayTimer, stepTimer;

        glm::vec3 startPosition;

        std::string checkCollision(std::vector<Box> boxes, std::vector<Sphere> spheres) const;
    };

    struct Camera {
        static float dofBlurSize, dofFocusDistance, fov;
        static void initialize(float dofBlurSize, float dofFocusDistance, float fov);
    };

    class Denoiser {
    public:
        static int denoiseFrame;

        static void initialize(glm::uvec2 size);
        static void resize(glm::uvec2 size);
        static void reloadShaders();

        static void render(TT::Time time, Player player);
        static void reset();
        static void clear();
        static void clearShaders();
        static void clearFrameBuffers();

        static TT::FrameBuffer* getRenderFrame();
        static TT::FrameBuffer* getBackFrame();
    private:
        static TT::ShaderProgram *raytraceProgram, *screenProgram;
        static TT::FrameBuffer *lastFrameBuffer, *frameBuffer;
    };

    class DebugHud {
    public:
        static void initialize();
        static void render(Player& player);

        static bool getFrameScaleMode();
        static int getFrameScale();
    private:
        static bool frameScaleMode;
        static int frameScale;
    };
}