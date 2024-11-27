@0xc6a50c260f47dde1;

struct Input {
    type :union {
        unset @0: Void;
        keypress @1: KeyboardInput;
        disconnectRequest @2: Text; 
    }

    enum KeyboardInput {
        right @0;
        left @1;
        up @2;
        down @3;
    }
}

struct Event {
    type :union {
        unset @0: Void;
        disconnect @1: Void;
        connect @2: Void;
        error @3: Text;
    }
}

struct Square {
    x @0: UInt32;
    y @1: UInt32;
    width @2: UInt32;
    height @3: UInt32;
}

struct Squares {
    people @0: List(Square);
}

struct MessageToClient {
    timestamp @0: Float64;

    data :union {
        squares @1: Squares;
        event @2: Event;
    }
}
