// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "gregory_patch.h"

namespace embree
{  
  class __aligned(64) DenseGregoryPatch3fa
  {    
  public:

    __forceinline DenseGregoryPatch3fa (const GregoryPatch3fa& patch)
    {
      for (size_t y=0; y<4; y++)
	for (size_t x=0; x<4; x++)
	  matrix[y][x] = patch.v[y][x];
      
      matrix[0][0].w = patch.f[0][0].x;
      matrix[0][1].w = patch.f[0][0].y;
      matrix[0][2].w = patch.f[0][0].z;
      matrix[0][3].w = 0.0f;
      
      matrix[1][0].w = patch.f[0][1].x;
      matrix[1][1].w = patch.f[0][1].y;
      matrix[1][2].w = patch.f[0][1].z;
      matrix[1][3].w = 0.0f;
      
      matrix[2][0].w = patch.f[1][1].x;
      matrix[2][1].w = patch.f[1][1].y;
      matrix[2][2].w = patch.f[1][1].z;
      matrix[2][3].w = 0.0f;
      
      matrix[3][0].w = patch.f[1][0].x;
      matrix[3][1].w = patch.f[1][0].y;
      matrix[3][2].w = patch.f[1][0].z;
      matrix[3][3].w = 0.0f;
    }

    __forceinline void extract_f_m(Vec3fa f_m[2][2]) const
    {
#if defined(__MIC__)
      const vfloat16 row0 = vfloat16::load((float*)&matrix[0][0]);
      const vfloat16 row1 = vfloat16::load((float*)&matrix[1][0]);
      const vfloat16 row2 = vfloat16::load((float*)&matrix[2][0]);
      const vfloat16 row3 = vfloat16::load((float*)&matrix[3][0]);
      
      compactustore16f_low(0x8888,(float*)&f_m[0][0],row0);
      compactustore16f_low(0x8888,(float*)&f_m[0][1],row1);
      compactustore16f_low(0x8888,(float*)&f_m[1][1],row2);
      compactustore16f_low(0x8888,(float*)&f_m[1][0],row3);
#else
      f_m[0][0] = Vec3fa( matrix[0][0].w, matrix[0][1].w, matrix[0][2].w );
      f_m[0][1] = Vec3fa( matrix[1][0].w, matrix[1][1].w, matrix[1][2].w );
      f_m[1][1] = Vec3fa( matrix[2][0].w, matrix[2][1].w, matrix[2][2].w );
      f_m[1][0] = Vec3fa( matrix[3][0].w, matrix[3][1].w, matrix[3][2].w );      
#endif     
    }

    __forceinline Vec3fa eval(const float uu, const float vv) const
    {
      __aligned(64) Vec3fa f_m[2][2]; extract_f_m(f_m);
      return GregoryPatch3fa::eval(matrix,f_m,uu,vv);
    }

    __forceinline Vec3fa normal(const float uu, const float vv) const
    {
      __aligned(64) Vec3fa f_m[2][2]; extract_f_m(f_m);
      return GregoryPatch3fa::normal(matrix,f_m,uu,vv);
    }

    template<class T>
      __forceinline Vec3<T> eval(const T &uu, const T &vv) const 
    {
      Vec3<T> f_m[2][2];
      f_m[0][0] = Vec3<T>( matrix[0][0].w, matrix[0][1].w, matrix[0][2].w );
      f_m[0][1] = Vec3<T>( matrix[1][0].w, matrix[1][1].w, matrix[1][2].w );
      f_m[1][1] = Vec3<T>( matrix[2][0].w, matrix[2][1].w, matrix[2][2].w );
      f_m[1][0] = Vec3<T>( matrix[3][0].w, matrix[3][1].w, matrix[3][2].w );
      return GregoryPatch3fa::eval_t(matrix,f_m,uu,vv);
    }
    
    template<class T>
      __forceinline Vec3<T> normal(const T &uu, const T &vv) const 
    {
      Vec3<T> f_m[2][2];
      f_m[0][0] = Vec3<T>( matrix[0][0].w, matrix[0][1].w, matrix[0][2].w );
      f_m[0][1] = Vec3<T>( matrix[1][0].w, matrix[1][1].w, matrix[1][2].w );
      f_m[1][1] = Vec3<T>( matrix[2][0].w, matrix[2][1].w, matrix[2][2].w );
      f_m[1][0] = Vec3<T>( matrix[3][0].w, matrix[3][1].w, matrix[3][2].w );
      return GregoryPatch3fa::normal_t(matrix,f_m,uu,vv);
    }

    __forceinline void eval(const float u, const float v, Vec3fa* P, Vec3fa* dPdu, Vec3fa* dPdv, const float dscale = 1.0f) const
    {
      __aligned(64) Vec3fa f_m[2][2]; extract_f_m(f_m);
      if (P)    *P    = GregoryPatch3fa::eval(matrix,f_m,u,v); 
      if (dPdu) *dPdu = GregoryPatch3fa::tangentU(matrix,f_m,u,v)*dscale; 
      if (dPdv) *dPdv = GregoryPatch3fa::tangentV(matrix,f_m,u,v)*dscale; 
    }

    template<typename vbool, typename vfloat>
    __forceinline void eval(const vbool& valid, const vfloat& uu, const vfloat& vv, float* P, float* dPdu, float* dPdv, const float dscale, const size_t dstride, const size_t N) const 
    {
      __aligned(64) Vec3fa f_m[2][2]; extract_f_m(f_m);
      return GregoryPatch3fa::eval(matrix,f_m,valid,uu,vv,P,dPdu,dPdv,dscale,dstride,N);
    }

  private:
    Vec3fa matrix[4][4]; // f_p/m points are stored in 4th component
  };
}
