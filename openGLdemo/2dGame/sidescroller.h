#pragma once
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include "obj.h"
#include "aabb_collide.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <vector>

extern const int default_window_width;
extern const int default_window_height;

static std::unique_ptr<GLSLShader> _2d_shader;
int tex1_id;

namespace sidescroller {

enum class GAMESTATE { PLAYING, WON, LOST } gamestate;

struct Player {
  float speed = 165.f;
  float jump_power = 140.f;
  obj body = { glm::vec3(-475, -225, -1), glm::vec3(15,15,1) };
  bool left = false, right = false, jumping = false;
  bool falling = true;
} player;

// assets
std::vector<obj> objects = {
  /* location */                /* scale */
  { glm::vec3(-600, -200, -1), glm::vec3(25,25,1)},
  { glm::vec3(-400, -300, -1), glm::vec3(100,50,1)},
  { glm::vec3(-200, -300, -1), glm::vec3(100,50,1)},
  { glm::vec3(0,   -200, -1), glm::vec3(100, 10,1)},
  { glm::vec3(200, -300, -1), glm::vec3(100, 50,1)},
  { glm::vec3(400, -300, -1), glm::vec3(100, 50,1)}
};

obj goal = { glm::vec3(475, -225, -1), glm::vec3(5,5,1) };

std::vector<obj> lost_display = {
  { glm::vec3(0, 0, -1), glm::vec3(25, 100,1), glm::vec3(.7f,.0f, .1f), glm::vec3(0, 0, glm::radians(45.f)) },
  { glm::vec3(0, 0, -1), glm::vec3(25, 100,1), glm::vec3(.7f,.0f, .1f), glm::vec3(0, 0, glm::radians(-45.f)) }
};

std::vector<obj> won_display = {
  { glm::vec3(-81, -25, -1), glm::vec3(25, 50,1), glm::vec3(0), glm::vec3(0, 0, glm::radians(45.f)) },
  { glm::vec3(0, 0, -1), glm::vec3(25, 100,1), glm::vec3(0), glm::vec3(0, 0, glm::radians(-45.f)) }
};

void Reset() {
  player = { 165.f, 140.f, glm::vec3(-475, -225, -1), glm::vec3(15,15,1), glm::vec3(.3, .3, .75) };
  gamestate = GAMESTATE::PLAYING;
}

void Setup() {
  //prep
  float aspect = (float)default_window_width / default_window_height;
  float half_height = default_window_height / 2.f;  // also called ortho size
  float half_width = half_height * aspect;
  // setup a to draw a 2d world
  std::string vert_code = ReadFileToString("..\\openGLdemo\\GLSL_src\\vert_2DProjected.glsl");
  std::string frag_code = ReadFileToString("..\\openGLdemo\\GLSL_src\\frag_Textured.glsl");
  _2d_shader = std::make_unique<GLSLShader>(vert_code.c_str(), frag_code.c_str());
  QueryInputAttribs(_2d_shader->GetHandle());
  QueryUniforms(_2d_shader->GetHandle());
  _2d_shader->Use();
  _2d_shader->SetMat4("uProjectionMatrix", glm::ortho(-half_width, half_width, -half_height, half_height, ortho_near, ortho_far));
  glm::mat4 view_matrix(1);
  //view_matrix = glm::lookAt(
  //  glm::vec3(-100, 100, 200),  /* eye */
  //  glm::vec3(0, 0, -1),        /* target */
  //  glm::vec3(0, 1, 0));        /* up */
  _2d_shader->SetMat4("uViewMatrix", view_matrix);

  //upload texture
  std::string texture1 = "..\\resources\\demo_bars.png";
  int w, h, comp;
  unsigned char* image = stbi_load(texture1.c_str(), &w, &h, &comp, STBI_rgb_alpha);
  tex1_id = UploadTexture(w, h, image, true);
  stbi_image_free(image);
}

void Update(float dt) {
  // update gamestate
  static const float end_display_length = 1.2f;
  static float end_display_timer = 0.f;
  switch (gamestate) {
  case GAMESTATE::PLAYING:
    break;
  case GAMESTATE::WON:
  case GAMESTATE::LOST:
    end_display_timer += dt;
    if (end_display_timer > end_display_length) {
      end_display_timer = 0.f;
      Reset();
    }
    return;
  }

  // save details before update
  obj prev_player_body = player.body;

  // jump logic
  static float jump_timer = 0.f;
  if (player.jumping && !player.falling) {
    jump_timer += dt;
    if (jump_timer < .6f) {
      player.body.location.y += player.jump_power * dt;
    } else {
      player.falling = true;
      jump_timer = 0.f;
    }
  }

  // falling or gravity logic
  static float fall_timer = 0.f;
  static float last_fall_velocity = 0.f;
  if (player.falling) {
    static const float grav = 4.1f;
    fall_timer += dt;
    player.body.location.y -= grav * fall_timer;
    last_fall_velocity = grav * fall_timer;
  } else {
    fall_timer = 0.f;
  }



  // move left or right
  if (player.left) {
    player.body.location.x -= player.speed * dt;
  }

  if (player.right) {
    player.body.location.x += player.speed * dt;
  }


  // check for new collisions
  if (player.left || player.right || player.jumping || player.falling) {

    // check grounding
    bool on_something = false;
    if (!player.jumping && !player.falling && (player.left || player.right)) {
      obj feet = player.body;
      feet.location.y -= last_fall_velocity;
      for (const auto& o : objects) {
        if (CollidesWith(feet, o)) {
          on_something = true;
        }
      }
      if (!on_something && !player.jumping) {
        player.falling = true;
      }
    }

    // check colliding
    for (const auto& o : objects) {
      if (CollidesWith(player.body, o)) {
        //printf("collided\n");

        if (player.falling) {
          player.body.location.y = prev_player_body.location.y;
          player.falling = false;
          jump_timer = 0.f;
        }

        if (player.left || player.right) {
          player.body.location.x = prev_player_body.location.x;
        }
        break;
      }
    }
  }

  // check end game conditions
  if (CollidesWith(player.body, goal)) {
    gamestate = GAMESTATE::WON;
    //std::cout << "won!\n";
    return;
  }

  if (player.body.location.y < -400) {
    gamestate = GAMESTATE::LOST;
    //std::cout << "lost!\n";
  }
}

void PlayerControls(GLFWwindow* window) {
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    player.left = true;
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_RELEASE) {
    player.left = false;
  }

  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    player.right = true;
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_RELEASE) {
    player.right = false;
  }

  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
    if (!player.jumping) {
      player.jumping = true;
    }
  }
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
    if (player.jumping) {
      player.jumping = false;
      if (!player.falling)
        player.falling = true;
    }
  }
}

