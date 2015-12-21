#ifdef WIN32
#include <Windows.h>
#endif

#include "application.h"

// STD
#include <iostream>
#include <fstream>
#include <time.h>
#include <random>

// GL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>

#include "../StadiumGenerator/stadium.h"


// Static Members
GLFWwindow*				    Application::m_window = 0;
unsigned int			    Application::m_width = 0;
unsigned int			    Application::m_height = 0;
bool					        Application::m_controlKeyHold = false;
bool					        Application::m_altKeyHold = false;
bool					        Application::m_w_pressed = false;
bool					        Application::m_s_pressed = false;
bool					        Application::m_a_pressed = false;
bool					        Application::m_d_pressed = false;
bool					        Application::m_q_pressed = false;
bool					        Application::m_e_pressed = false;
bool                  Application::m_mouse_left_drag = false;
bool                  Application::m_mouse_middle_drag = false;
bool                  Application::m_mouse_right_drag = false;

// read the file content and generate a string from it.
static std::string convertFileToString(const std::string& filename) {
  std::ifstream ifile(filename);
  if (!ifile){
    return std::string("");
  }

  return std::string(std::istreambuf_iterator<char>(ifile), (std::istreambuf_iterator<char>()));

}

static void show_compiler_error(GLuint shader) {
  GLint isCompiled;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
  if (isCompiled == GL_FALSE)
  {
    GLint maxLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

    // The maxLength includes the NULL character
    std::vector<GLchar> errorLog(maxLength);
    glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);

    std::cout << "Shader " << shader << " Log: \n";
    std::cout << ((char*)&errorLog[0]);

    // Provide the infolog in whatever manor you deem best.
    // Exit with failure.
    glDeleteShader(shader); // Don't leak the shader.
    return;
  }
}

static GLuint compile_link_vs_fs(const std::string& vert_shader_file, const std::string& frag_shader_file) {
  GLuint vertex_shader, fragment_shader;
  std::string vertex_shader_source = convertFileToString(vert_shader_file);
  std::string coord_sys_shader_fragment_source = convertFileToString(frag_shader_file);

  if (vertex_shader_source.size() == 0 || coord_sys_shader_fragment_source.size() == 0){
    std::cout << "Problem in finding " << vert_shader_file << " or " << frag_shader_file << std::endl;
    return -1;
  }


  vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

  const GLchar* vertex_shader_sourcePtr = vertex_shader_source.c_str();
  const GLchar* fragment_shader_sourcePtr = coord_sys_shader_fragment_source.c_str();

  glShaderSource(vertex_shader, 1, &vertex_shader_sourcePtr, NULL);
  glShaderSource(fragment_shader, 1, &fragment_shader_sourcePtr, NULL);

  glCompileShader(vertex_shader);
  glCompileShader(fragment_shader);

  show_compiler_error(vertex_shader);
  show_compiler_error(fragment_shader);

  GLuint shader_program = glCreateProgram();
  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);
  glLinkProgramARB(shader_program);

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  return shader_program;
}


Application::Application() {
  initialization_step = true;
  m_worldmat = m_viewmat = m_projmat = glm::mat4(1.0f);
}

