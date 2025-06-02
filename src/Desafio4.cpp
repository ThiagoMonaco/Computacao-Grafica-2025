#include <iostream>
#include <string>
#include <assert.h>
#include <vector>
#include <fstream>
#include <sstream>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace glm;

#include <cmath>

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader();
int setupGeometry();
GLuint loadTexture(string filePath, int &width, int &height);
bool loadMTL(const char* path, vec3& ka, vec3& kd, vec3& ks, float& ns);
void drawGeometry(GLuint shaderID, GLuint VAO, vec3 position, vec3 dimensions, float angle, int nVertices, vec3 color= vec3(1.0,0.0,0.0), vec3 axis = vec3(0.0, 0.0, 1.0));
GLuint generateSphere(float radius, int latSegments, int lonSegments, int &nVertices);
bool loadOBJ(const char* path, vector<vec3>& out_vertices, vector<vec2>& out_uvs, vector<vec3>& out_normals);
GLuint createVAOFromOBJ(const char* objPath, int& nVertices);

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 800, HEIGHT = 800;

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar *vertexShaderSource = R"(
#version 400
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 texc;

uniform mat4 projection;
uniform mat4 model;
uniform mat4 view;

out vec2 texCoord;
out vec3 fragNormal;
out vec3 fragPos;
out vec4 vColor;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0);
    fragPos = vec3(model * vec4(position, 1.0));
    fragNormal = mat3(transpose(inverse(model))) * normal;
    texCoord = texc;
    vColor = vec4(color, 1.0);
})";

// Código fonte do Fragment Shader (em GLSL): ainda hardcoded
const GLchar *fragmentShaderSource = R"(
#version 400
in vec2 texCoord;
in vec3 fragNormal;
in vec3 fragPos;
in vec4 vColor;

uniform sampler2D texBuff;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;

uniform vec3 Ka;
uniform vec3 Kd;
uniform vec3 Ks;
uniform float Ns;

out vec4 color;

void main()
{
    vec3 normal = normalize(fragNormal);
    vec3 lightDir = normalize(lightPos - fragPos);
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);

    vec3 ambient = Ka * lightColor;

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = Kd * diff * lightColor;

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), Ns);
    vec3 specular = Ks * spec * lightColor;

    vec3 result = (ambient + diffuse) * vec3(vColor) + specular;
    color = vec4(result, 1.0);
})";

GLuint generateSphere(float radius, int latSegments, int lonSegments, int &nVertices) {
    vector<GLfloat> vBuffer;

    vec3 color = vec3(1.0f, 0.0f, 0.0f);

    auto calcPosUVNormal = [&](int lat, int lon, vec3& pos, vec2& uv, vec3& normal) {
        float theta = lat * pi<float>() / latSegments;
        float phi = lon * 2.0f * pi<float>() / lonSegments;

        pos = vec3(
            radius * cos(phi) * sin(theta),
            radius * cos(theta),
            radius * sin(phi) * sin(theta)
        );

        uv = vec2(
            phi / (2.0f * pi<float>()),
            theta / pi<float>()
        );

        normal = normalize(pos);
    };

    for (int i = 0; i < latSegments; ++i) {
        for (int j = 0; j < lonSegments; ++j) {
            vec3 v0, v1, v2, v3;
            vec2 uv0, uv1, uv2, uv3;
            vec3 n0, n1, n2, n3;

            calcPosUVNormal(i, j, v0, uv0, n0);
            calcPosUVNormal(i + 1, j, v1, uv1, n1);
            calcPosUVNormal(i, j + 1, v2, uv2, n2);
            calcPosUVNormal(i + 1, j + 1, v3, uv3, n3);

            vBuffer.insert(vBuffer.end(), { v0.x, v0.y, v0.z, color.r, color.g, color.b, n0.x, n0.y, n0.z, uv0.x, uv0.y });
            vBuffer.insert(vBuffer.end(), { v1.x, v1.y, v1.z, color.r, color.g, color.b, n1.x, n1.y, n1.z, uv1.x, uv1.y });
            vBuffer.insert(vBuffer.end(), { v2.x, v2.y, v2.z, color.r, color.g, color.b, n2.x, n2.y, n2.z, uv2.x, uv2.y });

            vBuffer.insert(vBuffer.end(), { v1.x, v1.y, v1.z, color.r, color.g, color.b, n1.x, n1.y, n1.z, uv1.x, uv1.y });
            vBuffer.insert(vBuffer.end(), { v3.x, v3.y, v3.z, color.r, color.g, color.b, n3.x, n3.y, n3.z, uv3.x, uv3.y });
            vBuffer.insert(vBuffer.end(), { v2.x, v2.y, v2.z, color.r, color.g, color.b, n2.x, n2.y, n2.z, uv2.x, uv2.y });
        }
    }

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(0));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(9 * sizeof(GLfloat)));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);

    nVertices = vBuffer.size() / 11;

    return VAO;
}

