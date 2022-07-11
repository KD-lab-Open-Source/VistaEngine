#include "StdAfx.h"
#include "RigidBodyBase.h"
#include "NormalMap.h"
#include "CD\CDDual.h"

ContactInfo::ContactInfo() : 
	bodyPart1(-1), 
	bodyPart2(-1),
	unit1(0),
	unit2(0)
{
}

GeomSphere::GeomSphere(float radius) :
	sphere_(radius) 
{
	geomType_ = GEOM_SPHERE; 
}

bool GeomSphere::groundCollision(RigidBodyCollision* body) const
{
	bool collide(false);
	Vect3f center(body->centreOfGravity());
	float xmin = center.x - sphere_.getRadius();
	float xmax = center.x + sphere_.getRadius();
	float ymin = center.y - sphere_.getRadius();
	float ymax = center.y + sphere_.getRadius();
	float shift = 1.0f / (1 << NormalMapTile::tileShift);
	xmin *= shift;
	xmax *= shift;
	ymin *= shift;
	ymax *= shift;
	int xl = round(xmin - 0.5);
	int xr = round(xmax + 0.5);
	int yl = round(ymin - 0.5);
	int yr = round(ymax + 0.5);
	for(int i = xl; i <= xr; i++)
		for(int j = yl; j <= yr; j++)
			if(planeCollision(i, j, body))
				collide = true;
	return collide;
}

bool GeomSphere::waterCollision(RigidBodyCollision* body) const
{
	bool collide(false);
	Vect3f center(body->centreOfGravity());
	float xmin = center.x - sphere_.getRadius();
	float xmax = center.x + sphere_.getRadius();
	float ymin = center.y - sphere_.getRadius();
	float ymax = center.y + sphere_.getRadius();
	int xl = int(xmin) >> cWater::grid_shift;
	int xr = int(xmax) >> cWater::grid_shift;
	int yl = int(ymin) >> cWater::grid_shift;
	int yr = int(ymax) >> cWater::grid_shift;
	for(int i = xl; i <= xr; i++)
		for(int j = yl; j <= yr; j++)
			if(planeCollisionWater(i, j, body))
				collide = true;
	return collide;
}

bool GeomSphere::bodyCollision(RigidBodyCollision* body1, RigidBodyCollision* body2, ContactInfo& contactInfo) const
{
	xassert(body1);
	if(!body2)
		return false;
	start_timer_auto();
	Vect3f center1(body1->centreOfGravity());
	Vect3f center2(body2->centreOfGravity());
	float dist = center1.distance(center2);
	float penetration = body1->boundRadius() + body2->boundRadius() - dist;
	if(penetration < 0)
		return false;
	Vect3f cp1, cp2;
	const Geom* geom2 = body2->collisionObject();
	switch(geom2->geomType()){
	case GEOM_SPHERE:
		if(dist > FLT_EPS){	
			cp1.sub(center2, center1);
			cp1.normalize();
			cp2.negate(cp1);
			cp1 *= body1->boundRadius();
			cp2 *= -body2->boundRadius();
		}
		else{
			cp1.set(0.01f, 0, 0);
			cp2.set(-0.01f, 0, 0);
		}
		cp1.add(center1);
		cp2.add(center2);
		break;
	case GEOM_BOX:
	case GEOM_MESH:
		if(!sphere_.computeBoxPenetration(cp1, cp2, center1, safe_cast<const GeomBox*>(geom2)->box(), 
			center2, body2->orientation()))
			return false;
	}

	contactInfo.cp1 = cp1;
	contactInfo.cp2 = cp2;

	return true;

}

bool GeomSphere::planeCollision(int x, int y, RigidBodyCollision* body) const
{
	bool collide(false);
	float xl =  float(x << NormalMapTile::tileShift);
	float xr =  float((x + 1) << NormalMapTile::tileShift);
	float yl =  float(y << NormalMapTile::tileShift);
	float yr =  float((y + 1) << NormalMapTile::tileShift);
	Vect3f p1(xl, yl, normalMap->height(x,y) - body->lavaLevel());
	Vect3f p2(xr, yl, normalMap->height(x+1,y) - body->lavaLevel());
	Vect3f p3(xl, yr, normalMap->height(x,y+1) - body->lavaLevel());
	if(triangleCollision(p1, p2, p3, body))
		collide = true;
	p1.set(xr, yr, normalMap->height(x+1,y+1) - body->lavaLevel());
	if(triangleCollision(p3, p2, p1, body))
		collide = true;
	return collide;
}

