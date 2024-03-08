//#include <vector>
//#include "rtx.h"
//
//int main() {
//	TT::Window::create(1920, 1080, "Example", true, true);
//
//	TT::ShaderProgram raytraceProgram;
//	raytraceProgram.addShader(TT::Shader("res/shaders/example.frag", GL_FRAGMENT_SHADER));
//	raytraceProgram.compile();
//
//	std::vector<RTX::Sphere> spheres;
//	std::vector<RTX::Box> boxes;
//
//	while (TT::Window::isRunning()) {
//		TT::Window::update();
//
//		raytraceProgram.load();
//
//		for (int i = 0; i < spheres.size(); i++) {
//			raytraceProgram.setUniform((
//				std::string("spheres[") +
//				std::to_string(i) +
//				"].position"
//			).c_str(), spheres[i].position);
//		}
//
//		glBegin(GL_QUADS);
//		glVertex2i(-1, -1);
//		glVertex2i(1, -1);
//		glVertex2i(1, 1);
//		glVertex2i(-1, 1);
//		glEnd();
//
//		raytraceProgram.unload();
//	}
//
//	raytraceProgram.clear();
//	TT::Window::close();
//
//	return 0;
//}