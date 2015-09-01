#pragma once

#include "citrus/err.hpp"
#include "citrus/types.hpp"

namespace ctr {
    namespace app {
        bool init();
        void exit();
    }

    namespace battery {
        bool init();
        void exit();
    }

    namespace err {
        bool init();
        void exit();

        void parse(ctr::err::Source source, u32 raw);
    }

    namespace fs {
        bool init();
        void exit();
    }

    namespace gpu {
        bool init();
        void exit();
    }

    namespace gput {
        bool init();
        void exit();
    }

    namespace hid {
        bool init();
        void exit();
    }

    namespace ir {
        bool init();
        void exit();
    }

    namespace news {
        bool init();
        void exit();
    }

    namespace nor {
        bool init();
        void exit();
    }

    namespace snd {
        bool init();
        void exit();
    }

    namespace soc {
        bool init();
        void exit();
    }

    namespace ui {
        bool init();
        void exit();
    }

    namespace utf {
        bool init();
        void exit();
    }

    namespace wifi {
        bool init();
        void exit();
    }
}