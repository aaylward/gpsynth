/* ==================================== JUCER_BINARY_RESOURCE ====================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

namespace BinaryData
{

//================== kissfft.hh ==================
static const unsigned char temp_32c8297a[] =
"#ifndef KISSFFT_CLASS_HH\r\n"
"#include <complex>\r\n"
"#include <vector>\r\n"
"\r\n"
"namespace kissfft_utils {\r\n"
"\r\n"
"template <typename T_scalar>\r\n"
"struct traits\r\n"
"{\r\n"
"    typedef T_scalar scalar_type;\r\n"
"    typedef std::complex<scalar_type> cpx_type;\r\n"
"    void fill_twiddles( std::complex<T_scalar> * dst ,int nfft,bool inverse)\r\n"
"    {\r\n"
"        T_scalar phinc =  (inverse?2:-2)* acos( (T_scalar) -1)  / nfft;\r\n"
"        for (int i=0;i<nfft;++i)\r\n"
"            dst[i] = exp( std::complex<T_scalar>(0,i*phinc) );\r\n"
"    }\r\n"
"\r\n"
"    void prepare(\r\n"
"            std::vector< std::complex<T_scalar> > & dst,\r\n"
"            int nfft,bool inverse, \r\n"
"            std::vector<int> & stageRadix, \r\n"
"            std::vector<int> & stageRemainder )\r\n"
"    {\r\n"
"        _twiddles.resize(nfft);\r\n"
"        fill_twiddles( &_twiddles[0],nfft,inverse);\r\n"
"        dst = _twiddles;\r\n"
"\r\n"
"        //factorize\r\n"
"        //start factoring out 4's, then 2's, then 3,5,7,9,...\r\n"
"        int n= nfft;\r\n"
"        int p=4;\r\n"
"        do {\r\n"
"            while (n % p) {\r\n"
"                switch (p) {\r\n"
"                    case 4: p = 2; break;\r\n"
"                    case 2: p = 3; break;\r\n"
"                    default: p += 2; break;\r\n"
"                }\r\n"
"                if (p*p>n)\r\n"
"                    p=n;// no more factors\r\n"
"            }\r\n"
"            n /= p;\r\n"
"            stageRadix.push_back(p);\r\n"
"            stageRemainder.push_back(n);\r\n"
"        }while(n>1);\r\n"
"    }\r\n"
"    std::vector<cpx_type> _twiddles;\r\n"
"\r\n"
"\r\n"
"    const cpx_type twiddle(int i) { return _twiddles[i]; }\r\n"
"};\r\n"
"\r\n"
"}\r\n"
"\r\n"
"template <typename T_Scalar,\r\n"
"         typename T_traits=kissfft_utils::traits<T_Scalar> \r\n"
"         >\r\n"
"class kissfft\r\n"
"{\r\n"
"    public:\r\n"
"        typedef T_traits traits_type;\r\n"
"        typedef typename traits_type::scalar_type scalar_type;\r\n"
"        typedef typename traits_type::cpx_type cpx_type;\r\n"
"\r\n"
"        kissfft(int nfft,bool inverse,const traits_type & traits=traits_type() ) \r\n"
"            :_nfft(nfft),_inverse(inverse),_traits(traits)\r\n"
"        {\r\n"
"            _traits.prepare(_twiddles, _nfft,_inverse ,_stageRadix, _stageRemainder);\r\n"
"        }\r\n"
"\r\n"
"        void transform(const cpx_type * src , cpx_type * dst)\r\n"
"        {\r\n"
"            kf_work(0, dst, src, 1,1);\r\n"
"        }\r\n"
"\r\n"
"    private:\r\n"
"        void kf_work( int stage,cpx_type * Fout, const cpx_type * f, size_t fstride,size_t in_stride)\r\n"
"        {\r\n"
"            int p = _stageRadix[stage];\r\n"
"            int m = _stageRemainder[stage];\r\n"
"            cpx_type * Fout_beg = Fout;\r\n"
"            cpx_type * Fout_end = Fout + p*m;\r\n"
"\r\n"
"            if (m==1) {\r\n"
"                do{\r\n"
"                    *Fout = *f;\r\n"
"                    f += fstride*in_stride;\r\n"
"                }while(++Fout != Fout_end );\r\n"
"            }else{\r\n"
"                do{\r\n"
"                    // recursive call:\r\n"
"                    // DFT of size m*p performed by doing\r\n"
"                    // p instances of smaller DFTs of size m, \r\n"
"                    // each one takes a decimated version of the input\r\n"
"                    kf_work(stage+1, Fout , f, fstride*p,in_stride);\r\n"
"                    f += fstride*in_stride;\r\n"
"                }while( (Fout += m) != Fout_end );\r\n"
"            }\r\n"
"\r\n"
"            Fout=Fout_beg;\r\n"
"\r\n"
"            // recombine the p smaller DFTs \r\n"
"            switch (p) {\r\n"
"                case 2: kf_bfly2(Fout,fstride,m); break;\r\n"
"                case 3: kf_bfly3(Fout,fstride,m); break;\r\n"
"                case 4: kf_bfly4(Fout,fstride,m); break;\r\n"
"                case 5: kf_bfly5(Fout,fstride,m); break;\r\n"
"                default: kf_bfly_generic(Fout,fstride,m,p); break;\r\n"
"            }\r\n"
"        }\r\n"
"\r\n"
"        // these were #define macros in the original kiss_fft\r\n"
"        void C_ADD( cpx_type & c,const cpx_type & a,const cpx_type & b) { c=a+b;}\r\n"
"        void C_MUL( cpx_type & c,const cpx_type & a,const cpx_type & b) { c=a*b;}\r\n"
"        void C_SUB( cpx_type & c,const cpx_type & a,const cpx_type & b) { c=a-b;}\r\n"
"        void C_ADDTO( cpx_type & c,const cpx_type & a) { c+=a;}\r\n"
"        void C_FIXDIV( cpx_type & ,int ) {} // NO-OP for float types\r\n"
"        scalar_type S_MUL( const scalar_type & a,const scalar_type & b) { return a*b;}\r\n"
"        scalar_type HALF_OF( const scalar_type & a) { return a*.5;}\r\n"
"        void C_MULBYSCALAR(cpx_type & c,const scalar_type & a) {c*=a;}\r\n"
"\r\n"
"        void kf_bfly2( cpx_type * Fout, const size_t fstride, int m)\r\n"
"        {\r\n"
"            for (int k=0;k<m;++k) {\r\n"
"                cpx_type t = Fout[m+k] * _traits.twiddle(k*fstride);\r\n"
"                Fout[m+k] = Fout[k] - t;\r\n"
"                Fout[k] += t;\r\n"
"            }\r\n"
"        }\r\n"
"\r\n"
"        void kf_bfly4( cpx_type * Fout, const size_t fstride, const size_t m)\r\n"
"        {\r\n"
"            cpx_type scratch[7];\r\n"
"            int negative_if_inverse = _inverse * -2 +1;\r\n"
"            for (size_t k=0;k<m;++k) {\r\n"
"                scratch[0] = Fout[k+m] * _traits.twiddle(k*fstride);\r\n"
"                scratch[1] = Fout[k+2*m] * _traits.twiddle(k*fstride*2);\r\n"
"                scratch[2] = Fout[k+3*m] * _traits.twiddle(k*fstride*3);\r\n"
"                scratch[5] = Fout[k] - scratch[1];\r\n"
"\r\n"
"                Fout[k] += scratch[1];\r\n"
"                scratch[3] = scratch[0] + scratch[2];\r\n"
"                scratch[4] = scratch[0] - scratch[2];\r\n"
"                scratch[4] = cpx_type( scratch[4].imag()*negative_if_inverse , -scratch[4].real()* negative_if_inverse );\r\n"
"\r\n"
"                Fout[k+2*m]  = Fout[k] - scratch[3];\r\n"
"                Fout[k] += scratch[3];\r\n"
"                Fout[k+m] = scratch[5] + scratch[4];\r\n"
"                Fout[k+3*m] = scratch[5] - scratch[4];\r\n"
"            }\r\n"
"        }\r\n"
"\r\n"
"        void kf_bfly3( cpx_type * Fout, const size_t fstride, const size_t m)\r\n"
"        {\r\n"
"            size_t k=m;\r\n"
"            const size_t m2 = 2*m;\r\n"
"            cpx_type *tw1,*tw2;\r\n"
"            cpx_type scratch[5];\r\n"
"            cpx_type epi3;\r\n"
"            epi3 = _twiddles[fstride*m];\r\n"
"\r\n"
"            tw1=tw2=&_twiddles[0];\r\n"
"\r\n"
"            do{\r\n"
"                C_FIXDIV(*Fout,3); C_FIXDIV(Fout[m],3); C_FIXDIV(Fout[m2],3);\r\n"
"\r\n"
"                C_MUL(scratch[1],Fout[m] , *tw1);\r\n"
"                C_MUL(scratch[2],Fout[m2] , *tw2);\r\n"
"\r\n"
"                C_ADD(scratch[3],scratch[1],scratch[2]);\r\n"
"                C_SUB(scratch[0],scratch[1],scratch[2]);\r\n"
"                tw1 += fstride;\r\n"
"                tw2 += fstride*2;\r\n"
"\r\n"
"                Fout[m] = cpx_type( Fout->real() - HALF_OF(scratch[3].real() ) , Fout->imag() - HALF_OF(scratch[3].imag() ) );\r\n"
"\r\n"
"                C_MULBYSCALAR( scratch[0] , epi3.imag() );\r\n"
"\r\n"
"                C_ADDTO(*Fout,scratch[3]);\r\n"
"\r\n"
"                Fout[m2] = cpx_type(  Fout[m].real() + scratch[0].imag() , Fout[m].imag() - scratch[0].real() );\r\n"
"\r\n"
"                C_ADDTO( Fout[m] , cpx_type( -scratch[0].imag(),scratch[0].real() ) );\r\n"
"                ++Fout;\r\n"
"            }while(--k);\r\n"
"        }\r\n"
"\r\n"
"        void kf_bfly5( cpx_type * Fout, const size_t fstride, const size_t m)\r\n"
"        {\r\n"
"            cpx_type *Fout0,*Fout1,*Fout2,*Fout3,*Fout4;\r\n"
"            size_t u;\r\n"
"            cpx_type scratch[13];\r\n"
"            cpx_type * twiddles = &_twiddles[0];\r\n"
"            cpx_type *tw;\r\n"
"            cpx_type ya,yb;\r\n"
"            ya = twiddles[fstride*m];\r\n"
"            yb = twiddles[fstride*2*m];\r\n"
"\r\n"
"            Fout0=Fout;\r\n"
"            Fout1=Fout0+m;\r\n"
"            Fout2=Fout0+2*m;\r\n"
"            Fout3=Fout0+3*m;\r\n"
"            Fout4=Fout0+4*m;\r\n"
"\r\n"
"            tw=twiddles;\r\n"
"            for ( u=0; u<m; ++u ) {\r\n"
"                C_FIXDIV( *Fout0,5); C_FIXDIV( *Fout1,5); C_FIXDIV( *Fout2,5); C_FIXDIV( *Fout3,5); C_FIXDIV( *Fout4,5);\r\n"
"                scratch[0] = *Fout0;\r\n"
"\r\n"
"                C_MUL(scratch[1] ,*Fout1, tw[u*fstride]);\r\n"
"                C_MUL(scratch[2] ,*Fout2, tw[2*u*fstride]);\r\n"
"                C_MUL(scratch[3] ,*Fout3, tw[3*u*fstride]);\r\n"
"                C_MUL(scratch[4] ,*Fout4, tw[4*u*fstride]);\r\n"
"\r\n"
"                C_ADD( scratch[7],scratch[1],scratch[4]);\r\n"
"                C_SUB( scratch[10],scratch[1],scratch[4]);\r\n"
"                C_ADD( scratch[8],scratch[2],scratch[3]);\r\n"
"                C_SUB( scratch[9],scratch[2],scratch[3]);\r\n"
"\r\n"
"                C_ADDTO( *Fout0, scratch[7]);\r\n"
"                C_ADDTO( *Fout0, scratch[8]);\r\n"
"\r\n"
"                scratch[5] = scratch[0] + cpx_type(\r\n"
"                        S_MUL(scratch[7].real(),ya.real() ) + S_MUL(scratch[8].real() ,yb.real() ),\r\n"
"                        S_MUL(scratch[7].imag(),ya.real()) + S_MUL(scratch[8].imag(),yb.real())\r\n"
"                        );\r\n"
"\r\n"
"                scratch[6] =  cpx_type( \r\n"
"                        S_MUL(scratch[10].imag(),ya.imag()) + S_MUL(scratch[9].imag(),yb.imag()),\r\n"
"                        -S_MUL(scratch[10].real(),ya.imag()) - S_MUL(scratch[9].real(),yb.imag()) \r\n"
"                        );\r\n"
"\r\n"
"                C_SUB(*Fout1,scratch[5],scratch[6]);\r\n"
"                C_ADD(*Fout4,scratch[5],scratch[6]);\r\n"
"\r\n"
"                scratch[11] = scratch[0] + \r\n"
"                    cpx_type(\r\n"
"                            S_MUL(scratch[7].real(),yb.real()) + S_MUL(scratch[8].real(),ya.real()),\r\n"
"                            S_MUL(scratch[7].imag(),yb.real()) + S_MUL(scratch[8].imag(),ya.real())\r\n"
"                            );\r\n"
"\r\n"
"                scratch[12] = cpx_type(\r\n"
"                        -S_MUL(scratch[10].imag(),yb.imag()) + S_MUL(scratch[9].imag(),ya.imag()),\r\n"
"                        S_MUL(scratch[10].real(),yb.imag()) - S_MUL(scratch[9].real(),ya.imag())\r\n"
"                        );\r\n"
"\r\n"
"                C_ADD(*Fout2,scratch[11],scratch[12]);\r\n"
"                C_SUB(*Fout3,scratch[11],scratch[12]);\r\n"
"\r\n"
"                ++Fout0;++Fout1;++Fout2;++Fout3;++Fout4;\r\n"
"            }\r\n"
"        }\r\n"
"\r\n"
"        /* perform the butterfly for one stage of a mixed radix FFT */\r\n"
"        void kf_bfly_generic(\r\n"
"                cpx_type * Fout,\r\n"
"                const size_t fstride,\r\n"
"                int m,\r\n"
"                int p\r\n"
"                )\r\n"
"        {\r\n"
"            int u,k,q1,q;\r\n"
"            cpx_type * twiddles = &_twiddles[0];\r\n"
"            cpx_type t;\r\n"
"            int Norig = _nfft;\r\n"
"            cpx_type scratchbuf[p];\r\n"
"\r\n"
"            for ( u=0; u<m; ++u ) {\r\n"
"                k=u;\r\n"
"                for ( q1=0 ; q1<p ; ++q1 ) {\r\n"
"                    scratchbuf[q1] = Fout[ k  ];\r\n"
"                    C_FIXDIV(scratchbuf[q1],p);\r\n"
"                    k += m;\r\n"
"                }\r\n"
"\r\n"
"                k=u;\r\n"
"                for ( q1=0 ; q1<p ; ++q1 ) {\r\n"
"                    int twidx=0;\r\n"
"                    Fout[ k ] = scratchbuf[0];\r\n"
"                    for (q=1;q<p;++q ) {\r\n"
"                        twidx += fstride * k;\r\n"
"                        if (twidx>=Norig) twidx-=Norig;\r\n"
"                        C_MUL(t,scratchbuf[q] , twiddles[twidx] );\r\n"
"                        C_ADDTO( Fout[ k ] ,t);\r\n"
"                    }\r\n"
"                    k += m;\r\n"
"                }\r\n"
"            }\r\n"
"        }\r\n"
"\r\n"
"        int _nfft;\r\n"
"        bool _inverse;\r\n"
"        std::vector<cpx_type> _twiddles;\r\n"
"        std::vector<int> _stageRadix;\r\n"
"        std::vector<int> _stageRemainder;\r\n"
"        traits_type _traits;\r\n"
"};\r\n"
"#endif\r\n";

