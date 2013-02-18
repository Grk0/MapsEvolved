#ifndef ODM__WINWRAP_H
#define ODM__WINWRAP_H

#include <memory>
#include <string>
#include <vector>
#include <functional>

#include <Windows.h>
#include <commctrl.h>

#include "util.h"
#include "odm_config.h"

extern HINSTANCE g_hinst;

class PrinterDC {
    public:
        explicit PrinterDC(HDC hdc) : m_hdc(hdc) {};
        ~PrinterDC() { Cleanup(); };

        void Reset(HDC hdc) { Cleanup(); m_hdc = hdc; };
        HDC Get() const { return m_hdc; };
    private:
        DISALLOW_COPY_AND_ASSIGN(PrinterDC);
        void Cleanup() { if (m_hdc) { DeleteDC(m_hdc); m_hdc = NULL; } };
        HDC m_hdc;
};

class TemporaryWindowDisable {
    public:
        TemporaryWindowDisable(HWND hwnd);
        ~TemporaryWindowDisable();
        void EnableNow();
    private:
        DISALLOW_COPY_AND_ASSIGN(TemporaryWindowDisable);
        HWND m_hwnd;
};

class GlobalMem {
    public:
        explicit GlobalMem(HGLOBAL hglobal) : m_hg(hglobal) {};
        ~GlobalMem() { Cleanup(); };

        void Reset(HGLOBAL hglobal) { Cleanup(); m_hg = hglobal; };
        HGLOBAL Get() const { return m_hg; };
    private:
        DISALLOW_COPY_AND_ASSIGN(GlobalMem);
        void Cleanup() { if (m_hg) { GlobalFree(m_hg); m_hg = NULL; } };
        HGLOBAL m_hg;
};

class DevContext {
    public:
        explicit DevContext(HWND hwnd);
        ~DevContext();

        void SetPixelFormat();
        void ForceRepaint();
        HDC Get() const { return m_hdc; };
    private:
        DISALLOW_COPY_AND_ASSIGN(DevContext);
        HWND m_hwnd;
        HDC m_hdc;
};

class OGLContext {
    public:
        explicit OGLContext(const std::shared_ptr<DevContext> &device);
        ~OGLContext();
        HGLRC Get() const { return m_hglrc; };
        class DevContext *GetDevContext() { return m_device.get(); };
    private:
        DISALLOW_COPY_AND_ASSIGN(OGLContext);
        HGLRC m_hglrc;

        // We keep the shared_ptr around so the DevContext can't be destroyed
        // before the OGLContext
        const std::shared_ptr<DevContext> m_device;
};

class PaintContext {
    public:
        PaintContext(HWND hwnd);
        ~PaintContext();

        HDC GetHDC() const { return m_hdc; };
        PAINTSTRUCT *GetPS() { return &m_ps; };
    private:
        DISALLOW_COPY_AND_ASSIGN(PaintContext);
        HWND m_hwnd;
        HDC m_hdc;
        PAINTSTRUCT m_ps;
};

class ImageList {
    public:
        ImageList(int width, int height, unsigned int flags, int length);
        ~ImageList();
        HIMAGELIST Get() { return m_handle; };
        void AddIcon(const class IconHandle &icon);
    private:
        DISALLOW_COPY_AND_ASSIGN(ImageList);
        HIMAGELIST m_handle;
};

class IconHandle {
    public:
        IconHandle(HINSTANCE hInstance, LPCTSTR lpIconName);
        ~IconHandle();
        HICON Get() const { return m_handle; };
    private:
        DISALLOW_COPY_AND_ASSIGN(IconHandle);
        HICON m_handle;

};

class BitmapHandle {
    public:
        BitmapHandle(HINSTANCE hInstance, LPCTSTR lpIconName);
        ~BitmapHandle();
        HBITMAP Get() { return m_handle; };
    private:
        DISALLOW_COPY_AND_ASSIGN(BitmapHandle);
        HBITMAP m_handle;
};

class Window
{
    public:
        HWND GetHWND() { return m_hwnd; }
    protected:
        virtual LRESULT HandleMessage(
                UINT uMsg, WPARAM wParam, LPARAM lParam);
        virtual void PaintContent(PAINTSTRUCT *pps) { };
        virtual LPCTSTR ClassName() = 0;
        virtual UINT WCStyle() { return 0; };
        virtual BOOL WinRegisterClass(WNDCLASS *pwc)
                { return RegisterClass(pwc); }
        virtual ~Window() { }

        HWND WinCreateWindow(DWORD dwExStyle, LPCTSTR pszName,
                DWORD dwStyle, int x, int y, int cx, int cy,
                HWND hwndParent, HMENU hmenu);
    private:
        void Register();
        void OnPaint();
        void OnPrintClient(HDC hdc);
        static LRESULT CALLBACK s_WndProc(HWND hwnd,
                UINT uMsg, WPARAM wParam, LPARAM lParam);
    protected:
        HWND m_hwnd;
};

POINT GetClientMousePos(HWND hwnd);
HWND FindWindowWithinProcess(const std::wstring &wndClass);
std::wstring LoadResString(UINT uID, HINSTANCE hInst = (HINSTANCE)-1);
const wchar_t *LoadResPWCHAR(UINT uID, HINSTANCE hInst = (HINSTANCE)-1);


class ODM_INTERFACE IPrintAbortHandler {
    public:
        virtual bool PrintAbortCallback(int iCode) = 0;
};

class PrintAbortTicket {
    friend class PrintAbortManager;
    private:
        PrintAbortTicket(class PrintAbortManager *pam, HDC hdc);
        std::shared_ptr<class PrintAbortImpl> m_impl;
};

