#include <libpressio_jit_ext/cpp/generator.h>
#include <regex>

namespace libpressio_jit { namespace template_generator {
    class template_generator final : public pressio_generator_plugin {
        std::string generate_impl() override {
            std::string out = source;
            for (auto const& i : variables) {
                out = std::regex_replace(out, std::regex(i.first), i.second);
            }
            return out;
        }
        int set_options(pressio_options const& opts) override {
            get(opts, "template:source", &source);
            std::vector<std::string> keys, values;
            if((get(opts, "template:keys", &keys) == pressio_options_key_set) && 
               (get(opts, "template:values", &values) == pressio_options_key_set)) {
                if(keys.size() == values.size()) {
                    variables.clear();
                    for (size_t i = 0; i < keys.size(); ++i) {
                        variables[keys[i]] = values[i];
                    }
                } else {
                    return set_error(1, "keys and values must have the same size");
                }
            }
            return 0;
        }
        pressio_options get_options() const override {
            pressio_options opts;
            set(opts, "template:source", source);
            std::vector<std::string> keys, values;
            keys.reserve(variables.size());
            values.reserve(variables.size());
            for (auto const& i : variables) {
                keys.emplace_back(i.first);
                values.emplace_back(i.second);
            }
            set(opts, "template:keys", keys);
            set(opts, "template:values", values);
            return opts;
        }
        pressio_options get_documentation() const override {
            pressio_options opts;
            set(opts, "pressio:description", R"(generates source code from a template)");
            set(opts, "template:source", "source code to compile");
            set(opts, "template:keys", "keys to use in substitution");
            set(opts, "template:values", "keys to use in substitution");
            return opts;
        }
        pressio_options get_configuration_impl() const override {
            pressio_options opts;
            set(opts, "pressio:thread_safe", pressio_thread_safety_multiple);
            set(opts, "pressio:stability", "experimental");
            set(opts, "pressio:highlevel", std::vector<std::string>{"template:source", "template:keys", "template:values"});
            return opts;
        }

        const char* prefix() const final {
            return "template";
        };

        std::unique_ptr<pressio_generator_plugin> clone() override {
            return std::make_unique<template_generator>(*this);
        }

        std::string source;
        std::map<std::string, std::string> variables;
    };
}
    static pressio_register template_generator_plugin(generator_plugins(), "template", [](){
            return std::make_unique<template_generator::template_generator>();
    });
}