float GeomSphere::getWaterHeight(int x, int y, RigidBodyCollision* body) const
{
	float waterHeight(environment->water()->GetRelativeZ(x, y));
	waterHeight -= body->waterLevel();
	if(waterHeight > 0.0f){
		if(waterHeight > body->lavaLevel())
			return normalMap->height(x, y) + waterHeight - body->lavaLevel();
		return normalMap->height(x, y) + waterHeight - body->lavaLevel();
	}
	return normalMap->height(x, y) - body->lavaLevel();

}

bool GeomSphere::planeCollisionWater(int x, int y, RigidBodyCollision* body) const
{
	bool collide(false);
	float xl =  float(x << cWater::grid_shift);
	float xr =  float((x + 1) << cWater::grid_shift);
	float yl =  float(y << cWater::grid_shift);
	float yr =  float((y + 1) << cWater::grid_shift);
	Vect3f p1(xl, yl, getWaterHeight(x, y, body));
	Vect3f p2(xr, yl, getWaterHeight(x + 1, y, body));
	Vect3f p3(xl, yr, getWaterHeight(x, y + 1, body));
	if(triangleCollision(p1, p2, p3, body))
		collide = true;
	p1.set(xr, yr, getWaterHeight(x + 1, y + 1, body));
	if(triangleCollision(p3, p2, p1, body))
		collide = true;
	return collide;
}

bool GeomSphere::triangleCollision(const Vect3f& p1, const Vect3f& p2, const Vect3f& p3, RigidBodyCollision* body) const
{
	Vect3f edge1(p2);
	edge1.sub(p1);
	Vect3f edge2(p3);
	edge2.sub(p1);
	Vect3f normal;
	normal.cross(edge1, edge2);
	normal.normalize();
	Vect3f center(body->centreOfGravity());
	Vect3f point(center);
	point.sub(p1);
	float dist = normal.dot(point);
	if(dist >= sphere_.getRadius())
		return false;
	point = center;
	point.scaleAdd(normal, -dist);
	if(pointInTriangle(p1,p2,p3,point)) {
		body->addContact(point, normal, sphere_.getRadius() - dist);
		return true;
	}
	point = closestPointToTriange(p1, p2, p3, center);
	dist = point.distance2(center);
	if(dist < sqr(sphere_.getRadius())) {
		dist = sqrtf(dist);
		if(dist > 0.00001f){
			normal = center - point;
			normal /= dist;
		}
		body->addContact(point, normal, sphere_.getRadius() - dist);
		return true;
	}
	return false;
}

bool GeomSphere::pointInTriangle(const Vect2f& p1, const Vect2f& p2, const Vect2f& p3, const Vect2f& p) const
{
	Vect2f a1 = p, a2 = p, a3 = p;
	a1 -= p1;
	a2 -= p2;
	a3 -= p3;
	float d1 = a1 % a2;
	float d2 = a2 % a3;
	return ((d1 *= d2) > 0.0f && (d2 *= a3 % a1) > 0.0f);
}

Vect3f GeomSphere::closestPointOnLine(const Vect3f& a, const Vect3f& b, const Vect3f& p) const
{
	Vect3f c(p);
	c.sub(a);
	Vect3f v(b);
	v.sub(a);
	float t = v.dot(c);
	if(t < 0.0f) 
		return a;
	float d2 = v.norm2();
	if(t > d2) 
		return b;
	v.scale(t / d2);
	v.add(a);
	return v;
}

