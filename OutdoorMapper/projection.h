#ifndef ODM__PROJECTION_H
#define ODM__PROJECTION_H

#include <memory>

#include "util.h"

class Projection {
    public:
        explicit Projection(const std::string &proj_str);

        void PCSToLatLong(double &x, double &y) const;
        void LatLongToPCS(double &x, double &y) const;
        const std::string &GetProjString() const { return m_proj_str; };

        double CalcDistance(double lat1, double long1,
                            double lat2, double long2) const;

        operator bool() const { return !!m_proj; }
    private:
        std::shared_ptr<class ProjWrap> m_proj;
        std::string m_proj_str;
};


#endif
