#include "util.h"

#include <algorithm>
#include <cmath>
#include <cassert>
#include <climits>
#include <string>
#include <locale>
#include <codecvt>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <map>

#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>

#include <boost/timer/timer.hpp>

#include "odm_config.h"

int round_to_neg_inf(int value, int round_to) {
    if (value >= 0) {
        return (value / round_to) * round_to;
    } else {
        return ((value - round_to + 1) / round_to) * round_to;
    }
}

int round_to_neg_inf(double value) {
    double floorval = std::floor(value);
    assert(INT_MIN <= floorval && floorval <= INT_MAX);
    return (int)floorval;
}

int round_to_neg_inf(double value, int round_to) {
    int intval = (int)round_to_neg_inf(value);
    return round_to_neg_inf(intval, round_to);
}

int round_to_int(double r) {
    return static_cast<int>((r > 0.0) ? floor(r + 0.5) : ceil(r - 0.5));
}

bool starts_with(const std::wstring &fullString, const std::wstring &start) {
    if (fullString.length() >= start.length()) {
        return 0 == fullString.compare(0, start.length(), start);
    } else {
        return false;
    }
}
bool ends_with(const std::wstring &fullString, const std::wstring &ending) {
    if (fullString.length() >= ending.length()) {
        return 0 == fullString.compare(fullString.length() - ending.length(),
                                       ending.length(), ending);
    } else {
        return false;
    }
}

void replace_all(std::wstring& str,
                 const std::wstring& from,
                 const std::wstring& to)
{
    if(from.empty())
        return;
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        // Skip over 'to' in case it contains 'from'.
        start_pos += to.length();
    }
}

std::wstring url_encode(const std::wstring &value) {
    // https://stackoverflow.com/questions/154536
    // We leave " " (0x20, SPACE) unescaped, that's OK for our purposes.
    std::wostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (auto it = value.begin(); it != value.end(); ++it) {
        auto c = *it;
        if (iswalnum(c) ||
            c == '-' || c == '_' || c == '.' || c == '~' || c == ' ' ||
            c == ':' || c == '/' || c == '\\')
        {
            escaped << c;
        } else {
            escaped << '%' << std::setw(2) << ((int) c) << std::setw(0);
        }
    }

    return escaped.str();
}

std::wstring url_decode(const std::wstring &value) {
    std::wostringstream result;

    for (auto it = value.begin(); it != value.end(); ++it) {
        auto c = *it;
        if (c != L'%') {
            result << c;
            continue;
        }
        std::wstring hex(it + 1, it + 3);
        result << static_cast<wchar_t>(std::stoi(hex, 0, 16));
        it += 2;
    }

    return result.str();
}

// std::codecvt_byname doesn't have a public dtor.
// Derive from it and make the dtor public to actually make it useful.
// Cf. http://stackoverflow.com/a/7599989
class UsableCodecvt
    : public std::codecvt_byname<wchar_t, char, std::mbstate_t>
{
    public:
        explicit UsableCodecvt(const char* name="")
        : std::codecvt_byname<wchar_t, char, std::mbstate_t>(name)
        {};
        // Make destructor public to work with wstring_convert.
        virtual ~UsableCodecvt() {}
};

typedef std::wstring_convert<UsableCodecvt> UsableConvert;

std::string StringFromWString(const std::wstring &string,
                              const char* encoding)
{
    auto encoder = UsableConvert(new UsableCodecvt(encoding));
    return encoder.to_bytes(string);
}

std::wstring WStringFromString(const std::string &string,
                               const char* encoding)
{
    auto encoder = UsableConvert(new UsableCodecvt(encoding));
    return encoder.from_bytes(string);
}


unsigned int HSV_to_RGB(unsigned char H, unsigned char S, unsigned char V) {
    unsigned char hi = H * 6 / 255;
    unsigned char f = H * 6 - hi * 255;
    unsigned char p = V - V*S / 255;
    unsigned char q = V - V*S*f / (255*255);
    unsigned char t = V - V*S / 255 + V*S*f / (255*255);

    switch (hi) {
        case 0: // fallthrough
        case 6:
            return makeRGB(V, t, p);
        case 1:
            return makeRGB(q, V, p);
        case 2:
            return makeRGB(p, V, t);
        case 3:
            return makeRGB(p, q, V);
        case 4:
            return makeRGB(t, p, V);
        case 5:
            return makeRGB(V, p, q);
    }
    assert(false);
    return 0;
}