Vect3f GeomSphere::closestPointToTriange(const Vect3f& a, const Vect3f& b, const Vect3f& c, const Vect3f& p) const
{
	Vect3f Rab = closestPointOnLine(a, b, p);
	Vect3f Rbc = closestPointOnLine(b, c, p);
	Vect3f Rca = closestPointOnLine(c, a, p);
	float dab = Rab.distance2(p);
	float dbc = Rbc.distance2(p);
	float dca = Rca.distance2(p);

	return (dab < dbc) ? ((dab < dca) ? Rab :Rca ) : ((dbc < dca) ? Rbc : Rca);
}

float GeomSphere::getVolume() const 
{ 
	return 4.188790204786f * sphere().getRadius() * sphere().getRadius() * sphere().getRadius(); 
}

void GeomSphere::getTOI(float mass, Vect3f& toi) const
{
	toi[0] = toi[1] = toi[2] = 0.40f * mass * sqr(sphere_.getRadius());
}

bool GeomSphere::waterContact(RigidBodyCollision* body) const
{
	Vect3f tmp = body->centreOfGravity();
	tmp.z -= sphere_.getRadius();
	return environment->water()->isUnderWater(tmp);
}

GeomBox::GeomBox(const Vect3f& extent) : 
	box_(extent) 
{
	geomType_ = GEOM_BOX;
}

void GeomBox::computeNormals(Vect3f normals[3], const QuatF& orientation) const
{
	float sx(sqr(orientation.x()));
	float sy(sqr(orientation.y()));
	float sz(sqr(orientation.z()));
	normals[0].set(1 - 2 * (sy + sz),
		2 * (orientation.s() * orientation.z() + orientation.x() * orientation.y()),
		2 * (orientation.x() * orientation.z() - orientation.s() * orientation.y()));
	normals[1].set(2 * (orientation.x() * orientation.y() - orientation.s() * orientation.z()),
		1 - 2 * (sx + sz),
		2 * (orientation.y() * orientation.z() + orientation.s() * orientation.x()));
	normals[2].set(2 * (orientation.x() * orientation.z() + orientation.s() * orientation.y()),
		2 * (orientation.y() * orientation.z() - orientation.s() * orientation.x()),
		1 - 2 * (sx + sy));
}

void GeomBox::computeVertex(Vect3f vertex[8], const Vect3f& position, const Vect3f normals[3]) const
{
	vertex[7].scale(normals[0], box_.getExtent().x);
	vertex[4].scale(normals[1], box_.getExtent().y);
	vertex[5].scale(normals[2], box_.getExtent().z);
	vertex[0].sub(position, vertex[7]);
	vertex[0].sub(vertex[4]);
	vertex[0].sub(vertex[5]);            //p0
	vertex[7].scale(2.0f);
	vertex[4].scale(2.0f);
	vertex[5].scale(2.0f);
	vertex[1].add(vertex[0], vertex[5]); //p0         + z
	vertex[2].add(vertex[1], vertex[7]); //p0 + x     + z
	vertex[3].add(vertex[0], vertex[7]); //p0 + x
	vertex[4].add(vertex[0]);            //p0     + y
	vertex[5].add(vertex[4]);            //p0     + y + z
	vertex[6].add(vertex[5], vertex[7]); //p0 + x + y + z 
	vertex[7].add(vertex[4]);            //p0 + x + y 
}

void GeomBox::computeVertex(Vect3f vertex[8], const Vect3f& position, const QuatF& orientation) const
{
	computeNormals(vertex, orientation);
	computeVertex(vertex, position, vertex);
}

float GeomBox::computeGroundPenetration(const Vect3f& centreOfGravity, const QuatF& orientation) const
{
	Vect3f vertex[4];
	computeNormals(vertex, orientation);
	vertex[0].scale(box_.getExtent().x);
	vertex[1].scale(box_.getExtent().y);
	vertex[2].scale(box_.getExtent().z);
	vertex[3].sub(centreOfGravity, vertex[0]);
	vertex[3].sub(vertex[1]);
	vertex[3].sub(vertex[2]);            //p0
	vertex[0].scale(2.0f);
	vertex[1].scale(2.0f);
	vertex[2].add(vertex[3], vertex[0]); //p0 + x
	vertex[0].add(vertex[3], vertex[1]); //p0     + y
	vertex[1].add(vertex[2]);            //p0 + x + y 
	float penetration(normalMap->heightLinear(vertex[0].x, vertex[0].y) - vertex[0].z);
	for(int i = 1; i < 4; i++){	
		float p(normalMap->heightLinear(vertex[i].x, vertex[i].y) - vertex[i].z);
		if(p > penetration)
			penetration = p;
	}
	return penetration;
}

