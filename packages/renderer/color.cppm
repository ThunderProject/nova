module;
#include <algorithm>
#include <cmath>
#include <cstdint>
export module nova.render.color;

import nova.math.vector;

export namespace nova::graphics {
    struct hsv_color {
        float hue{};
        float saturation{};
        float value{};
    };

    class color_converter final {
    public:
        [[nodiscard]] static constexpr std::uint8_t float_to_byte(const float value) noexcept {
            if(!(value > 0.0f)) [[unlikely]] {
                return 0;
            }
            if(value >= 1.0f) [[unlikely]] {
                return 255;
            }
            return static_cast<std::uint8_t>(value * 255.0f + 0.5f);
        }

        [[nodiscard]] static constexpr float byte_to_float(const std::uint8_t value) noexcept {
            return static_cast<float>(value) * (1.0f / 255.0f);
        }

        [[nodiscard]] static constexpr float srgb_byte_to_linear(const std::uint8_t value) noexcept {
            return srgb_to_linear_lut[value];
        }
    private:
        static constexpr std::array<float, 256> srgb_to_linear_lut{
            0.0F, 0.000303526984F, 0.000607053967F, 0.000910580951F, 0.00121410793F, 0.00151763492F, 0.0018211619F, 
            0.00212468888F, 0.00242821587F, 0.00273174285F, 0.00303526984F, 0.00334653576F, 0.00367650732F, 
            0.00402471702F, 0.00439144204F, 0.00477695348F, 0.0051815167F, 0.00560539162F, 0.00604883302F, 
            0.00651209079F, 0.00699541019F, 0.00749903204F, 0.00802319299F, 0.00856812562F, 0.0091340587F, 
            0.00972121732F, 0.010329823F, 0.010960094F, 0.0116122452F, 0.0122864884F, 0.0129830323F, 0.013702083F,
            0.0144438436F, 0.0152085144F, 0.0159962934F, 0.0168073758F, 0.0176419545F, 0.0185002201F, 0.019382361F, 
            0.0202885631F, 0.0212190104F, 0.0221738848F, 0.0231533662F, 0.0241576324F, 0.0251868596F, 0.0262412219F, 
            0.0273208916F, 0.0284260395F, 0.0295568344F, 0.0307134437F, 0.0318960331F, 0.0331047666F, 0.0343398068F, 
            0.0356013149F, 0.0368894504F, 0.0382043716F, 0.0395462353F, 0.0409151969F, 0.0423114106F, 0.0437350293F,
            0.0451862044F, 0.0466650863F, 0.0481718242F, 0.049706566F, 0.0512694584F, 0.052860647F, 0.0544802764F, 
            0.05612849F, 0.0578054302F, 0.0595112382F, 0.0612460542F, 0.0630100177F, 0.0648032667F, 0.0666259386F, 
            0.0684781698F, 0.0703600957F, 0.0722718507F, 0.0742135684F, 0.0761853815F, 0.0781874218F, 0.0802198203F,
            0.0822827071F, 0.0843762115F, 0.086500462F, 0.0886555863F, 0.0908417112F, 0.0930589628F, 0.0953074666F, 
            0.0975873471F, 0.0998987282F, 0.102241733F, 0.104616484F, 0.107023103F, 0.109461711F, 0.111932428F, 
            0.114435374F, 0.116970668F, 0.119538428F, 0.122138772F, 0.124771818F, 0.12743768F, 0.130136477F, 
            0.132868322F, 0.13563333F, 0.138431615F, 0.141263291F, 0.144128471F, 0.147027266F, 0.14995979F, 
            0.152926152F, 0.155926464F, 0.158960835F, 0.162029376F, 0.165132195F, 0.1682694F, 0.171441101F,
            0.174647404F, 0.177888416F, 0.181164244F, 0.184474995F, 0.187820772F, 0.191201683F, 0.19461783F, 
            0.19806932F, 0.201556254F, 0.205078736F, 0.20863687F, 0.212230757F, 0.2158605F, 0.2195262F, 0.223227957F,
            0.226965874F, 0.230740049F, 0.234550582F, 0.238397574F, 0.242281122F, 0.246201327F, 0.250158285F,
            0.254152094F, 0.258182853F, 0.262250658F, 0.266355605F, 0.270497791F, 0.274677312F, 0.278894263F, 
            0.28314874F, 0.287440838F, 0.29177065F, 0.296138271F, 0.300543794F, 0.304987314F, 0.309468923F, 
            0.313988713F, 0.318546778F, 0.323143209F, 0.327778098F, 0.332451536F, 0.337163615F, 0.341914425F,
            0.346704056F, 0.3515326F, 0.356400144F, 0.36130678F, 0.366252596F, 0.37123768F, 0.376262123F, 0.381326011F,
            0.386429434F, 0.391572478F, 0.396755231F, 0.40197778F, 0.407240212F, 0.412542613F, 0.417885071F,
            0.42326767F, 0.428690497F, 0.434153636F, 0.439657174F, 0.445201195F, 0.450785783F, 0.456411023F, 0.462077F,
            0.467783796F, 0.473531496F, 0.479320183F, 0.48514994F, 0.49102085F, 0.496932995F, 0.502886458F,
            0.508881321F, 0.514917665F, 0.520995573F, 0.527115126F, 0.533276404F, 0.539479489F, 0.545724461F, 
            0.552011402F, 0.55834039F, 0.564711506F, 0.571124829F, 0.57758044F, 0.584078418F, 0.590618841F,
            0.597201788F, 0.603827339F, 0.610495571F, 0.617206562F, 0.623960392F, 0.630757136F, 0.637596874F,
            0.644479682F, 0.651405637F, 0.658374817F, 0.665387298F, 0.672443157F, 0.67954247F, 0.686685312F, 
            0.693871761F, 0.701101892F, 0.70837578F, 0.715693501F, 0.723055129F, 0.73046074F, 0.737910409F, 
            0.74540421F, 0.752942217F, 0.760524505F, 0.768151147F, 0.775822218F, 0.783537792F, 0.79129794F,
            0.799102738F, 0.806952258F, 0.814846572F, 0.822785754F, 0.830769877F, 0.838799012F, 0.846873232F, 
            0.854992608F, 0.863157213F, 0.871367119F, 0.879622397F, 0.887923118F, 0.896269353F, 0.904661174F, 
            0.913098652F, 0.921581856F, 0.930110858F, 0.938685728F, 0.947306537F, 0.955973353F, 0.964686248F, 
            0.97344529F, 0.98225055F, 0.991102097F, 1.0F
        };
    };

