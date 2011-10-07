
/** \ingroup SPF */
/*@{*/

/*! \file Tpem.h
 *
 *  
 *
 */
#ifndef TPEM_H
#define TPEM_H
#include <gsl/gsl_integration.h>
#include "CrsMatrix.h"
#include <cassert>

namespace Spf {

	template<typename BaseFunctorType>
	class Tpem {
		typedef typename BaseFunctorType::RealType RealType;
	public:
		
		typedef PsimagLite::CrsMatrix<RealType> TpemSparseType;

		struct MyFunctionParams {
			MyFunctionParams(const BaseFunctorType& functor1)
			: functor(functor1) { }

			const BaseFunctorType& functor;
			size_t m;
		};

		typedef MyFunctionParams MyFunctionParamsType;

		Tpem()
		{}

		void calcCoeffs(std::vector<RealType> &vobs,
		                const BaseFunctorType& obsFunc)
		{
			std::vector<RealType> pts(2);
			pts[0]= -1.0;
			pts[1] = 1.0;
			int npts = pts.size();
			RealType epsabs=1e-9;
			RealType epsrel=1e-9;
			
			int limit = 1e6;
			gsl_integration_workspace *workspace= gsl_integration_workspace_alloc(limit+2);
			
			RealType result = 0,abserr = 0;
			
			gsl_function f;
			f.function= &Tpem<BaseFunctorType>::myFunction;
			MyFunctionParamsType params(obsFunc);
			f.params = &params;
			
			for (size_t m=0;m<cutoff_;m++) {
				params.m = m;
				gsl_integration_qagp(&f,&(pts[0]),npts,epsabs,epsrel,limit,workspace,&result,&abserr);
				vobs[m] = result;
			}
			gsl_integration_workspace_free (workspace);
		}
		
		void calcMomentsDiff(std::vector<RealType> &moments,
		                     const TpemSparseType& matrix0,
		                     const TpemSparseType& matrix1) const
		{	
			TpemSubspace info(matrix0.rank());
			if (tpemOptions.algorithm==TPEM) {
				info.fill(matrix0, matrix1, moment0, moment1);
			} else {
				info.fill();
			}
			
			size_t n=moments.size();
			std::vector<RealType>  moment0(n), moment1(n);
			
			moment0[0] = moment1[0] = (RealType) matrix0->rank;
			
			size_t total = info.top-info->stack;

			for (size_t k=0;k<total;k++) {
				size_t p= info.stack +k;
				if (p>=info.top) {
					moment0[0]=moment1[0]=0.0;
					break;
				}
				diagonalElement(matrix0, moment0, *p);
				diagonalElement(matrix1, moment1, *p);
			}

			for (i = 2; i < n; i += 2) {
				moment0[i] = 2.0 * moment0[i] - moment0[0];
				moment1[i] = 2.0 * moment1[i] - moment1[0];
			}

			for (i = 3; i < n - 1; i += 2) {
				moment0[i] = 2.0 * moment0[i] - moment0[1];
				moment1[i] = 2.0 * moment1[i] - moment1[1];
			}

			for (i = 0; i < n; i++)
				moment[i] = moment0[i] - moment1[i];

		}

		RealType expand(const std::vector<RealType>& moments,
		                const std::vector<RealType>& coeffs) const
		{
			assert(moments.size()==coeffs.size());

			RealType ret = 0.0;
			for (size_t i = 0; i < moments.size(); ++i)
				ret += moments[i] * coeffs[i];
			return ret;
		}

	private:
		
		static RealType myFunction (RealType x, void * p) 
		{
			MyFunctionParamsType* params = (MyFunctionParamsType*)p;
			
			/* return  params->funk(params->m,x);*/
			RealType tmp;
			RealType tmp2= (RealType)1.0/M_PI;
			int m=params->m;
			RealType factorAlpha = (m==0) ? 1 : 2;
			tmp = params->functor(x) *  tmp2 * factorAlpha * chebyshev(m,x)/sqrt(1.0-x*x);
			return tmp;
		}

		static RealType chebyshev(int m,RealType x) {
			RealType tmp;
			int p;
			if (m==0) return 1;
			if (m==1) return x;
			
			if ((m%2)==0) {
				p=m/2;
				tmp=chebyshev(p,x);
				return (2*tmp*tmp-1);
			}
			else {
				p=(m-1)/2;
				return (2*chebyshev(p,x)*chebyshev(p+1,x)-x);
			}
		}
	}; // class Tpem
} // namespace Spf

/*@}*/
#endif // TPEM_H