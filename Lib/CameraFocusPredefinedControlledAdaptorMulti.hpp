#pragma once

#include <kvs/PointRenderer>
#include <kvs/ParticleBasedRenderer>

#include <array>
#include <limits>
#include <cstring>
#include <GL/gl.h> // glReadPixels

namespace InSituVis
{

// =============================================================
// Full Tricubic Hermite implementation (same as your current one)
// =============================================================
namespace FullTriCubic
{
inline int clampi(int v, int lo, int hi) { return std::max(lo, std::min(v, hi)); }
inline float clampf(float v, float lo, float hi) { return std::max(lo, std::min(v, hi)); }

inline float norm3(const kvs::Vec3& v)
{
    return std::sqrt(v.x()*v.x() + v.y()*v.y() + v.z()*v.z());
}

inline bool solve3x3(const float A[3][3], const float b[3], float x[3])
{
    float M[3][4] =
    {
        {A[0][0], A[0][1], A[0][2], b[0]},
        {A[1][0], A[1][1], A[1][2], b[1]},
        {A[2][0], A[2][1], A[2][2], b[2]}
    };

    for (int col=0; col<3; ++col)
    {
        int piv = col;
        float best = std::fabs(M[col][col]);
        for (int r=col+1; r<3; ++r)
        {
            float v = std::fabs(M[r][col]);
            if (v > best) { best=v; piv=r; }
        }
        if (best < 1e-10f) return false;

        if (piv != col)
        {
            for (int c=col; c<4; ++c) std::swap(M[col][c], M[piv][c]);
        }

        const float inv = 1.0f / M[col][col];
        for (int c=col; c<4; ++c) M[col][c] *= inv;

        for (int r=0; r<3; ++r)
        {
            if (r==col) continue;
            const float f = M[r][col];
            for (int c=col; c<4; ++c) M[r][c] -= f * M[col][c];
        }
    }

    x[0]=M[0][3]; x[1]=M[1][3]; x[2]=M[2][3];
    return true;
}

inline void eigenSym3_Jacobi(const float H[3][3], float eig[3], int max_sweeps=30)
{
    float A[3][3] =
    {
        {H[0][0], H[0][1], H[0][2]},
        {H[1][0], H[1][1], H[1][2]},
        {H[2][0], H[2][1], H[2][2]}
    };

    for (int sweep=0; sweep<max_sweeps; ++sweep)
    {
        int p=0,q=1;
        float maxv = std::fabs(A[0][1]);
        auto upd = [&](int i,int j){
            float v = std::fabs(A[i][j]);
            if (v > maxv) { maxv=v; p=i; q=j; }
        };
        upd(0,2); upd(1,2);
        if (maxv < 1e-10f) break;

        const float app=A[p][p], aqq=A[q][q], apq=A[p][q];
        const float tau=(aqq-app)/(2.0f*apq);
        const float t = (tau>=0.0f)
            ? 1.0f/(tau + std::sqrt(1.0f+tau*tau))
            : -1.0f/(-tau + std::sqrt(1.0f+tau*tau));
        const float c = 1.0f/std::sqrt(1.0f+t*t);
        const float s = t*c;

        A[p][p]=app - t*apq;
        A[q][q]=aqq + t*apq;
        A[p][q]=A[q][p]=0.0f;

        for (int r=0;r<3;++r)
        {
            if (r==p||r==q) continue;
            const float arp=A[r][p], arq=A[r][q];
            A[r][p]=A[p][r]=c*arp - s*arq;
            A[r][q]=A[q][r]=c*arq + s*arp;
        }
    }

    eig[0]=A[0][0]; eig[1]=A[1][1]; eig[2]=A[2][2];
    if (eig[0]>eig[1]) std::swap(eig[0],eig[1]);
    if (eig[1]>eig[2]) std::swap(eig[1],eig[2]);
    if (eig[0]>eig[1]) std::swap(eig[0],eig[1]);
}

inline long long hashPos(const kvs::Vec3& p, float q=0.02f)
{
    const int xi=(int)std::floor(p.x()/q);
    const int yi=(int)std::floor(p.y()/q);
    const int zi=(int)std::floor(p.z()/q);
    return ((long long)xi<<42) ^ ((long long)yi<<21) ^ (long long)zi;
}

struct Basis1D
{
    float v[2];
    float d[2];
    float v1[2];
    float d1[2];
    float v2[2];
    float d2[2];
};

inline Basis1D basis1D(float t)
{
    const float t2=t*t;
    const float t3=t2*t;

    Basis1D B;

    const float h00 =  2*t3 - 3*t2 + 1;
    const float h01 = -2*t3 + 3*t2;
    const float h10 =    t3 - 2*t2 + t;
    const float h11 =    t3 -   t2;

    const float dh00 =  6*t2 - 6*t;
    const float dh01 = -6*t2 + 6*t;
    const float dh10 =  3*t2 - 4*t + 1;
    const float dh11 =  3*t2 - 2*t;

    const float d2h00 = 12*t - 6;
    const float d2h01 = -12*t + 6;
    const float d2h10 =  6*t - 4;
    const float d2h11 =  6*t - 2;

    B.v[0]=h00;  B.v[1]=h01;
    B.d[0]=h10;  B.d[1]=h11;

    B.v1[0]=dh00; B.v1[1]=dh01;
    B.d1[0]=dh10; B.d1[1]=dh11;

    B.v2[0]=d2h00; B.v2[1]=d2h01;
    B.d2[0]=d2h10; B.d2[1]=d2h11;

    return B;
}

struct SampleVGH
{
    float v = 0.0f;
    kvs::Vec3 g = kvs::Vec3(0,0,0);
    float H[3][3] = {{0,0,0},{0,0,0},{0,0,0}};
};

enum class CritType { Max, Min, Saddle, Degenerate, NotConverged };

class Sampler
{
public:
    void bind(
        const kvs::ValueArray<float>& f,
        const kvs::ValueArray<float>& fx,
        const kvs::ValueArray<float>& fy,
        const kvs::ValueArray<float>& fz,
        const kvs::ValueArray<float>& fxy,
        const kvs::ValueArray<float>& fxz,
        const kvs::ValueArray<float>& fyz,
        const kvs::ValueArray<float>& fxyz,
        const kvs::Vec3ui& dims )
    {
        m_f=&f; m_fx=&fx; m_fy=&fy; m_fz=&fz;
        m_fxy=&fxy; m_fxz=&fxz; m_fyz=&fyz; m_fxyz=&fxyz;
        m_dims=dims;
    }