bool GeomBox::groundCollision(RigidBodyCollision* body) const
{
	bool collide(false);
	Vect3f normals[3];
	computeNormals(normals, body->orientation());
	Vect3f vertex[8];
	computeVertex(vertex, body->centreOfGravity(), normals);
	if(collisionVertex(body, vertex))
		collide = true;
	if(collisionPlane(body, vertex[2], vertex[3], vertex[7], vertex[6], normals[0]))
		collide = true;
	normals[0].negate();
	if(collisionPlane(body, vertex[0], vertex[1], vertex[5], vertex[4], normals[0]))
		collide = true;
	if(collisionPlane(body, vertex[4], vertex[5], vertex[6], vertex[7], normals[1]))
		collide = true;
	normals[1].negate();
	if(collisionPlane(body, vertex[0], vertex[1], vertex[2], vertex[3], normals[1]))
		collide = true;
	if(collisionPlane(body, vertex[1], vertex[2], vertex[6], vertex[5], normals[2]))
		collide = true;
	normals[2].negate();
	if(collisionPlane(body, vertex[0], vertex[3], vertex[7], vertex[4], normals[2]))
		collide = true;
	return collide;
}

bool GeomBox::waterCollision(RigidBodyCollision* body) const
{
	bool collide(false);
	Vect3f normals[3];
	computeNormals(normals, body->orientation());
	Vect3f vertex[8];
	computeVertex(vertex, body->centreOfGravity(), normals);
	if(collisionVertexWater(body, vertex))
		collide = true;
	if(collisionPlaneWater(body, vertex[2], vertex[3], vertex[7], vertex[6], normals[0]))
		collide = true;
	normals[0].negate();
	if(collisionPlaneWater(body, vertex[0], vertex[1], vertex[5], vertex[4], normals[0]))
		collide = true;
	if(collisionPlaneWater(body, vertex[4], vertex[5], vertex[6], vertex[7], normals[1]))
		collide = true;
	normals[1].negate();
	if(collisionPlaneWater(body, vertex[0], vertex[1], vertex[2], vertex[3], normals[1]))
		collide = true;
	if(collisionPlaneWater(body, vertex[1], vertex[2], vertex[6], vertex[5], normals[2]))
		collide = true;
	normals[2].negate();
	if(collisionPlaneWater(body, vertex[0], vertex[3], vertex[7], vertex[4], normals[2]))
		collide = true;
	return collide;
}

bool GeomBox::bodyCollision(RigidBodyCollision* body1, RigidBodyCollision* body2, ContactInfo& contactInfo) const
{
	xassert(body1);
	if(!body2)
		return false;
	start_timer_auto();
	Vect3f center1(body1->centreOfGravity());
	Vect3f center2(body2->centreOfGravity());
	float dist2 = center1.distance2(center2);
	if(sqr(body1->boundRadius() + body2->boundRadius()) < dist2)
		return false;
	Vect3f cp1, cp2;
	const Geom* geom2 = body2->collisionObject();
	switch(geom2->geomType()){
	case GEOM_SPHERE:
		if(!safe_cast<const GeomSphere*>(geom2)->sphere().computeBoxPenetration(cp2, cp1, 
			center2, box_, center1, body1->orientation()))
			return false;
		break;
	case GEOM_BOX:
	case GEOM_MESH:
		Se3f X12(body2->orientation(), center2);
		X12.invert();
		X12.trans().add(X12.xformVect(center1));
		X12.rot().postmult(body1->orientation());
		CD::CDDuality penetrate(CD::Transform(X12, box_), safe_cast<const GeomBox*>(geom2)->box());
		//if(!penetrate.computePenetrationDistance(cp1, cp2))
		if(!penetrate.computeBoxBoxPenetration(cp1, cp2))
			return false;
		body2->orientation().xform(cp1);
		cp1.add(center2);
		body2->orientation().xform(cp2);
		cp2.add(center2);
	}
	contactInfo.cp1 = cp1;
	contactInfo.cp2 = cp2;
	return true;
}