void Application::init(const unsigned int& width, const unsigned int& height) {

  m_width = width; m_height = height;

  glfwSetErrorCallback(error_callback);
  if (!glfwInit())
    exit(EXIT_FAILURE);

  m_window = glfwCreateWindow(width, height, "Stream Surface Generator (Demo): Beta", NULL, NULL);
  if (!m_window)
  {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwMakeContextCurrent(m_window);

  glfwSetKeyCallback(m_window, key_callback);
  glfwSetWindowSizeCallback(m_window, WindowSizeCB);
  glfwSetCursorPosCallback(m_window, EventMousePos);
  glfwSetScrollCallback(m_window, EventMouseWheel);
  glfwSetMouseButtonCallback(m_window, (GLFWmousebuttonfun)EventMouseButton);
  glfwSetKeyCallback(m_window, (GLFWkeyfun)EventKey);

  // - Directly redirect GLFW char events to AntTweakBar
  glfwSetCharCallback(m_window, (GLFWcharfun)EventChar);

  if (glewInit() != GLEW_OK){
    std::cout << "cannot intialize the glew.\n";
    exit(EXIT_FAILURE);
  }

  init();
}

void Application::init() {
  GLenum e = glGetError();
  glClearColor(19.0f / 255.0f, 9.0f / 255.0f, 99.0f / 255.0f, 1.0f);
  e = glGetError();

  e = glGetError();
  glEnable(GL_DEPTH_TEST);
  e = glGetError();
}

void Application::create() {
  compileShaders();


  Stadium stadium;
  read_stadium_definition("../StadiumGenerator/stadium.def", stadium);

  std::vector<float> points;
  std::vector<float> colors;
  std::vector<int> indices;

  float len[] = { 0.25f, 0.25f, 0.25f };

  for (int l = 0; l < stadium.num_layers; l++){
    float layer_dim[3] = {
      stadium.layer_bbox[l].max.v[0] - stadium.layer_bbox[l].min.v[0],
      stadium.layer_bbox[l].max.v[1] - stadium.layer_bbox[l].min.v[1],
      stadium.layer_bbox[l].max.v[2] - stadium.layer_bbox[l].min.v[2]
    };

    float  elem_dim_z = layer_dim[2];
    float  offset_z = stadium.layer_bbox[l].min.v[2]; // static_cast<float>(l) / (stadium.num_layers - 1);

    int layer_type = stadium.layers[l];

    for (size_t i = 0; i < stadium.layer_types[layer_type].size(); i++){
      for (size_t j = 0; j < stadium.layer_types[layer_type][i].size(); j++){

        float elem_dim_x = 1.0f / static_cast<float>(stadium.layer_types[layer_type].size()) * layer_dim[0];
        float elem_dim_y = 1.0f / static_cast<float>(stadium.layer_types[layer_type][i].size()) * layer_dim[1];

        float offset_x = stadium.layer_bbox[l].min.v[0] + static_cast<float>(i)* elem_dim_x;
        float offset_y = stadium.layer_bbox[l].min.v[1] + static_cast<float>(j)* elem_dim_y;

        uint32_t block_type = stadium.layer_types[layer_type][i][j];
        int dims[3] = { stadium.block_sizes[3 * block_type + 0], stadium.block_sizes[3 * block_type + 1], stadium.block_sizes[3 * block_type + 2] };

        uint32_t offset_points = static_cast<uint32_t>(points.size() / 3);

        // Generating the points
        for (int d0 = 0; d0 <= dims[0]; d0++)
          for (int d1 = 0; d1 <= dims[1]; d1++)
            for (int d2 = 0; d2 <= dims[2]; d2++){

              float local_x = static_cast<float>(d0) / static_cast<float>(dims[0]);
              float local_y = static_cast<float>(d1) / static_cast<float>(dims[1]);
              float local_z = static_cast<float>(d2) / static_cast<float>(dims[2]);

              float x = (offset_x + local_x * elem_dim_x);
              float y = (offset_y + local_y * elem_dim_y);
              float z = (offset_z + local_z * elem_dim_z);

              points.push_back(x * len[0]);
              points.push_back(y * len[1]);
              points.push_back(z * len[2]);

              if (x + y + z > 0.6f){
                colors.push_back(x);
                colors.push_back(y);
                colors.push_back(z);
              }
              else {
                colors.push_back(0.2f);
                colors.push_back(0.2f);
                colors.push_back(0.2f);
              }
            }

        // Adding the points to the lists
        for (int d0 = 0; d0 < dims[0]; d0++)
          for (int d1 = 0; d1 < dims[1]; d1++)
            for (int d2 = 0; d2 < dims[2]; d2++){

              int p0 = (offset_points +  d0      * (dims[1] + 1) * (dims[2] + 1) +  d1       * (dims[2] + 1) + d2    );
              int p1 = (offset_points + (d0 + 1) * (dims[1] + 1) * (dims[2] + 1) +  d1       * (dims[2] + 1) + d2    );
              int p2 = (offset_points + (d0 + 1) * (dims[1] + 1) * (dims[2] + 1) +  d1       * (dims[2] + 1) + d2 + 1);
              int p3 = (offset_points +  d0      * (dims[1] + 1) * (dims[2] + 1) +  d1       * (dims[2] + 1) + d2 + 1);
              int p4 = (offset_points +  d0      * (dims[1] + 1) * (dims[2] + 1) + (d1 + 1)  * (dims[2] + 1) + d2    );
              int p5 = (offset_points + (d0 + 1) * (dims[1] + 1) * (dims[2] + 1) + (d1 + 1)  * (dims[2] + 1) + d2    );
              int p6 = (offset_points + (d0 + 1) * (dims[1] + 1) * (dims[2] + 1) + (d1 + 1)  * (dims[2] + 1) + d2 + 1);
              int p7 = (offset_points +  d0      * (dims[1] + 1) * (dims[2] + 1) + (d1 + 1)  * (dims[2] + 1) + d2 + 1);

              indices.push_back(p0);
              indices.push_back(p1);

              indices.push_back(p1);
              indices.push_back(p2);

              indices.push_back(p2);
              indices.push_back(p3);

              indices.push_back(p3);
              indices.push_back(p0);


              indices.push_back(p4);
              indices.push_back(p5);

              indices.push_back(p5);
              indices.push_back(p6);

              indices.push_back(p6);
              indices.push_back(p7);

              indices.push_back(p7);
              indices.push_back(p4);


              indices.push_back(p0);
              indices.push_back(p4);

              indices.push_back(p1);
              indices.push_back(p5);

              indices.push_back(p2);
              indices.push_back(p6);

              indices.push_back(p3);
              indices.push_back(p7);

          }
      }
    }
  }

  DrawElementsIndirectCommand indirect_cmds[4] = {
    {
      indices.size(),
      1,
      0,
      0,
      0
    }
  };

  glGenBuffers(1, &indirect_buffer);
  glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect_buffer);
  glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(indirect_cmds), indirect_cmds, GL_STATIC_DRAW);

  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(float), points.data(), GL_STATIC_DRAW);

  glGenBuffers(1, &color_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, color_buffer);
  glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(float), colors.data(), GL_STATIC_DRAW);

  glGenBuffers(1, &index_buffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW);
}

