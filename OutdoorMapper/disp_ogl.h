#ifndef ODM__DISP_OGL_H
#define ODM__DISP_OGL_H

#include <list>

#include "util.h"
#include "odm_config.h"
#include "coordinates.h"

class DevContext;
class OGLContext;
class TextureCache;

class EXPORT Display {
    public:
        virtual unsigned int GetDisplayWidth() const = 0;
        virtual unsigned int GetDisplayHeight() const = 0;
        virtual DisplayDelta GetDisplaySize() const = 0;

        virtual void Render(class std::list<class DisplayOrder> &orders) = 0;
        virtual void Resize(unsigned int width, unsigned int height) = 0;
        virtual void ForceRepaint() = 0;
};

EXPORT DisplayCoordCentered CenteredCoordFromDisplay(
    const DisplayCoord& dc, const Display& disp);

EXPORT DisplayCoord DisplayCoordFromCentered(
    const DisplayCoordCentered& dc, const Display& disp);


class EXPORT DispOpenGL : public Display {
    public:
        explicit DispOpenGL(const std::shared_ptr<OGLContext> &ogl_context);

        virtual unsigned int GetDisplayWidth() const;
        virtual unsigned int GetDisplayHeight() const;
        virtual DisplayDelta GetDisplaySize() const;

        virtual void Render(class std::list<class DisplayOrder> &orders);
        virtual void Resize(unsigned int width, unsigned int height);
        virtual void ForceRepaint();

    private:
        DISALLOW_COPY_AND_ASSIGN(DispOpenGL);

        const std::shared_ptr<OGLContext> m_opengl;
        const std::shared_ptr<class TextureCache> m_texcache;
};

class Texture {
    public:
        Texture(unsigned int width, unsigned int height,
                const unsigned int *pixels);
        explicit Texture(const class TileCode& tilecode);
        ~Texture();

        void Activate();
        void Deactivate();

        unsigned int GetWidth() const { return m_width; };
        unsigned int GetHeight() const { return m_height; };

    private:
        DISALLOW_COPY_AND_ASSIGN(Texture);

        void MakeTexture(const unsigned int *pixels);

        unsigned int m_width;
        unsigned int m_height;
        unsigned int m_texhandle;
};

#endif
