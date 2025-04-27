//
// Created by victor on 20/04/25.
//

#include "App.h"

#include <algorithm>
#include <iostream>
#include <bits/ostream.tcc>

#include "imgui_internal.h"
#include "GLFW/glfw3.h"
#include "USB/UsbUtils.h"

App::App() {
}

const char *new_slot_format_names[] = {
    "8 bits hex",
    "16 bits hex",
    "32 bits hex",
    "8 bits dec",
    "16 bits dec",
    "32 bits dec",
    "8 bits bin",
    "16 bits bin",
    "32 bits bin",
    "String"};

bool App::init() {
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Create a window with graphics context
    state.window = glfwCreateWindow(1280, 720, "Hedios", nullptr, nullptr);
    if (state.window == nullptr) {
        std::cout << "Error creating window" << std::endl;
        return false;
    }
    glfwMakeContextCurrent(state.window);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();

    ImGui::CreateContext();
    state.io = &ImGui::GetIO();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(state.window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    glfwSwapInterval(0); // Disable V-Sync

    return true;
}

void App::main_loop() {

    while (!glfwWindowShouldClose(state.window)) {

        state.hedios_handler.process_all_devices();

        // Compute sleep duration based on previous frame infos
        auto per_frame_target = std::chrono::duration<double, std::milli>(1000 / state.fps_target);
        if (state.last_frame_time < per_frame_target)
            std::this_thread::sleep_for(per_frame_target - state.last_frame_time);

        auto frame_start = std::chrono::steady_clock::now();

        glfwPollEvents();
        draw_ui();

        auto frame_end = std::chrono::steady_clock::now();
        auto elapsed = frame_end - frame_start;
        state.last_frame_time = elapsed;
    }
}

void App::draw_ui() {

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::DockSpaceOverViewport();


    draw_device_manager();
    draw_hedios_manager();



    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(state.window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(state.clear_color.x * state.clear_color.w, state.clear_color.y * state.clear_color.w,
        state.clear_color.z * state.clear_color.w, state.clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(state.window);
}

void App::cleanup() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(state.window);
    glfwTerminate();
}

void App::draw_device_manager() {
    if (ImGui::Begin("Device manager", &state.is_device_manager_open)) {
        if (!state.hedios_handler.is_valid_device_idx(state.sel_device_idx)) {
            ImGui::Text("Invalid device index");
        }
        else {
            auto& device = state.hedios_handler.state.device_pool[state.sel_device_idx];

            ImGui::Text("Device ID: %d", device.device_idx);
            ImGui::Text("Description: %s", device.uart_device.info.Description);
            ImGui::Text("%s", device.uart_device.uart_info.is_connected ? "Connected" : "Disconnected");

            ImGui::SeparatorText("Values");

            if (ImGui::BeginTable("values", 2)) {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Value");
                ImGui::TableHeadersRow();

                char buffer[64];

                for (int i = 0; i < device.used_slot; i ++) {
                    ImGui::PushID(i);
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%s", device.slot_label[i]);
                    ImGui::TableNextColumn();

                    fmt_value(buffer, 64, device.slot_format[i], device.slot_value[i]);
                    ImGui::Text(buffer);
                    ImGui::SameLine();
                    if (ImGui::Button("Update")) {
                        state.hedios_handler.ask_specific_slot_update(state.sel_device_idx, i);
                    }
                    ImGui::PopID();
                }

                ImGui::EndTable();
            }


            if (ImGui::Button("Add values")) {

                ImGui::OpenPopup("Add values");
            }

            ImGui::SameLine();

            if (ImGui::Button("Update values")) {
                state.hedios_handler.ask_all_slot_update(state.sel_device_idx);
            }

            ImGui::SeparatorText("HediosActions");
            ImGui::Text("Var action count : %d", device.var_action_count);
            ImGui::Text("Varless action count : %d", device.varless_action_count);

            if (ImGui::Button("Detect action count")) {
                state.hedios_handler.detect_action_count(state.sel_device_idx);
            }

            ImGui::SeparatorText("Device control");

            if (ImGui::Button("Process Queue")) {
                state.hedios_handler.process_input_queue(state.sel_device_idx);
            }

            if (ImGui::Button("Detect slot count")) {
                state.hedios_handler.detect_slot_count(state.sel_device_idx);
            }

            ImGui::SameLine();




            if (ImGui::Button("Send ping")) {
                state.hedios_handler.send_ping(state.sel_device_idx);
            }

            ImGui::SameLine();

            if (ImGui::Button("Reset device")) {
                state.hedios_handler.send_reset(state.sel_device_idx);
            }


            if (ImGui::BeginPopupModal("Add values", NULL, ImGuiWindowFlags_AlwaysAutoResize)){
                ImGui::Text("Open a new slot");
                ImGui::InputText("Label", state.new_slot_label, 64);
                const char* combo_preview_value = new_slot_format_names[state.new_slot_format];

                if (ImGui::BeginCombo("combo 1", combo_preview_value))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(new_slot_format_names); n++)
                    {
                        const bool is_selected = (state.new_slot_format == n);
                        if (ImGui::Selectable(new_slot_format_names[n], is_selected))
                            state.new_slot_format = static_cast<HediosValueFormat>(n);

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                ImGui::Separator();
                if (ImGui::Button("close")) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Add")) {
                    ImGui::CloseCurrentPopup();
                    open_new_slot_sel_dv();
                }
                ImGui::EndPopup();
            }

        }

        ImGui::End();

    }
}

void App::open_new_slot_sel_dv() {
    auto& device = state.hedios_handler.state.device_pool[state.sel_device_idx];
    std::copy(state.new_slot_label, state.new_slot_label + 64, device.slot_label[device.used_slot]);
    device.slot_format[device.used_slot] = state.new_slot_format;
    device.used_slot ++;
}

void App::draw_hedios_manager() {
    ImGui::Begin("Hedios manager");

    if (ImGui::Button("Find hedios devices")) {
        state.hedios_handler.find_all_hedios_devices();
    }

    ImGui::SeparatorText("Hedios devices");
    int i = 0;
    for (auto& device : state.hedios_handler.state.device_pool) {
        ImGui::PushID(++i);

        ImGui::Text("Device ID: %d", device.device_idx);
        ImGui::Text("Description: %s", device.uart_device.info.Description);
        ImGui::Text("%s", device.uart_device.uart_info.is_connected ? "Connected" : "Disconnected");
        if (device.uart_device.uart_info.is_connected) {
            device.uart_device.update_rxQueue();
            ImGui::Text("RX Queue: %d", device.uart_device.rxQueue());
        }

        ImGui::BeginDisabled(device.uart_device.is_open);
        if (ImGui::Button("Connect")) {
            state.hedios_handler.connect_device(device.device_idx);
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Open in device manager")) {
            state.sel_device_idx = device.device_idx;
            state.is_device_manager_open = true;
        }
        ImGui::PopID();
    }

    ImGui::End();

}





