#ifndef MODEL_H
#define MODEL_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <string>
#include <iostream>

// Структура вершины
struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;

    Vertex() : Position(glm::vec3(0.0f)), Normal(glm::vec3(0.0f)) {}
    Vertex(glm::vec3 pos, glm::vec3 norm) : Position(pos), Normal(norm) {}
};

// Класс Mesh
class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO, VBO, EBO;

    Mesh(std::vector<Vertex> verts, std::vector<unsigned int> inds) {
        vertices = verts;
        indices = inds;
        setupMesh();
    }

    void setupMesh() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

        // Позиция
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));
        glEnableVertexAttribArray(0);

        // Нормаль
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    // Исправленный метод Draw
    void Draw(GLuint shaderProgram) {
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    ~Mesh() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
};

// Класс Model
class Model {
public:
    std::vector<Mesh> meshes;
    std::string directory;

    Model(const std::string& path) {
        loadModel(path);
    }

    // ИСПРАВЛЕННЫЙ МЕТОД DRAW - добавьте его сюда!
    void Draw(GLuint shaderProgram) {
        for (auto& mesh : meshes) {
            mesh.Draw(shaderProgram);
        }
    }

private:
    void loadModel(const std::string& path) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path,
            aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_JoinIdenticalVertices |
            aiProcess_OptimizeMeshes);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "Ошибка Assimp: " << importer.GetErrorString() << std::endl;
            return;
        }

        directory = path.substr(0, path.find_last_of('/'));
        if (directory.empty()) directory = path.substr(0, path.find_last_of('\\'));

        processNode(scene->mRootNode, scene);

        std::cout << "Загружено мешей: " << meshes.size() << std::endl;
    }

    void processNode(aiNode* node, const aiScene* scene) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }
    }

    Mesh processMesh(aiMesh* mesh, const aiScene* scene) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        vertices.reserve(mesh->mNumVertices);

        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            vertex.Position = glm::vec3(
                static_cast<float>(mesh->mVertices[i].x),
                static_cast<float>(mesh->mVertices[i].y),
                static_cast<float>(mesh->mVertices[i].z)
            );

            if (mesh->HasNormals()) {
                vertex.Normal = glm::vec3(
                    static_cast<float>(mesh->mNormals[i].x),
                    static_cast<float>(mesh->mNormals[i].y),
                    static_cast<float>(mesh->mNormals[i].z)
                );
            }
            vertices.push_back(vertex);
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                indices.push_back(face.mIndices[j]);
            }
        }

        return Mesh(vertices, indices);
    }
};

#endif