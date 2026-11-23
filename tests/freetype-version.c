#include <stdio.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#if FREETYPE_MAJOR != 2 || FREETYPE_MINOR != 14 || FREETYPE_PATCH != 1
#error Unexpected FreeType headers
#endif

int main(void) {
    FT_Library library;
    FT_Int major, minor, patch;
    if (FT_Init_FreeType(&library)) return 1;
    FT_Library_Version(library, &major, &minor, &patch);
    printf("FreeType %d.%d.%d\n", major, minor, patch);
    FT_Done_FreeType(library);
    return major == 2 && minor == 14 && patch == 1 ? 0 : 2;
}
