#include "StdAfx.h"
#include "ObjectSpreader.h"

#include <algorithm>

ObjectSpreader::ObjectSpreader()
{
	radius_.set (15.0f, 22.0f);
	outline_ = 0;
}

ObjectSpreader::Circle ObjectSpreader::adjacentCircle (const Circle& circle, float radius)
{
    float angle = float(rand()) * M_PI * 2 / (RAND_MAX + 1.0f);
    Vect2f offset (cos (angle) * (radius + circle.radius),
                   sin (angle) * (radius + circle.radius));

    Circle result;
    result.position = circle.position + offset;

    result.radius = radius;
    return result;
}

ObjectSpreader::Circle ObjectSpreader::thirdCircle(const Circle& c1, const Circle& c2, float radius)
{
    float SmallValue = 0.0001f;

    float r1 = (c1.radius + radius);
    float r2 = (c2.radius + radius); 
    
    Circle result;

    Vect2f u = c2.position - c1.position;
    Vect2f v (u.y, -u.x);
    float ulen2 = u.x*u.x + u.y*u.y;
    float s = 0.5f * ((r1*r1 - r2*r2) / ulen2 + 1);
	float to_be_rooted = r1*r1/ulen2 - s*s;
	
    float t = 0.0f;
	if (to_be_rooted > 0.0f) {
		t = sqrt (to_be_rooted);
	} else {
		t = 0.0f;
	}
    result.position = c1.position + u * s + v * t;
    result.radius = radius;
    return result;
}

int ObjectSpreader::outlineLength () 
{
	Node* current = outline_;
	int count = 0;
	do {
		++count;
		xassert (current->next->prev == current);
		xassert (current->prev->next == current);
		current = current->next;
	} while (current != outline_);
	return count;
}

int ObjectSpreader::addCircle (float radius, int outlineIndex)
{
    Node* current = outline_;
	int index = 0;
	
	while (index != outlineIndex) {
		++index;
		current = current->next;
	}
    return addCircle (radius, current);
}

int ObjectSpreader::addCircle (float radius, Node* node)
{
    Circle circle;

	if (node->next == node && node->prev == node) {
        circle = adjacentCircle (getCircle(node), radius);
    } else {
        circle = thirdCircle (getCircle(node), getCircle(node->next), radius);
    }

    ShareHandle<Node> new_node = new Node;
    new_node->prev = node;
    new_node->next = node->next;
    new_node->next->prev = new_node;
    node->next = new_node;
    new_node->index = objects_.size();
    objects_.push_back (circle);
	if (outlineLength () > 3)
		repairOutline ();
	return objects_.size() - 1;
}

void ObjectSpreader::updateObjects ()
{
    clear ();

    Circle c;
    c.position.set (0.0f, 0.0f);
    c.radius = newRadius ();
    objects_.push_back (c);
    outline_ = new Node;
    outline_->index = 0;
    outline_->next = outline_;
    outline_->prev = outline_;

	for (int i = 0; i < 30; ++i) {
		Node* current = outline_;
		int step = rand() % (i + 1);
		for (int j = 0; j < step; ++j)
			current = current->next;
        addCircle (newRadius(), current);
    }
}

float ObjectSpreader::angle (Node* node1, Node* node2, Node* node3)
{
	Vect2f v1 = getCircle(node1).position - getCircle(node2).position;
	Vect2f v2 = getCircle(node3).position - getCircle(node2).position;
	Vect2f a = getCircle(node2).position - getCircle(node1).position;
	Vect2f b = getCircle(node3).position - getCircle(node2).position;

	v1.normalize(1.0f);
	v2.normalize(1.0f);
	float angle = acos (v1.dot (v2));

	if ((Vect3f (a.x, a.y, 0.0f) % Vect3f (b.x, b.y, 0.0f)).z < 0)
		angle = M_PI * 2 - angle;
	return angle;
}

void ObjectSpreader::eraseNode (Node* node)
{
	xassert (node);
	ShareHandle<Node> next = node->next;
	ShareHandle<Node> prev = node->prev;
	next->prev = prev;
	prev->next = next;
	xassert (next && prev);
	node->next = 0;
	node->prev = 0;
	if (node == outline_) {
		outline_ = prev;
	}
}

void ObjectSpreader::repairOutline ()
{
    ShareHandle<Node> current = outline_;
    do {
        bool pre_condition = false;
        Circle test = thirdCircle (getCircle(current->prev), getCircle(current), nextRadius ());
        if (test.intersect (getCircle(current->next)) || test.intersect (getCircle(current->prev->prev))) {
            pre_condition = true;
        } else {
			Circle t2 = thirdCircle (getCircle(current), getCircle(current->next), nextRadius ());
			if (t2.intersect (getCircle(current->prev)) || t2.intersect (getCircle(current->next->next)))
				pre_condition = true;
        }
        if (pre_condition) {
			float current_angle = angle (current->prev->prev, current->prev, current) + 
								  angle (current, current->next, current->next->next);
			float new_angle = angle (current->prev->prev, current->prev, current->next) + 
							  angle (current->prev, current->next, current->next->next);

			if (new_angle > current_angle) {
				ShareHandle<Node> prev = current->prev;
				eraseNode (current);
				current = prev;
			}
        } else {
        }
        current = current->next;
    } while (current != outline_);
}

ObjectSpreader::Circle& ObjectSpreader::getCircle (int index)
{
    xassert (index >= 0 && index < objects_.size ());
    return objects_.at(index);
}

ObjectSpreader::Circle& ObjectSpreader::getCircle (Node* node)
{
    xassert (node);
    return getCircle(node->index);
}

struct IsInactive {
	bool operator() (const ObjectSpreader::Circle& circle) {
		return circle.active == false;
	}
};

void ObjectSpreader::eraseInactive ()
{
	objects_.erase (std::remove_if (objects_.begin(), objects_.end(), IsInactive ()), objects_.end ());
}

void ObjectSpreader::clear ()
{
    objects_.clear ();

	if (ShareHandle<Node> current = outline_) {
		do {
			ShareHandle<Node> next = current->next;
			current->next = 0;
			current->prev = 0;
			current = next;
		} while (current != outline_);
		current = 0;
		outline_ = 0;
	}
}

int ObjectSpreader::getNextCircle ()
{
    Node* current = outline_;
    float min_dist2 = 10e+15f;
    int result = 0;
    int index = 0;
    do {
        float dist2 = ((getCircle (current).position + getCircle (current->next).position) * 0.5f).norm2 ();
        if (dist2 < min_dist2) {
            min_dist2 = dist2;
            result = index;
        }
        ++index;
		current = current->next;
    } while (current != outline_);
    return result;
}


ObjectSpreader::Node* ObjectSpreader::advanceNode (Node* node, int offset)
{
	Node* current = node;
	for (int i = 0; i < offset; ++i)
		current = current->next;
	return current;
}
