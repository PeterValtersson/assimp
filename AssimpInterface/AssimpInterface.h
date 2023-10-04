#pragma once

#include <string_view>
#include <Resource.h>

namespace Assimp {
    class AssimpImporter {
    public:
        static void import(std::string_view file);
    };
} // namespace Assimp