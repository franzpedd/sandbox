#include "Core/Editor.h"
#include <iostream>

int main(int argc, char* argv[])
{
    std::cout << "Hello World" << std::endl;

    Cosmos::Editor app;
    app.Run();

    return 0;
}