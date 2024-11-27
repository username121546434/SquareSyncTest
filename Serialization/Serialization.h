#pragma once

#include "pch.h"
#include "square.capnp.h"

union InputType {
    bool unset;
    Input::KeyboardInput keypress;
    std::string disconnect_request;
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

::capnp::FlatArrayMessageReader get_message_reader(const std::vector<uint8_t> &data);
Square::Reader get_square(const std::vector<uint8_t> &data);
Squares::Reader get_squares(const std::vector<uint8_t> &data);
Input::Reader get_input(const std::vector<uint8_t> &data);
Event::Reader get_event(const std::vector<uint8_t> &data);
MessageToClient::Reader get_message_to_client(const std::vector<uint8_t> &data);

kj::ArrayPtr<const char> message_to_vector(::capnp::MallocMessageBuilder &msg);
kj::ArrayPtr<const char> create_square(::capnp::MallocMessageBuilder &msg, uint32_t x, uint32_t y, uint32_t h, uint32_t w);
kj::ArrayPtr<const char> create_squares(::capnp::MallocMessageBuilder &msg, const std::vector<Square::Reader> &squares);
kj::ArrayPtr<const char> create_input(::capnp::MallocMessageBuilder &msg, Input::Type::Which type, const InputType &value);
kj::ArrayPtr<const char> create_event(::capnp::MallocMessageBuilder &msg, Event::Type::Which type, const EventType &value);
kj::ArrayPtr<const char> create_squares_message_to_client(::capnp::MallocMessageBuilder &msg, const std::vector<Square::Builder> &value, double timestamp);
kj::ArrayPtr<const char> create_event_message_to_client(::capnp::MallocMessageBuilder &msg, const Event::Type::Which type, double timestamp, std::string err = "");

