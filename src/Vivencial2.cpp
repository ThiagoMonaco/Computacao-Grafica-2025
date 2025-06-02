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

// Estrutura para armazenar informações da luz
struct Light {
    vec3 position;
    vec3 color;
    float intensity;
    bool enabled;
};

// Variáveis globais para as luzes
Light keyLight, fillLight, backLight;

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader();
GLuint loadTexture(string filePath, int &width, int &height);
bool loadMTL(const char* path, vec3& ka, vec3& kd, vec3& ks, float& ns);
bool loadOBJ(const char* path, vector<vec3>& out_vertices, vector<vec2>& out_uvs, vector<vec3>& out_normals);
GLuint createVAOFromOBJ(const char* objPath, int& nVertices);
void setupLights(const vec3& objectPosition, const vec3& objectScale);
void printInstructions();

// Dimensões da janela
const GLuint WIDTH = 800, HEIGHT = 800;

// Código fonte do Vertex Shader
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

// Código fonte do Fragment Shader
const GLchar *fragmentShaderSource = R"(
#version 400
in vec2 texCoord;
in vec3 fragNormal;
in vec3 fragPos;
in vec4 vColor;

uniform sampler2D texBuff;
uniform vec3 viewPos;
uniform bool useTexture;

// Material properties
uniform vec3 Ka;
uniform vec3 Kd;
uniform vec3 Ks;
uniform float Ns;

// Light properties (arrays for 3 lights)
uniform vec3 lightPositions[3];
uniform vec3 lightColors[3];
uniform float lightIntensities[3];
uniform bool lightEnabled[3];

out vec4 color;

float calculateAttenuation(float distance) {
    float constant = 1.0;
    float linear = 0.09;
    float quadratic = 0.032;
    return 1.0 / (constant + linear * distance + quadratic * distance * distance);
}

void main()
{
    vec3 normal = normalize(fragNormal);
    vec3 viewDir = normalize(viewPos - fragPos);
    
    vec3 result = vec3(0.0);
    vec4 texColor = texture(texBuff, texCoord);
    vec4 baseColor = useTexture ? texColor : vColor;
    
    // Calculate contribution from each light
    for(int i = 0; i < 3; i++) {
        if(lightEnabled[i]) {
            vec3 lightDir = normalize(lightPositions[i] - fragPos);
            float distance = length(lightPositions[i] - fragPos);
            float attenuation = calculateAttenuation(distance);
            
            // Ambient
            vec3 ambient = Ka * lightColors[i] * lightIntensities[i];
            
            // Diffuse
            float diff = max(dot(normal, lightDir), 0.0);
            vec3 diffuse = Kd * diff * lightColors[i] * lightIntensities[i] * attenuation;
            
            // Specular
            vec3 reflectDir = reflect(-lightDir, normal);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), Ns);
            vec3 specular = Ks * spec * lightColors[i] * lightIntensities[i];
            
            result += (ambient + diffuse + specular);
        }
    }
    
    color = vec4(result * vec3(baseColor), baseColor.a);
})";

void setupLights(const vec3& objectPosition, const vec3& objectScale) {
    float maxScale = std::max(std::max(objectScale.x, objectScale.y), objectScale.z);
    float distance = maxScale * 3.0f;  // Aumentei a distância para 3x

    // Key Light (principal) - mais intensa, posicionada à frente e ligeiramente acima
    keyLight.position = objectPosition + vec3(distance, distance * 1.5f, distance);
    keyLight.color = vec3(1.0f, 0.95f, 0.8f);  // Luz levemente amarelada
    keyLight.intensity = 2.0f;  // Aumentei a intensidade
    keyLight.enabled = true;

    // Fill Light (preenchimento) - menos intensa, oposta à key light
    fillLight.position = objectPosition + vec3(-distance, 0.0f, distance * 0.5f);
    fillLight.color = vec3(0.4f, 0.4f, 0.8f);  // Azulada mais suave
    fillLight.intensity = 1.0f;
    fillLight.enabled = true;

    // Back Light (contraluz) - média intensidade, atrás do objeto
    backLight.position = objectPosition + vec3(0.0f, distance * 0.8f, -distance);
    backLight.color = vec3(0.8f, 0.8f, 1.0f);  // Levemente azulada
    backLight.intensity = 1.5f;  // Aumentei um pouco
    backLight.enabled = true;
}

// Função de callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // Controles das luzes
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_1:  // Key Light
                keyLight.enabled = !keyLight.enabled;
                break;
            case GLFW_KEY_2:  // Fill Light
                fillLight.enabled = !fillLight.enabled;
                break;
            case GLFW_KEY_3:  // Back Light
                backLight.enabled = !backLight.enabled;
                break;
        }
    }
}