bool loadOBJ(const char* path, vector<vec3>& out_vertices, vector<vec2>& out_uvs, vector<vec3>& out_normals) {
    vector<unsigned int> vertexIndices, uvIndices, normalIndices;
    vector<vec3> temp_vertices;
    vector<vec2> temp_uvs;
    vector<vec3> temp_normals;

    FILE* file = fopen(path, "r");
    if (file == NULL) {
        printf("Erro ao abrir o arquivo OBJ!\n");
        return false;
    }

    while (1) {
        char lineHeader[128];
        int res = fscanf(file, "%s", lineHeader);
        if (res == EOF)
            break;

        if (strcmp(lineHeader, "v") == 0) {
            vec3 vertex;
            fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
            temp_vertices.push_back(vertex);
        }
        else if (strcmp(lineHeader, "vt") == 0) {
            vec2 uv;
            fscanf(file, "%f %f\n", &uv.x, &uv.y);
            temp_uvs.push_back(uv);
        }
        else if (strcmp(lineHeader, "vn") == 0) {
            vec3 normal;
            fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
            temp_normals.push_back(normal);
        }
        else if (strcmp(lineHeader, "f") == 0) {
            unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
            int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n",
                &vertexIndex[0], &uvIndex[0], &normalIndex[0],
                &vertexIndex[1], &uvIndex[1], &normalIndex[1],
                &vertexIndex[2], &uvIndex[2], &normalIndex[2]);

            if (matches != 9) {
                printf("Arquivo OBJ não pode ser lido. Tente exportar com outras opções\n");
                return false;
            }

            vertexIndices.push_back(vertexIndex[0]);
            vertexIndices.push_back(vertexIndex[1]);
            vertexIndices.push_back(vertexIndex[2]);
            uvIndices.push_back(uvIndex[0]);
            uvIndices.push_back(uvIndex[1]);
            uvIndices.push_back(uvIndex[2]);
            normalIndices.push_back(normalIndex[0]);
            normalIndices.push_back(normalIndex[1]);
            normalIndices.push_back(normalIndex[2]);
        }
    }

    // Processando os índices
    for (unsigned int i = 0; i < vertexIndices.size(); i++) {
        unsigned int vertexIndex = vertexIndices[i];
        unsigned int uvIndex = uvIndices[i];
        unsigned int normalIndex = normalIndices[i];

        vec3 vertex = temp_vertices[vertexIndex - 1];
        vec2 uv = temp_uvs[uvIndex - 1];
        vec3 normal = temp_normals[normalIndex - 1];

        out_vertices.push_back(vertex);
        out_uvs.push_back(uv);
        out_normals.push_back(normal);
    }

    fclose(file);
    return true;
}