// draw game stuff
void Render(const std::vector<DrawStripDetails>& drawdetails) {

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex1_id);
  _2d_shader->Use();
  _2d_shader->SetInt("uTexture", 0);

  switch (gamestate) {
  case GAMESTATE::PLAYING:
    // draw base objects
    for (const auto& ob : objects) {
      _2d_shader->SetMat4("uModelMatrix", glm::scale(glm::translate(glm::mat4(1), ob.location), ob.scale));
      DrawStrip(drawdetails);
    }

    // draw player
    _2d_shader->SetMat4("uModelMatrix", glm::scale(glm::translate(glm::mat4(1), player.body.location), player.body.scale));
    DrawStrip(drawdetails);
    
    // draw goal
    _2d_shader->SetMat4("uModelMatrix", glm::scale(glm::translate(glm::mat4(1), goal.location), goal.scale));
    DrawStrip(drawdetails);
    break;

  case GAMESTATE::WON:
    // draw check mark (made out of 2 rectangles)
    for (const auto& wi : won_display) {
      _2d_shader->SetMat4("uModelMatrix", glm::scale(glm::rotate(glm::translate(glm::mat4(1), wi.location), wi.rotation.z, glm::vec3(0, 0, 1)), wi.scale));
      DrawStrip(drawdetails);
    }
    break;

  case GAMESTATE::LOST:
    // draw X (made out of 2 rectangles)
    for (const auto& lo : lost_display) {
      _2d_shader->SetMat4("uModelMatrix", glm::scale(glm::rotate(glm::translate(glm::mat4(1), lo.location), lo.rotation.z, glm::vec3(0, 0, 1)), lo.scale));
      DrawStrip(drawdetails);
    }
    break;

  }

}

} // end namespace demo2d