class PrintAbortManager {
    public:
        static PrintAbortManager *Instance();
        PrintAbortTicket RegisterAbort(HDC hdc, IPrintAbortHandler *pdlg);

    private:
        DISALLOW_COPY_AND_ASSIGN(PrintAbortManager);

        PrintAbortManager();
        static BOOL CALLBACK s_AbortProc(HDC hdcPrn, int iCode);
        void ReturnTicket(HDC hdc);

        HDC m_hdc;
        IPrintAbortHandler *m_pdlg;

    friend class PrintAbortImpl;
};


class PrintDialog : public IPrintAbortHandler {
    public:
        PrintDialog(HWND hwnd_parent, TemporaryWindowDisable *twd,
                    const std::wstring &app_name);
        ~PrintDialog();
        void Close();

        // SetAbortProc callback function/message pump
        virtual bool PrintAbortCallback(int iCode);
    private:
        DISALLOW_COPY_AND_ASSIGN(PrintDialog);

        // Dialog callback functions
        BOOL PrintDlgProc(UINT message, WPARAM wParam, LPARAM lParam);
        static BOOL CALLBACK s_PrintDlgProc(HWND hDlg, UINT message,
                                            WPARAM wParam, LPARAM lParam);

        HWND m_hwnd, m_hwnd_parent;
        TemporaryWindowDisable *m_twd;
        bool m_user_abort;
        const std::wstring m_app_name;
};

class ODM_INTERFACE IPrintClient {
    public:
        virtual bool Print(HDC hdc) = 0;
};

struct PrintOrder {
    std::wstring DocName;
    std::wstring PrintDialogName;
    IPrintClient &print_client;
};
bool Print(HWND hwnd_parent, const struct PrintOrder &order);
bool PageSetupDialog(HWND hwnd_parent);

bool FileExists(const wchar_t* fname);


class ListViewItem {
    public:
        ListViewItem();
        ListViewItem(const LVITEM &lvitem, const std::wstring &text);
        ListViewItem(const ListViewItem &rhs);

        friend void swap(ListViewItem& first, ListViewItem& second) {
            using std::swap; // enable ADL
            swap(first.m_lvitem, second.m_lvitem);
            swap(first.m_text, second.m_text);
        }

        ListViewItem& ListViewItem::operator=(ListViewItem rhs) {
            swap(*this, rhs);
            return *this;
        }

        const LVITEM &GetLVITEM() const { return m_lvitem; }
        const std::wstring &GetText() const { return m_text; }
    private:
        LVITEM m_lvitem;
        std::wstring m_text;
};

ListViewItem ListViewTextItem(const std::wstring &text);
ListViewItem ListViewTextImageItem(const std::wstring &text, int image_id);

class ListViewRow {
    public:
        ListViewRow() : m_vec(), m_color(0), m_want_color(false) {};
        void AddItem(const ListViewItem& item);
        unsigned int GetColor() const { return m_color; }
        unsigned int WantColor() const { return m_want_color; }
        void SetColor(unsigned int color) {
            m_want_color = true;
            m_color = color;
        }

        std::vector<const ListViewItem>::const_iterator cbegin() const {
            return m_vec.cbegin();
        }
        std::vector<const ListViewItem>::const_iterator cend() const {
            return m_vec.cend();
        }
    private:
        std::vector<const ListViewItem> m_vec;
        unsigned int m_color;
        bool m_want_color;
};

class ListViewColumn {
    public:
        ListViewColumn();
        ListViewColumn(const LVCOLUMN &lvcolumn, const std::wstring &text);
        ListViewColumn(const ListViewColumn &rhs);

        friend void swap(ListViewColumn& first, ListViewColumn& second) {
            using std::swap; // enable ADL
            swap(first.m_lvcolumn, second.m_lvcolumn);
            swap(first.m_text, second.m_text);
        }

        ListViewColumn& ListViewColumn::operator=(ListViewColumn rhs) {
            swap(*this, rhs);
            return *this;
        }

        const LVCOLUMN &GetLVCOLUMN() const { return m_lvcolumn; }
        const std::wstring &GetText() const { return m_text; }
    private:
        LVCOLUMN m_lvcolumn;
        std::wstring m_text;

        friend class ListView;
};

struct ListViewEvents {
    std::function<LRESULT(const class ListView& lv, LPNMLISTVIEW pnmlv)> ItemChanged;
    std::function<LRESULT(const class ListView& lv, LPNMITEMACTIVATE pnmlv)> DoubleClick;
};

class ListView {
    public:
        ListView() : m_hwnd(0), m_imagelist(), m_columns() {};
        ~ListView();

        void Create(HWND hwndParent, const RECT &rect);
        void SetImageList(std::unique_ptr<class ImageList> &&imagelist);
        void InsertColumns(int n_columns, const LVCOLUMN columns[]);
        void InsertRow(const ListViewRow &row, int desired_index);
        void RegisterEventHandlers(const ListViewEvents &events);
        bool HandleNotify(WPARAM wParam, LPARAM lParam);

        HWND GetHWND() const { return m_hwnd; };
    private:
        HWND m_hwnd;
        std::unique_ptr<class ImageList> m_imagelist;
        std::vector<ListViewColumn> m_columns;
        std::vector<ListViewRow> m_rows;
        ListViewEvents m_events;

        LRESULT SubClassProc(HWND hwnd, UINT msg,
                             WPARAM wParam, LPARAM lParam);
        static LRESULT CALLBACK s_SubClassProc(HWND hwnd, UINT msg,
                                               WPARAM wParam, LPARAM lParam,
                                               UINT_PTR uId, DWORD_PTR data);

        DISALLOW_COPY_AND_ASSIGN(ListView);
};

#endif
