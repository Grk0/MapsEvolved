#ifndef ODM__UTIL_H
#define ODM__UTIL_H

#include <string>
#include <memory>


#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&)


template <typename T>
class ArrayDeleter {
    public:
        void operator () (T* d) const {
            delete [] d;
        }
};


template <typename T>
class FreeDeleter {
    public:
        void operator () (T* d) const {
            if (d) free(d);
        }
};

int round_to_neg_inf(double value);
int round_to_neg_inf(int value, int round_to);
int round_to_neg_inf(double value, int round_to);
int round_to_int(double r);

inline double lerp(double factor, double v1, double v2) {
    return v1 + factor * (v2 - v1);
}


template <typename T>
T ValueBetween(T v_min, T value, T v_max) {
    if (value < v_min) return v_min;
    if (value > v_max) return v_max;
    return value;
}

inline bool IsInRect(double x, double y, double width, double height) {
    return x >= 0 && y >= 0 && x < width && y < height;
}

bool ends_with(const std::wstring &fullString, const std::wstring &ending);
std::string UTF8FromWString(const std::wstring &string);
std::wstring WStringFromUTF8(const std::string &string);

// Array size helper and macro
template <size_t N> struct ArraySizeHelper { char _[N]; };
template <typename T, size_t N>
ArraySizeHelper<N> makeArraySizeHelper(T(&)[N]);
#define ARRAY_SIZE(a)  sizeof(makeArraySizeHelper(a))

inline
unsigned int makeRGB(unsigned char r, unsigned char g,
                     unsigned char b, unsigned int a = 0)
{
    return r | (g << 8) | (b << 16) | (a << 24);
}

inline unsigned char extractR(unsigned int pix) { return pix >> 0; }
inline unsigned char extractG(unsigned int pix) { return pix >> 8; }
inline unsigned char extractB(unsigned int pix) { return pix >> 16; }
inline unsigned char extractA(unsigned int pix) { return pix >> 24; }

unsigned int HSV_to_RGB(unsigned char H, unsigned char S, unsigned char V);


#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

static const int FULL_CIRCLE = 360;
static const double RAD_to_DEG = 57.29577951308232;
static const double DEG_to_RAD = .0174532925199432958;
static const double INCH_to_MM = 25.4;
static const double MM_to_INCH = 0.0393700787;

// return degrees normalized to [0.0, 360.0)
double normalize_direction(double degrees);

// 0.3 -> N; 114.2 -> ESE; 174.0 -> S; 348 -> NNW; 349 -> N
std::wstring CompassPointFromDirection(double degrees);

std::wstring string_format(const std::wstring fmt, ...);

void ShrinkImage(unsigned int *src,
                 unsigned int s_width, unsigned int s_height,
                 unsigned int *dest,
                 unsigned int d_x, unsigned int d_y,
                 unsigned int d_width, unsigned int d_height,
                 unsigned int scale);


struct BasicBitmap {
    unsigned int width, height, bpp;
    std::shared_ptr<unsigned int> pixels;
};
BasicBitmap LoadBufferFromBMP(const std::wstring &fname);
void SaveBufferAsBMP(const std::wstring &fname, void *buffer,
                     unsigned int width, unsigned int height,
                     unsigned int bpp);

std::wstring GetProgramPath_wchar();
std::string GetProgramPath_char();
std::wstring GetProgramDir_wchar();
std::string GetProgramDir_char();

extern const char *ODM_PathSep_char;
extern const wchar_t *ODM_PathSep_wchar;

#endif