void Application::update(float time, float timeSinceLastFrame) {
  float v = (float)clock() / 3000.0f * glm::pi<float>();
  m_inv_viewmat = glm::inverse(m_viewmat);
  m_viewmat = glm::lookAt(6.0f * glm::vec3(sin(v), 1.0f, cos(v)), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  m_projmat = glm::perspective(glm::pi<float>() / 3.0f, (float)m_width / m_height, 0.1f, 1000.0f);
}

void Application::draw() {
  GLenum e = glGetError();
  glViewport(0, 0, m_width, m_height);
  e = glGetError();
  float back_color[] = { 0, 0.0, 0.0 };
  float zero[] = { 0.0f, 0.0f, 0.0f, 0.0f };
  float one = 1.0f;
  e = glGetError();
  glClearBufferfv(GL_COLOR, 0, back_color);
  glClearBufferfv(GL_DEPTH, 0, &one);
  e = glGetError();
  glUseProgram(shader);
  e = glGetError();

  GLint proj_mat = glGetUniformLocation(shader, "proj_mat");
  GLint view_mat = glGetUniformLocation(shader, "view_mat");
  GLint world_mat = glGetUniformLocation(shader, "world_mat");

  glUniformMatrix4fv(proj_mat, 1, GL_FALSE, &m_projmat[0][0]);
  glUniformMatrix4fv(view_mat, 1, GL_FALSE, &m_viewmat[0][0]);
  glUniformMatrix4fv(world_mat, 1, GL_FALSE, &m_worldmat[0][0]);

  e = glGetError();
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, NULL, 0);

  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ARRAY_BUFFER, color_buffer);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, NULL, 0);
  e = glGetError();

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);

