
#include <assimp/Importer.hpp>

int main() {
    auto importer = new Assimp::Importer();
    auto b = importer->ReadFile("Bodach.fbx", 0);
}