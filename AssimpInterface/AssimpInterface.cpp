
#include "AssimpInterface.h"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <ECS/Components/MeshInfo.h>
#include <Utilities/PathHelpers.h>

#include <filesystem>

int main() {
    std::filesystem::remove("test.dat");
    auto ra = ResourceHandler::IResourceArchive::create_binary_archive("test.dat", ResourceHandler::AccessMode::read_write);
    auto rh = ResourceHandler::IResourceHandler::create(ResourceHandler::AccessMode::read_write, ra);
    ResourceHandler::IResourceHandler::set(rh);

    Assimp::AssimpImporter::import("SM_shantyMetalKit_Fence.fbx");
}

struct Attribute {
    ai_real *data;
    int size;
    void interleave(ai_real *fdata, int index) {
    }
};
struct Mesh {
    aiMesh *mesh;
    std::vector<Attribute> attributes;
};

void Assimp::AssimpImporter::import(std::string_view file) {
    auto importer = new Assimp::Importer();
    auto scene = importer->ReadFile(file.data(), 0);
    auto filename = Utilities::Path::get_filename(file);
    auto filename_no_extension = Utilities::Path::remove_extension(filename);
    
    ECS::MeshInfo info;
    memset(&info, 0, sizeof(info));
    std::vector<Mesh> meshes;
    info.sub_mesh_count = static_cast<uint8_t>(scene->mNumMeshes);
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        Mesh mesh;
        mesh.mesh = scene->mMeshes[i];
        if (mesh.mesh->mName.length > 0)
            memcpy(&info.name[i], mesh.mesh->mName.C_Str(), mesh.mesh->mName.length + 1);
        else {
            std::string name = filename_no_extension + std::to_string(i);
            memcpy(&info.name[i], name.c_str(), name.size());
        }

        size_t vertex_count = 0;
        info.vertex_count[i] = mesh.mesh->mNumVertices;
        info.index_count[i] = mesh.mesh->mNumFaces * 3; // Only triangles ATM.
        if (i == 0) {
            info.vertex_start[i] = 0;
            info.index_start[i] = 0;
        } else {
            info.vertex_start[i] = info.vertex_start[i - 1] + info.vertex_count[i - 1];
            info.index_start[i] = info.index_start[i - 1] + info.index_count[i - 1];
        }

        mesh.attributes.push_back({ (ai_real *)mesh.mesh->mVertices, 3 });
        if (mesh.mesh->HasTextureCoords(0)) {
            mesh.attributes.push_back({ (ai_real *)mesh.mesh->mTextureCoords[0], 2 });
            info.flags[i] |= ECS::MeshFlags::Textured;
        }
        if (mesh.mesh->HasVertexColors(0)) {
            mesh.attributes.push_back({ (ai_real *)mesh.mesh->mColors[0], 4 });
            info.flags[i] |= ECS::MeshFlags::Color;
        }
        if (mesh.mesh->HasNormals()) {
            mesh.attributes.push_back({ (ai_real *)mesh.mesh->mNormals, 3 });
            info.flags[i] |= ECS::MeshFlags::Normals;
        }

        if (mesh.mesh->HasTangentsAndBitangents()) {
            mesh.attributes.push_back({ (ai_real *)mesh.mesh->mTangents, 3 });
            mesh.attributes.push_back({ (ai_real *)mesh.mesh->mBitangents, 3 });
            info.flags[i] |= ECS::MeshFlags::TangetBiTangent;
        }

        meshes.push_back(std::move(mesh));
    }

    auto ra = ResourceHandler::IResourceHandler::get()->get_archive();
    if (ra->exists(Utilities::GUID(filename_no_extension)))
        return;

    auto resource_ID = ra->create_from_name(filename_no_extension);
    ra->set_type(resource_ID, "Mesh");

    ResourceHandler::Resource output(resource_ID);
    output.modify_data([&](Utilities::Memory::MemoryBlock data) {
        data.write(&info, sizeof(info));
        for (auto &mesh : meshes) {
            for (unsigned int i = 0; i < mesh.mesh->mNumVertices; i++) {
                for (auto &attribute : mesh.attributes) {
                    data.append(attribute.data + i * attribute.size, sizeof(ai_real) * attribute.size);
                }
            }
        }
        for (auto &mesh : meshes) {
            for (unsigned int i = 0; i < mesh.mesh->mNumFaces; i++) {
                auto face = mesh.mesh->mFaces[i];
                data.append(face.mIndices, sizeof(unsigned int) * face.mNumIndices);
            }
        }
    });

    output.remove_flag(ResourceHandler::Flags::Modifying);
    importer->FreeScene();

   // ResourceHandler::IResourceHandler::get()->save_all();
}

