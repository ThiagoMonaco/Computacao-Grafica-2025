/*
 *  Codificado por Rossana Baptista Queiroz
 *  para as disciplinas de Processamento Gráfico/Computação Gráfica - Unisinos
 *  Versão inicial: 07/04/2017
 *  Última atualização: 14/03/2025
 *
 *  Este arquivo contém a função `loadSimpleOBJ`, responsável por carregar arquivos
 *  no formato Wavefront .OBJ e armazenar seus vértices em um VAO para renderização
 *  com OpenGL.
 *
 *  Forma de uso (carregamento de um .obj)
 *  -----------------
 *  ...
 *  int nVertices;
 *  GLuint objVAO = loadSimpleOBJ("../Modelos3D/Cube.obj", nVertices);
 *  ...
 *
 *  Chamada de desenho (Polígono Preenchido - GL_TRIANGLES), no loop do programa:
 *  ----------------------------------------------------------
 *  ...
 *  glBindVertexArray(objVAO);
 *  glDrawArrays(GL_TRIANGLES, 0, nVertices);
 *
 */

 // Cabeçalhos necessários (para esta função), acrescentar ao seu código 
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

//GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// stb_image
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

struct Mesh 
{
    GLuint VAO; 

};

struct Material {
    string name;
    string texturePath;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

bool loadMTL(const string& filename, map<string, Material>& materials) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Erro ao abrir arquivo MTL: " << filename << endl;
        return false;
    }

    Material* currentMaterial = nullptr;
    string line;
    
    while (getline(file, line)) {
        istringstream iss(line);
        string token;
        iss >> token;

        if (token == "newmtl") {
            string name;
            iss >> name;
            materials[name] = Material();
            currentMaterial = &materials[name];
            currentMaterial->name = name;
            currentMaterial->texturePath = "../assets/tex/pixelWall.png"; // Inicializa o caminho da textura
        }
        else if (token == "map_Kd" && currentMaterial) {
            string texPath;
            // Lê o caminho completo da textura (pode conter espaços)
            getline(iss >> ws, texPath);
            currentMaterial->texturePath = texPath;
            cout << "Textura encontrada: " << texPath << endl;
        }
        else if (token == "Ka" && currentMaterial) {
            iss >> currentMaterial->ambient.x >> currentMaterial->ambient.y >> currentMaterial->ambient.z;
        }
        else if (token == "Kd" && currentMaterial) {
            iss >> currentMaterial->diffuse.x >> currentMaterial->diffuse.y >> currentMaterial->diffuse.z;
        }
        else if (token == "Ks" && currentMaterial) {
            iss >> currentMaterial->specular.x >> currentMaterial->specular.y >> currentMaterial->specular.z;
        }
    }

    file.close();
    return true;
}

GLuint loadTexture(const string& path) {
    cout << "Tentando carregar textura: " << path << endl;
    
    GLuint textureID;
    glGenTextures(1, &textureID);
    
    int width, height, nrComponents;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
    
    if (data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        cout << "Dimensões da textura: " << width << "x" << height << " com " << nrComponents << " componentes" << endl;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        cout << "Textura carregada com sucesso: " << path << endl;
    }
    else {
        cout << "Falha ao carregar textura: " << path << endl;
        cout << "Erro STB: " << stbi_failure_reason() << endl;
        stbi_image_free(data);
        return 0;
    }

    return textureID;
}

