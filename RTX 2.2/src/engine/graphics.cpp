#include "graphics.h"

GLFWwindow* TT::Window::window = nullptr;

bool TT::Window::create(int width, int height, const char* title, bool resizable, bool verticalSync) {
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

	if (glewInit() != GLEW_OK)
		throw std::runtime_error("Tvoy glew koncheny, ne rabotayet - LOH!\n");

	return true;
}
void TT::Window::update() {
	glfwSwapBuffers(window);
	glfwPollEvents();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
void TT::Window::close() {
	glfwDestroyWindow(window);
	glfwTerminate();
}

void TT::Window::setTitle(const char* title) {
	glfwSetWindowTitle(window, title);
}

void TT::Window::initializeImGui(unsigned int theme) {
	IMGUI_CHECKVERSION();

	ImGui::CreateContext();

	if (theme == TT_IMGUI_THEME_DARK) ImGui::StyleColorsDark();
	else if (theme == TT_IMGUI_THEME_LIGHT) ImGui::StyleColorsLight();
	else if (theme == TT_IMGUI_THEME_CLASSIC) ImGui::StyleColorsClassic();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 120");
}
void TT::Window::beginImGui() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	ImGui::NewFrame();
}
void TT::Window::endImGui() {
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
void TT::Window::clearImGui() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();

	ImGui::DestroyContext();
}

bool TT::Window::isRunning() {
	return !glfwWindowShouldClose(window);
}

glm::vec2 TT::Window::getSize() {
	int width = 0, height = 0;
	glfwGetWindowSize(window, &width, &height);

	return glm::vec2(width, height);
}

GLFWwindow* TT::Window::getId() {
	return window;
}

TT::Shader::Shader(const char* location, GLenum type) {
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

void TT::Shader::clear() const {
	glDeleteShader(id);
}
int TT::Shader::getId() const {
	return id;
}

TT::ShaderProgram::ShaderProgram() {
	id = glCreateProgram();
}

void TT::ShaderProgram::addShader(Shader shader) {
	shaders.push_back(shader);
	glAttachShader(id, shader.getId());
}
bool TT::ShaderProgram::compile() const {
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

void TT::ShaderProgram::load() const {
	glUseProgram(id);
}
void TT::ShaderProgram::unload() {
	glUseProgram(0);
}
void TT::ShaderProgram::clear() const {
	for (auto& shader : shaders) {
		glDetachShader(id, shader.getId());
		shader.clear();
	}

	glDeleteProgram(id);
}

void TT::ShaderProgram::setUniform(const char* id, int value) const {
	glUniform1i(glGetUniformLocation(this->id, id), value);
}
void TT::ShaderProgram::setUniform(const char* id, float value) const {
	glUniform1f(glGetUniformLocation(this->id, id), value);
}
void TT::ShaderProgram::setUniform(const char* id, glm::vec2 value) const {
	glUniform2f(glGetUniformLocation(this->id, id), value.x, value.y);
}
void TT::ShaderProgram::setUniform(const char* id, glm::vec3 value) const {
	glUniform3f(glGetUniformLocation(this->id, id), value.x, value.y, value.z);
}
void TT::ShaderProgram::setUniform(const char* id, glm::vec4 value) const {
	glUniform4f(glGetUniformLocation(this->id, id), value.x, value.y, value.z, value.w);
}

void TT::FrameBuffer::unload() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, (int)Window::getSize().x, (int)Window::getSize().y);
}

TT::FrameBuffer::FrameBuffer(int width, int height) {
	this->width = width;
	this->height = height;

	glGenFramebuffers(1, &fboId);
	glBindFramebuffer(GL_FRAMEBUFFER, fboId);

	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);

	unload();
}

void TT::FrameBuffer::load() const {
	glBindFramebuffer(GL_FRAMEBUFFER, fboId);
	
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT);
}
void TT::FrameBuffer::clear() const {
	glDeleteFramebuffers(1, &fboId);
	glDeleteTextures(1, &textureId);
}

int TT::FrameBuffer::getTexture() const {
	return textureId;
}
int TT::FrameBuffer::getWidth() const {
	return width;
}
int TT::FrameBuffer::getHeight() const {
	return height;
}

int TT::Texture::loadFromFile(const char* location, GLint filter) {
	stbi_set_flip_vertically_on_load(true);

	int width, height, channels;
	unsigned char* image = stbi_load(location, &width, &height, &channels, 0);

	if (!image) std::cerr << "Could not open image: \"" << location << '\"';

	GLuint textureId;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);

	GLint format = GL_RGBA;
	if (channels == 3) format = GL_RGB;
	if (channels == 2) format = GL_RG;
	if (channels == 1) format = GL_R;

	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, image);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(image);

	return textureId;
}

void TT::Texture::load(GLuint texture, int id) {
	glActiveTexture(GL_TEXTURE0 + id);
	glBindTexture(GL_TEXTURE_2D, texture);
}
void TT::Texture::unload() {
	glBindTexture(GL_TEXTURE_2D, 0);
}
void TT::Texture::clear(GLuint texture) {
	glDeleteTextures(1, &texture);
}