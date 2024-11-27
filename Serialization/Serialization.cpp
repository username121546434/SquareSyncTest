// Serialization.cpp : Defines the functions for the static library.
//
#include "pch.h"
#include "Serialization.h"

::capnp::FlatArrayMessageReader get_message_reader(const std::vector<uint8_t> &data) {
    kj::ArrayPtr<const capnp::word> words(
        reinterpret_cast<const capnp::word *>(data.data()),
        data.size() / sizeof(capnp::word));
    ::capnp::FlatArrayMessageReader reader(words);
    return reader;
}

Square::Reader get_square(const std::vector<uint8_t> &data) {
    auto reader = get_message_reader(data);
    auto square = reader.getRoot<Square>();
    std::cout << square.getX() << " " << square.getY() << " "
        << square.getWidth() << " " << square.getHeight() << std::endl;
    return square;
}

Squares::Reader get_squares(const std::vector<uint8_t> &data) {
    auto reader = get_message_reader(data);
    auto squares = reader.getRoot<Squares>();
    return squares;
}

Input::Reader get_input(const std::vector<uint8_t> &data) {
    auto reader = get_message_reader(data);
    auto input = reader.getRoot<Input>();
    return input;
}

Event::Reader get_event(const std::vector<uint8_t> &data) {
    auto reader = get_message_reader(data);
    auto event = reader.getRoot<Event>();
    return event;
}

MessageToClient::Reader get_message_to_client(const std::vector<uint8_t> &data) {
    auto reader = get_message_reader(data);
    auto message_to_client = reader.getRoot<MessageToClient>();
    return message_to_client;
}

kj::ArrayPtr<const char> message_to_vector(::capnp::MallocMessageBuilder &msg) {
    return msg.getSegmentsForOutput().asChars();
}

kj::ArrayPtr<const char> create_square(::capnp::MallocMessageBuilder &msg, uint32_t x, uint32_t y, uint32_t h, uint32_t w) {
    Square::Builder new_sq = msg.initRoot<Square>();
    new_sq.setX(x);
    new_sq.setY(y);
    new_sq.setHeight(h);
    new_sq.setWidth(w);

    return message_to_vector(msg);
}

kj::ArrayPtr<const char> create_squares(::capnp::MallocMessageBuilder &msg, const std::vector<Square::Reader> &squares) {
    Squares::Builder sq_obj = msg.initRoot<Squares>();
    ::capnp::List<Square>::Builder people = sq_obj.initPeople(squares.size());
    
    for (unsigned s_idx {0}; s_idx < squares.size(); s_idx++) {
        Square::Builder sq = people[s_idx];
        Square::Reader sqr = squares[s_idx];
        sq.setX(sqr.getX());
        sq.setY(sqr.getY());
        sq.setHeight(sqr.getHeight());
        sq.setWidth(sqr.getWidth());
    }

    return message_to_vector(msg);
}

kj::ArrayPtr<const char> create_input(::capnp::MallocMessageBuilder &msg, Input::Type::Which type, const InputType &value) {
    Input::Builder input = msg.initRoot<Input>();
    auto val {input.getType()};

    switch (type) {
        case Input::Type::Which::UNSET:
            val.setUnset();
            break;
        case Input::Type::Which::KEYPRESS:
            val.setKeypress(value.keypress);
            break;
        case Input::Type::Which::DISCONNECT_REQUEST:
            val.setDisconnectRequest(value.disconnect_request);
    }

    return message_to_vector(msg);
}

kj::ArrayPtr<const char> create_event(::capnp::MallocMessageBuilder &msg, Event::Type::Which type, const EventType &value) {
    Event::Builder event = msg.initRoot<Event>();
    auto val {event.getType()};
    switch (type) {
        case Event::Type::UNSET:
            val.setUnset();
            break;
        case Event::Type::DISCONNECT:
            val.setDisconnect();
            break;
        case Event::Type::CONNECT:
            val.setConnect();
            break;
        case Event::Type::ERROR:
            val.setError(value.error);
            break;
    }
    return message_to_vector(msg);
}

kj::ArrayPtr<const char> create_squares_message_to_client(::capnp::MallocMessageBuilder &msg, const std::vector<Square::Builder> &sqs, double timestamp) {
    MessageToClient::Builder builder = msg.initRoot<MessageToClient>();
    auto val {builder.getData()};

    auto squares = val.initSquares();
    squares.initPeople(sqs.size());
    auto people = squares.getPeople();
    for (unsigned s_idx {0}; s_idx < sqs.size(); s_idx++) {
        auto sq {sqs[s_idx]};
        auto builder {people[s_idx]};
        
        builder.setX(sq.getX());
        builder.setY(sq.getY());
        builder.setHeight(sq.getHeight());
        builder.setWidth(sq.getWidth());
    }

    builder.setTimestamp(timestamp);
    return message_to_vector(msg);
}

kj::ArrayPtr<const char> create_event_message_to_client(::capnp::MallocMessageBuilder &msg, const Event::Type::Which type, double timestamp, std::string error) {
    MessageToClient::Builder builder = msg.initRoot<MessageToClient>();
    auto val {builder.getData()};

    auto event {val.initEvent()};
    auto typ {event.initType()};
    
    switch (type) {
        case Event::Type::Which::CONNECT:
            typ.setConnect();
            break;
        case Event::Type::Which::DISCONNECT:
            typ.setDisconnect();
            break;
        case Event::Type::Which::ERROR: {
            assert(error != "");
            auto err = typ.initError(error.size());
            for (size_t i {0}; i < error.size(); i++)
                err[i] = error[i];
            break;
        }
    }

    builder.setTimestamp(timestamp);
    return message_to_vector(msg);
}