    class color;
    class color_base_u8 {
    public:
        constexpr color_base_u8() noexcept = default;
        constexpr color_base_u8(
            const std::uint8_t red,
            const std::uint8_t green,
            const std::uint8_t blue,
            const std::uint8_t alpha
        ) noexcept
            :
            m_color{red, green, blue, alpha}
        {}

        [[nodiscard]] constexpr std::uint8_t red() const noexcept {
            return m_color[0];
        }

        [[nodiscard]] constexpr std::uint8_t green() const noexcept {
            return m_color[1];
        }

        [[nodiscard]] constexpr std::uint8_t blue() const noexcept {
            return m_color[2];
        }

        [[nodiscard]] constexpr std::uint8_t alpha() const noexcept {
            return m_color[3];
        }

        constexpr void set_red(const std::uint8_t value) noexcept {
            m_color[0] = value;
        }

        constexpr void set_green(const std::uint8_t value) noexcept {
            m_color[1] = value;
        }

        constexpr void set_blue(const std::uint8_t value) noexcept {
            m_color[2] = value;
        }

        constexpr void set_alpha(const std::uint8_t value) noexcept {
            m_color[3] = value;
        }

        [[nodiscard]] constexpr const std::array<std::uint8_t, 4>& data() const noexcept {
            return m_color;
        }

