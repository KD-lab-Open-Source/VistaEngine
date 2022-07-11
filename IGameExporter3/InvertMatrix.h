#pragma once
bool invertmatrix(Mat4f& b);
void MinQuadMatrix(Mat4f& out,int num);
void MinQuadMatrixSimply(Mat4f& out,int num);
void CalcLagrange(vector<real>& data,real& a0,real& a1,real& a2,real& a3);//a0+a1*t+a2*t^2+a3*t^3;
void CalcLinear(vector<real>& data,real& a0,real& a1);

void CalcLagrangeWeight(vector<real>& data,vector<real>& weight,real& a0,real& a1,real& a2,real& a3);

//a0+a1*t+a2*t^2+a3*t^3==a0`+a1`*t`+a2`*t`^2+a3`*t`^3, где t`=t*tscale+toffset
void ScaleSpline(real& a0,real& a1,real& a2,real& a3,real tscale,real toffset);
