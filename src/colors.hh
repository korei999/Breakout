#pragma once
#include "adt/math.hh"

/* https://www.rapidtables.com/web/color/RGB_Color.html */

using namespace adt;

namespace colors
{
    enum class IDX : int
    {
        MAROON,
        DARKRED,
        BROWN,
        FIREBRICK,
        CRIMSON,
        RED,
        TOMATO,
        CORAL,
        INDIANRED,
        LIGHTCORAL,
        DARKSALMON,
        SALMON,
        LIGHTSALMON,
        ORANGERED,
        DARKORANGE,
        ORANGE,
        GOLD,
        DARKGOLDENROD,
        GOLDENROD,
        PALEGOLDENROD,
        DARKKHAKI,
        KHAKI,
        OLIVE,
        YELLOW,
        YELLOWGREEN,
        DARKOLIVEGREEN,
        OLIVEDRAB,
        LAWNGREEN,
        CHARTREUSE,
        GREENYELLOW,
        DARKGREEN,
        GREEN,
        FORESTGREEN,
        LIME,
        LIMEGREEN,
        LIGHTGREEN,
        PALEGREEN,
        DARKSEAGREEN,
        MEDIUMSPRINGGREEN,
        SPRINGGREEN,
        SEAGREEN,
        MEDIUMAQUAMARINE,
        MEDIUMSEAGREEN,
        LIGHTSEAGREEN,
        DARKSLATEGRAY,
        TEAL,
        DARKCYAN,
        AQUA,
        CYAN,
        LIGHTCYAN,
        DARKTURQUOISE,
        TURQUOISE,
        MEDIUMTURQUOISE,
        PALETURQUOISE,
        AQUAMARINE,
        POWDERBLUE,
        CADETBLUE,
        STEELBLUE,
        CORNFLOWERBLUE,
        DEEPSKYBLUE,
        DODGERBLUE,
        LIGHTBLUE,
        SKYBLUE,
        LIGHTSKYBLUE,
        MIDNIGHTBLUE,
        NAVY,
        DARKBLUE,
        MEDIUMBLUE,
        BLUE,
        ROYALBLUE,
        BLUEVIOLET,
        INDIGO,
        DARKSLATEBLUE,
        SLATEBLUE,
        MEDIUMSLATEBLUE,
        MEDIUMPURPLE,
        DARKMAGENTA,
        DARKVIOLET,
        DARKORCHID,
        MEDIUMORCHID,
        PURPLE,
        THISTLE,
        PLUM,
        VIOLET,
        MAGENTA,
        ORCHID,
        MEDIUMVIOLETRED,
        PALEVIOLETRED,
        DEEPPINK,
        HOTPINK,
        LIGHTPINK,
        PINK,
        ANTIQUEWHITE,
        BEIGE,
        BISQUE,
        BLANCHEDALMOND,
        WHEAT,
        CORNSILK,
        LEMONCHIFFON,
        LIGHTGOLDENRODYELLOW,
        LIGHTYELLOW,
        SADDLEBROWN,
        SIENNA,
        CHOCOLATE,
        PERU,
        SANDYBROWN,
        BURLYWOOD,
        TAN,
        ROSYBROWN,
        MOCCASIN,
        NAVAJOWHITE,
        PEACHPUFF,
        MISTYROSE,
        LAVENDERBLUSH,
        LINEN,
        OLDLACE,
        PAPAYAWHIP,
        SEASHELL,
        MINTCREAM,
        SLATEGRAY,
        LIGHTSLATEGRAY,
        LIGHTSTEELBLUE,
        LAVENDER,
        FLORALWHITE,
        ALICEBLUE,
        GHOSTWHITE,
        HONEYDEW,
        IVORY,
        AZURE,
        SNOW,
        BLACK,
        DIMGREY,
        GREY,
        DARKGREY,
        SILVER,
        LIGHTGREY,
        GAINSBORO,
        WHITESMOKE,
        WHITE,
        ESIZE
    };

