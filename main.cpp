#include "Engine/feoo_app.hpp"

#include <cstdlib>
#include <iostream>
#include <print>

/// <summary>
/// 程序入口
/// </summary>
int main() {
	std::cout.setf(std::ios::unitbuf);
	std::cin.setf(std::ios::unitbuf);
	feoo::FeooApp app{};
	try {
		std::println("Application started.");
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << "An error occurred: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}