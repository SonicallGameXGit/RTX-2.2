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
		
		static bool isRunning();

		static glm::vec2 getSize();
		static GLFWwindow* getId();
	private:
		static GLFWwindow* window;
	};

	class Shader {
	public:
		Shader(const char* code, GLenum type);
		void clear();

		int getId();
	private:
		int id;
	};
	class ShaderProgram {
	public:
		ShaderProgram();
		
		void addShader(Shader shader);
		bool compile();

		void load();
		static void unload();

		void clear();

		void setUniform(const char* id, float value);
		void setUniform(const char* id, glm::vec3 value);
	private:
		int id;
		std::vector<Shader> shaders;
	};
}