double normalize_direction(double degrees) {
    degrees = fmod(degrees + FULL_CIRCLE, FULL_CIRCLE);
    if (degrees == FULL_CIRCLE)
        degrees = 0.0;

    assert(degrees >= 0 && degrees < FULL_CIRCLE);
    return degrees;
}

std::wstring CompassPointFromDirection(double degree) {
    static const double AMOUNT = FULL_CIRCLE / 32;
    degree = normalize_direction(degree);

    if (degree <=  1 * AMOUNT) return L"N";
    if (degree <=  3 * AMOUNT) return L"NNE";
    if (degree <=  5 * AMOUNT) return L"NE";
    if (degree <=  7 * AMOUNT) return L"ENE";
    if (degree <=  9 * AMOUNT) return L"E";
    if (degree <= 11 * AMOUNT) return L"ESE";
    if (degree <= 13 * AMOUNT) return L"SE";
    if (degree <= 15 * AMOUNT) return L"SSE";
    if (degree <= 17 * AMOUNT) return L"S";
    if (degree <= 19 * AMOUNT) return L"SSW";
    if (degree <= 21 * AMOUNT) return L"SW";
    if (degree <= 23 * AMOUNT) return L"WSW";
    if (degree <= 25 * AMOUNT) return L"W";
    if (degree <= 27 * AMOUNT) return L"WNW";
    if (degree <= 29 * AMOUNT) return L"NW";
    if (degree <= 31 * AMOUNT) return L"NNW";
    return L"N";
}

// From: http://stackoverflow.com/a/8098080/25097
// See the comments for the answer, too
std::string string_format(const std::string fmt, ...) {
    int size = 256;
    va_list ap;
    while (1) {
        std::unique_ptr<char[]> buf(new char[size]);

        va_start(ap, fmt);
        int n = _vsnprintf_s(buf.get(), size, _TRUNCATE, fmt.c_str(), ap);
        va_end(ap);
        if (n <= -1) {
            // Not enough memory available
            size *= 2;
        } else if (n < size) {
            // Complete string written, success
            return std::string(buf.get());
        } else {
            // String written except for final \0; make space for that
            size = n + 1;
        }
    }
}
std::wstring string_format(const std::wstring fmt, ...) {
    int size = 256;
    va_list ap;
    while (1) {
        std::unique_ptr<wchar_t[]> buf(new wchar_t[size]);

        va_start(ap, fmt);
        int n = _vsnwprintf_s(buf.get(), size, _TRUNCATE, fmt.c_str(), ap);
        va_end(ap);
        if (n <= -1) {
            // Not enough memory available
            size *= 2;
        } else if (n < size) {
            // Complete string written, success
            return std::wstring(buf.get());
        } else {
            // String written except for final \0; make space for that
            size = n + 1;
        }
    }
}


#define SRC(xx, yy) src[(xx) + s_width * (yy)]
#define DEST(xx, yy) dest[(xx + d_x) + s_width * (yy + d_y)]
inline unsigned int sample_pixels(unsigned int *src,
                                  unsigned int s_x, unsigned int s_y,
                                  unsigned int s_width, unsigned int s_height,
                                  unsigned int scale)
{
    unsigned int sum_r = 0;
    unsigned int sum_g = 0;
    unsigned int sum_b = 0;
    unsigned int sum_a = 0;
    for (unsigned int y=0; y < scale; y++) {
        for (unsigned int x=0; x < scale; x++) {
            sum_r += extractR(SRC(s_x + x, s_y + y));
            sum_g += extractG(SRC(s_x + x, s_y + y));
            sum_b += extractB(SRC(s_x + x, s_y + y));
            sum_a += extractA(SRC(s_x + x, s_y + y));
        }
    }
    unsigned int num_pixels = scale * scale;
    return makeRGB(sum_r / num_pixels, sum_g / num_pixels,
                   sum_b / num_pixels, sum_a / num_pixels);
}

void ShrinkImage(unsigned int *src,
                 unsigned int s_width, unsigned int s_height,
                 unsigned int *dest,
                 unsigned int d_x, unsigned int d_y,
                 unsigned int d_width, unsigned int d_height,
                 unsigned int scale)
{
    assert(s_width % scale == 0);
    assert(s_height % scale == 0);

    for (unsigned int y = 0; y < s_height / scale; y++) {
        for (unsigned int x = 0; x < s_width / scale; x++) {
            DEST(x, y) = sample_pixels(src, x*scale, y*scale,
                                       s_width, s_height, scale);
        }
    }
}
#undef SRC
#undef DEST