    SampleVGH sampleVGH(const kvs::Vec3& pin) const
    {
        SampleVGH out;
        const int nx = (int)m_dims[0], ny = (int)m_dims[1], nz = (int)m_dims[2];

        float x = clampf(pin.x(), 0.0f, float(nx - 1));
        float y = clampf(pin.y(), 0.0f, float(ny - 1));
        float z = clampf(pin.z(), 0.0f, float(nz - 1));

        int i = (int)std::floor(x);
        int j = (int)std::floor(y);
        int k = (int)std::floor(z);
        if (i >= nx - 1) i = nx - 2;
        if (j >= ny - 1) j = ny - 2;
        if (k >= nz - 1) k = nz - 2;

        const float tx = x - (float)i;
        const float ty = y - (float)j;
        const float tz = z - (float)k;

        const auto Bx = basis1D(tx);
        const auto By = basis1D(ty);
        const auto Bz = basis1D(tz);

        float v  = 0.0f, gx = 0.0f, gy = 0.0f, gz = 0.0f;
        float Hxx = 0.0f, Hyy = 0.0f, Hzz = 0.0f, Hxy = 0.0f, Hxz = 0.0f, Hyz = 0.0f;

        for (int cz = 0; cz < 2; ++cz)
        for (int cy = 0; cy < 2; ++cy)
        for (int cx = 0; cx < 2; ++cx)
        {
            const int X = i + cx, Y = j + cy, Z = k + cz;

            const float F    = at(*m_f,    X, Y, Z);
            const float Fx   = at(*m_fx,   X, Y, Z);
            const float Fy   = at(*m_fy,   X, Y, Z);
            const float Fz   = at(*m_fz,   X, Y, Z);
            const float Fxy  = at(*m_fxy,  X, Y, Z);
            const float Fxz  = at(*m_fxz,  X, Y, Z);
            const float Fyz  = at(*m_fyz,  X, Y, Z);
            const float Fxyz = at(*m_fxyz, X, Y, Z);

            const float vx  = Bx.v[cx],   dx  = Bx.d[cx];
            const float vy  = By.v[cy],   dy  = By.d[cy];
            const float vz  = Bz.v[cz],   dz  = Bz.d[cz];

            const float vx1 = Bx.v1[cx],  dx1 = Bx.d1[cx];
            const float vy1 = By.v1[cy],  dy1 = By.d1[cy];
            const float vz1 = Bz.v1[cz],  dz1 = Bz.d1[cz];

            const float vx2 = Bx.v2[cx],  dx2 = Bx.d2[cx];
            const float vy2 = By.v2[cy],  dy2 = By.d2[cy];
            const float vz2 = Bz.v2[cz],  dz2 = Bz.d2[cz];

            // value
            v += vx*vy*vz*F
               + dx*vy*vz*Fx
               + vx*dy*vz*Fy
               + vx*vy*dz*Fz
               + dx*dy*vz*Fxy
               + dx*vy*dz*Fxz
               + vx*dy*dz*Fyz
               + dx*dy*dz*Fxyz;

            // gradient
            gx += vx1*vy*vz*F
               +  dx1*vy*vz*Fx
               +  vx1*dy*vz*Fy
               +  vx1*vy*dz*Fz
               +  dx1*dy*vz*Fxy
               +  dx1*vy*dz*Fxz
               +  vx1*dy*dz*Fyz
               +  dx1*dy*dz*Fxyz;

            gy += vx*vy1*vz*F
               + dx*vy1*vz*Fx
               + vx*dy1*vz*Fy
               + vx*vy1*dz*Fz
               + dx*dy1*vz*Fxy
               + dx*vy1*dz*Fxz
               + vx*dy1*dz*Fyz
               + dx*dy1*dz*Fxyz;

            gz += vx*vy*vz1*F
               + dx*vy*vz1*Fx
               + vx*dy*vz1*Fy
               + vx*vy*dz1*Fz
               + dx*dy*vz1*Fxy
               + dx*vy*dz1*Fxz
               + vx*dy*dz1*Fyz
               + dx*dy*dz1*Fxyz;

            // Hessian
            Hxx += vx2*vy*vz*F
                +  dx2*vy*vz*Fx
                +  vx2*dy*vz*Fy
                +  vx2*vy*dz*Fz
                +  dx2*dy*vz*Fxy
                +  dx2*vy*dz*Fxz
                +  vx2*dy*dz*Fyz
                +  dx2*dy*dz*Fxyz;

            Hyy += vx*vy2*vz*F
                + dx*vy2*vz*Fx
                + vx*dy2*vz*Fy
                + vx*vy2*dz*Fz
                + dx*dy2*vz*Fxy
                + dx*vy2*dz*Fxz
                + vx*dy2*dz*Fyz
                + dx*dy2*dz*Fxyz;

            Hzz += vx*vy*vz2*F
                + dx*vy*vz2*Fx
                + vx*dy*vz2*Fy
                + vx*vy*dz2*Fz
                + dx*dy*vz2*Fxy
                + dx*vy*dz2*Fxz
                + vx*dy*dz2*Fyz
                + dx*dy*dz2*Fxyz;

            Hxy += vx1*vy1*vz*F
                +  dx1*vy1*vz*Fx
                +  vx1*dy1*vz*Fy
                +  vx1*vy1*dz*Fz
                +  dx1*dy1*vz*Fxy
                +  dx1*vy1*dz*Fxz
                +  vx1*dy1*dz*Fyz
                +  dx1*dy1*dz*Fxyz;

            Hxz += vx1*vy*vz1*F
                +  dx1*vy*vz1*Fx
                +  vx1*dy*vz1*Fy
                +  vx1*vy*dz1*Fz
                +  dx1*dy*vz1*Fxy
                +  dx1*vy*dz1*Fxz
                +  vx1*dy*dz1*Fyz
                +  dx1*dy*dz1*Fxyz;

            Hyz += vx*vy1*vz1*F
                + dx*vy1*vz1*Fx
                + vx*dy1*vz1*Fy
                + vx*vy1*dz1*Fz
                + dx*dy1*vz1*Fxy
                + dx*vy1*dz1*Fxz
                + vx*dy1*dz1*Fyz
                + dx*dy1*dz1*Fxyz;
        }

        out.v = v;
        out.g = kvs::Vec3(gx, gy, gz);

        out.H[0][0] = Hxx; out.H[0][1] = Hxy; out.H[0][2] = Hxz;
        out.H[1][0] = Hxy; out.H[1][1] = Hyy; out.H[1][2] = Hyz;
        out.H[2][0] = Hxz; out.H[2][1] = Hyz; out.H[2][2] = Hzz;

        return out;
    }

private:
    const kvs::ValueArray<float>* m_f=nullptr;
    const kvs::ValueArray<float>* m_fx=nullptr;
    const kvs::ValueArray<float>* m_fy=nullptr;
    const kvs::ValueArray<float>* m_fz=nullptr;
    const kvs::ValueArray<float>* m_fxy=nullptr;
    const kvs::ValueArray<float>* m_fxz=nullptr;
    const kvs::ValueArray<float>* m_fyz=nullptr;
    const kvs::ValueArray<float>* m_fxyz=nullptr;
    kvs::Vec3ui m_dims{0,0,0};

