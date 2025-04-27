//
// Created by victor on 20/04/25.
//

#ifndef APP_H
#define APP_H

#include <chrono>

#include "GLFW/glfw3.h"
#include "imgui.h"
#include <thread>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "USB/UartDevice.h"
#include "HediosHandler.h"

struct APP_state {
    // GLFW related
    GLFWwindow* window = nullptr;

    // ImGui related
    ImGuiIO* io = nullptr;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // For fps purpose
    std::chrono::duration<double, std::milli> last_frame_time;
    double fps_target = 120.0;

    // Hedios
    HediosHandler hedios_handler;

    // Window managment
    bool is_device_manager_open = false;
    int sel_device_idx = -1;

    // Open new slot modal popup
    char new_slot_label[32] = {};
    HediosValueFormat new_slot_format = HDVF_UINT_8_HEX;
};

class App {
public:
    App();
    bool init();
    void cleanup();

    void main_loop();
    void draw_ui();


    void draw_hedios_manager();
    void draw_device_manager();

    void open_new_slot_sel_dv();

private:
    APP_state state;

};



#endif //APP_H
