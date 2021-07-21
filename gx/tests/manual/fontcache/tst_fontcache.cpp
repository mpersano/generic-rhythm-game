#include <gx/fontcache.h>

#include <fstream>
#include <iostream>
#include <string>

using namespace std::string_literals;

void dumpPgm(const std::string &path, int width, int height, const unsigned char *data)
{
    std::ofstream file(path);
    if (!file.is_open())
        return;

    file << "P2\n";
    file << width << ' ' << height << '\n';
    file << "255\n";

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            file << static_cast<int>(data[i * width + j]) << ' ';
        }
        file << '\n';
    }

    file.close();
}

int main()
{
    GX::FontCache fontCache;

    const auto font = "/usr/share/fonts/truetype/takao-gothic/TakaoPGothic.ttf"s;
    if (!fontCache.load(font, 50)) {
        std::cout << "Failed to load" << font << '\n';
        return 1;
    }

    const auto text =
            U"Lorem ipsum dolor sit amet, consectetur "
            U"adipiscing elit, sed do eiusmod tempor incididunt "
            U"ut labore et dolore magna aliqua. Ut enim ad "
            U"minim veniam, quis nostrud exercitation ullamco "
            U"laboris nisi ut aliquip ex ea commodo consequat. "
            U"Duis aute irure dolor in reprehenderit in "
            U"voluptate velit esse cillum dolore eu fugiat nulla "
            U"pariatur. Excepteur sint occaecat cupidatat non "
            U"proident, sunt in culpa qui officia deserunt mollit "
            U"anim id est laborum. "
            U"しかし時には、参考文献に掲載されている文章をそのまま転載"
            U"し、読者に読ませることによって、記事が説明しようとする事"
            U"項に対する読者の理解が著しく向上することがあります。たと"
            U"えば、作家を主題とする記事において、その作家の作風が色濃"
            U"く反映された作品の一部を掲載したり、政治家を主題とする記"
            U"事において、その政治家の重要演説の一部を掲載すれば、理解"
            U"の助けとなるでしょう。このような執筆方法は、ウィキペディ"
            U"アが検証可能性の担保を重要方針に掲げる趣旨に、決して反す"
            U"るものではありません。"s;

    for (char32_t ch : text) {
        // insert glyph into font cache
        fontCache.getGlyph(ch);
    }

    const auto &textureAtlas = fontCache.textureAtlas();

    // dump the generated texture atlases to pgm image files
    for (int i = 0, count = textureAtlas.pageCount(); i < count; ++i) {
        const auto &page = textureAtlas.page(i);
        const auto path = std::to_string(i) + std::string(".pgm");
        const auto *pm = page.pixmap();
        dumpPgm(path, pm->width, pm->height, pm->pixels.data());
        std::cout << "wrote " << path << '\n';
    }
}
