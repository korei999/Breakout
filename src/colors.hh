#pragma once
#include "adt/math.hh"

/* https://www.rapidtables.com/web/color/RGB_Color.html */

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

    constexpr adt::math::V3 map[] {
        adt::math::V3{0.501961f, 0.0f, 0.0f}, /* MAROON, */
        adt::math::V3{0.545098f, 0.0f, 0.0f}, /* DARKRED */
        adt::math::V3{0.647059f, 0.164706f, 0.164706f}, /* BROWN */
        adt::math::V3{0.698039f, 0.133333f, 0.133333f}, /* FIREBRICK */
        adt::math::V3{0.862745f, 0.0784314f, 0.235294f}, /* CRIMSON */
        adt::math::V3{1.0f, 0.0f, 0.0f}, /* RED */
        adt::math::V3{1.0f, 0.388235f, 0.278431f}, /* TOMATO */
        adt::math::V3{1.0f, 0.498039f, 0.313726f}, /* CORAL */
        adt::math::V3{0.803922f, 0.360784f, 0.360784f}, /* INDIANRED */
        adt::math::V3{0.941176f, 0.501961f, 0.501961f}, /* LIGHTCORAL */
        adt::math::V3{0.913725f, 0.588235f, 0.478431f}, /* DARKSALMON */
        adt::math::V3{0.980392f, 0.501961f, 0.447059f}, /* SALMON */
        adt::math::V3{1.0f, 0.627451f, 0.478431f}, /* LIGHTSALMON */
        adt::math::V3{1.0f, 0.270588f, 0.0f}, /* ORANGERED */
        adt::math::V3{1.0f, 0.54902f, 0.0f}, /* DARKORANGE */
        adt::math::V3{1.0f, 0.647059f, 0.0f}, /* ORANGE */
        adt::math::V3{1.0f, 0.843137f, 0.0f}, /* GOLD */
        adt::math::V3{0.721569f, 0.52549f, 0.0431373f}, /* DARKGOLDENROD */
        adt::math::V3{0.854902f, 0.647059f, 0.12549f}, /* GOLDENROD */
        adt::math::V3{0.933333f, 0.909804f, 0.666667f}, /* PALEGOLDENROD */
        adt::math::V3{0.741176f, 0.717647f, 0.419608f}, /* DARKKHAKI */
        adt::math::V3{0.941176f, 0.901961f, 0.54902f}, /* KHAKI */
        adt::math::V3{0.501961f, 0.501961f, 0.0f}, /* OLIVE */
        adt::math::V3{1.0f, 1.0f, 0.0f}, /* YELLOW */
        adt::math::V3{0.603922f, 0.803922f, 0.196078f}, /* YELLOWGREEN */
        adt::math::V3{0.333333f, 0.419608f, 0.184314f}, /* DARKOLIVEGREEN */
        adt::math::V3{0.419608f, 0.556863f, 0.137255f}, /* OLIVEDRAB */
        adt::math::V3{0.486275f, 0.988235f, 0.0f}, /* LAWNGREEN */
        adt::math::V3{0.498039f, 1.0f, 0.0f}, /* CHARTREUSE */
        adt::math::V3{0.678431f, 1.0f, 0.184314f}, /* GREENYELLOW */
        adt::math::V3{0.0f, 0.392157f, 0.0f}, /* DARKGREEN */
        adt::math::V3{0.0f, 0.501961f, 0.0f}, /* GREEN */
        adt::math::V3{0.133333f, 0.545098f, 0.133333f}, /* FORESTGREEN */
        adt::math::V3{0.0f, 1.0f, 0.0f}, /* LIME */
        adt::math::V3{0.196078f, 0.803922f, 0.196078f}, /* LIMEGREEN */
        adt::math::V3{0.564706f, 0.933333f, 0.564706f}, /* LIGHTGREEN */
        adt::math::V3{0.596078f, 0.984314f, 0.596078f}, /* PALEGREEN */
        adt::math::V3{0.560784f, 0.737255f, 0.560784f}, /* DARKSEAGREEN */
        adt::math::V3{0.0f, 0.980392f, 0.603922f}, /* MEDIUMSPRINGGREEN */
        adt::math::V3{0.0f, 1.0f, 0.498039f}, /* SPRINGGREEN */
        adt::math::V3{0.180392f, 0.545098f, 0.341176f}, /* SEAGREEN */
        adt::math::V3{0.4f, 0.803922f, 0.666667f}, /* MEDIUMAQUAMARINE */
        adt::math::V3{0.235294f, 0.701961f, 0.443137f}, /* MEDIUMSEAGREEN */
        adt::math::V3{0.12549f, 0.698039f, 0.666667f}, /* LIGHTSEAGREEN */
        adt::math::V3{0.184314f, 0.309804f, 0.309804f}, /* DARKSLATEGRAY */
        adt::math::V3{0.0f, 0.501961f, 0.501961f}, /* TEAL */
        adt::math::V3{0.0f, 0.545098f, 0.545098f}, /* DARKCYAN */
        adt::math::V3{0.0f, 1.0f, 1.0f}, /* AQUA */
        adt::math::V3{0.0f, 1.0f, 1.0f}, /* CYAN */
        adt::math::V3{0.878431f, 1.0f, 1.0f}, /* LIGHTCYAN */
        adt::math::V3{0.0f, 0.807843f, 0.819608f}, /* DARKTURQUOISE */
        adt::math::V3{0.25098f, 0.878431f, 0.815686f}, /* TURQUOISE */
        adt::math::V3{0.282353f, 0.819608f, 0.8f}, /* MEDIUMTURQUOISE */
        adt::math::V3{0.686275f, 0.933333f, 0.933333f}, /* PALETURQUOISE */
        adt::math::V3{0.498039f, 1.0f, 0.831373f}, /* AQUAMARINE */
        adt::math::V3{0.690196f, 0.878431f, 0.901961f}, /* POWDERBLUE */
        adt::math::V3{0.372549f, 0.619608f, 0.627451f}, /* CADETBLUE */
        adt::math::V3{0.27451f, 0.509804f, 0.705882f}, /* STEELBLUE */
        adt::math::V3{0.392157f, 0.584314f, 0.929412f}, /* CORNFLOWERBLUE */
        adt::math::V3{0.0f, 0.74902f, 1.0f}, /* DEEPSKYBLUE */
        adt::math::V3{0.117647f, 0.564706f, 1.0f}, /* DODGERBLUE */
        adt::math::V3{0.678431f, 0.847059f, 0.901961f}, /* LIGHTBLUE */
        adt::math::V3{0.529412f, 0.807843f, 0.921569f}, /* SKYBLUE */
        adt::math::V3{0.529412f, 0.807843f, 0.980392f}, /* LIGHTSKYBLUE */
        adt::math::V3{0.0980392f, 0.0980392f, 0.439216f}, /* MIDNIGHTBLUE */
        adt::math::V3{0.0f, 0.0f, 0.501961f}, /* NAVY */
        adt::math::V3{0.0f, 0.0f, 0.545098f}, /* DARKBLUE */
        adt::math::V3{0.0f, 0.0f, 0.803922f}, /* MEDIUMBLUE */
        adt::math::V3{0.0f, 0.0f, 1.0f}, /* BLUE */
        adt::math::V3{0.254902f, 0.411765f, 0.882353f}, /* ROYALBLUE */
        adt::math::V3{0.541176f, 0.168627f, 0.886275f}, /* BLUEVIOLET */
        adt::math::V3{0.294118f, 0.0f, 0.509804f}, /* INDIGO */
        adt::math::V3{0.282353f, 0.239216f, 0.545098f}, /* DARKSLATEBLUE */
        adt::math::V3{0.415686f, 0.352941f, 0.803922f}, /* SLATEBLUE */
        adt::math::V3{0.482353f, 0.407843f, 0.933333f}, /* MEDIUMSLATEBLUE */
        adt::math::V3{0.576471f, 0.439216f, 0.858824f}, /* MEDIUMPURPLE */
        adt::math::V3{0.545098f, 0.0f, 0.545098f}, /* DARKMAGENTA */
        adt::math::V3{0.580392f, 0.0f, 0.827451f}, /* DARKVIOLET */
        adt::math::V3{0.6f, 0.196078f, 0.8f}, /* DARKORCHID */
        adt::math::V3{0.729412f, 0.333333f, 0.827451f}, /* MEDIUMORCHID */
        adt::math::V3{0.501961f, 0.0f, 0.501961f}, /* PURPLE */
        adt::math::V3{0.847059f, 0.74902f, 0.847059f}, /* THISTLE */
        adt::math::V3{0.866667f, 0.627451f, 0.866667f}, /* PLUM */
        adt::math::V3{0.933333f, 0.509804f, 0.933333f}, /* VIOLET */
        adt::math::V3{1.0f, 0.0f, 1.0f}, /* MAGENTA */
        adt::math::V3{0.854902f, 0.439216f, 0.839216f}, /* ORCHID */
        adt::math::V3{0.780392f, 0.0823529f, 0.521569f}, /* MEDIUMVIOLETRED */
        adt::math::V3{0.858824f, 0.439216f, 0.576471f}, /* PALEVIOLETRED */
        adt::math::V3{1.0f, 0.0784314f, 0.576471f}, /* DEEPPINK */
        adt::math::V3{1.0f, 0.411765f, 0.705882f}, /* HOTPINK */
        adt::math::V3{1.0f, 0.713726f, 0.756863f}, /* LIGHTPINK */
        adt::math::V3{1.0f, 0.752941f, 0.796078f}, /* PINK */
        adt::math::V3{0.980392f, 0.921569f, 0.843137f}, /* ANTIQUEWHITE */
        adt::math::V3{0.960784f, 0.960784f, 0.862745f}, /* BEIGE */
        adt::math::V3{1.0f, 0.894118f, 0.768627f}, /* BISQUE */
        adt::math::V3{1.0f, 0.921569f, 0.803922f}, /* BLANCHEDALMOND */
        adt::math::V3{0.960784f, 0.870588f, 0.701961f}, /* WHEAT */
        adt::math::V3{1.0f, 0.972549f, 0.862745f}, /* CORNSILK */
        adt::math::V3{1.0f, 0.980392f, 0.803922f}, /* LEMONCHIFFON */
        adt::math::V3{0.980392f, 0.980392f, 0.823529f}, /* LIGHTGOLDENRODYELLOW */
        adt::math::V3{1.0f, 1.0f, 0.878431f}, /* LIGHTYELLOW */
        adt::math::V3{0.545098f, 0.270588f, 0.0745098f}, /* SADDLEBROWN */
        adt::math::V3{0.627451f, 0.321569f, 0.176471f}, /* SIENNA */
        adt::math::V3{0.823529f, 0.411765f, 0.117647f}, /* CHOCOLATE */
        adt::math::V3{0.803922f, 0.521569f, 0.247059f}, /* PERU */
        adt::math::V3{0.956863f, 0.643137f, 0.376471f}, /* SANDYBROWN */
        adt::math::V3{0.870588f, 0.721569f, 0.529412f}, /* BURLYWOOD */
        adt::math::V3{0.823529f, 0.705882f, 0.54902f}, /* TAN */
        adt::math::V3{0.737255f, 0.560784f, 0.560784f}, /* ROSYBROWN */
        adt::math::V3{1.0f, 0.894118f, 0.709804f}, /* MOCCASIN */
        adt::math::V3{1.0f, 0.870588f, 0.678431f}, /* NAVAJOWHITE */
        adt::math::V3{1.0f, 0.854902f, 0.72549f}, /* PEACHPUFF */
        adt::math::V3{1.0f, 0.894118f, 0.882353f}, /* MISTYROSE */
        adt::math::V3{1.0f, 0.941176f, 0.960784f}, /* LAVENDERBLUSH */
        adt::math::V3{0.980392f, 0.941176f, 0.901961f}, /* LINEN */
        adt::math::V3{0.992157f, 0.960784f, 0.901961f}, /* OLDLACE */
        adt::math::V3{1.0f, 0.937255f, 0.835294f}, /* PAPAYAWHIP */
        adt::math::V3{1.0f, 0.960784f, 0.933333f}, /* SEASHELL */
        adt::math::V3{0.960784f, 1.0f, 0.980392f}, /* MINTCREAM */
        adt::math::V3{0.439216f, 0.501961f, 0.564706f}, /* SLATEGRAY */
        adt::math::V3{0.466667f, 0.533333f, 0.6f}, /* LIGHTSLATEGRAY */
        adt::math::V3{0.690196f, 0.768627f, 0.870588f}, /* LIGHTSTEELBLUE */
        adt::math::V3{0.901961f, 0.901961f, 0.980392f}, /* LAVENDER */
        adt::math::V3{1.0f, 0.980392f, 0.941176f}, /* FLORALWHITE */
        adt::math::V3{0.941176f, 0.972549f, 1.0f}, /* ALICEBLUE */
        adt::math::V3{0.972549f, 0.972549f, 1.0f}, /* GHOSTWHITE */
        adt::math::V3{0.941176f, 1.0f, 0.941176f}, /* HONEYDEW */
        adt::math::V3{1.0f, 1.0f, 0.941176f}, /* IVORY */
        adt::math::V3{0.941176f, 1.0f, 1.0f}, /* AZURE */
        adt::math::V3{1.0f, 0.980392f, 0.980392f}, /* SNOW */
        adt::math::V3{0.0f, 0.0f, 0.0f}, /* BLACK */
        adt::math::V3{0.411765f, 0.411765f, 0.411765f}, /* DIMGREY */
        adt::math::V3{0.501961f, 0.501961f, 0.501961f}, /* GREY */
        adt::math::V3{0.662745f, 0.662745f, 0.662745f}, /* DARKGREY */
        adt::math::V3{0.752941f, 0.752941f, 0.752941f}, /* SILVER */
        adt::math::V3{0.827451f, 0.827451f, 0.827451f}, /* LIGHTGREY */
        adt::math::V3{0.862745f, 0.862745f, 0.862745f}, /* GAINSBOR */
        adt::math::V3{0.960784f, 0.960784f, 0.960784f}, /* WHITESMOKE */
        adt::math::V3{1.0f, 1.0f, 1.0f}, /* WHITE */
    };

    constexpr adt::math::V3 get(IDX e) { return map[int(e)]; }

    constexpr adt::math::V4
    hexToV4(int hex)
    {
        return {
            ((hex >> 24) & 0xff) / 255.0f,
            ((hex >> 16) & 0xff) / 255.0f,
            ((hex >> 8 ) & 0xff) / 255.0f,
            ((hex)       & 0xff) / 255.0f
        };
    }

    constexpr adt::math::V3
    hexToV3(int hex)
    {
        return {
            ((hex >> 16) & 0xff) / 255.0f,
            ((hex >> 8 ) & 0xff) / 255.0f,
            ((hex)       & 0xff) / 255.0f
        };
    }

    inline constexpr int
    V4ToHex(adt::math::V4 v)
    {
        int r {};
        r |= (int(v.x * 255.0f) << 24);
        r |= (int(v.y * 255.0f) << 16);
        r |= (int(v.z * 255.0f) << 8);
        r |=  int(v.w * 255.0f);

        return r;
    }

    inline constexpr int
    V3ToHex(adt::math::V3 v)
    {
        int r {};
        r |= (int(v.x * 255.0f) << 16);
        r |= (int(v.y * 255.0f) << 8);
        r |=  int(v.z * 255.0f);

        return r;
    }
};