        [[nodiscard]] constexpr std::array<std::uint8_t, 4>& data() noexcept {
            return m_color;
        }

        [[nodiscard]] constexpr std::uint32_t to_rgba() const noexcept {
            return (static_cast<std::uint32_t>(red()) << 24U)
                | (static_cast<std::uint32_t>(green()) << 16U)
                | (static_cast<std::uint32_t>(blue()) << 8U)
                | static_cast<std::uint32_t>(alpha());
        }

        [[nodiscard]] constexpr std::uint32_t to_abgr() const noexcept {
            return (static_cast<std::uint32_t>(alpha()) << 24U)
                | (static_cast<std::uint32_t>(blue()) << 16U)
                | (static_cast<std::uint32_t>(green()) << 8U)
                | static_cast<std::uint32_t>(red());
        }
    private:
        std::array<std::uint8_t, 4> m_color{};
    };

    class color_linear_u8 final : public color_base_u8 {
    public:
        constexpr color_linear_u8() noexcept = default;
        constexpr color_linear_u8(
            const std::uint8_t red,
            const std::uint8_t green,
            const std::uint8_t blue,
            const std::uint8_t alpha = 255
        )
            : color_base_u8(red, green, blue, alpha)
        {}

        constexpr explicit color_linear_u8(const color& value) noexcept;

        [[nodiscard]] constexpr color to_linear_float() const noexcept;
    };

    class color_gamma_u8 final : public color_base_u8 {
    public:
        constexpr color_gamma_u8() noexcept = default;

        constexpr color_gamma_u8(
            const std::uint8_t red,
            const std::uint8_t green,
            const std::uint8_t blue,
            const std::uint8_t alpha = 255
        ) noexcept : color_base_u8{red, green, blue, alpha}
        {}

        explicit color_gamma_u8(const color& value) noexcept;

        [[nodiscard]] constexpr color to_linear_float() const noexcept;
    };

    class color final {
    public:
        constexpr color() noexcept = default;

        constexpr color(
            const float red,
            const float green,
            const float blue,
            const float alpha = 1.0F
        ) noexcept
            :
            m_color{red, green, blue, alpha}
        {}

        constexpr explicit color(const color_gamma_u8& gamma) noexcept
            : 
            color{gamma.to_linear_float()}
        {}

        explicit color(const hsv_color& hsv) noexcept
            :
            m_color{from_hsv(hsv)}
        {}

        [[nodiscard]] constexpr float red() const noexcept {
            return m_color.x();
        }

        [[nodiscard]] constexpr float green() const noexcept {
            return m_color.y();
        }

        [[nodiscard]] constexpr float blue() const noexcept {
            return m_color.z();
        }

        [[nodiscard]] constexpr float alpha() const noexcept {
            return m_color.w();
        }

        constexpr void set_red(const float value) noexcept {
            m_color.set_x(value);
        }

        constexpr void set_green(const float value) noexcept {
            m_color.set_y(value);
        }

        constexpr void set_blue(const float value) noexcept {
            m_color.set_z(value);
        }

        constexpr void set_alpha(const float value) noexcept {
            m_color.set_w(value);
        }

        [[nodiscard]] constexpr const math::vec4& data() const noexcept {
            return m_color;
        }

        [[nodiscard]] constexpr math::vec4& data() noexcept {
            return m_color;
        }

        [[nodiscard]] constexpr bool is_normalized() const noexcept {
            return red() >= 0.0F && red() <= 1.0F &&
                   green() >= 0.0F && green() <= 1.0F &&
                   blue() >= 0.0F && blue() <= 1.0F &&
                   alpha() >= 0.0F && alpha() <= 1.0F;
        }

        [[nodiscard]] constexpr std::uint32_t to_rgba8() const noexcept {
            return color_linear_u8{*this}.to_rgba();
        }

        [[nodiscard]] constexpr std::uint32_t to_abgr8() const noexcept {
            return color_linear_u8{*this}.to_abgr();
        }

