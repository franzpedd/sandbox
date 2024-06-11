#include <Engine.h>
#include <iostream>

#include "Platform/Detection.h"

int main(int argc, char* argv[])
{
    std::cout << "Hello World" << std::endl;

    Cosmos::Application app;
    app.Run();

    return 0;
}