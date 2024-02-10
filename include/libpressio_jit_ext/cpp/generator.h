#include <libpressio_ext/cpp/configurable.h>
#include <libpressio_ext/cpp/versionable.h>
#include <libpressio_ext/cpp/pressio.h>
#include <sstream>

namespace libpressio_jit
{
    struct pressio_generator_plugin : public pressio_configurable, public pressio_versionable {
        std::string generate() {
            return generate_impl();
        }

        pressio_options get_configuration() const final {
            return get_configuration_impl();
        }

        virtual const char* version() const {
            static std::string version_str = [this]() {
                std::stringstream ss;
                ss << this->major_version() << '.'
                    << this->minor_version() << '.'
                    << this->patch_version();
                return ss.str();
            }();
            return version_str.c_str();
        }
        std::string type() const final {
            return "generator";
        }

        virtual std::unique_ptr<pressio_generator_plugin> clone()=0;
    protected:
        virtual pressio_options get_configuration_impl() const=0;
        virtual std::string generate_impl()=0; 
    };

    struct pressio_generator {
        /**
         * \param[in] impl a newly constructed compressor plugin
         */
        pressio_generator(std::unique_ptr<pressio_generator_plugin>&& impl): plugin(std::forward<std::unique_ptr<pressio_generator_plugin>>(impl)) {}
        /**
         * defaults constructs a compressor with a nullptr
         */
        pressio_generator()=default;
        /**
         * copy constructs a compressor from another pointer by cloning
         */
        pressio_generator(pressio_generator const& compressor):
            plugin(compressor->clone()) {}
        /**
         * copy assigns a compressor from another pointer by cloning
         */
        pressio_generator& operator=(pressio_generator const& compressor) {
            if(&compressor == this) return *this;
            plugin = compressor->clone();
            return *this;
        }
        /**
         * move assigns a compressor from another pointer
         */
        pressio_generator& operator=(pressio_generator&& compressor) noexcept=default;
        /**
         * move constructs a compressor from another pointer
         */
        pressio_generator(pressio_generator&& compressor)=default;

        /** \returns true if the plugin is set */
        operator bool() const {
            return bool(plugin);
        }

        /** make pressio_generator_plugin behave like a unique_ptr */
        pressio_generator_plugin& operator*() const noexcept {
            return *plugin;
        }

        /** make pressio_generator_plugin behave like a unique_ptr */
        pressio_generator_plugin* operator->() const noexcept {
            return plugin.get();
        }

        /**
         * pointer to the implementation
         */
        std::unique_ptr<pressio_generator_plugin> plugin;
    };


    pressio_registry<std::unique_ptr<pressio_generator_plugin>>& generator_plugins();

} /* libpressio_jit */ 
