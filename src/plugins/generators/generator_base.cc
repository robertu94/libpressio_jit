#include <libpressio_jit_ext/cpp/generator.h>

namespace libpressio_jit {
pressio_registry<std::unique_ptr<pressio_generator_plugin>>& generator_plugins() {
    static pressio_registry<std::unique_ptr<pressio_generator_plugin>> plugins;
    return plugins;
}
}
