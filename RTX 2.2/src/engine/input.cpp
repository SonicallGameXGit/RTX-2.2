#include "input.h"

glm::vec2 RTX::Mouse::velocity;
glm::vec2 RTX::Mouse::lastPosition;

bool RTX::Keyboard::isKeyPressed(int key) {
	return glfwGetKey(RTX::Window::getId(), key) == GLFW_PRESS;
}

void RTX::Mouse::initialize() {
	lastPosition = getPosition();
}
void RTX::Mouse::update() {
	glm::vec2 position = getPosition();
	velocity = position - lastPosition;
	
	lastPosition = position;
}

void RTX::Mouse::setGrabbed(bool grabbed) {
	glfwSetInputMode(RTX::Window::getId(), GLFW_CURSOR, grabbed ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

glm::vec2 RTX::Mouse::getVelocity() {
	return glm::vec2(velocity);
}
glm::vec2 RTX::Mouse::getPosition() {
	double x, y;
	glfwGetCursorPos(RTX::Window::getId(), &x, &y);

	return glm::vec2((float)x, (float)y);
}