#define TRY_INDIRECT
#ifndef TRY_INDIRECT
  glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
#else
  glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect_buffer);
  glMultiDrawElementsIndirect(GL_LINES, GL_UNSIGNED_INT, 0, 1, 0);
#endif
  e = glGetError();

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
}

void Application::run() {
  create();
  double start_time;
  double start_frame;
  start_time = start_frame = glfwGetTime();

  while (!glfwWindowShouldClose(m_window))
  {
    double frame_start_time = glfwGetTime();
    draw();
    double frame_end_time = glfwGetTime();


    glfwSwapBuffers(m_window);
    glfwPollEvents();

    double current_time = glfwGetTime();
    double elapsed_since_start = current_time - start_time;
    double elapsed_since_last_frame = current_time - start_frame;

    start_frame = glfwGetTime();

    update(elapsed_since_start, elapsed_since_last_frame);
  }
}

void Application::shutdown() {
  glfwDestroyWindow(m_window);
  glfwTerminate();
  exit(EXIT_SUCCESS);
}

Application::~Application() {
}

void Application::compileShaders() {
  shader = compile_link_vs_fs("vertex.glsl", "fragment.glsl");
}

void Application::EventMouseButton(GLFWwindow* window, int button, int action, int mods) {
 
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    m_mouse_left_drag = true;

  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    m_mouse_left_drag = false;

  if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    m_mouse_right_drag = true;

  if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    m_mouse_right_drag = false;

  if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    m_mouse_middle_drag = true;

  if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    m_mouse_middle_drag = false;
}

void Application::EventMousePos(GLFWwindow* window, double xpos, double ypos) {
}

void Application::EventMouseWheel(GLFWwindow* window, double xoffset, double yoffset) {

}

void Application::EventKey(GLFWwindow* window, int key, int scancode, int action, int mods) {

  if (action == GLFW_PRESS){
    if (m_controlKeyHold && key == GLFW_KEY_W)  m_w_pressed = true;
    if (m_controlKeyHold && key == GLFW_KEY_S)  m_s_pressed = true;
    if (m_controlKeyHold && key == GLFW_KEY_A)  m_a_pressed = true;
    if (m_controlKeyHold && key == GLFW_KEY_D)  m_d_pressed = true;
    if (m_controlKeyHold && key == GLFW_KEY_Q)  m_q_pressed = true;
    if (m_controlKeyHold && key == GLFW_KEY_E)  m_e_pressed = true;

    if (key == GLFW_KEY_LEFT_CONTROL)           m_controlKeyHold = true;
    if (key == GLFW_KEY_LEFT_ALT)               m_altKeyHold = true;
  }

  if (action == GLFW_RELEASE){
    if (key == GLFW_KEY_W)  m_w_pressed = false;
    if (key == GLFW_KEY_S)  m_s_pressed = false;
    if (key == GLFW_KEY_A)  m_a_pressed = false;
    if (key == GLFW_KEY_D)  m_d_pressed = false;
    if (key == GLFW_KEY_Q)  m_q_pressed = false;
    if (key == GLFW_KEY_E)  m_e_pressed = false;

    if (key == GLFW_KEY_LEFT_CONTROL)           m_controlKeyHold = false;
    if (key == GLFW_KEY_LEFT_ALT)               m_altKeyHold = false;

    double xpos, ypos;
    glfwGetCursorPos(m_window, &xpos, &ypos);
  }
}

void Application::EventChar(GLFWwindow* window, int codepoint) {
}

// Callback function called by GLFW when window size changes
void Application::WindowSizeCB(GLFWwindow* window, int width, int height) {
  m_width = width; m_height = height;
  glViewport(0, 0, width, height);
}
void Application::error_callback(int error, const char* description) {
  fputs(description, stderr);
}
void Application::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
  }

  if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_PRESS){

  }

  if (key == GLFW_KEY_LEFT_CONTROL && action == GLFW_RELEASE){
  }
}