bool GeomBox::collisionPlane(RigidBodyCollision* body, Vect3f& p1, Vect3f& p2, Vect3f& p3, Vect3f& p4, Vect3f& normal) const
{
	bool collide(false);
	if(normal.z > 0)
		return false;
	float xmin = min(min(p1.x, p2.x), min(p3.x, p4.x));
	float xmax = max(max(p1.x, p2.x), max(p3.x, p4.x));
	float ymin = min(min(p1.y, p2.y), min(p3.y, p4.y));
	float ymax = max(max(p1.y, p2.y), max(p3.y, p4.y));
	float shift = 1.0f / (1 << NormalMapTile::tileShift);
	xmin *= shift;
	xmax *= shift;
	ymin *= shift;
	ymax *= shift;
	int xl = round(xmin - 0.5);
	int xr = round(xmax + 0.5);
	int yl = round(ymin - 0.5);
	int yr = round(ymax + 0.5);
	for(int i=xl; i<=xr; i++)
		for(int j=yl; j<=yr; j++){
			Vect3f point(float(i << NormalMapTile::tileShift), float(j << NormalMapTile::tileShift), normalMap->height(i, j));
			if(pointInPlane(p1, p2, p3, p4, normal, point)){
				Vect3f ptop(point);
				ptop.sub(p1);
				body->addContact(point, normalMap->normal(i,j), -normal.dot(ptop));
				collide = true;
			}
		}
	return collide;
}

bool GeomBox::collisionPlaneWater(RigidBodyCollision* body, Vect3f& p1, Vect3f& p2, Vect3f& p3, Vect3f& p4, Vect3f& normal) const
{
	bool collide(false);
	if(normal.z > 0)
		return false;
	float xmin = min(min(p1.x, p2.x), min(p3.x, p4.x));
	float xmax = max(max(p1.x, p2.x), max(p3.x, p4.x));
	float ymin = min(min(p1.y, p2.y), min(p3.y, p4.y));
	float ymax = max(max(p1.y, p2.y), max(p3.y, p4.y));
	int xl = int(xmin) >> cWater::grid_shift;
	int xr = int(xmax) >> cWater::grid_shift;
	int yl = int(ymin) >> cWater::grid_shift;
	int yr = int(ymax) >> cWater::grid_shift;
	for(int i=xl; i<=xr; i++)
		for(int j=yl; j<=yr; j++){
			float waterHeight = environment->water()->GetRelativeZ(i, j);
			waterHeight -= body->waterLevel();
			if(waterHeight > 0){
				Vect3f point(float(i << NormalMapTile::tileShift), float(j << NormalMapTile::tileShift), normalMap->height(i, j) + waterHeight);
				if(pointInPlane(p1, p2, p3, p4, normal, point)) {
					Vect3f ptop(point);
					ptop.sub(p1);
					body->addContact(point, Vect3f::K, -normal.dot(ptop) / 5.0f);
					collide = true;
				}
			}
			else{
				Vect3f point(float(i << NormalMapTile::tileShift), float(j << NormalMapTile::tileShift), normalMap->height(i, j));
				if(pointInPlane(p1, p2, p3, p4, normal, point)) {
					Vect3f ptop(point);
					ptop.sub(p1);
					body->addContact(point, normalMap->normal(i,j), -normal.dot(ptop));
					collide = true;
				}
			}
		}
	return collide;
}

