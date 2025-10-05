import hello_uFox;
import ufox_lib;
import ufox_windowing;


#include <iostream>


int main() {


    try {
        using ufox::windowing::sdl::WindowFlag;
        ufox::windowing::sdl::UFoxWindow window("UFoxEngine Test",
            WindowFlag::Vulkan | WindowFlag::Resizable);

        window.Show();


        bool running = true;

        window.Run(running);

        return 0;
    } catch (const ufox::windowing::sdl::SDLException& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
