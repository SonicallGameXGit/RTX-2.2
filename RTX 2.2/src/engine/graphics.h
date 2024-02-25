#pragma once
#include <GLEW/glew.h>
#include <GLFW/glfw3.h>
#include <GLM/glm.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <fstream>
#include <streambuf>
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_opengl3.h"
#include "../stb/stb_image.h"

#define RTX_IMGUI_THEME_DARK 0
#define RTX_IMGUI_THEME_LIGHT 1
#define RTX_IMGUI_THEME_CLASSIC 2

namespace RTX {
	class Window {
	public:
		static bool create(int width, int height, const char* title, bool resizable, bool verticalSync);
		static void update();
		static void close();

		static void initializeImGui(unsigned int theme);
		static void beginImGui();
		static void endImGui();
		static void clearImGui();

		static void setTitle(const char* title);
		
		static bool isRunning();

		static glm::vec2 getSize();
		static GLFWwindow* getId();
	private:
		static GLFWwindow* window;
	};

	class Shader {
	public:
		Shader(const char* code, GLenum type);
		
		void clear() const;
		int getId() const;
	private:
		int id;
	};
	class ShaderProgram {
	public:
		ShaderProgram();
		
		void addShader(Shader shader);
		bool compile() const;

		void load() const;
		static void unload();

		void clear() const;

		void setUniform(const char* id, int value) const;
		void setUniform(const char* id, float value) const;
		void setUniform(const char* id, glm::vec2 value) const;
		void setUniform(const char* id, glm::vec3 value) const;
		void setUniform(const char* id, glm::vec4 value) const;
	private:
		int id;
		std::vector<Shader> shaders;
	};
	class FrameBuffer {
	public:
		FrameBuffer(int width, int height);

		void load() const;
		void clear() const;

		static void unload();

		int getTexture() const;
		int getWidth() const;
		int getHeight() const;
	private:
		GLuint fboId, rboId, textureId;
		int width, height;
	};
	class Texture {
	public:
		static int loadFromFile(const char* location, int filter);
	};
}