GLuint createVAOFromOBJ(const char* objPath, int& nVertices) {
    vector<vec3> vertices;
    vector<vec2> uvs;
    vector<vec3> normals;
    vector<GLfloat> vboData;

    if (!loadOBJ(objPath, vertices, uvs, normals)) {
        return 0;
    }

    vec3 color(1.0f, 0.0f, 0.0f); // Cor padrão vermelha

    // Montando o VBO com todos os atributos
    for (size_t i = 0; i < vertices.size(); i++) {
        // Posição
        vboData.push_back(vertices[i].x);
        vboData.push_back(vertices[i].y);
        vboData.push_back(vertices[i].z);
        
        // Cor
        vboData.push_back(color.r);
        vboData.push_back(color.g);
        vboData.push_back(color.b);
        
        // Normal
        vboData.push_back(normals[i].x);
        vboData.push_back(normals[i].y);
        vboData.push_back(normals[i].z);
        
        // UV
        vboData.push_back(uvs[i].x);
        vboData.push_back(uvs[i].y);
    }

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vboData.size() * sizeof(GLfloat), vboData.data(), GL_STATIC_DRAW);

    // Posição (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // Cor (location = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // Normal (location = 2)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    // UV (location = 3)
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(9 * sizeof(GLfloat)));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);

    nVertices = vertices.size();
    return VAO;
}

int main()
{
    glfwInit();

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Desafio 4 - Iluminação de Phong", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    GLuint shaderID = setupShader();

    int nVertices;
    GLuint VAO = createVAOFromOBJ("../assets/Modelos3D/sphere.obj", nVertices);
    if (VAO == 0) {
        // Se falhar ao carregar o OBJ, usa a esfera gerada proceduralmente
        VAO = generateSphere(0.5, 50, 50, nVertices);
    }

    glUseProgram(shaderID);

    vec3 lightPos(2.0f, 2.0f, 2.0f);
    vec3 lightColor(1.0f, 1.0f, 1.0f);
    
    glUniform3fv(glGetUniformLocation(shaderID, "lightPos"), 1, value_ptr(lightPos));
    glUniform3fv(glGetUniformLocation(shaderID, "lightColor"), 1, value_ptr(lightColor));
    glUniform3fv(glGetUniformLocation(shaderID, "viewPos"), 1, value_ptr(vec3(0.0f, 0.0f, 5.0f)));

    // Carregando coeficientes do material do arquivo MTL
    vec3 ka, kd, ks;
    float ns;
    if (!loadMTL("../assets/materials/sphere.mtl", ka, kd, ks, ns)) {
        // Se falhar ao carregar o MTL, usa valores padrão
        ka = vec3(0.2f, 0.2f, 0.2f);
        kd = vec3(0.7f, 0.7f, 0.7f);
        ks = vec3(1.0f, 1.0f, 1.0f);
        ns = 32.0f;
    }

    glUniform3fv(glGetUniformLocation(shaderID, "Ka"), 1, value_ptr(ka));
    glUniform3fv(glGetUniformLocation(shaderID, "Kd"), 1, value_ptr(kd));
    glUniform3fv(glGetUniformLocation(shaderID, "Ks"), 1, value_ptr(ks));
    glUniform1f(glGetUniformLocation(shaderID, "Ns"), ns);

    mat4 view = lookAt(vec3(0.0f, 0.0f, 5.0f), vec3(0.0f), vec3(0.0f, 1.0f, 0.0f));
    mat4 projection = perspective(radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
    
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mat4 model = mat4(1.0f);
        model = rotate(model, (float)glfwGetTime(), vec3(0.5f, 1.0f, 0.0f));
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, nVertices);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}

int setupShader()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

bool loadMTL(const char* path, vec3& ka, vec3& kd, vec3& ks, float& ns) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cout << "Erro ao abrir arquivo MTL: " << path << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string token;
        iss >> token;

        if (token == "Ka") {
            float x, y, z;
            iss >> x >> y >> z;
            ka = vec3(x, y, z);
        }
        else if (token == "Kd") {
            float x, y, z;
            iss >> x >> y >> z;
            kd = vec3(x, y, z);
        }
        else if (token == "Ks") {
            float x, y, z;
            iss >> x >> y >> z;
            ks = vec3(x, y, z);
        }
        else if (token == "Ns") {
            iss >> ns;
        }
    }

    file.close();
    return true;
}