#include "graphics.h"

GLFWwindow* RTX::Window::window = nullptr;

bool RTX::Window::create(int width, int height, const char* title, bool resizable, bool verticalSync) {
	if (!glfwInit()) {
		std::cerr << "Poshel k cherty, glfw ne rabotaet!\n";
		return false;
	}

	glfwDefaultWindowHints();
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	glfwWindowHint(GLFW_RESIZABLE, resizable);

	window = glfwCreateWindow(width, height, title, NULL, NULL);
	if (!window) {
		std::cerr << "Tebya Vindov ne lubit!\n";
		return false;
	}

	glfwMakeContextCurrent(window);
	glfwSetWindowSizeCallback(window, [](GLFWwindow* _window, int _width, int _height) {
		glViewport(0, 0, _width, _height);
	});

	const GLFWvidmode* videoMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	if (videoMode != NULL)
		glfwSetWindowPos(window, (videoMode->width - width) / 2, (videoMode->height - height) / 2);

	glfwSwapInterval(verticalSync);
	glfwShowWindow(window);

	if (glewInit() != GLEW_OK) {
		std::cerr << "Tvoy glew koncheny, ne rabotayet - LOH!\n";
		return false;
	}

	return true;
}
void RTX::Window::update() {
	glfwSwapBuffers(window);
	glfwPollEvents();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
void RTX::Window::close() {
	glfwDestroyWindow(window);
	glfwTerminate();
}

void RTX::Window::initializeImGui(unsigned int theme) {
	IMGUI_CHECKVERSION();

	ImGui::CreateContext();

	if (theme == RTX_IMGUI_THEME_DARK) ImGui::StyleColorsDark();
	else if (theme == RTX_IMGUI_THEME_LIGHT) ImGui::StyleColorsLight();
	else if (theme == RTX_IMGUI_THEME_CLASSIC) ImGui::StyleColorsClassic();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 120");
}
void RTX::Window::beginImGui() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	ImGui::NewFrame();
}
void RTX::Window::endImGui() {
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
void RTX::Window::clearImGui() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();

	ImGui::DestroyContext();
}

bool RTX::Window::isRunning() {
	return !glfwWindowShouldClose(window);
}

glm::vec2 RTX::Window::getSize() {
	int width = 0, height = 0;
	glfwGetWindowSize(window, &width, &height);

	return glm::vec2(width, height);
}

GLFWwindow* RTX::Window::getId() {
	return window;
}

RTX::Shader::Shader(const char* location, GLenum type) {
	std::ifstream stream(location);
	std::string code{ std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>() };

	const char* charCode = code.c_str();

	id = glCreateShader(type);
	glShaderSource(id, 1, &charCode, NULL);
	glCompileShader(id);

	int compiled = 0;
	glGetShaderiv(id, GL_COMPILE_STATUS, &compiled);

	if (!compiled) {
		char log[512];

		glGetShaderInfoLog(id, 512, NULL, log);
		std::cerr << "Tvoy sheyder polnoe govno! Ispravlyay! Oshibka:\n" << log << '\n';
	}
}

void RTX::Shader::clear() {
	glDeleteShader(id);
}
int RTX::Shader::getId() {
	return id;
}

RTX::ShaderProgram::ShaderProgram() {
	id = glCreateProgram();
}

void RTX::ShaderProgram::addShader(Shader shader) {
	shaders.push_back(shader);
	glAttachShader(id, shader.getId());
}
bool RTX::ShaderProgram::compile() {
	bool success = true;

	glLinkProgram(id);

	int linked = 0;
	glGetProgramiv(id, GL_LINK_STATUS, &linked);
	
	if (!linked) {
		char log[512];
		glGetProgramInfoLog(id, 512, NULL, log);

		std::cerr << "Ono ne shtototam ne linkuetsa pochemuto\nOshibka:\n" << log << '\n';
		success = false;
	}


	glValidateProgram(id);

	int compiled = 0;
	glGetProgramiv(id, GL_VALIDATE_STATUS, &compiled);

	if (!compiled) {
		char log[512];
		glGetProgramInfoLog(id, 512, NULL, log);

		std::cerr << "Ne pravilno, dva!\nOshibka:\n" << log << '\n';
		success = false;
	}

	return success;
}

void RTX::ShaderProgram::load() {
	glUseProgram(id);
}
void RTX::ShaderProgram::unload() {
	glUseProgram(0);
}
void RTX::ShaderProgram::clear() {
	for (auto& shader : shaders) {
		glDetachShader(id, shader.getId());
		shader.clear();
	}

	glDeleteProgram(id);
}

void RTX::ShaderProgram::setUniform(const char* id, float value) {
	glUniform1f(glGetUniformLocation(this->id, id), value);
}
void RTX::ShaderProgram::setUniform(const char* id, glm::vec2 value) {
	glUniform2f(glGetUniformLocation(this->id, id), value.x, value.y);
}
void RTX::ShaderProgram::setUniform(const char* id, glm::vec3 value) {
	glUniform3f(glGetUniformLocation(this->id, id), value.x, value.y, value.z);
}