bool GeomBox::collisionVertex(RigidBodyCollision* body, Vect3f vertex[8]) const
{
	bool collide(false);
	float shift = 1.0f / (1 << NormalMapTile::tileShift);
	for(int i = 0; i < 8; i++){	
		float hl(normalMap->heightLinear(vertex[i].x, vertex[i].y));
		if(hl > vertex[i].z) {
			Vect3f ptp(0,0,hl - vertex[i].z);
			Vect3f ntn = normalMap->normalLinear(vertex[i].x, vertex[i].y);
			body->addContact(vertex[i], ntn, ntn.dot(ptp));
			collide = true;
		}
	}
	return collide;
}

bool GeomBox::collisionVertexWater(RigidBodyCollision* body, Vect3f vertex[8]) const
{
	bool collide(false);
	for(int i = 0; i < 8; i++){
		float waterHeight(environment->water()->GetDeepWaterFast(vertex[i].xi(), vertex[i].yi()));
		waterHeight -= body->waterLevel();
		if(waterHeight > 0){
			float hl(normalMap->heightLinear(vertex[i].x, vertex[i].y));
			hl += waterHeight;
			if(hl > vertex[i].z){
				body->addContact(vertex[i], Vect3f::K, (hl - vertex[i].z) / 5.0f);
				collide = true;
			}
		}
		else{
			float hl(normalMap->heightLinear(vertex[i].x, vertex[i].y));
			if(hl > vertex[i].z) {
				Vect3f ptp(0, 0, hl - vertex[i].z);
				Vect3f ntn(normalMap->normalLinear(vertex[i].x, vertex[i].y));
				body->addContact(vertex[i], ntn, ntn.dot(ptp));
				collide = true;
			}
		}
	}
	return collide;
}

bool GeomBox::pointInPlane(Vect3f& p1, Vect3f& p2, Vect3f& p3, Vect3f& p4, Vect3f& normal, Vect3f& point) const
{
	Vect3f ptop;
	ptop.sub(point, p1);
	if(normal.dot(ptop) > 0)
		return false;
	Vect2f pp1(p1.x, p1.y);
	Vect2f pp2(p2.x, p2.y);
	Vect2f pp3(p4.x, p4.y);
	Vect2f pp(point.x, point.y);
	Vect2f app(pp - pp1);
	if((pp1 - pp3).dot(app) < 0)
		return false;
	if((pp1 - pp2).dot(app) < 0)
		return false;
	app = pp - pp3; 
	pp1.set(p3.x, p3.y);
	if((pp1 - pp2).dot(app) < 0)
		return false;
	if((pp1 - pp3).dot(app) < 0)
		return false;
	return true;
}

float GeomBox::getVolume() const 
{ 
	return 8 * box_.getExtent().x * box_.getExtent().y * box_.getExtent().z; 
}

void GeomBox::getTOI(float mass, Vect3f& toi) const
{
	float extentx2(sqr(box_.getExtent().x));
	float extenty2(sqr(box_.getExtent().y));
	float extentz2(sqr(box_.getExtent().z));
	toi.set(mass * (extenty2 + extentz2) / 3.0f,
		mass * (extentx2 + extentz2) / 3.0f,
		mass * (extentx2 + extenty2) / 3.0f
		);
}

bool GeomBox::waterContact(RigidBodyCollision* body) const
{
	Vect3f vertex[8];
	computeVertex(vertex, body->centreOfGravity(), body->orientation());
	for(int i = 0; i < 8; i++)
		if(environment->water()->isUnderWater(vertex[i]))
			return true;
	return false;
}

GeomMesh::GeomMesh(vector<Vect3f>& vertex) :
	GeomBox(Vect3f::ZERO)
{
	geomType_ = GEOM_MESH; 
	points.swap(vertex);
	simplifyMesh(25);
}

bool GeomMesh::groundCollision(RigidBodyCollision* body) const
{
	bool collide(false);
	for(int i=0; i<points.size(); i++){
		Vect3f point = points[i];
		body->pose().xformPoint(point);
		float z = normalMap->heightLinear(point.x, point.y);
		if(point.z < z){
			body->addContact(point, normalMap->normalLinear(point.x, point.y), z - point.z);
			collide = true;
		}
	}
	return collide;
}

