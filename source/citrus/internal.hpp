#pragma once

#include "citrus/types.hpp"

namespace ctr {
    namespace app {
        bool init();
        void exit();
    }

    namespace err {
        bool init();
        void exit();

        void parse(u32 raw);
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

    namespace snd {
        bool init();
        void exit();
    }

    namespace soc {
        bool init();
        void exit();
    }
}