PACKED_STRUCT(
    struct BFH {
            int16_t    bfType;
            uint32_t   bfSize;
            int16_t    bfReserved1;
            int16_t    bfReserved2;
            uint32_t   bfOffBits;
    };
);

struct BIH {
        uint32_t      biSize;
        int32_t       biWidth;
        int32_t       biHeight;
        int16_t       biPlanes;
        int16_t       biBitCount;
        uint32_t      biCompression;
        uint32_t      biSizeImage;
        int32_t       biXPelsPerMeter;
        int32_t       biYPelsPerMeter;
        uint32_t      biClrUsed;
        uint32_t      biClrImportant;
};

static const unsigned long int BIH_COMPRESSION_RGB = 0L;

BasicBitmap LoadBufferFromBMP(const std::wstring &fname) {
    BFH bfh;
    BIH bih;
    std::ifstream file(fname, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Error opening BMP file for reading.");
    }
    file.read(reinterpret_cast<char*>(&bfh), sizeof(bfh));
    file.read(reinterpret_cast<char*>(&bih), sizeof(bih));
    if (bih.biBitCount != 32 || bih.biCompression != BIH_COMPRESSION_RGB) {
        throw std::runtime_error("Format of  the BMP file is not supported.");
    }

    std::shared_ptr<unsigned int> buffer(
            new unsigned int[bih.biWidth * bih.biHeight],
            ArrayDeleter<unsigned int>());
    file.read(reinterpret_cast<char*>(buffer.get()),
              bih.biWidth * bih.biHeight * bih.biBitCount / 8);
    BasicBitmap res = { bih.biWidth, bih.biHeight, bih.biBitCount, buffer };
    return res;
}

void SaveBufferAsBMP(const std::wstring &fname, void *buffer,
                     unsigned int width, unsigned int height, unsigned int bpp)
{
    using std::ios;
    static const unsigned short int BMP_MAGIC = 0x4D42;
    static const unsigned int SZ_BMP_HDR = sizeof(BFH) +
                                           sizeof(BIH);
    BFH bfh = { BMP_MAGIC, SZ_BMP_HDR + width * height * bpp / 8,
                0, 0, SZ_BMP_HDR };
    BIH bih = { sizeof(BIH),
                width, height, 1, bpp, BIH_COMPRESSION_RGB, 0, 0, 0, 0, 0 };
    std::ofstream file(fname, ios::out | ios::trunc | ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Error opening BMP file for writing.");
    }
    file.write(reinterpret_cast<char*>(&bfh), sizeof(bfh));
    file.write(reinterpret_cast<char*>(&bih), sizeof(bih));
    file.write(reinterpret_cast<char*>(buffer), width * height * bpp / 8);
    file.close();
}


TimerStats::TimerStats(std::ostream &ostream, bool report_on_delete)
    : m_timermap(), m_ostream(ostream), m_report_on_delete(report_on_delete)
{}

TimerStats::~TimerStats() {
    if (m_report_on_delete) {
        report();
    }
}

void TimerStats::add_sample(const std::string &group, int64_t val) {
    auto &values = m_timermap[group];
    values.push_back(val);
}

void TimerStats::report() {
    for (auto it = m_timermap.begin(); it != m_timermap.end(); ++it) {
        auto &group = it->first;
        auto &values = it->second;

        std::sort(values.begin(), values.end());
        auto median_ns =   values[values.size() * 50 / 500];
        auto quantile_ns = values[values.size() * 95 / 100];
        auto max_ns = values.back();

        m_ostream << group
                  << " (" << values.size() << " samples)" << std::endl;
        output("  Median:    ", median_ns);
        output("  95% Quant: ", quantile_ns);
        output("  Max:       ", max_ns);
    }
}

void TimerStats::output(const std::string &caption, int64_t value_ns) {
    auto value_us = value_ns / 1000;
    m_ostream << caption << std::setw(6) << value_us << " us" << std::endl;
}


/** A global, static `TimerStats` class for use by `TimerProfile` */
static TimerStats timerstats(std::cerr, true);

