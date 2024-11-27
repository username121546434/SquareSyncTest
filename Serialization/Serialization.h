#pragma once

#include "pch.h"
#include "square.capnp.h"
#include <new>

union InputType {
    bool unset;
    Input::KeyboardInput keypress;
    std::string disconnect_request;

    InputType() { new(&disconnect_request) std::string(); } // Placement new for std::string
    ~InputType() { disconnect_request.~basic_string(); } // Explicit destructor call
};

union EventType {
    bool unset;
    bool disconnect;
    bool connect;
    std::string error;
};

union MessageToClientType {
    std::vector<Square::Builder> *squares;
    Event::Builder *event;
};

inline ::capnp::FlatArrayMessageReader get_message_reader(const std::vector<uint8_t> &data);
Square::Reader get_square(::capnp::FlatArrayMessageReader &msg);
Squares::Reader get_squares(::capnp::FlatArrayMessageReader &msg);
Input::Reader get_input(::capnp::FlatArrayMessageReader &msg);
Event::Reader get_event(::capnp::FlatArrayMessageReader &msg);
MessageToClient::Reader get_message_to_client(::capnp::FlatArrayMessageReader &msg);

std::vector<uint8_t> message_to_vector(::capnp::MallocMessageBuilder &msg);
std::vector<uint8_t> create_square(::capnp::MallocMessageBuilder &msg, uint32_t x, uint32_t y, uint32_t h, uint32_t w);
std::vector<uint8_t> create_squares(::capnp::MallocMessageBuilder &msg, const std::vector<Square::Reader> &squares);
std::vector<uint8_t> create_input(::capnp::MallocMessageBuilder &msg, Input::Type::Which type, const InputType &value);
std::vector<uint8_t> create_event(::capnp::MallocMessageBuilder &msg, Event::Type::Which type, const EventType &value);
std::vector<uint8_t> create_squares_message_to_client(::capnp::MallocMessageBuilder &msg, const std::vector<Square::Builder> &value, double timestamp);
std::vector<uint8_t> create_event_message_to_client(::capnp::MallocMessageBuilder &msg, const Event::Type::Which type, double timestamp, std::string err = "");