        // RGBA byte order in memory on little-endian targets
        [[nodiscard]] constexpr std::uint32_t packed_rgba8() const noexcept {
           return to_abgr8();
        }


        [[nodiscard]] hsv_color to_hsv() const noexcept {
            const float r = linear_to_gamma(red());
            const float g = linear_to_gamma(green());
            const float b = linear_to_gamma(blue());

            const float maximum = std::max({r, g, b});
            const float minimum = std::min({r, g, b});
            const float delta = maximum - minimum;

            hsv_color result {
                .hue = 0.0F,
                .saturation = maximum > 0.0F ? delta / maximum : 0.0F,
                .value = maximum
            };

            if(!(delta > 0.0F)) {
                return result;
            }

            if(maximum == r) {
                result.hue = 60.0F * std::fmod((g - b) / delta, 6.0F);
            }
            else if(maximum == g) {
                result.hue = 60.0F * (((b - r) / delta) + 2.0F);
            }
            else {
                result.hue = 60.0F * (((r - g) / delta) + 4.0F);
            }

            if(result.hue < 0.0F) {
                result.hue += 360.0F;
            }

            return result;
        }

        [[nodiscard]] static float gamma_to_linear(const float gamma) noexcept {
            return gamma <= 0.04045F
                ? gamma * (1.0F / 12.92F)
                : std::pow((gamma + 0.055F) * (1.0F / 1.055F), 2.4F);
        }

        [[nodiscard]] static math::vec3 gamma_to_linear(const math::vec3& gamma) noexcept {
            return {
                gamma_to_linear(gamma.x()),
                gamma_to_linear(gamma.y()),
                gamma_to_linear(gamma.z())
            };
        }

        [[nodiscard]] static float linear_to_gamma(const float linear) noexcept {
            return linear <= 0.0031308F
                ? 12.92F * linear
                : 1.055F * std::pow(linear, 1.0F / 2.4F) - 0.055F;
        }

        [[nodiscard]] static math::vec3 linear_to_gamma(const math::vec3& linear) noexcept {
            return {
                linear_to_gamma(linear.x()),
                linear_to_gamma(linear.y()),
                linear_to_gamma(linear.z())
            };
        }
    private:
        [[nodiscard]] static float saturate(const float value) noexcept {
            if(!(value > 0.0f)) {
                return 0.0f;
            }
            return value < 1.0f ? value : 1.0f;
        }

        [[nodiscard]] static math::vec4 from_hsv(const hsv_color& hsv) noexcept {
            float hue = std::isfinite(hsv.hue) ? std::fmod(hsv.hue, 360.0f) : 0.0f;

            if(hue < 0.0f) {
                hue += 360.0f;
            }

            const float saturation = saturate(hsv.saturation);
            const float value = saturate(hsv.value);
            const float chroma = saturation * value;
            const float x = chroma * (1.0f - std::abs(std::fmod(hue * (1.0f / 60.0f), 2.0f) - 1.0f));
            const float m = value - chroma;
            const auto sector = static_cast<std::uint32_t>(hue * (1.0f / 60.0f));

            float red = 0.0f;
            float green = 0.0f;
            float blue = 0.0f;

            switch(sector) {
                case 0:
                    red = chroma;
                    green = x;
                    break;
                case 1:
                    red = x;
                    green = chroma;
                    break;
                case 2:
                    green = chroma;
                    blue = x;
                    break;
                case 3:
                    green = x;
                    blue = chroma;
                    break;
                case 4:
                    red = x;
                    blue = chroma;
                    break;
                default:
                    red = chroma;
                    blue = x;
                    break;
            }

            return {
                gamma_to_linear(red + m),
                gamma_to_linear(green + m),
                gamma_to_linear(blue + m),
                1.0f
            };
        }

        math::vec4 m_color;
    };

    constexpr color_linear_u8::color_linear_u8(const color& value) noexcept
        : color_base_u8 {
            color_converter::float_to_byte(value.red()),
            color_converter::float_to_byte(value.green()),
            color_converter::float_to_byte(value.blue()),
            color_converter::float_to_byte(value.alpha())
        }
    {}

