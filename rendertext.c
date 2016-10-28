#include <Python.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <unicode/ustring.h>
#include <unicode/ubrk.h>
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
// #include <harfbuzz/hb-icu.h>


typedef struct Size {
    int width;
    int height;
} Size;

static PyObject *Error;
void renderTextBlock(UChar *text, int32_t start, int32_t end,
                     const FT_Face *face, int *x, int *y) {

}

void renderEmojiBlock(UChar *text, int32_t start, int32_t end,
                      const FT_Face *face, int *x, int *y) {

}

Size blockSize(UChar *text, int32_t start, int32_t end, const FT_Face *face) {
    return (Size){0, 0};
}

static PyObject *renderText(PyObject *self, PyObject *args) {
    const Py_UNICODE *text32;
    int fontSize;
    const char *textFontPath;
    const char *emojiFontPath;
    int width = 0;
    int height = 0;
    const char *outputPath;

    // Parse and validate arguments.
    if (!PyArg_ParseTuple(args, "uissiis", &text32, &fontSize,
                          &textFontPath, &emojiFontPath,
                          &width, &height, &outputPath)) {
        return NULL;
    }

    if (width <= 0 || height <= 0) {
        PyErr_SetString(Error, "Invalid output size");
        return NULL;
    }

    // Initialize Freetype.
    FT_Library ftLibrary;
    int error = FT_Init_FreeType(&ftLibrary);
    if (error) {
        PyErr_SetString(Error, "Failed to initialize freetype");
        return NULL;
    }

    // Loading text font.
    FT_Face textFace;
    if (FT_New_Face(ftLibrary, textFontPath, 0, &textFace)) {
        PyErr_SetString(Error, "Failed to load text font");
        FT_Done_FreeType(ftLibrary);
        return NULL;
    }
    FT_Set_Pixel_Sizes(textFace, 0, fontSize);
    hb_font_t *textFont = hb_ft_font_create_referenced(textFace);
    FT_Done_Face(textFace);

    // Loading emoji font.
    FT_Face emojiFace;
    if (FT_New_Face(ftLibrary, emojiFontPath, 0, &emojiFace)) {
        PyErr_SetString(Error, "Failed to load emoji font");
        hb_font_destroy(textFont);
        FT_Done_FreeType(ftLibrary);
        return NULL;
    }
    if (emojiFace->num_fixed_sizes != 0) {
        int bestMatch = 0;
        int diff = abs(fontSize - emojiFace->available_sizes[0].width);
        for (int i = 1; i < emojiFace->num_fixed_sizes; ++i) {
          int ndiff = abs(fontSize - emojiFace->available_sizes[i].width);
          if (ndiff < diff) {
            bestMatch = i;
            diff = ndiff;
          }
        }
        if (FT_Select_Size(emojiFace, bestMatch)) {
            PyErr_SetString(Error, "Failed to select emoji size");
            hb_font_destroy(textFont);
            FT_Done_Face(emojiFace);
            FT_Done_FreeType(ftLibrary);
            return NULL;
        }
    }
    hb_font_t *emojiFont = hb_ft_font_create_referenced(emojiFace);
    FT_Done_Face(emojiFace);

    // FT_LOAD_COLOR;
    // FT_RENDER_MODE_NORMAL
    //

    // Converting string to UChar *.
    int32_t textSize = 4 * wcslen(text32) + 1;
    UChar *text = calloc(1, textSize);
    UErrorCode errorCode = U_ZERO_ERROR;
    int32_t textLength;
    if (!u_strFromWCS(text, textSize, &textLength, text32, -1, &errorCode)) {
        PyErr_SetString(Error, "Conversion to UChar failed");
        hb_font_destroy(textFont);
        hb_font_destroy(emojiFont);
        FT_Done_FreeType(ftLibrary);
        free(text);
        return NULL;
    }

    // Preparing to iterate over words.
    errorCode = U_ZERO_ERROR;
    UBreakIterator *iterator = ubrk_open(UBRK_WORD, "ru", text, -1, &errorCode);
    if (iterator == NULL) {
        PyErr_SetString(Error, "Failed to iterate over words");
        hb_font_destroy(textFont);
        hb_font_destroy(emojiFont);
        FT_Done_FreeType(ftLibrary);
        free(text);
        return NULL;
    }

    // Laying out and rendering words.
    unsigned char *bitmap = calloc(1, width * height * 4);
    int x = 0, y = 0;
    int32_t end;
    int32_t start = ubrk_first(iterator);
    for (end = ubrk_next(iterator); end != UBRK_DONE;
                    start = end, end = ubrk_next(iterator)) {
        UChar32 c;
        U16_GET(text, 0, start, textLength, c);
        if (u_hasBinaryProperty(c, UCHAR_EMOJI)) {
            renderTextBlock(text, start, end, &emojiFace, &x, &y);
        }
        else {
            renderEmojiBlock(text, start, end, &textFace, &x, &y);
        }
    }

    // Cleanup.
    ubrk_close(iterator);
    hb_font_destroy(textFont);
    hb_font_destroy(emojiFont);
    FT_Done_FreeType(ftLibrary);
    free(text);
    free(bitmap);
    return Py_None;
}