bool GeomMesh::waterCollision(RigidBodyCollision* body) const
{
	bool collide(false);
	for(int i=0; i<points.size(); i++){
		Vect3f point = points[i];
		body->pose().xformPoint(point);
		float waterHeight(environment->water()->GetDeepWaterFast(point.xi(), point.yi()));
		waterHeight -= body->waterLevel();
		if(waterHeight > 0){
			float hl(normalMap->heightLinear(point.x, point.y));
			hl += waterHeight;
			if(hl > point.z){
				body->addContact(point, Vect3f::K, (hl - point.z) / 5.0f);
				collide = true;
			}
		}
		else{
			float hl(normalMap->heightLinear(point.x, point.y));
			if(hl > point.z) {
				Vect3f ptp(0, 0, hl - point.z);
				Vect3f ntn(normalMap->normalLinear(point.x, point.y));
				body->addContact(point, ntn, ntn.dot(ptp));
				collide = true;
			}
		}
	}
	return collide;
}

Vect3f GeomMesh::computeCentreOfGravity()
{
	Vect3f centreOfGravity = Vect3f::ZERO;
	if(!points.empty()){
		for(int i = 0; i < points.size(); i++)
			centreOfGravity += points[i];
		centreOfGravity /= float(points.size());
	}
	Vect3f extent = Vect3f::ZERO;
	vector<Vect3f>::const_iterator point;
	FOR_EACH(points, point)
	{
		float temp = fabs(point->x - centreOfGravity.x);
		if(temp > extent.x)
			extent.x = temp;

		temp = fabs(point->y - centreOfGravity.y);
		if(temp > extent.y)
			extent.y = temp;

		temp = fabs(point->z - centreOfGravity.z);
		if(temp > extent.z)
			extent.z = temp;
	}
	setExtent(extent);
	return centreOfGravity;
}
void GeomMesh::getTOI(const Vect3f& centreOfGravity, float mass, Vect3f& toi) const
{
	toi = Vect3f::ZERO;
	for(int i=0; i < points.size(); i++) {
		toi.x += sqr(points[i].y - centreOfGravity.y) + sqr(points[i].z - centreOfGravity.z);
		toi.y += sqr(points[i].x - centreOfGravity.x) + sqr(points[i].z - centreOfGravity.z);
		toi.z += sqr(points[i].x - centreOfGravity.x) + sqr(points[i].y - centreOfGravity.y);
	}
	float pointMass = mass / points.size();
	toi *= pointMass;
}

bool GeomMesh::waterContact(RigidBodyCollision* body) const
{
	for(int i = 0; i < points.size(); i++) {
		Vect3f point = points[i];
		body->pose().xformPoint(point);
		if(environment->water()->isUnderWater(point))
			return true;
	}
	return false;
}

void GeomMesh::simplifyMesh(int vertexNum)
{
	start_timer_auto();
	if(vertexNum >= points.size())
		return;
	vector<float> dist2(points.size() - 1, 0);
	for(int i = 0; i < vertexNum; ++i) {
		vector<Vect3f>::iterator j(points.begin() + i);
		vector<float>::iterator d(dist2.begin() + i);
		float maxDist(0);
		int maxNum(0);
		vector<Vect3f>::iterator k;
		for(k = j + 1; k < points.end(); ++k) {
			float& dist(*d);
			dist = i ? min(dist, j->distance(*k)) : j->distance(*k);
			if(dist > maxDist){
				maxDist = dist;
				maxNum = k - points.begin();
			}
			++d;
		}
		if(maxNum){
			vector<float>::iterator d1(dist2.begin() + maxNum - 1);
			float temp(*d1);
			d = dist2.begin() + i;
			*d1 = *d;
			*d = temp;
			k = points.begin() + maxNum;
			Vect3f t(*k);
			*k = *(++j);
			*j = t;
		}
	}
	points.resize(vertexNum);
}

void GeomMesh::showDebugInfo(const Se3f& pose) const
{
	vector<Vect3f>::const_iterator p;
	FOR_EACH(points, p){
		Vect3f point(*p);
		pose.xformPoint(point);
		show_vector(point, Color4c::RED);
	}
}