    constexpr math::V3 map[] {
        math::V3{0.501961f, 0.0f, 0.0f},
        math::V3{0.545098f, 0.0f, 0.0f},
        math::V3{0.647059f, 0.164706f, 0.164706f},
        math::V3{0.698039f, 0.133333f, 0.133333f},
        math::V3{0.862745f, 0.0784314f, 0.235294f},
        math::V3{1.0f, 0.0f, 0.0f},
        math::V3{1.0f, 0.388235f, 0.278431f},
        math::V3{1.0f, 0.498039f, 0.313726f},
        math::V3{0.803922f, 0.360784f, 0.360784f},
        math::V3{0.941176f, 0.501961f, 0.501961f},
        math::V3{0.913725f, 0.588235f, 0.478431f},
        math::V3{0.980392f, 0.501961f, 0.447059f},
        math::V3{1.0f, 0.627451f, 0.478431f},
        math::V3{1.0f, 0.270588f, 0.0f},
        math::V3{1.0f, 0.54902f, 0.0f},
        math::V3{1.0f, 0.647059f, 0.0f},
        math::V3{1.0f, 0.843137f, 0.0f},
        math::V3{0.721569f, 0.52549f, 0.0431373f},
        math::V3{0.854902f, 0.647059f, 0.12549f},
        math::V3{0.933333f, 0.909804f, 0.666667f},
        math::V3{0.741176f, 0.717647f, 0.419608f},
        math::V3{0.941176f, 0.901961f, 0.54902f},
        math::V3{0.501961f, 0.501961f, 0.0f},
        math::V3{1.0f, 1.0f, 0.0f},
        math::V3{0.603922f, 0.803922f, 0.196078f},
        math::V3{0.333333f, 0.419608f, 0.184314f},
        math::V3{0.419608f, 0.556863f, 0.137255f},
        math::V3{0.486275f, 0.988235f, 0.0f},
        math::V3{0.498039f, 1.0f, 0.0f},
        math::V3{0.678431f, 1.0f, 0.184314f},
        math::V3{0.0f, 0.392157f, 0.0f},
        math::V3{0.0f, 0.501961f, 0.0f},
        math::V3{0.133333f, 0.545098f, 0.133333f},
        math::V3{0.0f, 1.0f, 0.0f},
        math::V3{0.196078f, 0.803922f, 0.196078f},
        math::V3{0.564706f, 0.933333f, 0.564706f},
        math::V3{0.596078f, 0.984314f, 0.596078f},
        math::V3{0.560784f, 0.737255f, 0.560784f},
        math::V3{0.0f, 0.980392f, 0.603922f},
        math::V3{0.0f, 1.0f, 0.498039f},
        math::V3{0.180392f, 0.545098f, 0.341176f},
        math::V3{0.4f, 0.803922f, 0.666667f},
        math::V3{0.235294f, 0.701961f, 0.443137f},
        math::V3{0.12549f, 0.698039f, 0.666667f},
        math::V3{0.184314f, 0.309804f, 0.309804f},
        math::V3{0.0f, 0.501961f, 0.501961f},
        math::V3{0.0f, 0.545098f, 0.545098f},
        math::V3{0.0f, 1.0f, 1.0f},
        math::V3{0.0f, 1.0f, 1.0f},
        math::V3{0.878431f, 1.0f, 1.0f},
        math::V3{0.0f, 0.807843f, 0.819608f},
        math::V3{0.25098f, 0.878431f, 0.815686f},
        math::V3{0.282353f, 0.819608f, 0.8f},
        math::V3{0.686275f, 0.933333f, 0.933333f},
        math::V3{0.498039f, 1.0f, 0.831373f},
        math::V3{0.690196f, 0.878431f, 0.901961f},
        math::V3{0.372549f, 0.619608f, 0.627451f},
        math::V3{0.27451f, 0.509804f, 0.705882f},
        math::V3{0.392157f, 0.584314f, 0.929412f},
        math::V3{0.0f, 0.74902f, 1.0f},
        math::V3{0.117647f, 0.564706f, 1.0f},
        math::V3{0.678431f, 0.847059f, 0.901961f},
        math::V3{0.529412f, 0.807843f, 0.921569f},
        math::V3{0.529412f, 0.807843f, 0.980392f},
        math::V3{0.0980392f, 0.0980392f, 0.439216f},
        math::V3{0.0f, 0.0f, 0.501961f},
        math::V3{0.0f, 0.0f, 0.545098f},
        math::V3{0.0f, 0.0f, 0.803922f},
        math::V3{0.0f, 0.0f, 1.0f},
        math::V3{0.254902f, 0.411765f, 0.882353f},
        math::V3{0.541176f, 0.168627f, 0.886275f},
        math::V3{0.294118f, 0.0f, 0.509804f},
        math::V3{0.282353f, 0.239216f, 0.545098f},
        math::V3{0.415686f, 0.352941f, 0.803922f},
        math::V3{0.482353f, 0.407843f, 0.933333f},
        math::V3{0.576471f, 0.439216f, 0.858824f},
        math::V3{0.545098f, 0.0f, 0.545098f},
        math::V3{0.580392f, 0.0f, 0.827451f},
        math::V3{0.6f, 0.196078f, 0.8f},
        math::V3{0.729412f, 0.333333f, 0.827451f},
        math::V3{0.501961f, 0.0f, 0.501961f},
        math::V3{0.847059f, 0.74902f, 0.847059f},
        math::V3{0.866667f, 0.627451f, 0.866667f},
        math::V3{0.933333f, 0.509804f, 0.933333f},
        math::V3{1.0f, 0.0f, 1.0f},
        math::V3{0.854902f, 0.439216f, 0.839216f},
        math::V3{0.780392f, 0.0823529f, 0.521569f},
        math::V3{0.858824f, 0.439216f, 0.576471f},
        math::V3{1.0f, 0.0784314f, 0.576471f},
        math::V3{1.0f, 0.411765f, 0.705882f},
        math::V3{1.0f, 0.713726f, 0.756863f},
        math::V3{1.0f, 0.752941f, 0.796078f},
        math::V3{0.980392f, 0.921569f, 0.843137f},
        math::V3{0.960784f, 0.960784f, 0.862745f},
        math::V3{1.0f, 0.894118f, 0.768627f},
        math::V3{1.0f, 0.921569f, 0.803922f},
        math::V3{0.960784f, 0.870588f, 0.701961f},
        math::V3{1.0f, 0.972549f, 0.862745f},
        math::V3{1.0f, 0.980392f, 0.803922f},
        math::V3{0.980392f, 0.980392f, 0.823529f},
        math::V3{1.0f, 1.0f, 0.878431f},
        math::V3{0.545098f, 0.270588f, 0.0745098f},
        math::V3{0.627451f, 0.321569f, 0.176471f},
        math::V3{0.823529f, 0.411765f, 0.117647f},
        math::V3{0.803922f, 0.521569f, 0.247059f},
        math::V3{0.956863f, 0.643137f, 0.376471f},
        math::V3{0.870588f, 0.721569f, 0.529412f},
        math::V3{0.823529f, 0.705882f, 0.54902f},
        math::V3{0.737255f, 0.560784f, 0.560784f},
        math::V3{1.0f, 0.894118f, 0.709804f},
        math::V3{1.0f, 0.870588f, 0.678431f},
        math::V3{1.0f, 0.854902f, 0.72549f},
        math::V3{1.0f, 0.894118f, 0.882353f},
        math::V3{1.0f, 0.941176f, 0.960784f},
        math::V3{0.980392f, 0.941176f, 0.901961f},
        math::V3{0.992157f, 0.960784f, 0.901961f},
        math::V3{1.0f, 0.937255f, 0.835294f},
        math::V3{1.0f, 0.960784f, 0.933333f},
        math::V3{0.960784f, 1.0f, 0.980392f},
        math::V3{0.439216f, 0.501961f, 0.564706f},
        math::V3{0.466667f, 0.533333f, 0.6f},
        math::V3{0.690196f, 0.768627f, 0.870588f},
        math::V3{0.901961f, 0.901961f, 0.980392f},
        math::V3{1.0f, 0.980392f, 0.941176f},
        math::V3{0.941176f, 0.972549f, 1.0f},
        math::V3{0.972549f, 0.972549f, 1.0f},
        math::V3{0.941176f, 1.0f, 0.941176f},
        math::V3{1.0f, 1.0f, 0.941176f},
        math::V3{0.941176f, 1.0f, 1.0f},
        math::V3{1.0f, 0.980392f, 0.980392f},
        math::V3{0.0f, 0.0f, 0.0f},
        math::V3{0.411765f, 0.411765f, 0.411765f},
        math::V3{0.501961f, 0.501961f, 0.501961f},
        math::V3{0.662745f, 0.662745f, 0.662745f},
        math::V3{0.752941f, 0.752941f, 0.752941f},
        math::V3{0.827451f, 0.827451f, 0.827451f},
        math::V3{0.862745f, 0.862745f, 0.862745f},
        math::V3{0.960784f, 0.960784f, 0.960784f},
        math::V3{1.0f, 1.0f, 1.0f},
    };

    constexpr math::V3 get(IDX e) { return map[int(e)]; }

    constexpr math::V4
    hexToV4(int hex)
    {
        return {
            ((hex >> 24) & 0xff) / 255.0f,
            ((hex >> 16) & 0xff) / 255.0f,
            ((hex >> 8 ) & 0xff) / 255.0f,
            ((hex)       & 0xff) / 255.0f
        };
    }

    constexpr math::V3
    hexToV3(int hex)
    {
        return {
            ((hex >> 16) & 0xff) / 255.0f,
            ((hex >> 8 ) & 0xff) / 255.0f,
            ((hex)       & 0xff) / 255.0f
        };
    }
};
