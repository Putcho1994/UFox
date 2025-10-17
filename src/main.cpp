
#include <exception>
#include <iostream>
#include <thread>

#include "vulkan/vulkan.hpp"

import ufox_engine;


int main() {

    try {
        ufox::Engine ide;
        ide.Init();
        //ide.Run();
        std::this_thread::sleep_for( std::chrono::milliseconds( 10000 ) );
    } catch ( vk::SystemError & err )
    {
        std::cout << "vk::SystemError: " << err.what() << std::endl;
        exit( -1 );
    }
    catch ( std::exception & err )
    {
        std::cout << "std::exception: " << err.what() << std::endl;
        exit( -1 );
    }
    catch ( ... )
    {
        std::cout << "unknown error\n";
        exit( -1 );
    }
    return 0;


}