int loadSimpleOBJ(string filePATH, int &nVertices, GLuint &textureID)
{
    vector<glm::vec3> vertices;
    vector<glm::vec2> texCoords;
    vector<glm::vec3> normals;
    vector<GLfloat> vBuffer;
    map<string, Material> materials;
    string currentMaterial;
    
    string mtlPath;
    glm::vec3 color = glm::vec3(1.0, 0.0, 0.0);

    ifstream arqEntrada(filePATH.c_str());
    if (!arqEntrada.is_open()) {
        cerr << "Erro ao tentar ler o arquivo " << filePATH << endl;
        return -1;
    }

    string line;
    string directory;
    bool hasTexCoords = false;

    // Obtém o diretório do arquivo OBJ
    size_t lastSlash = filePATH.find_last_of("/\\");
    if (lastSlash != string::npos) {
        directory = filePATH.substr(0, lastSlash + 1);
    }

    while (getline(arqEntrada, line)) {
        istringstream ssline(line);
        string word;
        ssline >> word;

        if (word == "mtllib") {
            ssline >> mtlPath;
            string fullMtlPath = directory + mtlPath;
            cout << "Carregando arquivo MTL: " << fullMtlPath << endl;
            if (!loadMTL(fullMtlPath, materials)) {
                cerr << "Erro ao carregar arquivo MTL" << endl;
            }
        }
        else if (word == "usemtl") {
            ssline >> currentMaterial;
            cout << "Usando material: " << currentMaterial << endl;
        }
        else if (word == "v") {
            glm::vec3 vertice;
            ssline >> vertice.x >> vertice.y >> vertice.z;
            vertices.push_back(vertice);
        }
        else if (word == "vt") {
            glm::vec2 vt;
            ssline >> vt.s >> vt.t;
            // Inverte a coordenada V da textura (padrão OpenGL)
            vt.t = 1.0f - vt.t;
            texCoords.push_back(vt);
            hasTexCoords = true;
        }
        else if (word == "vn") {
            glm::vec3 normal;
            ssline >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        }
        else if (word == "f") {
            vector<int> vertexIndices, texIndices, normalIndices;
            string vertex;

            while (ssline >> vertex) {
                istringstream vStream(vertex);
                string indexStr;
                vector<int> indices;

                // Parse os índices separados por '/'
                while (getline(vStream, indexStr, '/')) {
                    if (indexStr.empty()) {
                        indices.push_back(-1);
                    } else {
                        indices.push_back(stoi(indexStr) - 1);
                    }
                }

                // Garante que temos 3 índices (v/vt/vn)
                while (indices.size() < 3) {
                    indices.push_back(-1);
                }

                vertexIndices.push_back(indices[0]);
                texIndices.push_back(indices[1]);
                normalIndices.push_back(indices[2]);
            }

            // Processa os vértices da face
            for (size_t i = 0; i < vertexIndices.size(); i++) {
                int vi = vertexIndices[i];
                int ti = texIndices[i];
                int ni = normalIndices[i];

                // Posição do vértice
                if (vi >= 0 && vi < vertices.size()) {
                    vBuffer.push_back(vertices[vi].x);
                    vBuffer.push_back(vertices[vi].y);
                    vBuffer.push_back(vertices[vi].z);
                }

                // Coordenadas de textura
                if (ti >= 0 && ti < texCoords.size()) {
                    vBuffer.push_back(texCoords[ti].s);
                    vBuffer.push_back(texCoords[ti].t);
                }
                else {
                    vBuffer.push_back(0.0f);
                    vBuffer.push_back(0.0f);
                }

                // Cor (será substituída pela textura se houver)
                vBuffer.push_back(color.r);
                vBuffer.push_back(color.g);
                vBuffer.push_back(color.b);
            }
        }
    }

    arqEntrada.close();

    cout << "Gerando o buffer de geometria..." << endl;
    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);
    
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    // Posição
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // Coordenadas de textura
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // Cor
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(5 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices = vBuffer.size() / 8;  // x, y, z, s, t, r, g, b (valores por vértice)

    // Carrega a textura se houver material com textura
    if (!materials.empty() && !currentMaterial.empty()) {
        auto& material = materials[currentMaterial];
        if (!material.texturePath.empty()) {
            // Usa o caminho da textura diretamente, já que é um caminho relativo completo
            cout << "Carregando textura: " << material.texturePath << endl;
            textureID = loadTexture(material.texturePath);
            if (textureID == 0) {
                cerr << "Erro ao carregar a textura. Tentando caminho alternativo..." << endl;
                // Tenta um caminho alternativo
                string altPath = "assets/tex/pixelWall.png";
                textureID = loadTexture(altPath);
                if (textureID == 0) {
                    cerr << "Também falhou com o caminho alternativo" << endl;
                }
            }
        }
    }

    return VAO;
}