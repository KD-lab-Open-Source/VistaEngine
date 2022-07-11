
#pragma once

void DrawXYZ(cInterfaceRenderDevice* pRenderDevice, const MatXf& pos);
void DrawBox(cInterfaceRenderDevice* pRenderDevice, const MatXf& pos, const Vect3f& min, const Vect3f& max, const sColor4c& Color);
void DrawCircle(cInterfaceRenderDevice* pRenderDevice, const MatXf& pos,float r, const sColor4c& Color);
void DrawSphere(cInterfaceRenderDevice* pRenderDevice, const MatXf& pos,float r, const sColor4c& c1, const sColor4c& c2, const sColor4c& c3);
void DrawCylinder(cInterfaceRenderDevice* pRenderDevice, const MatXf& pos,float r,float r2,float h, const sColor4c& c);
void DrawSector(cInterfaceRenderDevice* pRenderDevice, const MatXf& m,float r, float begin_a,float end_a, float begin_t, float end_t, const sColor4c& c);