void printInstructions() {
    cout << "=== Instruções de Controle ===" << endl;
    cout << "Tecla 1: Liga/Desliga Key Light (luz principal, mais intensa)" << endl;
    cout << "Tecla 2: Liga/Desliga Fill Light (luz de preenchimento, suaviza sombras)" << endl;
    cout << "Tecla 3: Liga/Desliga Back Light (contraluz, adiciona profundidade)" << endl;
    cout << "ESC: Fecha a aplicação" << endl;
    cout << "===========================" << endl;
}

int main()
{
    glfwInit();

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Vivencial 2 - Iluminação de 3 Pontos", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    // Imprime as instruções de controle
    printInstructions();

    GLuint shaderID = setupShader();

    int nVertices;
    GLuint VAO = createVAOFromOBJ("../assets/Modelos3D/Suzanne.obj", nVertices);

    glUseProgram(shaderID);

    // Configuração inicial do objeto
    vec3 objectPosition(0.0f, 0.0f, 0.0f);
    vec3 objectScale(0.5f, 0.5f, 0.5f);
    setupLights(objectPosition, objectScale);

    // Configuração do material
    vec3 ka, kd, ks;
    float ns;
    if (!loadMTL("../assets/Modelos3D/Suzanne.mtl", ka, kd, ks, ns)) {
        ka = vec3(0.2f);      // Ambiente
        kd = vec3(0.8f);      // Difuso - aumentei para 0.8
        ks = vec3(1.0f);      // Especular
        ns = 64.0f;           // Shininess - aumentei para 64
    }

    // Uniforms para material
    glUniform3fv(glGetUniformLocation(shaderID, "Ka"), 1, value_ptr(ka));
    glUniform3fv(glGetUniformLocation(shaderID, "Kd"), 1, value_ptr(kd));
    glUniform3fv(glGetUniformLocation(shaderID, "Ks"), 1, value_ptr(ks));
    glUniform1f(glGetUniformLocation(shaderID, "Ns"), ns);

    // Arrays para as propriedades das luzes
    vec3 lightPositions[3] = {keyLight.position, fillLight.position, backLight.position};
    vec3 lightColors[3] = {keyLight.color, fillLight.color, backLight.color};
    float lightIntensities[3] = {keyLight.intensity, fillLight.intensity, backLight.intensity};
    int lightEnabled[3] = {keyLight.enabled, fillLight.enabled, backLight.enabled};

    // Uniforms para luzes
    glUniform3fv(glGetUniformLocation(shaderID, "lightPositions"), 3, value_ptr(lightPositions[0]));
    glUniform3fv(glGetUniformLocation(shaderID, "lightColors"), 3, value_ptr(lightColors[0]));
    glUniform1fv(glGetUniformLocation(shaderID, "lightIntensities"), 3, lightIntensities);
    glUniform1iv(glGetUniformLocation(shaderID, "lightEnabled"), 3, lightEnabled);

    // Configuração da câmera
    vec3 cameraPos = vec3(0.0f, 0.0f, 3.0f);
    mat4 view = lookAt(cameraPos, vec3(0.0f), vec3(0.0f, 1.0f, 0.0f));
    mat4 projection = perspective(radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);

    glUniform3fv(glGetUniformLocation(shaderID, "viewPos"), 1, value_ptr(cameraPos));
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));

    // Carregamento da textura
    int texWidth, texHeight;
    GLuint texID = loadTexture("../assets/tex/pixelWall.png", texWidth, texHeight);
    
    // Ativa a textura
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID);
    glUniform1i(glGetUniformLocation(shaderID, "texBuff"), 0);
    glUniform1i(glGetUniformLocation(shaderID, "useTexture"), true);

    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Atualiza estado das luzes
        int lightStates[3] = {keyLight.enabled, fillLight.enabled, backLight.enabled};
        glUniform1iv(glGetUniformLocation(shaderID, "lightEnabled"), 3, lightStates);

        // Matriz de modelo com rotação
        mat4 model = mat4(1.0f);
        model = rotate(model, (float)glfwGetTime(), vec3(0.0f, 1.0f, 0.0f));
        model = scale(model, objectScale);
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

int setupShader() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
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

GLuint loadTexture(string filePath, int &width, int &height)
{
    cout << "Tentando carregar textura: " << filePath << endl;
    
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int nrChannels;
    stbi_set_flip_vertically_on_load(true);

    unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

    if (data)
    {
        cout << "Textura carregada com sucesso!" << endl;
        cout << "Dimensões: " << width << "x" << height << endl;
        cout << "Canais: " << nrChannels << endl;

        if (nrChannels == 3) // jpg, bmp
        {
            cout << "Formato: RGB" << endl;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        }
        else // png com alpha
        {
            cout << "Formato: RGBA" << endl;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        cout << "Falha ao carregar textura: " << filePath << endl;
        cout << "Erro STB: " << stbi_failure_reason() << endl;
    }

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texID;
}