const char* kissfft_hh = (const char*) temp_32c8297a;

//================== GPMutatableParam.h ==================
static const unsigned char temp_dc44fa2e[] =
"#ifndef GPMUTATABLEPARAM_H\r\n"
"#define GPMUTATABLEPARAM_H\r\n"
"\r\n"
"#include <assert.h>\r\n"
"#include <string>\r\n"
"#include <sstream>\r\n"
"#include \"../Common/GPRandom.h\"\r\n"
"\r\n"
"class GPMutatableParam {\r\n"
"public:\r\n"
"    // discrete constructor\r\n"
"    GPMutatableParam(std::string t, bool mut, int val, int min, int max) {\r\n"
"        assert(!mut || (min <= val && val <= max));\r\n"
"        type = t;\r\n"
"        continuous = false;\r\n"
"        mutatable = mut;\r\n"
"        dvalue = val;\r\n"
"        dminimum = min;\r\n"
"        dmaximum = max;\r\n"
"    }\r\n"
"\r\n"
"    // continuous constructor\r\n"
"    GPMutatableParam(std::string t, bool mut, float val, float min, float max) {\r\n"
"        assert(!mut || (min <= val && val <= max));\r\n"
"        type = t;\r\n"
"        continuous = true;\r\n"
"        mutatable = mut;\r\n"
"        cvalue = val;\r\n"
"        cminimum = min;\r\n"
"        cmaximum = max;\r\n"
"    }\r\n"
"\r\n"
"    ~GPMutatableParam() {\r\n"
"    }\r\n"
"\r\n"
"    // copy method\r\n"
"    GPMutatableParam* getCopy() {\r\n"
"        if (continuous)\r\n"
"            return new GPMutatableParam(type, mutatable, cvalue, cminimum, cmaximum);\r\n"
"        else\r\n"
"            return new GPMutatableParam(type, mutatable, dvalue, dminimum, dmaximum);\r\n"
"    }\r\n"
"\r\n"
"    // render to a string stream\r\n"
"    void toString(std::stringstream& ss) {\r\n"
"        ss << \"{\";\r\n"
"        if (continuous) {\r\n"
"            ss << \"c \" << cminimum << \" \" << cvalue << \" \" << cmaximum;\r\n"
"        }\r\n"
"        else {\r\n"
"            ss << \"d \" << dminimum << \" \" << dvalue << \" \" << dmaximum;\r\n"
"        }\r\n"
"        ss << \"}\";\r\n"
"    }\r\n"
"\r\n"
"    // get mutatble param as string\r\n"
"    std::string toString(unsigned precision) {\r\n"
"        std::stringstream ss;\r\n"
"        ss.precision(precision);\r\n"
"        toString(ss);\r\n"
"        return ss.str();\r\n"
"    }\r\n"
"\r\n"
"    // set type\r\n"
"    void setType(std::string newtype) {\r\n"
"        type = newtype;\r\n"
"    }\r\n"
"\r\n"
"    // mark as mutatable\r\n"
"    void setMutatable() {\r\n"
"        mutatable = true;\r\n"
"    }\r\n"
"\r\n"
"    // mark as unmutatable\r\n"
"    void setUnmutatable() {\r\n"
"        mutatable = false;\r\n"
"    }\r\n"
"\r\n"
"    // set discrete data\r\n"
"    void setDData(int min, int val, int max) {\r\n"
"        assert(!continuous);\r\n"
"        assert(!mutatable || (min <= val && val <= max));\r\n"
"        dminimum = min;\r\n"
"        dvalue = val;\r\n"
"        dmaximum = max;\r\n"
"    }\r\n"
"\r\n"
"    // set continuous values\r\n"
"    void setDValue(int newvalue) {\r\n"
"        setDData(dminimum, newvalue, dmaximum);\r\n"
"    }\r\n"
"\r\n"
"    // set continuous data\r\n"
"    void setCData(float min, float val, float max) {\r\n"
"        assert(continuous);\r\n"
"        assert(!mutatable || (min <= val && val <= max));\r\n"
"        cminimum = min;\r\n"
"        cvalue = val;\r\n"
"        cmaximum = max;\r\n"
"    }\r\n"
"\r\n"
"    // set continuous values\r\n"
"    void setCValue(float newvalue) {\r\n"
"        setCData(cminimum, newvalue, cmaximum);\r\n"
"    }\r\n"
"\r\n"
"    // type accessor\r\n"
"    std::string getType() {\r\n"
"        return type;\r\n"
"    }\r\n"
"\r\n"
"    // continuous accessors\r\n"
"    float getCValue() {\r\n"
"        assert(continuous);\r\n"
"        return cvalue;\r\n"
"    }\r\n"
"\r\n"
"    float getCMin() {\r\n"
"        assert(continuous);\r\n"
"        return cminimum;\r\n"
"    }\r\n"
"\r\n"
"    float getCMax() {\r\n"
"        assert(continuous);\r\n"
"        return cmaximum;\r\n"
"    }\r\n"
"\r\n"
"    // discrete accessors\r\n"
"    int getDValue() {\r\n"
"        assert(!continuous);\r\n"
"        return dvalue;\r\n"
"    }\r\n"
"\r\n"
"    int getDMin() {\r\n"
"        assert(!continuous);\r\n"
"        return dminimum;\r\n"
"    }\r\n"
"\r\n"
"    int getDMax() {\r\n"
"        assert(!continuous);\r\n"
"        return dmaximum;\r\n"
"    }\r\n"
"\r\n"
"    // combined accessors\r\n"
"    float getValue() {\r\n"
"        if (continuous)\r\n"
"            return cvalue;\r\n"
"        else\r\n"
"            return (float) dvalue;\r\n"
"    }\r\n"
"\r\n"
"    float getMin() {\r\n"
"\t\tif (continuous)\r\n"
"\t\t\treturn cminimum;\r\n"
"\t\telse\r\n"
"\t\t\treturn (float) dminimum;\r\n"
"    }\r\n"
"\r\n"
"    float getMax() {\r\n"
"\t\tif (continuous)\r\n"
"\t\t\treturn cmaximum;\r\n"
"\t\telse\r\n"
"\t\t\treturn (float) dmaximum;\r\n"
"    }\r\n"
"\r\n"
"    bool isDiscrete() {\r\n"
"        return !continuous;\r\n"
"    }\r\n"
"\r\n"
"    bool isContinuous() {\r\n"
"        return continuous;\r\n"
"    }\r\n"
"\r\n"
"    bool isMutatable() {\r\n"
"        return mutatable;\r\n"
"    }\r\n"
"\r\n"
"    bool isUnmutatable() {\r\n"
"        return !mutatable;\r\n"
"    }\r\n"
"\r\n"
"    void ephemeralRandom(GPRandom* rng) {\r\n"
"        if (continuous && mutatable)\r\n"
"            cvalue = ((float) rng->random() * (cmaximum - cminimum)) + cminimum;\r\n"
"        else if (!continuous && mutatable)\r\n"
"            dvalue = (rng->random((dmaximum - dminimum) + 1)) + dminimum;\r\n"
"    }\r\n"
"\r\n"
"\r\n"
"private:\r\n"
"    std::string type;\r\n"
"    int dvalue;\r\n"
"    int dminimum;\r\n"
"    int dmaximum;\r\n"
"    float cvalue;\r\n"
"    float cminimum;\r\n"
"    float cmaximum;\r\n"
"    bool continuous;\r\n"
"    bool mutatable;\r\n"
"};\r\n"
"\r\n"
"#endif\r\n";

const char* GPMutatableParam_h = (const char*) temp_dc44fa2e;


const char* getNamedResource (const char*, int&) throw();
const char* getNamedResource (const char* resourceNameUTF8, int& numBytes) throw()
{
    unsigned int hash = 0;
    if (resourceNameUTF8 != 0)
        while (*resourceNameUTF8 != 0)
            hash = 31 * hash + (unsigned int) *resourceNameUTF8++;

    switch (hash)
    {
        case 0x0c396f29:  numBytes = 10920; return kissfft_hh;
        case 0x83bd8326:  numBytes = 4493; return GPMutatableParam_h;
        default: break;
    }

    numBytes = 0;
    return 0;
}

}
