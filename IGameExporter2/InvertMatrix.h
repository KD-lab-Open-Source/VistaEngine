#pragma once
bool invertmatrix(Mat4f& b);
void MinQuadMatrix(Mat4f& out,int num);
void MinQuadMatrixSimply(Mat4f& out,int num);
void CalcLagrange(vector<real>& data,real& a0,real& a1,real& a2,real& a3);
void CalcLinear(vector<real>& data,real& a0,real& a1);

void CalcLagrangeWeight(vector<real>& data,vector<real>& weight,real& a0,real& a1,real& a2,real& a3);
