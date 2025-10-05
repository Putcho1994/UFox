import hello_uFox;
import ufox_lib;
import ufox_windowing;
import ufox_engine;

#include <iostream>


int main() {
    using ufox::windowing::sdl::WindowFlag;
    ufox::IDE ide;
    ide.Init();
    return ide.Run();
    // try {
    //
    //     ufox::windowing::sdl::UFoxWindow window("UFox",
    //         WindowFlag::Vulkan | WindowFlag::Resizable);
    //
    //     window.ShowAndRun();
    //
    //     return 0;
    // } catch (const ufox::windowing::sdl::SDLException& e) {
    //     std::cerr << e.what() << std::endl;
    //     return 1;
    // }
}
