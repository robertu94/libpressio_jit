#include "gtest/gtest.h"
#include <libpressio_jit.h>
#include <libpressio_ext/cpp/libpressio.h>

TEST(libpressio_jit, poorjit) {
    libpressio_jit_register_all();
    pressio lib;
    pressio_compressor comp = lib.get_compressor("poorjit");
    if(!comp) {
        GTEST_SKIP() << "poorjit not supported";
        return;
    }

    std::string source {R"cpp(
#include <boost/config.hpp>
#include "std_compat/memory.h"
#include "libpressio_ext/cpp/compressor.h"
#include "libpressio_ext/cpp/data.h"
#include "libpressio_ext/cpp/options.h"
#include "libpressio_ext/cpp/pressio.h"

class example_compressor_plugin : public libpressio_compressor_plugin {
public:
  struct pressio_options get_options_impl() const override
  {
    struct pressio_options options;
    set(options, "example:a", a);
    return options;
  }

  struct pressio_options get_configuration_impl() const override
  {
    struct pressio_options options;
    set(options, "pressio:thread_safe", pressio_thread_safety_multiple);
    set(options, "pressio:stability", "experimental");
    return options;
  }

  struct pressio_options get_documentation_impl() const override
  {
    struct pressio_options options;
    set(options, "pressio:description", R"(example jit compressor)");
    return options;
  }


  int set_options_impl(struct pressio_options const& options) override
  {
    get(options, "example:a", &a);
    return 0;
  }

  int compress_impl(const pressio_data* input,
                    struct pressio_data* output) override
  {
    *output = *input;
    return 0;
  }

  int decompress_impl(const pressio_data* input,
                      struct pressio_data* output) override
  {
    *output = *input;
    return 0;
  }

  int major_version() const override { return 0; }
  int minor_version() const override { return 0; }
  int patch_version() const override { return 1; }
  const char* version() const override { return "0.0.1"; }
  const char* prefix() const override { return "example"; }

  pressio_options get_metrics_results_impl() const override {
    return {};
  }

  std::shared_ptr<libpressio_compressor_plugin> clone() override
  {
    return compat::make_unique<example_compressor_plugin>(*this);
  }

  int32_t a = 3;
};

extern "C" BOOST_SYMBOL_EXPORT example_compressor_plugin plugin;
example_compressor_plugin plugin;
    )cpp"};
    
    comp->set_options({
        {"template:source", source},
        {"poorjit:pkgconfig", std::vector<std::string>{"libpressio_cxx"}}
    });

    pressio_data d(pressio_data::owning(pressio_float_dtype, {10, 10}));
    pressio_data compressed(pressio_data::empty(pressio_byte_dtype, d.dimensions()));
    pressio_data decompressed(pressio_data::owning(d.dtype(), d.dimensions()));
    float* f = static_cast<float*>(d.data());
    float* dec = static_cast<float*>(decompressed.data());
    size_t stride = d.get_dimension(0);
    for (int j = 0; j < d.get_dimension(1); ++j) {
    for (int i = 0; i < d.get_dimension(0); ++i) {
        f[j*stride+i] = i*j;
        dec[j*stride+i] = 0.0;
    }}

    EXPECT_NE(d, decompressed) << "expected that the data would not equal";
    ASSERT_EQ(comp->compress(&d, &compressed), 0) << "compression failed: "<< comp->error_msg();
    ASSERT_EQ(comp->decompress(&compressed, &decompressed), 0) << "decompression failed: "<< comp->error_msg();
    EXPECT_EQ(d, decompressed) << "expected that the data would be equal";
}
