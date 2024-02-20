#pragma once
#include <GLFW/glfw3.h>

namespace RTX {
	class Time {
	public:
		Time();

		void update();

		float getDelta();
		float getTime();
	private:
		float lastTime, delta, time;
	};
}