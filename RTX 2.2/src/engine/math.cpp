#include "math.h"

RTX::Time::Time() {
	lastTime = (float) glfwGetTime();
	delta = 0.0f;
	time = 0.0f;
}

void RTX::Time::update() {
	float time = (float) glfwGetTime();
	delta = time - lastTime;
	lastTime = time;

	this->time += delta;
}

float RTX::Time::getDelta() {
	return delta;
}
float RTX::Time::getTime() {
	return time;
}