    [[nodiscard]] constexpr color color_linear_u8::to_linear_float() const noexcept {
        return {
            color_converter::byte_to_float(red()),
            color_converter::byte_to_float(green()),
            color_converter::byte_to_float(blue()),
            color_converter::byte_to_float(alpha())
        };
    }

    color_gamma_u8::color_gamma_u8(const color& value) noexcept
        : color_base_u8 {
            color_converter::float_to_byte(color::linear_to_gamma(value.red())),
            color_converter::float_to_byte(color::linear_to_gamma(value.green())),
            color_converter::float_to_byte(color::linear_to_gamma(value.blue())),
            color_converter::float_to_byte(value.alpha())
        }
    {}

    [[nodiscard]] constexpr color color_gamma_u8::to_linear_float() const noexcept {
        return {
            color_converter::srgb_byte_to_linear(red()),
            color_converter::srgb_byte_to_linear(green()),
            color_converter::srgb_byte_to_linear(blue()),
            color_converter::byte_to_float(alpha())
        };
    }

    namespace colors {
        inline constexpr color alice_blue{color_gamma_u8{0xf0, 0xf8, 0xff}};
        inline constexpr color antique_white{color_gamma_u8{0xfa, 0xeb, 0xd7}};
        inline constexpr color aqua{color_gamma_u8{0x00, 0xff, 0xff}};
        inline constexpr color aquamarine{color_gamma_u8{0x7f, 0xff, 0xd4}};
        inline constexpr color azure{color_gamma_u8{0xf0, 0xff, 0xff}};
        inline constexpr color beige{color_gamma_u8{0xf5, 0xf5, 0xdc}};
        inline constexpr color bisque{color_gamma_u8{0xff, 0xe4, 0xc4}};
        inline constexpr color black{color_gamma_u8{0x00, 0x00, 0x00}};
        inline constexpr color blanched_almond{color_gamma_u8{0xff, 0xeb, 0xcd}};
        inline constexpr color blue{color_gamma_u8{0x00, 0x00, 0xff}};
        inline constexpr color blue_violet{color_gamma_u8{0x8a, 0x2b, 0xe2}};
        inline constexpr color brown{color_gamma_u8{0xa5, 0x2a, 0x2a}};
        inline constexpr color burly_wood{color_gamma_u8{0xde, 0xb8, 0x87}};
        inline constexpr color cadet_blue{color_gamma_u8{0x5f, 0x9e, 0xa0}};
        inline constexpr color chartreuse{color_gamma_u8{0x7f, 0xff, 0x00}};
        inline constexpr color chocolate{color_gamma_u8{0xd2, 0x69, 0x1e}};
        inline constexpr color coral{color_gamma_u8{0xff, 0x7f, 0x50}};
        inline constexpr color cornflower_blue{color_gamma_u8{0x64, 0x95, 0xed}};
        inline constexpr color cornsilk{color_gamma_u8{0xff, 0xf8, 0xdc}};
        inline constexpr color crimson{color_gamma_u8{0xdc, 0x14, 0x3c}};
        inline constexpr color cyan{color_gamma_u8{0x00, 0xff, 0xff}};
        inline constexpr color dark_blue{color_gamma_u8{0x00, 0x00, 0x8b}};
        inline constexpr color dark_cyan{color_gamma_u8{0x00, 0x8b, 0x8b}};
        inline constexpr color dark_golden_rod{color_gamma_u8{0xb8, 0x86, 0x0b}};
        inline constexpr color dark_gray{color_gamma_u8{0xa9, 0xa9, 0xa9}};
        inline constexpr color dark_grey{color_gamma_u8{0xa9, 0xa9, 0xa9}};
        inline constexpr color dark_green{color_gamma_u8{0x00, 0x64, 0x00}};
        inline constexpr color dark_khaki{color_gamma_u8{0xbd, 0xb7, 0x6b}};
        inline constexpr color dark_magenta{color_gamma_u8{0x8b, 0x00, 0x8b}};
        inline constexpr color dark_olive_green{color_gamma_u8{0x55, 0x6b, 0x2f}};
        inline constexpr color dark_orange{color_gamma_u8{0xff, 0x8c, 0x00}};
        inline constexpr color dark_orchid{color_gamma_u8{0x99, 0x32, 0xcc}};
        inline constexpr color dark_red{color_gamma_u8{0x8b, 0x00, 0x00}};
        inline constexpr color dark_salmon{color_gamma_u8{0xe9, 0x96, 0x7a}};
        inline constexpr color dark_sea_green{color_gamma_u8{0x8f, 0xbc, 0x8f}};
        inline constexpr color dark_slate_blue{color_gamma_u8{0x48, 0x3d, 0x8b}};
        inline constexpr color dark_slate_gray{color_gamma_u8{0x2f, 0x4f, 0x4f}};
        inline constexpr color dark_slate_grey{color_gamma_u8{0x2f, 0x4f, 0x4f}};
        inline constexpr color dark_turquoise{color_gamma_u8{0x00, 0xce, 0xd1}};
        inline constexpr color dark_violet{color_gamma_u8{0x94, 0x00, 0xd3}};
        inline constexpr color deep_pink{color_gamma_u8{0xff, 0x14, 0x93}};
        inline constexpr color deep_sky_blue{color_gamma_u8{0x00, 0xbf, 0xff}};
        inline constexpr color dim_gray{color_gamma_u8{0x69, 0x69, 0x69}};
        inline constexpr color dim_grey{color_gamma_u8{0x69, 0x69, 0x69}};
        inline constexpr color dodger_blue{color_gamma_u8{0x1e, 0x90, 0xff}};
        inline constexpr color fire_brick{color_gamma_u8{0xb2, 0x22, 0x22}};
        inline constexpr color floral_white{color_gamma_u8{0xff, 0xfa, 0xf0}};
        inline constexpr color forest_green{color_gamma_u8{0x22, 0x8b, 0x22}};
        inline constexpr color fuchsia{color_gamma_u8{0xff, 0x00, 0xff}};
        inline constexpr color gainsboro{color_gamma_u8{0xdc, 0xdc, 0xdc}};
        inline constexpr color ghost_white{color_gamma_u8{0xf8, 0xf8, 0xff}};
        inline constexpr color gold{color_gamma_u8{0xff, 0xd7, 0x00}};
        inline constexpr color golden_rod{color_gamma_u8{0xda, 0xa5, 0x20}};
        inline constexpr color gray{color_gamma_u8{0x80, 0x80, 0x80}};
        inline constexpr color grey{color_gamma_u8{0x80, 0x80, 0x80}};
        inline constexpr color green{color_gamma_u8{0x00, 0x80, 0x00}};
        inline constexpr color green_yellow{color_gamma_u8{0xad, 0xff, 0x2f}};
        inline constexpr color honey_dew{color_gamma_u8{0xf0, 0xff, 0xf0}};
        inline constexpr color hot_pink{color_gamma_u8{0xff, 0x69, 0xb4}};
        inline constexpr color indian_red{color_gamma_u8{0xcd, 0x5c, 0x5c}};
        inline constexpr color indigo{color_gamma_u8{0x4b, 0x00, 0x82}};
        inline constexpr color ivory{color_gamma_u8{0xff, 0xff, 0xf0}};
        inline constexpr color khaki{color_gamma_u8{0xf0, 0xe6, 0x8c}};
        inline constexpr color lavender{color_gamma_u8{0xe6, 0xe6, 0xfa}};
        inline constexpr color lavender_blush{color_gamma_u8{0xff, 0xf0, 0xf5}};
        inline constexpr color lawn_green{color_gamma_u8{0x7c, 0xfc, 0x00}};
        inline constexpr color lemon_chiffon{color_gamma_u8{0xff, 0xfa, 0xcd}};
        inline constexpr color light_blue{color_gamma_u8{0xad, 0xd8, 0xe6}};
        inline constexpr color light_coral{color_gamma_u8{0xf0, 0x80, 0x80}};
        inline constexpr color light_cyan{color_gamma_u8{0xe0, 0xff, 0xff}};
        inline constexpr color light_golden_rod_yellow{color_gamma_u8{0xfa, 0xfa, 0xd2}};
        inline constexpr color light_gray{color_gamma_u8{0xd3, 0xd3, 0xd3}};
        inline constexpr color light_grey{color_gamma_u8{0xd3, 0xd3, 0xd3}};
        inline constexpr color light_green{color_gamma_u8{0x90, 0xee, 0x90}};
        inline constexpr color light_pink{color_gamma_u8{0xff, 0xb6, 0xc1}};
        inline constexpr color light_salmon{color_gamma_u8{0xff, 0xa0, 0x7a}};
        inline constexpr color light_sea_green{color_gamma_u8{0x20, 0xb2, 0xaa}};
        inline constexpr color light_sky_blue{color_gamma_u8{0x87, 0xce, 0xfa}};
        inline constexpr color light_slate_gray{color_gamma_u8{0x77, 0x88, 0x99}};
        inline constexpr color light_slate_grey{color_gamma_u8{0x77, 0x88, 0x99}};
        inline constexpr color light_steel_blue{color_gamma_u8{0xb0, 0xc4, 0xde}};
        inline constexpr color light_yellow{color_gamma_u8{0xff, 0xff, 0xe0}};
        inline constexpr color lime{color_gamma_u8{0x00, 0xff, 0x00}};
        inline constexpr color lime_green{color_gamma_u8{0x32, 0xcd, 0x32}};
        inline constexpr color linen{color_gamma_u8{0xfa, 0xf0, 0xe6}};
        inline constexpr color magenta{color_gamma_u8{0xff, 0x00, 0xff}};
        inline constexpr color maroon{color_gamma_u8{0x80, 0x00, 0x00}};
        inline constexpr color medium_aqua_marine{color_gamma_u8{0x66, 0xcd, 0xaa}};
        inline constexpr color medium_blue{color_gamma_u8{0x00, 0x00, 0xcd}};
        inline constexpr color medium_orchid{color_gamma_u8{0xba, 0x55, 0xd3}};
        inline constexpr color medium_purple{color_gamma_u8{0x93, 0x70, 0xdb}};
        inline constexpr color medium_sea_green{color_gamma_u8{0x3c, 0xb3, 0x71}};
        inline constexpr color medium_slate_blue{color_gamma_u8{0x7b, 0x68, 0xee}};
        inline constexpr color medium_spring_green{color_gamma_u8{0x00, 0xfa, 0x9a}};
        inline constexpr color medium_turquoise{color_gamma_u8{0x48, 0xd1, 0xcc}};
        inline constexpr color medium_violet_red{color_gamma_u8{0xc7, 0x15, 0x85}};
        inline constexpr color midnight_blue{color_gamma_u8{0x19, 0x19, 0x70}};
        inline constexpr color mint_cream{color_gamma_u8{0xf5, 0xff, 0xfa}};
        inline constexpr color misty_rose{color_gamma_u8{0xff, 0xe4, 0xe1}};
        inline constexpr color navajo_white{color_gamma_u8{0xff, 0xde, 0xad}};
        inline constexpr color navy{color_gamma_u8{0x00, 0x00, 0x80}};
        inline constexpr color old_lace{color_gamma_u8{0xfd, 0xf5, 0xe6}};
        inline constexpr color olive{color_gamma_u8{0x80, 0x80, 0x00}};
        inline constexpr color olive_drab{color_gamma_u8{0x6b, 0x8e, 0x23}};
        inline constexpr color orange{color_gamma_u8{0xff, 0xa5, 0x00}};
        inline constexpr color orange_red{color_gamma_u8{0xff, 0x45, 0x00}};
        inline constexpr color orchid{color_gamma_u8{0xda, 0x70, 0xd6}};
        inline constexpr color pale_golden_rod{color_gamma_u8{0xee, 0xe8, 0xaa}};
        inline constexpr color pale_green{color_gamma_u8{0x98, 0xfb, 0x98}};
        inline constexpr color pale_turquoise{color_gamma_u8{0xaf, 0xee, 0xee}};
        inline constexpr color pale_violet_red{color_gamma_u8{0xdb, 0x70, 0x93}};
        inline constexpr color papaya_whip{color_gamma_u8{0xff, 0xef, 0xd5}};
        inline constexpr color peach_puff{color_gamma_u8{0xff, 0xda, 0xb9}};
        inline constexpr color peru{color_gamma_u8{0xcd, 0x85, 0x3f}};
        inline constexpr color pink{color_gamma_u8{0xff, 0xc0, 0xcb}};
        inline constexpr color plum{color_gamma_u8{0xdd, 0xa0, 0xdd}};
        inline constexpr color powder_blue{color_gamma_u8{0xb0, 0xe0, 0xe6}};
        inline constexpr color purple{color_gamma_u8{0x80, 0x00, 0x80}};
        inline constexpr color rebecca_purple{color_gamma_u8{0x66, 0x33, 0x99}};
        inline constexpr color red{color_gamma_u8{0xff, 0x00, 0x00}};
        inline constexpr color rosy_brown{color_gamma_u8{0xbc, 0x8f, 0x8f}};
        inline constexpr color royal_blue{color_gamma_u8{0x41, 0x69, 0xe1}};
        inline constexpr color saddle_brown{color_gamma_u8{0x8b, 0x45, 0x13}};
        inline constexpr color salmon{color_gamma_u8{0xfa, 0x80, 0x72}};
        inline constexpr color sandy_brown{color_gamma_u8{0xf4, 0xa4, 0x60}};
        inline constexpr color sea_green{color_gamma_u8{0x2e, 0x8b, 0x57}};
        inline constexpr color sea_shell{color_gamma_u8{0xff, 0xf5, 0xee}};
        inline constexpr color sienna{color_gamma_u8{0xa0, 0x52, 0x2d}};
        inline constexpr color silver{color_gamma_u8{0xc0, 0xc0, 0xc0}};
        inline constexpr color sky_blue{color_gamma_u8{0x87, 0xce, 0xeb}};
        inline constexpr color slate_blue{color_gamma_u8{0x6a, 0x5a, 0xcd}};
        inline constexpr color slate_gray{color_gamma_u8{0x70, 0x80, 0x90}};
        inline constexpr color slate_grey{color_gamma_u8{0x70, 0x80, 0x90}};
        inline constexpr color snow{color_gamma_u8{0xff, 0xfa, 0xfa}};
        inline constexpr color spring_green{color_gamma_u8{0x00, 0xff, 0x7f}};
        inline constexpr color steel_blue{color_gamma_u8{0x46, 0x82, 0xb4}};
        inline constexpr color tan{color_gamma_u8{0xd2, 0xb4, 0x8c}};
        inline constexpr color teal{color_gamma_u8{0x00, 0x80, 0x80}};
        inline constexpr color thistle{color_gamma_u8{0xd8, 0xbf, 0xd8}};
        inline constexpr color tomato{color_gamma_u8{0xff, 0x63, 0x47}};
        inline constexpr color turquoise{color_gamma_u8{0x40, 0xe0, 0xd0}};
        inline constexpr color violet{color_gamma_u8{0xee, 0x82, 0xee}};
        inline constexpr color wheat{color_gamma_u8{0xf5, 0xde, 0xb3}};
        inline constexpr color white{color_gamma_u8{0xff, 0xff, 0xff}};
        inline constexpr color white_smoke{color_gamma_u8{0xf5, 0xf5, 0xf5}};
        inline constexpr color yellow{color_gamma_u8{0xff, 0xff, 0x00}};
        inline constexpr color yellow_green{color_gamma_u8{0x9a, 0xcd, 0x32}};
    }
}