    inline float at(const kvs::ValueArray<float>& A, int x,int y,int z) const
    {
        const int nx=(int)m_dims[0], ny=(int)m_dims[1];
        const size_t id = (size_t)x + (size_t)y*(size_t)nx + (size_t)z*(size_t)nx*(size_t)ny;
        return A[id];
    }
};

inline bool findCriticalPointInCell(
    const Sampler& S,
    int i,int j,int k,
    kvs::Vec3& p_out,
    float& v_out,
    CritType& type_out,
    int   max_iters = 25,
    float grad_tol  = 1e-5f,
    float step_tol  = 1e-6f,
    float eig_eps   = 1e-7f )
{
    kvs::Vec3 p(i + 0.5f, j + 0.5f, k + 0.5f);

    const float cell_eps = 1e-6f;

    auto clampClosedToCell = [&](const kvs::Vec3& q)->kvs::Vec3
    {
        kvs::Vec3 r = q;
        const float xmin = (float)i - cell_eps;
        const float xmax = (float)(i + 1) + cell_eps;
        const float ymin = (float)j - cell_eps;
        const float ymax = (float)(j + 1) + cell_eps;
        const float zmin = (float)k - cell_eps;
        const float zmax = (float)(k + 1) + cell_eps;

        r.x() = std::max(xmin, std::min(xmax, r.x()));
        r.y() = std::max(ymin, std::min(ymax, r.y()));
        r.z() = std::max(zmin, std::min(zmax, r.z()));
        return r;
    };

    auto copyH3 = [&](const SampleVGH& s, float H3[3][3])
    {
        for (int a = 0; a < 3; ++a)
        for (int b = 0; b < 3; ++b)
        {
            H3[a][b] = s.H[a][b];
        }
    };

    auto classify = [&](const float H3[3][3])->CritType
    {
        float eig[3];
        eigenSym3_Jacobi(H3, eig);

        const bool pos0 = (eig[0] >  eig_eps);
        const bool pos1 = (eig[1] >  eig_eps);
        const bool pos2 = (eig[2] >  eig_eps);
        const bool neg0 = (eig[0] < -eig_eps);
        const bool neg1 = (eig[1] < -eig_eps);
        const bool neg2 = (eig[2] < -eig_eps);

        if (neg0 && neg1 && neg2) return CritType::Max;
        if (pos0 && pos1 && pos2) return CritType::Min;
        if ((pos0 || neg0) && (pos1 || neg1) && (pos2 || neg2)) return CritType::Saddle;
        return CritType::Degenerate;
    };

    for (int it = 0; it < max_iters; ++it)
    {
        const auto s = S.sampleVGH(p);
        const float gnorm = norm3(s.g);

        if (gnorm < grad_tol)
        {
            float H3[3][3];
            copyH3(s, H3);

            type_out = classify(H3);
            p_out = p;
            v_out = s.v;
            return true;
        }

        const float b[3] = { -s.g.x(), -s.g.y(), -s.g.z() };
        float delta[3] = { 0,0,0 };

        float H3[3][3];
        copyH3(s, H3);

        bool ok = solve3x3(H3, b, delta);

        kvs::Vec3 dir;
        if (ok)
        {
            dir = kvs::Vec3(delta[0], delta[1], delta[2]);
            const float dirn = norm3(dir);
            if (!(dirn > 0.0f) || !std::isfinite(dirn)) ok = false;
        }

        if (!ok)
        {
            const float eta = 0.1f;
            dir = -eta * s.g;
        }

        kvs::Vec3 p_new = p;
        {
            float alpha = 1.0f;
            const float g0 = gnorm;

            for (int ls = 0; ls < 12; ++ls)
            {
                kvs::Vec3 trial = clampClosedToCell(p + alpha * dir);
                const float g1 = norm3(S.sampleVGH(trial).g);

                if (g1 <= g0 * 0.9995f)
                {
                    p_new = trial;
                    break;
                }
                alpha *= 0.5f;
            }

            if (p_new == p)
            {
                kvs::Vec3 trial = clampClosedToCell(p + 1e-3f * dir);
                p_new = trial;
            }
        }

        const float step = norm3(p_new - p);
        p = p_new;

        if (step < step_tol)
        {
            const auto s2 = S.sampleVGH(p);
            const float g2 = norm3(s2.g);

            if (g2 < grad_tol)
            {
                float H32[3][3];
                copyH3(s2, H32);

                type_out = classify(H32);
                p_out = p;
                v_out = s2.v;
                return true;
            }

            type_out = CritType::NotConverged;
            return false;
        }
    }

    type_out = CritType::NotConverged;
    return false;
}



} // namespace FullTriCubic


// =============================================================
// Derivatives: boundary one-sided (same as yours)
// =============================================================
inline void CameraFocusPredefinedControlledAdaptor::computeDerivativesFull(
    const kvs::ValueArray<float>& f,
    const kvs::Vec3ui& dims,
    kvs::ValueArray<float>& fx,
    kvs::ValueArray<float>& fy,
    kvs::ValueArray<float>& fz,
    kvs::ValueArray<float>& fxy,
    kvs::ValueArray<float>& fxz,
    kvs::ValueArray<float>& fyz,
    kvs::ValueArray<float>& fxyz )
{
    const int nx=(int)dims[0], ny=(int)dims[1], nz=(int)dims[2];
    const size_t N=(size_t)nx*ny*nz;

    fx   = kvs::ValueArray<float>( N );
    fy   = kvs::ValueArray<float>( N );
    fz   = kvs::ValueArray<float>( N );
    fxy  = kvs::ValueArray<float>( N );
    fxz  = kvs::ValueArray<float>( N );
    fyz  = kvs::ValueArray<float>( N );
    fxyz = kvs::ValueArray<float>( N );

    auto idx=[&](int x,int y,int z)->size_t{
        return (size_t)x + (size_t)y*(size_t)nx + (size_t)z*(size_t)nx*(size_t)ny;
    };
    auto V=[&](int x,int y,int z)->float{
        return f[idx(x,y,z)];
    };

    for (int z=0; z<nz; ++z)
    for (int y=0; y<ny; ++y)
    for (int x=0; x<nx; ++x)
    {
        const float fxv =
            (x==0)    ? (V(1,y,z) - V(0,y,z)) :
            (x==nx-1) ? (V(nx-1,y,z) - V(nx-2,y,z)) :
                        0.5f*(V(x+1,y,z) - V(x-1,y,z));

        const float fyv =
            (y==0)    ? (V(x,1,z) - V(x,0,z)) :
            (y==ny-1) ? (V(x,ny-1,z) - V(x,ny-2,z)) :
                        0.5f*(V(x,y+1,z) - V(x,y-1,z));

        const float fzv =
            (z==0)    ? (V(x,y,1) - V(x,y,0)) :
            (z==nz-1) ? (V(x,y,nz-1) - V(x,y,nz-2)) :
                        0.5f*(V(x,y,z+1) - V(x,y,z-1));

        fx[idx(x,y,z)] = fxv;
        fy[idx(x,y,z)] = fyv;
        fz[idx(x,y,z)] = fzv;
    }

    for (int z=0; z<nz; ++z)
    for (int y=0; y<ny; ++y)
    for (int x=0; x<nx; ++x)
    {
        if (x==0 || x==nx-1 || y==0 || y==ny-1) fxy[idx(x,y,z)] = 0.0f;
        else fxy[idx(x,y,z)] = 0.25f*( V(x+1,y+1,z) - V(x+1,y-1,z) - V(x-1,y+1,z) + V(x-1,y-1,z) );

        if (x==0 || x==nx-1 || z==0 || z==nz-1) fxz[idx(x,y,z)] = 0.0f;
        else fxz[idx(x,y,z)] = 0.25f*( V(x+1,y,z+1) - V(x+1,y,z-1) - V(x-1,y,z+1) + V(x-1,y,z-1) );

        if (y==0 || y==ny-1 || z==0 || z==nz-1) fyz[idx(x,y,z)] = 0.0f;
        else fyz[idx(x,y,z)] = 0.25f*( V(x,y+1,z+1) - V(x,y+1,z-1) - V(x,y-1,z+1) + V(x,y-1,z-1) );
    }

    for (int z=0; z<nz; ++z)
    for (int y=0; y<ny; ++y)
    for (int x=0; x<nx; ++x)
    {
        if (x==0 || x==nx-1 || y==0 || y==ny-1 || z==0 || z==nz-1)
        {
            fxyz[idx(x,y,z)] = 0.0f;
        }
        else
        {
            const float ppp = V(x+1,y+1,z+1);
            const float ppm = V(x+1,y+1,z-1);
            const float pmp = V(x+1,y-1,z+1);
            const float pmm = V(x+1,y-1,z-1);

            const float mpp = V(x-1,y+1,z+1);
            const float mpm = V(x-1,y+1,z-1);
            const float mmp = V(x-1,y-1,z+1);
            const float mmm = V(x-1,y-1,z-1);

            fxyz[idx(x,y,z)] = 0.125f * ( ppp - ppm - pmp + pmm
                                        - mpp + mpm + mmp - mmm );
        }
    }
}

inline float CameraFocusPredefinedControlledAdaptor::dist3_(
    const kvs::Vec3& a, const kvs::Vec3& b) const
{
    const kvs::Vec3 d = a - b;
    return std::sqrt(d.x()*d.x() + d.y()*d.y() + d.z()*d.z());
}

// =============================================================
// readbackFrameBuffer: BaseClass::readback(color) + glReadPixels(depth)
// =============================================================
inline CameraFocusPredefinedControlledAdaptor::FrameBuffer
CameraFocusPredefinedControlledAdaptor::readbackFrameBuffer( const Location& location )
{
    FrameBuffer fb;
    fb.width  = BaseClass::imageWidth();
    fb.height = BaseClass::imageHeight();

    // color buffer (RGBA)
    fb.color_buffer = BaseClass::readback( location );

    // depth buffer
    fb.depth_buffer.allocate( fb.width * fb.height );
    ::glReadPixels(
        0, 0,
        (GLsizei)fb.width, (GLsizei)fb.height,
        GL_DEPTH_COMPONENT, GL_FLOAT,
        fb.depth_buffer.data()
    );

    return fb;
}


// =============================================================
// entropyRGBA
// =============================================================
inline float CameraFocusPredefinedControlledAdaptor::entropyRGBA(
    const kvs::ValueArray<kvs::UInt8>& rgba, size_t w, size_t h ) const
{
    if (rgba.size() < w*h*4) return 0.0f;
    const size_t bins = m_entropy_bins;
    std::vector<size_t> hist(bins, 0);

    const size_t n = w*h;
    for (size_t i = 0; i < n; ++i)
    {
        const size_t o = i*4;
        const float r = (float)rgba[o+0];
        const float g = (float)rgba[o+1];
        const float b = (float)rgba[o+2];
        const float yv = 0.299f*r + 0.587f*g + 0.114f*b; // 0..255

        size_t bi = (size_t)std::floor((yv/255.0f) * (float)(bins-1));
        if (bi >= bins) bi = bins-1;
        hist[bi]++;
    }

    const double N = (double)n;
    double H = 0.0;
    for (size_t k = 0; k < bins; ++k)
    {
        if (hist[k] == 0) continue;
        const double p = (double)hist[k] / N;
        H -= p * std::log2(p);
    }
    return (float)H;
}


// =============================================================
// cropFrameBuffer (tile)
// =============================================================
inline CameraFocusPredefinedControlledAdaptor::FrameBuffer
CameraFocusPredefinedControlledAdaptor::cropFrameBuffer(
    const FrameBuffer& fb, const kvs::Vec2i& ij ) const
{
    const size_t w = fb.width;
    const size_t h = fb.height;

    const size_t cw = w / m_frame_divs.x() + 1;
    const size_t ch = h / m_frame_divs.y() + 1;

    const size_t ox = (size_t)ij.x() * cw;
    const size_t oy = (size_t)ij.y() * ch;

    const size_t aw = (ox + cw <= w) ? cw : (w - ox);
    const size_t ah = (oy + ch <= h) ? ch : (h - oy);

    FrameBuffer out;
    out.width = aw;
    out.height = ah;
    out.color_buffer.allocate(aw*ah*4);
    out.depth_buffer.allocate(aw*ah);

    for (size_t y = 0; y < ah; ++y)
    {
        const size_t src_row = (oy + y) * w + ox;
        const size_t dst_row = y * aw;

        std::memcpy(out.color_buffer.data() + dst_row*4,
                    fb.color_buffer.data() + src_row*4,
                    aw*4*sizeof(kvs::UInt8));

        std::memcpy(out.depth_buffer.data() + dst_row,
                    fb.depth_buffer.data() + src_row,
                    aw*sizeof(kvs::Real32));
    }

    return out;
}


// =============================================================
// look_at_in_window: max entropy tile center + min depth in that tile
// =============================================================
inline kvs::Vec3 CameraFocusPredefinedControlledAdaptor::look_at_in_window( const FrameBuffer& fb ) const
{
    const size_t w = fb.width;
    const size_t h = fb.height;

    const size_t cw = w / m_frame_divs.x() + 1;
    const size_t ch = h / m_frame_divs.y() + 1;

    float bestH = -1.0f;
    kvs::Vec2i best_ij(0,0);

    for (size_t j = 0; j < m_frame_divs.y(); ++j)
    for (size_t i = 0; i < m_frame_divs.x(); ++i)
    {
        const auto tile = cropFrameBuffer(fb, kvs::Vec2i((int)i,(int)j));
        const float H = entropyRGBA(tile.color_buffer, tile.width, tile.height);
        if (H > bestH)
        {
            bestH = H;
            best_ij = kvs::Vec2i((int)i,(int)j);
        }
    }

    const auto tile = cropFrameBuffer(fb, best_ij);

    // depth: use min (<1.0) in tile
    float mind = 1.0f;
    for (size_t k = 0; k < tile.depth_buffer.size(); ++k)
    {
        const float d = tile.depth_buffer[k];
        if (d < 1.0f) mind = kvs::Math::Min(mind, d);
    }

    const int cx = (int)(best_ij.x() * (int)cw + (int)(cw*0.5));
    const int cy = (int)(best_ij.y() * (int)ch + (int)(ch*0.5));

    // Window Y: OpenGL origin bottom-left
    return kvs::Vec3((float)cx, (float)cy, mind);
}


// =============================================================
// window_to_object: UnProject (same spirit as yours)
// =============================================================
inline kvs::Vec3 CameraFocusPredefinedControlledAdaptor::window_to_object(
    const kvs::Vec3 win,
    const Location& location )
{
    auto* manager = this->screen().scene()->objectManager();
    auto* camera  = this->screen().scene()->camera();

    const auto p0 = camera->position();
    const auto a0 = camera->lookAt();
    const auto u0 = camera->upVector();

    camera->setPosition( location.position, location.look_at, location.up_vector );

    const auto xv = kvs::Xform( camera->viewingMatrix() );
    const auto xp = kvs::Xform( camera->projectionMatrix() );
    const auto xo = manager->xform();
    const auto xm = xv * xo;

    camera->setPosition( p0, a0, u0 );

    auto x_to_a = [] ( const kvs::Xform& x, double a[16] )
    {
        const auto m = x.toMatrix();
        a[0] = m[0][0]; a[4] = m[0][1]; a[8]  = m[0][2]; a[12] = m[0][3];
        a[1] = m[1][0]; a[5] = m[1][1]; a[9]  = m[1][2]; a[13] = m[1][3];
        a[2] = m[2][0]; a[6] = m[2][1]; a[10] = m[2][2]; a[14] = m[2][3];
        a[3] = m[3][0]; a[7] = m[3][1]; a[11] = m[3][2]; a[15] = m[3][3];
    };

    double m[16]; x_to_a( xm, m );
    double p[16]; x_to_a( xp, p );
    int v[4]; kvs::OpenGL::GetViewport( v );

    kvs::Vec3d obj( 0.0, 0.0, 0.0 );
    kvs::OpenGL::UnProject(
        win.x(), win.y(), win.z(), m, p, v,
        &obj[0], &obj[1], &obj[2] );

    return kvs::Vec3( obj );
}


// =============================================================
// output filename
// =============================================================
inline std::string CameraFocusPredefinedControlledAdaptor::outputFinalImageName(
    const size_t level, const size_t cid )
{
    const auto time = BaseClass::timeStep();
    const auto output_time  = kvs::String::From( time, 6, '0' );

    std::string output_basename;
    if (m_focus_mode == FocusMode::Extrema) output_basename = "extrema";
    else if (m_focus_mode == FocusMode::ImageEntropy) output_basename = "entropy";
    else output_basename = "output";

    const auto output_level = kvs::String::From( level, 6, '0' );
    const auto output_cand  = kvs::String::From( cid,   6, '0' );

    const auto output_filename =
        output_basename + "_" + output_time + "_" + output_level + "_cand" + output_cand;

    return BaseClass::outputDirectory().baseDirectoryName() + "/" + output_filename + ".bmp";
}


inline std::string CameraFocusPredefinedControlledAdaptor::outputInterpImageName(
    const size_t prev_id, const size_t curr_id, const size_t s_id ) 
{
    const auto time = BaseClass::timeStep();
    char buf[512];
    std::snprintf(
        buf, sizeof(buf),
        "extrema_%06zu_000000_pairP%06zu_C%06zu_s%06zu.bmp",
        (size_t)time, prev_id, curr_id, s_id );

    return BaseClass::outputDirectory().baseDirectoryName() + "/" +  std::string(buf);
}



inline void CameraFocusPredefinedControlledAdaptor::appendParamsCSVRow(
    const std::string& image_relpath,
    float pressure_value,
    const std::vector<float>& fp_from_prev,
    const std::vector<float>& cp_from_prev )
{
    if (!m_enable_output_params_csv) return;

    if (m_params_csv_path.empty())
    {
        m_params_csv_path =
            BaseClass::outputDirectory().baseDirectoryName() + "/output_video_params.csv";
    }

    std::ofstream ofs(m_params_csv_path, std::ios::out | std::ios::app);
    if (!ofs.is_open())
    {
        std::cerr << "[CSV] cannot open: " << m_params_csv_path << "\n";
        return;
    }

    if (!m_params_csv_header_written)
    {
        ofs << "filename,pressure";
        for (size_t i=0;i<m_focus_top_n;++i) ofs << ",pFP_from" << i;
        for (size_t i=0;i<m_focus_top_n;++i) ofs << ",pCP_from" << i;
        ofs << "\n";
        m_params_csv_header_written = true;
    }

    ofs << image_relpath << "," << pressure_value;
    for (float v: fp_from_prev) ofs << "," << v;
    for (float v: cp_from_prev) ofs << "," << v;
    ofs << "\n";
}


inline void CameraFocusPredefinedControlledAdaptor::updateCSVAndPrevCache_FP_CP(
    const std::vector<kvs::Vec3>& curr_focus_worlds,
    const std::vector<kvs::Vec3>& curr_camera_worlds,
    const std::vector<float>& curr_pressures )
{
    if (!m_enable_output_params_csv) return;

    

    const int cand = (int)curr_focus_worlds.size();
    if (cand <= 0) return;
    if ((int)curr_pressures.size() != cand) return;
    if ((int)curr_camera_worlds.size() != cand) return;

    if(BaseClass::timeStep() == 0){
        for (int to = 0; to < cand; ++to)
        {
            const std::string absname = this->outputFinalImageName(1, (size_t)to);
            const size_t pos = absname.find_last_of("/\\");
            const std::string base = (pos == std::string::npos) ? absname : absname.substr(pos + 1);
            const std::string rel  = std::string("Output/") + base;

            std::vector<float> empty; // ★空

            this->appendParamsCSVRow(rel, curr_pressures[to], empty, empty);
        }
    }
    else{
        for (int to = 0; to < cand; ++to)
        {
            std::vector<float> fp(cand, 0.0f);
            std::vector<float> cp(cand, 0.0f);

            if (m_has_prev_focus &&
                (int)m_prev_focus_worlds.size()  == cand &&
                (int)m_prev_camera_worlds.size() == cand)
            {
                for (int from = 0; from < cand; ++from)
                {
                    fp[from] = dist3_(curr_focus_worlds[to],  m_prev_focus_worlds[from]);
                    cp[from] = dist3_(curr_camera_worlds[to], m_prev_camera_worlds[from]); // ★pCP
                }
            }

            const std::string absname = this->outputFinalImageName(1, (size_t)to);
            const size_t pos = absname.find_last_of("/\\");
            const std::string base = (pos == std::string::npos) ? absname : absname.substr(pos + 1);
            const std::string rel  = std::string("Output/") + base;

            this->appendParamsCSVRow(rel, curr_pressures[to], fp, cp);
        }
    }
    

    // prev 更新
    m_prev_focus_worlds  = curr_focus_worlds;
    m_prev_camera_worlds = curr_camera_worlds;
    m_has_prev_focus = true;
}


// =============================================================
// execRendering (switch)
//  - ImageEntropy: CreateExtremaObject は絶対呼ばれない
// =============================================================
inline void CameraFocusPredefinedControlledAdaptor::execRendering()
{
    float rend_time = 0.0f;
    float save_time = 0.0f;

    kvs::Timer tr, ts;

    for (const auto& location : BaseClass::viewpoint().locations())
    {
        auto* scene = BaseClass::screen().scene();
        auto* om    = scene ? scene->objectManager() : nullptr;
        if (!scene || !om) return;

        auto updated = location;

        if (m_focus_mode == FocusMode::Extrema)
        {
            // まず top_n 個の extrema（max）点群を作る（内部で m_last_extrema_worlds が更新される）
            if (!m_last_maxima.empty())
            {
                this->CreateExtremaObject(m_last_maxima, +1, m_focus_top_n, m_last_dims);
            }
            else
            {
                // maxima がないなら worlds は空のままになる想定
                m_last_extrema_worlds.clear();
            }

            // ★世界座標の候補があるなら、それぞれで出力
            if (!m_last_extrema_worlds.empty())
            {
                // ベース位置（候補ごと・補間ごとにここへ戻す）
                const kvs::Vec3 base_pos = location.position;

                // 通常画像で用いた「ズーム後カメラ位置」を候補ごとに保持（pCP用）
                std::vector<kvs::Vec3> curr_camera_worlds;
                curr_camera_worlds.resize(m_last_extrema_worlds.size());

                // -------------------------
                // (A) 補間画像：全組み合わせ（ズーム適用）
                // -------------------------
                if (m_enable_focus_interpolation &&
                    m_has_prev_extrema_worlds &&
                    !m_prev_extrema_worlds.empty() &&
                    m_focus_interp_n > 0)
                {
                    for (size_t pid = 0; pid < m_prev_extrema_worlds.size(); ++pid)
                    {
                        const kvs::Vec3 prev = m_prev_extrema_worlds[pid];

                        for (size_t cid = 0; cid < m_last_extrema_worlds.size(); ++cid)
                        {
                            const kvs::Vec3 curr = m_last_extrema_worlds[cid];

                            for (size_t s = 1; s <= m_focus_interp_n; ++s)
                            {
                                const float tt = float(s) / float(m_focus_interp_n + 1); // 等間隔

                                // ★毎回リセットしてから補間注視点を設定
                                updated.position = base_pos;
                                updated.look_at  = lerpVec3(prev, curr, tt);

                                // まず通常姿勢
                                scene->camera()->setPosition(
                                    updated.position, updated.look_at, updated.up_vector);

                                // ★ズーム（通常画像と同じ）
                                const float zt = m_zoom_t; // (= 1/5 など)
                                updated.position =
                                    (1.0f - zt) * updated.position + zt * updated.look_at;

                                // ズーム後姿勢で描画
                                scene->camera()->setPosition(
                                    updated.position, updated.look_at, updated.up_vector);

                                tr.start();
                                const auto color = BaseClass::readback(updated);
                                tr.stop();
                                rend_time += m_rend_timer.time(tr);

                                ts.start();
                                if (m_enable_output_image)
                                {
                                    const auto size = this->outputImageSize(updated);
                                    kvs::ColorImage image(size.x(), size.y(), color);

                                    // ★補間専用の命名（prev id, curr id, s）
                                    image.write(this->outputInterpImageName(pid, cid, s));
                                }
                                ts.stop();
                                save_time += m_save_timer.time(ts);
                            }
                        }
                    }
                }

                // -------------------------
                // (B) 通常画像：現stepの各注視点（ズーム適用 + カメラ位置保存）
                // -------------------------
                for (size_t cid = 0; cid < m_last_extrema_worlds.size(); ++cid)
                {
                    // ★毎回リセット
                    updated.position = base_pos;
                    updated.look_at  = m_last_extrema_worlds[cid];

                    // まず通常姿勢
                    scene->camera()->setPosition(
                        updated.position, updated.look_at, updated.up_vector);

                    // ★ズーム
                    const float zt = m_zoom_t; // (= 1/5)
                    updated.position =
                        (1.0f - zt) * updated.position + zt * updated.look_at;

                    // ズーム後姿勢で描画
                    scene->camera()->setPosition(
                        updated.position, updated.look_at, updated.up_vector);

                    tr.start();
                    const auto color = BaseClass::readback(updated);
                    tr.stop();
                    rend_time += m_rend_timer.time(tr);

                    // ★pCP用：この候補での「ズーム後」カメラ位置(world)を保存
                    curr_camera_worlds[cid] = updated.position;

                    ts.start();
                    if (m_enable_output_image)
                    {
                        const auto size = this->outputImageSize(updated);
                        kvs::ColorImage image(size.x(), size.y(), color);

                        // 通常は従来命名（level=1, cid）
                        image.write(this->outputFinalImageName(1, cid));
                    }
                    ts.stop();
                    save_time += m_save_timer.time(ts);
                }

                // ---- CSV: pFP と pCP を出力（pCP=視点移動距離） ----
                if (m_curr_focus_pressure.size() == m_last_extrema_worlds.size())
                {
                    this->updateCSVAndPrevCache_FP_CP(
                        m_last_extrema_worlds,
                        curr_camera_worlds,
                        m_curr_focus_pressure );
                }

                // ★次stepのために prev 更新（補間用）
                m_prev_extrema_worlds = m_last_extrema_worlds;
                m_has_prev_extrema_worlds = true;
            }
            else
            {
                // 候補がない場合の fallback（旧挙動）
                if (m_has_last_extrema_world)
                {
                    updated.look_at = m_last_extrema_world;
                }
                else if (m_has_estimated_focus_phys)
                {
                    kvs::Vec3 focus_world = m_estimated_focus_phys;
                    if (this->hasVisualizationXform())
                    {
                        focus_world = this->visualizationXform().transform(focus_world);
                    }
                    updated.look_at = focus_world;
                }

                scene->camera()->setPosition(
                    updated.position, updated.look_at, updated.up_vector);

                tr.start();
                const auto color = BaseClass::readback(updated);
                tr.stop();
                rend_time += m_rend_timer.time(tr);

                ts.start();
                if (m_enable_output_image)
                {
                    const auto size = this->outputImageSize(updated);
                    kvs::ColorImage image(size.x(), size.y(), color);
                    image.write(this->outputFinalImageName(1, 0));
                }
                ts.stop();
                save_time += m_save_timer.time(ts);
            }
        }
        else
        {
            // ImageEntropy mode（そのまま）
            scene->camera()->setPosition(
                updated.position, updated.look_at, updated.up_vector);

            tr.start();
            auto fb = this->readbackFrameBuffer(updated);
            tr.stop();
            rend_time += m_rend_timer.time(tr);

            const auto win = this->look_at_in_window(fb);
            updated.look_at = this->window_to_object(win, updated);

            scene->camera()->setPosition(
                updated.position, updated.look_at, updated.up_vector);

            tr.start();
            fb = this->readbackFrameBuffer(updated);
            tr.stop();
            rend_time += m_rend_timer.time(tr);

            ts.start();
            if (m_enable_output_image)
            {
                const auto size = this->outputImageSize(updated);
                kvs::ColorImage image(size.x(), size.y(), fb.color_buffer);
                image.write(this->outputFinalImageName(1, 0));
            }
            ts.stop();
            save_time += m_save_timer.time(ts);
        }
    }

    m_rend_timer.stamp(rend_time);
    m_save_timer.stamp(save_time);
}




// =============================================================
// CreateExtremaObject (unchanged policy, but now NEVER called in ImageEntropy mode)
// =============================================================
inline void CameraFocusPredefinedControlledAdaptor::CreateExtremaObject(
    const std::vector<extremum>& extrema,
    int i,
    size_t top_n,
    const kvs::Vec3ui& /*dims*/ )
{
    auto* scene = BaseClass::screen().scene();
    if (!scene) return;

    const size_t n_all = extrema.size();

    const std::string object_name =
        (i > 0) ? "max_extrema" :
        (i < 0) ? "min_extrema" :
                  "saddle_extrema";

    // --- ★追加：world候補ベクタをクリア（毎回更新） ---
    m_last_extrema_worlds.clear();

    if (n_all == 0 || top_n == 0)
    {
        if (scene->hasObject(object_name)) scene->removeObject(object_name);
        m_has_last_extrema_world = false;
        return;
    }

    const size_t npoints = std::min(top_n, n_all);

    std::vector<size_t> sel(n_all);
    std::iota(sel.begin(), sel.end(), 0);

    if (i > 0)
    {
        std::partial_sort(sel.begin(), sel.begin() + npoints, sel.end(),
            [&](size_t a, size_t b){ return extrema[a].value > extrema[b].value; });
    }
    else if (i < 0)
    {
        std::partial_sort(sel.begin(), sel.begin() + npoints, sel.end(),
            [&](size_t a, size_t b){ return extrema[a].value < extrema[b].value; });
    }
    else
    {
        std::partial_sort(sel.begin(), sel.begin() + npoints, sel.end(),
            [&](size_t a, size_t b){
                return std::fabs(extrema[a].value) > std::fabs(extrema[b].value);
            });
    }
    sel.resize(npoints);

    kvs::ValueArray<float> coords(3 * npoints);
    for (size_t p = 0; p < npoints; ++p)
    {
        const auto& e = extrema[sel[p]];
        const kvs::Vec3 p_phys  = this->indexToPhysical(e.position);
        const kvs::Vec3 p_local = this->physicalToLocal(p_phys);

        coords[3*p + 0] = p_local.x();
        coords[3*p + 1] = p_local.y();
        coords[3*p + 2] = p_local.z();
    }

    kvs::ValueArray<kvs::UInt8> colors(3 * npoints);
    for (size_t p = 0; p < npoints; ++p)
    {
        if (i > 0)      { colors[3*p+0]=255; colors[3*p+1]=60;  colors[3*p+2]=60;  }
        else if (i < 0) { colors[3*p+0]=60;  colors[3*p+1]=60;  colors[3*p+2]=255; }
        else           { colors[3*p+0]=160; colors[3*p+1]=160; colors[3*p+2]=160; }
    }

    auto* point = new kvs::PointObject();
    point->setName(object_name);
    point->setCoords(coords);
    point->setColors(colors);
    point->setSize(1.0f);

    if (this->hasVisualizationXform())
    {
        point->multiplyXform(this->visualizationXform());
    }

    auto* glyph = new kvs::SphereGlyph();
    glyph->setNumberOfSlices(20);
    glyph->setNumberOfStacks(20);

    point->updateMinMaxCoords();

    if (scene->hasObject(object_name)) scene->removeObject(object_name);
    BaseClass::screen().registerObject(point, glyph);//球を表示

    // --- ★方針B：top_n 個の world 座標を作って保持 ---
    m_last_extrema_worlds.reserve(npoints);
    for (size_t p = 0; p < npoints; ++p)
    {
        const kvs::Vec3 p_obj(coords[3*p+0], coords[3*p+1], coords[3*p+2]);
        const kvs::Vec3 p_world = point->xform().transform(p_obj);
        // const kvs::Vec3 p_world = this->visualizationXform().transform(p_obj);
        m_last_extrema_worlds.push_back(p_world);
    }

    // 互換：旧コードが参照する1点も更新（top_n=1で一致しやすい）
    if (!m_last_extrema_worlds.empty())
    {
        m_last_extrema_world = m_last_extrema_worlds[0];
        m_has_last_extrema_world = true;
    }
    else
    {
        m_has_last_extrema_world = false;
    }
}


// =============================================================
// estimateFocusPoint (KEY): ONLY valid when FocusMode::Extrema
// If mode != Extrema, do nothing (safety net).
// =============================================================
inline void CameraFocusPredefinedControlledAdaptor::estimateFocusPoint(
    const kvs::ValueArray<float>& values,
    const kvs::Vec3ui& dims )
{
    if (m_focus_mode != FocusMode::Extrema)
    {
        return;
    }

    using namespace FullTriCubic;

    const int nx = static_cast<int>(dims[0]);
    const int ny = static_cast<int>(dims[1]);
    const int nz = static_cast<int>(dims[2]);

    if (nx < 2 || ny < 2 || nz < 2)
    {
        std::cerr << "[estimateFocusPoint] dims too small.\n";
        return;
    }

    // --- derivatives on grid ---
    kvs::ValueArray<float> fx, fy, fz, fxy, fxz, fyz, fxyz;
    this->computeDerivativesFull(values, dims, fx, fy, fz, fxy, fxz, fyz, fxyz);

    Sampler sampler;
    sampler.bind(values, fx, fy, fz, fxy, fxz, fyz, fxyz, dims);

    std::vector<extremum> maxima;
    std::vector<extremum> minima;
    std::vector<extremum> saddles;

    maxima.reserve(512);
    minima.reserve(512);
    saddles.reserve(512);

    std::unordered_set<long long> seen;
    seen.reserve(8192);

    const int   max_iters = 30;
    const float grad_tol  = 1e-5f;
    const float step_tol  = 1e-6f;
    const float eig_eps   = 1e-7f;

    // best（互換用。最後に1位候補で上書きする）
    kvs::Vec3 best_p_index( (float)(nx/2), (float)(ny/2), (float)(nz/2) );
    float best_v = 0.0f;
    bool  best_is_set = false;

    for (int k = 1; k < nz-2; ++k)
    for (int j = 1; j < ny-2; ++j)
    for (int i = 1; i < nx-2; ++i)
    {
        kvs::Vec3 p;
        float v;
        CritType type;

        const bool ok = findCriticalPointInCell(
            sampler, i, j, k, p, v, type,
            max_iters, grad_tol, step_tol, eig_eps );

        if (!ok) continue;

        // domain境界付近を除外
        const float eps_dom = 1e-4f;
        if (p.x() <= 0 + eps_dom || p.x() >= (nx-1) - eps_dom ||
            p.y() <= 0 + eps_dom || p.y() >= (ny-1) - eps_dom ||
            p.z() <= 0 + eps_dom || p.z() >= (nz-1) - eps_dom)
        {
            continue;
        }

        // 重複除去
        const long long key = hashPos(p, 0.02f);
        if (seen.find(key) != seen.end()) continue;
        seen.insert(key);

        extremum e;
        e.position = p;
        e.value    = v;

        if (type == CritType::Max)
        {
            maxima.push_back(e);

            // 従来のbest更新（後で1位候補で上書きするので互換用）
            if (!best_is_set || v > best_v)
            {
                best_is_set = true;
                best_v = v;
                best_p_index = p;
            }
        }
        else if (type == CritType::Min)
        {
            minima.push_back(e);

            // Maxが一度も無い場合の互換用
            if (!best_is_set)
            {
                best_is_set = true;
                best_v = v;
                best_p_index = p;
            }
        }
        else if (type == CritType::Saddle)
        {
            saddles.push_back(e);
        }
    }

    // ---- 上位n個（Max）を抽出 ----
    std::sort(maxima.begin(), maxima.end(),
            [](const extremum& a, const extremum& b){ return a.value > b.value; });

    const size_t n = std::min(m_focus_top_n, maxima.size());
    m_focus_candidates_local.clear();
    m_focus_candidates_local.reserve(n);

    m_curr_focus_pressure.clear();
    m_curr_focus_pressure.reserve(n);

    for (size_t t = 0; t < n; ++t)
    {
        const kvs::Vec3 p_phys  = this->indexToPhysical(maxima[t].position);
        const kvs::Vec3 p_local = this->physicalToLocal(p_phys);
        m_focus_candidates_local.push_back(p_local);

        // ★追加：評価値（圧力＝maxima[t].value）を同じ順で保持
        m_curr_focus_pressure.push_back(maxima[t].value);
    }

    // 互換用（必要なら）：1位を estimated にも入れておく
    if (!m_focus_candidates_local.empty())
    {
        // estimated は phys で持ってる設計なら phys を入れる（localじゃないので注意）
        const kvs::Vec3 p_phys0 = this->indexToPhysical(maxima[0].position);
        m_estimated_focus_phys = p_phys0;
        m_has_estimated_focus_phys = true;
    }

    // cache（描画・点群表示用）
    m_last_maxima  = maxima;
    m_last_minima  = minima;
    m_last_saddles = saddles;
    m_last_dims    = dims;

    // DEBUG
    std::cout
        << "[estimateFocusPoint] critical_points:"
        << " max=" << maxima.size()
        << " min=" << minima.size()
        << " saddle=" << saddles.size()
        << " total=" << (maxima.size() + minima.size() + saddles.size())
        << " topN=" << n
        << "\n";
}


} // end namespace InSituVis