TimerProfile::TimerProfile(const std::string &name)
    : m_timer(new boost::timer::cpu_timer()), m_name(name),
      m_report([name](const std::string &name, int64_t val) {
               timerstats.add_sample(name, val);
      })
{}

TimerProfile::TimerProfile(const std::string &name,
                           const TimerProfile::ReportFunction &report)
    : m_timer(new boost::timer::cpu_timer()), m_name(name), m_report(report)
{}

TimerProfile::~TimerProfile() {
    m_report(m_name, m_timer->elapsed().wall);
}


#include <Windows.h>

#define ODM_GetProgramPath_imp(hmod, char_type, suffix)                   \
    char_type path[MAX_PATH + 1];                                         \
    DWORD chars_written = GetModuleFileName##suffix(hmod, path,           \
                                                    ARRAY_SIZE(path));    \
    if (chars_written == 0 || chars_written >= ARRAY_SIZE(path)) {        \
        throw std::runtime_error("Could not retrieve program path.");     \
    }                                                                     \
                                                                          \
    char_type abspath[MAX_PATH + 1];                                      \
    char_type *p_fname;                                                   \
    chars_written = GetFullPathName##suffix(path, ARRAY_SIZE(abspath),    \
                                            abspath, &p_fname);           \
    if (chars_written == 0 || chars_written >= ARRAY_SIZE(abspath)) {     \
        throw std::runtime_error("Could not get absolute program path."); \
    }                                                                     \

extern HINSTANCE g_hinst;

std::wstring GetProgramPath_wchar() {
    ODM_GetProgramPath_imp(0, wchar_t, W);
    return std::wstring(abspath);
}

std::string GetProgramPath_char() {
    ODM_GetProgramPath_imp(0, char, A);
    return std::string(abspath);
}

std::wstring GetProgramDir_wchar() {
    ODM_GetProgramPath_imp(0, wchar_t, W);
    return std::wstring(abspath, p_fname - abspath);
}

std::string GetProgramDir_char() {
    ODM_GetProgramPath_imp(0, char, A);
    return std::string(abspath, p_fname - abspath);
}

std::wstring GetModulePath_wchar() {
    ODM_GetProgramPath_imp(g_hinst, wchar_t, W);
    return std::wstring(abspath);
}

std::string GetModulePath_char() {
    ODM_GetProgramPath_imp(g_hinst, char, A);
    return std::string(abspath);
}

std::wstring GetModuleDir_wchar() {
    ODM_GetProgramPath_imp(g_hinst, wchar_t, W);
    return std::wstring(abspath, p_fname - abspath);
}

std::string GetModuleDir_char() {
    ODM_GetProgramPath_imp(g_hinst, char, A);
    return std::string(abspath, p_fname - abspath);
}

const char *ODM_PathSep_char = "\\";
const wchar_t *ODM_PathSep_wchar = L"\\";


std::pair<std::wstring, std::wstring>
GetAbsPath(const std::wstring &rel_path) {
    wchar_t abspath[MAX_PATH + 1];
    wchar_t *p_fname;
    DWORD c_written;
    c_written = GetFullPathName(rel_path.c_str(), ARRAY_SIZE(abspath), abspath, &p_fname);
    if (c_written == 0 || c_written >= ARRAY_SIZE(abspath)) {
        throw std::runtime_error("Could not get absolute program path.");
    }
    return std::pair<std::wstring, std::wstring>(
              std::wstring(abspath, p_fname - abspath), std::wstring(p_fname));
}


long long int GetFilesize(const std::wstring &name) {
#if ODM_OS == WINDOWS
    // VS 2010 bug: std::streampos doesn't actually support >2GB files.
    // Cf. http://stackoverflow.com/q/13837810
    __stat64 st;
    if (_wstat64(name.c_str(), &st) != 0)
        throw std::runtime_error("Failed to get file size.");

    return st.st_size;
#else
    std::ifstream ifs(filename, std::ifstream::ate | std::ifstream::binary);
    return ifs.tellg();
#endif
}

bool FileExists(const std::string &name) {
    std::ifstream ifs(name, std::ifstream::in | std::ifstream::binary);
    return !ifs.fail();
}
bool FileExists(const std::wstring &name) {
    std::wifstream ifs(name, std::ifstream::in | std::ifstream::binary);
    return !ifs.fail();
}
