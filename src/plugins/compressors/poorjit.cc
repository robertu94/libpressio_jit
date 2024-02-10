#include "std_compat/memory.h"
#include "libpressio_ext/cpp/compressor.h"
#include "libpressio_ext/cpp/data.h"
#include "libpressio_ext/cpp/options.h"
#include "libpressio_ext/cpp/pressio.h"
#include <libpressio_jit_ext/cpp/generator.h>
#include <poorjit/poorjit.h>
#include <boost/process.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <string>

namespace libpressio { namespace poorjit_ns {
    using namespace libpressio_jit;
    namespace bp=boost::process;

    std::vector<std::string> pkgconfig(std::string  const&pkg) {
        std::vector<std::string> s;
        std::vector<std::string> args {
            bp::search_path("pkg-config").string(),
            "--cflags",
            "--libs",
            pkg
        };
        bp::ipstream os;
        bp::child c(args, bp::std_out > os);
        std::string link_line;
        std::getline(os, link_line);
        boost::algorithm::split_regex(s, link_line, boost::regex("\\s+"));
        return s;
    }

class poorjit_compressor_plugin : public libpressio_compressor_plugin {
public:
  struct pressio_options get_options_impl() const override
  {
    struct pressio_options options;
    set_meta(options, "poorjit:generator", gen_id, gen_impl);
    set(options, "poorjit:extra_args", extra_args);
    set(options, "poorjit:pkgconfig", pkgconfig_args);
    if(comp_impl) {
        options.copy_from(comp_impl->get_options());
    }
    return options;
  }

  struct pressio_options get_configuration_impl() const override
  {
    struct pressio_options options;
    set(options, "pressio:thread_safe", pressio_thread_safety_multiple);
    set(options, "pressio:stability", "experimental");
    std::vector<pressio_configurable const*> invalidation_children {&*gen_impl}; 
    if(comp_impl) {
        options.copy_from(comp_impl->get_configuration());
        invalidation_children.emplace_back(&*comp_impl);
    }
    
    std::vector<std::string> invalidations {"poorjit:extra_args", "poorjit:pkgconfig"}; 
    set(options, "predictors:error_dependent", get_accumulate_configuration("predictors:error_dependent", invalidation_children, invalidations));
    set(options, "predictors:error_agnostic", get_accumulate_configuration("predictors:error_agnostic", invalidation_children, invalidations));
    set(options, "predictors:runtime", get_accumulate_configuration("predictors:runtime", invalidation_children, invalidations));

    return options;
  }

  struct pressio_options get_documentation_impl() const override
  {
    struct pressio_options options;
    set(options, "pressio:description", R"(use a poorman's JIT library to run a compressor)");
    if(comp_impl) {
        options.copy_from(comp_impl->get_documentation());
    }
    return options;
  }


  int set_options_impl(struct pressio_options const& options) override
  {
    get_meta(options, "poorjit:generator", generator_plugins(), gen_id, gen_impl);
    get(options, "poorjit:extra_args", &extra_args);
    get(options, "poorjit:pkgconfig", &pkgconfig_args);
    std::vector<std::string> jit_args;
    jit_args.insert(jit_args.end(), extra_args.begin(), extra_args.end());
    for (auto pkg_id : pkgconfig_args) {
        auto pkg = pkgconfig(pkg_id);
        jit_args.insert(jit_args.end(), pkg.begin(), pkg.end());
    }
    std::string source = gen_impl->generate();
    if(!source.empty()) {
        try {
            comp_impl = comp_mgr.jit(source, jit_args).get();
        } catch(std::exception const& ex) {
            return set_error(1, ex.what());
        }
    }
    if(comp_impl) {
        return comp_impl->set_options(options);
    }
    return 0;
  }

  int compress_impl(const pressio_data* input,
                    struct pressio_data* output) override
  {
      if(comp_impl) return comp_impl->compress(input, output);
      else return set_error(1, "failed to compile the compressor");
  }

  int decompress_impl(const pressio_data* input,
                      struct pressio_data* output) override
  {
      if(comp_impl) return comp_impl->decompress(input, output);
      else return set_error(1, "failed to compile the compressor");
  }

  int compress_many_impl(compat::span<const pressio_data* const> const& inputs, compat::span<pressio_data*> & outputs) override {
      if(comp_impl) return comp_impl->compress_many(inputs.begin(), inputs.end(), outputs.begin(), outputs.end());
      else return set_error(1, "failed to compile the compressor");
  }

  int decompress_many_impl(compat::span<const pressio_data* const> const& inputs, compat::span<pressio_data* >& outputs) override {
      if(comp_impl) return comp_impl->decompress_many(inputs.begin(), inputs.end(), outputs.begin(), outputs.end());
      else return set_error(1, "failed to compile the compressor");
  }

  int major_version() const override { return 0; }
  int minor_version() const override { return 0; }
  int patch_version() const override { return 1; }
  const char* version() const override { return "0.0.1"; }
  const char* prefix() const override { return "poorjit"; }

  pressio_options get_metrics_results_impl() const override {
    if(comp_impl) return comp_impl->get_metrics_results();
    else return {};
  }

  void set_name_impl(std::string const& name) override {
      gen_impl->set_name(name + "/generator");
      if(comp_impl){
          comp_impl->set_name(name + "/compressor");
      }
  }
  std::vector<std::string> children_impl() const final {
      std::vector<std::string> v{gen_impl->get_name()};
      if(comp_impl) {
          v.emplace_back(comp_impl->get_name());
      }
      return v;
  }


  std::shared_ptr<libpressio_compressor_plugin> clone() override
  {
    //TODO implement a clone-able jit object container to use instead of jitlib
    return compat::make_unique<poorjit_compressor_plugin>(*this);
  }

  poorjit::jitmgr<libpressio_compressor_plugin> comp_mgr;
  poorjit::jitlib<libpressio_compressor_plugin> comp_impl;
  std::string gen_id = "template";
  pressio_generator gen_impl = generator_plugins().build(gen_id);
  std::vector<std::string> extra_args;
  std::vector<std::string> pkgconfig_args;
};

static pressio_register compressor_many_fields_plugin(compressor_plugins(), "poorjit", []() {
  return compat::make_unique<poorjit_compressor_plugin>();
});

} }
