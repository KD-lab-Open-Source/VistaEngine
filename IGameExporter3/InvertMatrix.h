#pragma once

void calcLinear(vector<float>& data,float& a0,float& a1);
void calcQuadrix(vector<float>& data,float& a0,float& a1,float& a2);
void calcCubic(vector<float>& data,float& a0,float& a1,float& a2,float& a3);

//a0+a1*t+a2*t^2+a3*t^3==a0`+a1`*t`+a2`*t`^2+a3`*t`^3, где t`=t*tscale+toffset
void ScaleSpline(float& a0,float& a1,float& a2,float& a3,float tscale,float toffset);