static PyMethodDef ModuleMethods[] = {
    {"render_text",  renderText, METH_VARARGS, "Render text to image."},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef Module = {
    PyModuleDef_HEAD_INIT,
    "rendertext",
    NULL,
    -1,
    ModuleMethods
};

PyMODINIT_FUNC PyInit_rendertext(void) {
    PyObject *m = PyModule_Create(&Module);
    if (m == NULL) {
        return NULL;
    }

    Error = PyErr_NewException("rendertext.error", NULL, NULL);
    Py_INCREF(Error);
    PyModule_AddObject(m, "error", Error);
    return m;
}

// #include <harfbuzz/hb.h>
// #include <harfbuzz/hb-ft.h>
// #include <harfbuzz/hb-icu.h>
//
// #define PNG_SKIP_SETJMP_CHECK
// #include <png.h>
//
// const int kBytesPerPixel = 4; // RGBA
// const int kDefaultPixelSize = 128;
// const int kSpaceWidth = kDefaultPixelSize / 2;
//
// // Only support horizontal direction.
//   void Advance(int dx) { pos_ += dx; }
//   uint8_t* GetDrawPosition(int row) {
//     uint32_t index =(row * width_ + pos_) * kBytesPerPixel;
//     assert(index < bitmap_.size());
//     return &bitmap_[index];
//   }
//
//   bool CalculateBox(uint32_t codepoint, uint32_t& width, uint32_t& height) {
//     if (!RenderGlyph(codepoint))
//       return false;
//     width += (face_->glyph->advance.x >> 6);
//     height = std::max(
//         height, static_cast<uint32_t>(face_->glyph->metrics.height >> 6));
//     return true;
//   }
//   bool DrawCodepoint(DrawContext& context, uint32_t codepoint) {
//     if (!RenderGlyph(codepoint))
//       return false;
//     printf("U+%08X -> %s\n", codepoint, font_file_.c_str());
//     return DrawBitmap(context, face_->glyph);
//   }
//   int Error() const { return error_; }
//
//   FT_Face Face() const { return face_; }
//
//  private:
//   FreeTypeFace(const FreeTypeFace&) = delete;
//   FreeTypeFace& operator=(const FreeTypeFace&) = delete;
//
//   bool RenderGlyph(uint32_t codepoint) {
//     if (codepoint == 0) return false;
//     if (!face_)
//       return false;
//     error_ = FT_Load_Glyph(face_, codepoint, options_.load_flags);
//     if (error_)
//       return false;
//     error_ = FT_Render_Glyph(face_->glyph, options_.render_mode);
//     if (error_)
//       return false;
//     return true;
//   }

//   bool DrawBitmap(DrawContext& context, FT_GlyphSlot slot) {
//     int pixel_mode = slot->bitmap.pixel_mode;
//     if (pixel_mode == FT_PIXEL_MODE_BGRA)
//       DrawColorBitmap(context, slot);
//     else
//       DrawNormalBitmap(context, slot);
//     context.Advance(slot->advance.x >> 6);
//     return true;
//   }
//   void DrawColorBitmap(DrawContext& context, FT_GlyphSlot slot) {
//     uint8_t* src = slot->bitmap.buffer;
//     // FIXME: Should use metrics for drawing. (e.g. calculate baseline)
//     int yoffset = context.Height() - slot->bitmap.rows;
//     for (int y = 0; y < slot->bitmap.rows; ++y) {
//       uint8_t* dest = context.GetDrawPosition(y + yoffset);
//       for (int x = 0; x < slot->bitmap.width; ++x) {
//         uint8_t b = *src++, g = *src++, r = *src++, a = *src++;
//         *dest++ = r; *dest++ = g; *dest++ = b; *dest++ = a;
//       }
//     }
//   }
//   void DrawNormalBitmap(DrawContext& context, FT_GlyphSlot slot) {
//     uint8_t* src = slot->bitmap.buffer;
//     // FIXME: Same as DrawColorBitmap()
//     int yoffset = context.Height() - slot->bitmap.rows;
//     for (int y = 0; y < slot->bitmap.rows; ++y) {
//       uint8_t* dest = context.GetDrawPosition(y + yoffset);
//       for (int x = 0; x < slot->bitmap.width; ++x) {
//         *dest++ = 255 - *src;
//         *dest++ = 255 - *src;
//         *dest++ = 255 - *src;
//         *dest++ = *src; // Alpha
//         ++src;
//       }
//     }
//   }
//
//   std::string font_file_;
//   FaceOptions options_;
//   FT_Face face_;
//   int error_;
// };
//
// class FontList {
//   typedef std::vector<std::unique_ptr<FreeTypeFace>> FaceList;
//  public:
//   FontList() {}
//
//   void AddFont(const std::string& font_file) {
//     auto face = std::unique_ptr<FreeTypeFace>(new FreeTypeFace(font_file));
//     face_list_.push_back(std::move(face));
//   }
//   void CalculateBox(uint32_t codepoint, uint32_t& width, uint32_t& height) {
//     static const uint32_t kSpace = 0x20;
//     if (codepoint == kSpace) {
//       width += kSpaceWidth;
//     } else {
//       for (auto& face : face_list_) {
//         if (face->CalculateBox(codepoint, width, height))
//           return;
//       }
//     }
//   }
//   void DrawCodepoint(DrawContext& context, uint32_t codepoint) {
//     for (auto& face : face_list_) {
//       if (face->DrawCodepoint(context, codepoint))
//         return;
//     }
//     std::cerr << "Missing glyph for codepoint: " << codepoint << std::endl;
//   }
//
//   const FaceList& GetFaceList() const { return face_list_; }
//
//  private:
//   FontList(const FontList&) = delete;
//   FontList& operator=(const FontList&) = delete;
//   FaceList face_list_;
// };
//












// class PngWriter {
//  public:
//   PngWriter(const std::string& outfile)
//       : outfile_(outfile), png_(nullptr), info_(nullptr)
//   {
//     fp_ = fopen(outfile_.c_str(), "wb");
//     if (!fp_) {
//       std::cerr << "Failed to open: " << outfile_ << std::endl;
//       Cleanup();
//       return;
//     }
//     png_ = png_create_write_struct(
//         PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
//     if (!png_) {
//       std::cerr << "Failed to create PNG file" << std::endl;
//       Cleanup();
//       return;
//     }
//     info_ = png_create_info_struct(png_);
//     if (!info_) {
//       std::cerr << "Failed to create PNG file" << std::endl;
//       Cleanup();
//       return;
//     }
//   }
//   ~PngWriter() { Cleanup(); }
//   bool Write(uint8_t* rgba, int width, int height) {
//     static const int kDepth = 8;
//     if (!png_) {
//       std::cerr << "Writer is not initialized" << std::endl;
//       return false;
//     }
//     if (setjmp(png_jmpbuf(png_))) {
//       std::cerr << "Failed to write PNG" << std::endl;
//       Cleanup();
//       return false;
//     }
//     png_set_IHDR(png_, info_, width, height, kDepth,
//                  PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
//                  PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
//     png_init_io(png_, fp_);
//     png_byte** row_pointers =
//         static_cast<png_byte**>(png_malloc(png_, height * sizeof(png_byte*)));
//     uint8_t* src = rgba;
//     for (int y = 0; y < height; ++y) {
//       png_byte* row =
//           static_cast<png_byte*>(png_malloc(png_, width * kBytesPerPixel));
//       row_pointers[y] = row;
//       for (int x = 0; x < width; ++x) {
//         *row++ = *src++;
//         *row++ = *src++;
//         *row++ = *src++;
//         *row++ = *src++;
//       }
//       assert(row - row_pointers[y] == width * kBytesPerPixel);
//     }
//     assert(src - rgba == width * height * kBytesPerPixel);
//     png_set_rows(png_, info_, row_pointers);
//     png_write_png(png_, info_, PNG_TRANSFORM_IDENTITY, 0);
//     for (int y = 0; y < height; y++)
//       png_free(png_, row_pointers[y]);
//     png_free(png_, row_pointers);
//     Cleanup();
//     return true;
//   }
//  private:
//   PngWriter(const PngWriter&) = delete;
//   PngWriter operator=(const PngWriter&) = delete;
//   void Cleanup() {
//     if (fp_) { fclose(fp_); }
//     if (png_) png_destroy_write_struct(&png_, &info_);
//     fp_ = nullptr; png_ = nullptr; info_ = nullptr;
//   }
//
//   std::string outfile_;
//   FILE* fp_;
//   png_structp png_;
//   png_infop info_;
//   char* rgba_;
//   uint32_t width_;
//   uint32_t height_;
// };
//


//     hb_font_t *font = hb_ft_font_create(font_list_.GetFaceList()[0]->Face(), NULL);
//     hb_buffer_t *buf = hb_buffer_create();
//     hb_buffer_set_unicode_funcs(buf, hb_icu_get_unicode_funcs());
//     hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
//
//     hb_buffer_add_utf8(buf, text, strlen(text), 0, strlen(text));
//     hb_shape(font, buf, NULL, 0);
//
//     unsigned int glyph_count;
//     hb_glyph_info_t *glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
//
//     printf("%d", glyph_count);
//     for (int i=0; i < glyph_count; ++i) {
//       codepoints_.push_back(glyph_info[i].codepoint);
//     }
//     return true;
//   }
//   void CalculateImageSize() {
//     uint32_t width = 0, height = 0;
//     for (auto c : codepoints_)
//       font_list_.CalculateBox(c, width, height);
//     printf("width: %u, height: %u\n", width, height);
//     draw_context_.SetSize(width, height);
//   }
//   void Draw() {
//     for (auto c : codepoints_)
//       font_list_.DrawCodepoint(draw_context_, c);
//   }
//   bool Output() {
//     PngWriter writer(kDefaultOutputFile);
//     return writer.Write(draw_context_.Bitmap(),
//                         draw_context_.Width(),
//                         draw_context_.Height());
//   }
//
//   std::vector<uint32_t> codepoints_;
//   FontList font_list_;
//   DrawContext draw_context_;
// };
